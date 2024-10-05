#include "gs_node.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

GSNode::GSNode()
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    model_uniform.data = webgpu_context->create_buffer(sizeof(glm::mat4x4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &get_global_model()[0], "model_matrix_buffer");
    model_uniform.binding = 0;
    model_uniform.buffer_size = sizeof(glm::mat4x4);

    std::vector<Uniform*> uniforms = { &model_uniform };
    model_bindgroup = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/gs_render.wgsl"), 0);
}

GSNode::~GSNode()
{
}

void GSNode::render()
{
}

void GSNode::update(float delta_time)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();
    webgpu_context->update_buffer(std::get<WGPUBuffer>(model_uniform.data), 0, &get_global_model()[0], sizeof(glm::mat4x4));
}

void GSNode::render_gui()
{
}

void GSNode::create_gs_data_buffer(const std::vector<sGSData> gs_data)
{
    gs_count = gs_data.size();
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();
    gs_buffer = webgpu_context->create_buffer(sizeof(sGSData) * gs_data.size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, gs_data.data(), ("gs_buffer_" + name).c_str());
}

uint32_t GSNode::get_gs_count()
{
    return gs_count;
}

uint32_t GSNode::get_gs_bytes_size()
{
    return gs_count * sizeof(sGSData);
}
