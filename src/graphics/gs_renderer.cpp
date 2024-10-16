#include "gs_renderer.h"

#include "graphics/shader.h"
#include "graphics/renderer_storage.h"
#include "engine/gs_engine.h"

#include "framework/nodes/gs_node.h"

#include "framework/camera/camera.h"
#include "framework/camera/orbit_camera.h"

#include "shaders/gaussian_splatting/gs_render.wgsl.gen.h"

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

    gs_render_shader = RendererStorage::get_shader_from_source(shaders::gs_render::source, shaders::gs_render::path);
    gs_render_pipeline.create_render_async(gs_render_shader, color_target, desc);

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
    camera->set_speed(0.2f);

    return 0;
}

void GSRenderer::clean()
{
    Renderer::clean();
}

void GSRenderer::update(float delta_time)
{
    Renderer::update(delta_time);
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
            wgpuRenderPassEncoderSetBindGroup(render_pass, 1, render_camera_bind_group, 1, &camera_stride_offset);

            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, gs_node->get_render_buffer(), 0, gs_node->get_splats_render_bytes_size());
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, gs_node->get_ids_buffer(), 0, gs_node->get_ids_render_bytes_size());

            wgpuRenderPassEncoderDraw(render_pass, 4, gs_node->get_splat_count(), 0, 0);
        }
    }
}
