#include "gs_node.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

GSNode::GSNode()
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    covariance_shader = RendererStorage::get_shader("data/shaders/gs_covariance.wgsl");
    covariance_pipeline.create_compute_async(covariance_shader);
}

GSNode::~GSNode()
{
}

void GSNode::initialize(uint32_t splat_count)
{
    this->splat_count = splat_count;
    this->padded_splat_count = ceil(splat_count / 512) * 512;
    this->workgroup_size = ceil(padded_splat_count / 256);

    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    float splat_positions[8] = {
         1.0f,  1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
        -1.0f, -1.0f
    };

    render_buffer = webgpu_context->create_buffer(sizeof(float) * 8, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage, splat_positions, ("gs_buffer_" + name).c_str());

    // Covariance
    {
        rotations_uniform.data = webgpu_context->create_buffer(sizeof(glm::quat) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "rotations_buffer");
        rotations_uniform.binding = 0;
        rotations_uniform.buffer_size = sizeof(glm::quat) * splat_count;

        scales_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec3) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "scales_buffer");
        scales_uniform.binding = 1;
        scales_uniform.buffer_size = sizeof(glm::vec3) * splat_count;

        covariance_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec3) * splat_count * 8, WGPUBufferUsage_Storage, nullptr, "covariance_buffer");
        covariance_uniform.binding = 2;
        covariance_uniform.buffer_size = sizeof(glm::vec3) * splat_count * 8;

        splat_count_uniform.data = webgpu_context->create_buffer(sizeof(uint32_t), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &splat_count, "splat_count_buffer");
        splat_count_uniform.binding = 3;
        splat_count_uniform.buffer_size = sizeof(uint32_t);

        std::vector<Uniform*> uniforms = { &rotations_uniform, &scales_uniform, &covariance_uniform, &splat_count_uniform };
        covariance_bindgroup = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/gs_covariance.wgsl"), 0);
    }

    // Render
    {
        model_uniform.data = webgpu_context->create_buffer(sizeof(glm::mat4x4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &get_global_model()[0], "model_matrix_buffer");
        model_uniform.binding = 0;
        model_uniform.buffer_size = sizeof(glm::mat4x4);

        centroid_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec3) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "centroids_buffer");
        centroid_uniform.binding = 1;
        centroid_uniform.buffer_size = sizeof(glm::vec3) * splat_count;

        basis_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_Storage, nullptr, "basis_buffer");
        basis_uniform.binding = 2;
        basis_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        color_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "colors_buffer");
        color_uniform.binding = 3;
        color_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        std::vector<Uniform*> uniforms = { &model_uniform, &centroid_uniform, &basis_uniform, &color_uniform };
        render_bindgroup = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/gs_render.wgsl"), 0);
    }
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

void GSNode::set_covariance_buffers(const std::vector<glm::quat>& rotations, const std::vector<glm::vec3>& scales)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    webgpu_context->update_buffer(std::get<WGPUBuffer>(rotations_uniform.data), 0, rotations.data(), sizeof(glm::quat) * splat_count);
    webgpu_context->update_buffer(std::get<WGPUBuffer>(scales_uniform.data), 0, scales.data(), sizeof(glm::vec3) * splat_count);
}

void GSNode::calculate_covariance()
{
    WGPUCommandEncoder command_encoder = Renderer::instance->get_global_command_encoder();

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    covariance_pipeline.set(compute_pass);
    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, covariance_bindgroup, 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroup_size, 1, 1);

    wgpuComputePassEncoderEnd(compute_pass);
    wgpuComputePassEncoderRelease(compute_pass);
}

void GSNode::set_render_buffers(const std::vector<glm::vec3>& positions, const std::vector<glm::vec4>& colors)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    webgpu_context->update_buffer(std::get<WGPUBuffer>(centroid_uniform.data), 0, positions.data(), sizeof(glm::vec3) * splat_count);
    webgpu_context->update_buffer(std::get<WGPUBuffer>(color_uniform.data), 0, colors.data(), sizeof(glm::vec4) * splat_count);
}

uint32_t GSNode::get_splat_count()
{
    return splat_count;
}

uint32_t GSNode::get_splats_render_bytes_size()
{
    return sizeof(float) * 8;
}
