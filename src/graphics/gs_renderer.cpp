#include "gs_renderer.h"

#include "graphics/shader.h"
#include "graphics/renderer_storage.h"
#include "engine/gs_engine.h"

#include "framework/nodes/gs_node.h"

#include "framework/camera/camera.h"
#include "framework/camera/orbit_camera.h"

#include "engine/scene.h"

GSRenderer::GSRenderer() : Renderer()
{

}

int GSRenderer::initialize(GLFWwindow* window, bool use_mirror_screen)
{
    Renderer::initialize(window, use_mirror_screen);

    clear_color = glm::vec4(0.22f, 0.22f, 0.22f, 1.0);

    WGPUTextureFormat swapchain_format = is_openxr_available ? webgpu_context->xr_swapchain_format : webgpu_context->swapchain_format;

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.blend = nullptr;
    color_target.writeMask = WGPUColorWriteMask_All;

    RenderPipelineDescription desc = { .topology = WGPUPrimitiveTopology_TriangleStrip };

    WGPUBlendState* blend_state = new WGPUBlendState;
    blend_state->color = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_SrcAlpha,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
    };
    blend_state->alpha = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_Zero,
            .dstFactor = WGPUBlendFactor_One,
    };

    color_target.blend = blend_state;

    desc.depth_write = WGPUOptionalBool_False;
    desc.blending_enabled = true;

    gs_render_shader = RendererStorage::get_shader("data/shaders/gs_render.wgsl");
    gs_render_pipeline.create_render_async(gs_render_shader, color_target, desc);

    // Sort
    {
        gs_scan_shader = RendererStorage::get_shader("data/shaders/gs_scan.wgsl");
        gs_scan_pipeline.create_compute_async(gs_scan_shader);

        gs_scan_sum_shader = RendererStorage::get_shader("data/shaders/gs_scan_sum.wgsl");
        gs_scan_sum_pipeline.create_compute_async(gs_scan_sum_shader);

        gs_shuffle_shader = RendererStorage::get_shader("data/shaders/gs_shuffle.wgsl");
        gs_shuffle_pipeline.create_compute_async(gs_shuffle_shader);
    }

    xr_gs_camera_buffer_stride = std::max(static_cast<uint32_t>(sizeof(sGSCameraData)), required_limits.limits.minUniformBufferOffsetAlignment);

    gs_camera_data_uniform.data = webgpu_context->create_buffer(xr_gs_camera_buffer_stride * EYE_COUNT, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr, "gs_camera_buffer");
    gs_camera_data_uniform.binding = 0;
    gs_camera_data_uniform.buffer_size = sizeof(sGSCameraData);

    std::vector<Uniform*> uniforms = { &gs_camera_data_uniform };
    gs_camera_data_bindgroup = webgpu_context->create_bind_group(uniforms, gs_render_shader, 1);

    set_custom_pass_user_data(this);

    custom_post_transparent_pass = [](void* user_data, WGPURenderPassEncoder render_pass, uint32_t camera_stride_offset = 0) {
        GSRenderer* gs_renderer = reinterpret_cast<GSRenderer*>(user_data);
        gs_renderer->render_gs(render_pass, camera_stride_offset);
    };

    if (camera) {
        delete camera;
    }

    camera = new OrbitCamera();
    camera->set_perspective(glm::radians(45.0f), webgpu_context->render_width / static_cast<float>(webgpu_context->render_height), z_near, z_far);
    camera->look_at(glm::vec3(0.0f, 0.8f, 1.6f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    camera->set_mouse_sensitivity(0.002f);
    camera->set_speed(0.1f);

    return 0;
}

void GSRenderer::clean()
{
    Renderer::clean();
}

void GSRenderer::update(float delta_time)
{
    Renderer::update(delta_time);

    GSEngine* gs_engine = static_cast<GSEngine*>(GSEngine::instance);

    Scene* scene = gs_engine->get_main_scene();

    gs_camera_data.view = camera->get_view();
    gs_camera_data.projection = camera->get_projection();
    gs_camera_data.screen_size = { webgpu_context->screen_width, webgpu_context->screen_height };

    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(gs_camera_data_uniform.data), 0, &gs_camera_data, sizeof(sGSCameraData));

    WGPUCommandEncoder command_encoder = Renderer::instance->get_global_command_encoder();

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;

    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    for (Node* node : scene->get_nodes()) {

        GSNode* gs_node = dynamic_cast<GSNode*>(node);

        if (gs_node) {

            gs_node->calculate_basis(compute_pass);
            gs_node->sort(compute_pass);
        }
    }

    wgpuComputePassEncoderEnd(compute_pass);
    wgpuComputePassEncoderRelease(compute_pass);
}

void GSRenderer::render()
{
    Renderer::render();
}

void GSRenderer::render_gs(WGPURenderPassEncoder render_pass, uint32_t camera_stride_offset)
{
    GSEngine* gs_engine = static_cast<GSEngine*>(GSEngine::instance);

    Scene* scene = gs_engine->get_main_scene();

    for (Node* node : scene->get_nodes()) {

        GSNode* gs_node = dynamic_cast<GSNode*>(node);

        if (gs_node) {

            gs_render_pipeline.set(render_pass);

            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, gs_node->get_render_bindgroup(), 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(render_pass, 1, gs_camera_data_bindgroup, 1, &camera_stride_offset);

            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, gs_node->get_render_buffer(), 0, gs_node->get_splats_render_bytes_size());
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, gs_node->get_ids_buffer(), 0, gs_node->get_ids_render_bytes_size());

            wgpuRenderPassEncoderDraw(render_pass, 4, gs_node->get_splat_count(), 0, 0);
        }
    }
}
