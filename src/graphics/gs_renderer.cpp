#include "gs_renderer.h"

#include "graphics/shader.h"
#include "graphics/renderer_storage.h"
#include "engine/gs_engine.h"

#include "framework/nodes/gs_node.h"

#include "engine/scene.h"

GSRenderer::GSRenderer() : Renderer()
{

}

int GSRenderer::initialize(GLFWwindow* window, bool use_mirror_screen)
{
    Renderer::initialize(window, use_mirror_screen);

    clear_color = glm::vec4(0.22f, 0.22f, 0.22f, 1.0);

    gs_shader = RendererStorage::get_shader("data/shaders/gs_render.wgsl");

    WGPUTextureFormat swapchain_format = is_openxr_available ? webgpu_context->xr_swapchain_format : webgpu_context->swapchain_format;

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.blend = nullptr;
    color_target.writeMask = WGPUColorWriteMask_All;

    PipelineDescription desc = { .topology = WGPUPrimitiveTopology_PointList };

    gs_pipeline.create_render(gs_shader, color_target, desc);

    set_custom_pass_user_data(this);

    custom_post_opaque_pass = [](void* user_data, WGPURenderPassEncoder render_pass, uint32_t camera_stride_offset = 0) {
        GSRenderer* gs_renderer = reinterpret_cast<GSRenderer*>(user_data);
        gs_renderer->render_gs(render_pass, camera_stride_offset);
    };

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

            gs_pipeline.set(render_pass);

            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, gs_node->get_model_bindgroup(), 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(render_pass, 1, render_bind_group_camera, 1, &camera_stride_offset);

            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, gs_node->get_gs_buffer(), 0, gs_node->get_gs_bytes_size());

            // Submit indirect drawcalls
            wgpuRenderPassEncoderDraw(render_pass, gs_node->get_gs_count(), 1, 0, 0);
        }
    }
}
