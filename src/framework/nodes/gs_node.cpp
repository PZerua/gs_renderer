#include "gs_node.h"

#include "graphics/gs_renderer.h"
#include "graphics/renderer_storage.h"

#include "framework/math/math_utils.h"

GSNode::GSNode()
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    covariance_shader = RendererStorage::get_shader("data/shaders/gs_covariance.wgsl");
    covariance_pipeline.create_compute_async(covariance_shader);

    basis_shader = RendererStorage::get_shader("data/shaders/gs_basis.wgsl");
    basis_pipeline.create_compute_async(basis_shader);
}

GSNode::~GSNode()
{
}

void GSNode::initialize(uint32_t splat_count)
{
    this->splat_count = splat_count;
    this->padded_splat_count = ceil(static_cast<float>(splat_count) / 512.0f) * 512;
    this->workgroup_size = ceil(static_cast<float>(padded_splat_count) / 256.0f);

    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    float splat_positions[8] = {
         1.0f,  1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
        -1.0f, -1.0f
    };

    std::vector<uint32_t> test;
    for (int i = 0; i < padded_splat_count; ++i) {
        test.push_back(i);
    }

    render_buffer = webgpu_context->create_buffer(sizeof(float) * 8, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage, splat_positions, ("gs_buffer_" + name).c_str());
    ids_buffer_ping_pong[0] = webgpu_context->create_buffer(sizeof(uint32_t) * padded_splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage, test.data(), ("ids_buffer_" + name).c_str());
    ids_buffer_ping_pong[1] = webgpu_context->create_buffer(sizeof(uint32_t) * padded_splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage, test.data(), ("ids_buffer_2_" + name).c_str());

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
        splat_count_uniform.binding = 6;
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

        color_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "colors_buffer");
        color_uniform.binding = 2;
        color_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        basis_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_Storage, nullptr, "basis_buffer");
        basis_uniform.binding = 3;
        basis_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        std::vector<Uniform*> uniforms = { &model_uniform, &centroid_uniform, &basis_uniform, &color_uniform };
        render_bindgroup = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/gs_render.wgsl"), 0);
    }

    // Basis
    {
        distances_ping_pong[0] = webgpu_context->create_buffer(sizeof(uint32_t) * padded_splat_count, WGPUBufferUsage_Storage, nullptr, "distances_buffer");
        distances_ping_pong[1] = webgpu_context->create_buffer(sizeof(uint32_t) * padded_splat_count, WGPUBufferUsage_Storage, nullptr, "distances_buffer_2");

        basis_distances_uniform.data = distances_ping_pong[0];
        basis_distances_uniform.binding = 4;
        basis_distances_uniform.buffer_size = sizeof(uint32_t) * padded_splat_count;

        ids_uniform.data = ids_buffer_ping_pong[0];
        ids_uniform.binding = 5;
        ids_uniform.buffer_size = sizeof(uint32_t) * padded_splat_count;

        std::vector<Uniform*> uniforms = { &model_uniform, &centroid_uniform, &covariance_uniform, &basis_uniform, &basis_distances_uniform, &ids_uniform, &splat_count_uniform };
        basis_uniform_bindgroup = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/gs_basis.wgsl"), 0);
    }

    // Sort
    {
        distances_uniforms[0].data = distances_ping_pong[0];
        distances_uniforms[0].binding = 0;
        distances_uniforms[0].buffer_size = sizeof(uint32_t) * padded_splat_count;

        distances_uniforms[1].data = distances_ping_pong[0];
        distances_uniforms[1].binding = 5;
        distances_uniforms[1].buffer_size = sizeof(uint32_t) * padded_splat_count;

        distances_uniforms[2].data = distances_ping_pong[1];
        distances_uniforms[2].binding = 0;
        distances_uniforms[2].buffer_size = sizeof(uint32_t) * padded_splat_count;

        distances_uniforms[3].data = distances_ping_pong[1];
        distances_uniforms[3].binding = 5;
        distances_uniforms[3].buffer_size = sizeof(uint32_t) * padded_splat_count;

        ids_buffer_uniforms[0].data = ids_buffer_ping_pong[0];
        ids_buffer_uniforms[0].binding = 1;
        ids_buffer_uniforms[0].buffer_size = sizeof(uint32_t) * padded_splat_count;

        ids_buffer_uniforms[1].data = ids_buffer_ping_pong[0];
        ids_buffer_uniforms[1].binding = 6;
        ids_buffer_uniforms[1].buffer_size = sizeof(uint32_t) * padded_splat_count;

        ids_buffer_uniforms[2].data = ids_buffer_ping_pong[1];
        ids_buffer_uniforms[2].binding = 1;
        ids_buffer_uniforms[2].buffer_size = sizeof(uint32_t) * padded_splat_count;

        ids_buffer_uniforms[3].data = ids_buffer_ping_pong[1];
        ids_buffer_uniforms[3].binding = 6;
        ids_buffer_uniforms[3].buffer_size = sizeof(uint32_t) * padded_splat_count;

        temp_uniform.data = webgpu_context->create_buffer(sizeof(uint32_t) * padded_splat_count * 4, WGPUBufferUsage_Storage, nullptr, ("scan_output_buffer_" + name).c_str());
        temp_uniform.binding = 2;
        temp_uniform.buffer_size = sizeof(uint32_t) * padded_splat_count * 4;

        uint32_t sum_size = next_power_of_two(ceil(static_cast<float>(padded_splat_count) / 512.0f));

        sum_array_uniform.data = webgpu_context->create_buffer(sizeof(uint32_t) * sum_size * 4, WGPUBufferUsage_Storage, nullptr, ("sum_array_buffer_" + name).c_str());
        sum_array_uniform.binding = 1;
        sum_array_uniform.buffer_size = sizeof(uint32_t) * sum_size * 4;

        output_sum_array_uniform.data = webgpu_context->create_buffer(sizeof(uint32_t) * sum_size * 4, WGPUBufferUsage_Storage, nullptr, ("scan_sum_output_buffer_" + name).c_str());
        output_sum_array_uniform.binding = 1;
        output_sum_array_uniform.buffer_size = sizeof(uint32_t) * sum_size * 4;

        std::vector<Uniform*> uniforms = { &distances_uniforms[0], &sum_array_uniform, &temp_uniform };
        scan_bindgroup_ping_pong[0] = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/gs_scan.wgsl"), 0);

        uniforms = { &distances_uniforms[2], &sum_array_uniform, &temp_uniform };
        scan_bindgroup_ping_pong[1] = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/gs_scan.wgsl"), 0);


    }

    GSRenderer* gs_renderer = static_cast<GSRenderer*>(Renderer::instance);

    std::vector<Uniform*> uniforms = { &gs_renderer->get_gs_camera_data_uniform() };
    gs_camera_data_bindgroup = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/gs_basis.wgsl"), 1);
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


void GSNode::calculate_basis()
{
    WGPUCommandEncoder command_encoder = Renderer::instance->get_global_command_encoder();

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    uint32_t camera_stride_offset = 0;

    basis_pipeline.set(compute_pass);
    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, basis_uniform_bindgroup, 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(compute_pass, 1, gs_camera_data_bindgroup, 1, &camera_stride_offset);
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

uint32_t GSNode::get_padded_splat_count()
{
    return padded_splat_count;
}

uint32_t GSNode::get_splats_render_bytes_size()
{
    return sizeof(float) * 8;
}

uint32_t GSNode::get_ids_render_bytes_size()
{
    return sizeof(uint32_t) * padded_splat_count;
}
