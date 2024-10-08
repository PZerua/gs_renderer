#pragma once

#include "includes.h"

#include "graphics/uniform.h"
#include "graphics/pipeline.h"

#include "framework/nodes/node_3d.h"

class Shader;

struct sGSRenderData {
    glm::vec2 position;
    uint32_t id;
};

class GSNode : public Node3D {

    uint32_t splat_count = 0;
    uint32_t padded_splat_count = 0;
    uint32_t workgroup_size = 0;

    WGPUBuffer render_buffer = nullptr;
    WGPUBuffer ids_buffers[2];

    // Render
    Uniform model_uniform;
    Uniform centroid_uniform;
    Uniform basis_uniform;
    Uniform color_uniform;
    WGPUBindGroup render_bindgroup;

    // Covariance
    Uniform rotations_uniform;
    Uniform scales_uniform;
    Uniform covariance_uniform;
    Uniform splat_count_uniform;
    WGPUBindGroup covariance_bindgroup;

    // Basis
    WGPUBuffer distances_ping_pong[2];

    Uniform basis_distances_uniform;
    Uniform ids_basis_uniform;
    WGPUBindGroup basis_uniform_bindgroup;

    // Sort
    Uniform distances_uniforms[4];
    Uniform ids_uniforms[4];
    Uniform temp_uniform;
    Uniform output_sum_array_uniform;
    Uniform sum_array_uniform;
    Uniform sum_size_uniform;

    Uniform radix_id_uniform_buffers[16];
    WGPUBindGroup radix_id_bingroups[16];

    WGPUBindGroup scan_bindgroups[2];
    WGPUBindGroup scan_sum_bindgroup;

    WGPUBindGroup shuffle_bindgroups[2];

    Shader* covariance_shader = nullptr;
    Pipeline covariance_pipeline;

    Shader* basis_shader = nullptr;
    Pipeline basis_pipeline;

    WGPUBindGroup gs_camera_data_bindgroup;

public:

    bool is_skinned = false;

    GSNode();
    ~GSNode();

    void initialize(uint32_t splat_count);

    virtual void render() override;
    virtual void update(float delta_time) override;

    void render_gui() override;

    WGPUBuffer get_render_buffer() {
        return render_buffer;
    }

    WGPUBuffer get_ids_buffer() {
        return ids_buffers[0];
    }

    WGPUBindGroup get_render_bindgroup() {
        return render_bindgroup;
    }

    void set_render_buffers(const std::vector<glm::vec4>& positions, const std::vector<glm::vec4>& colors);
    void set_covariance_buffers(const std::vector<glm::quat>& rotations, const std::vector<glm::vec4>& scales);

    void calculate_basis();
    void calculate_covariance();

    WGPUBindGroup* get_scan_bindgroups() { return scan_bindgroups; }
    WGPUBindGroup* get_radix_id_bindgroups() { return radix_id_bingroups; }
    WGPUBindGroup get_scan_sum_bindgroup() { return scan_sum_bindgroup; }
    WGPUBindGroup* get_shuffle_bindgroups() { return shuffle_bindgroups; }

    uint32_t get_splat_count();
    uint32_t get_padded_splat_count();
    uint32_t get_splats_render_bytes_size();
    uint32_t get_ids_render_bytes_size();

};
