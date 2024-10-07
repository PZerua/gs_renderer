#pragma once

#include "includes.h"

#include "graphics/renderer.h"

class GSNode;

struct sGSCameraData {
    glm::mat4x4 view;
    glm::mat4x4 projection;
    glm::vec2   screen_size;
    glm::vec2   dummy;
};

class GSRenderer : public Renderer {

    Shader* gs_render_shader = nullptr;
    Pipeline gs_render_pipeline;

    // pass 1
    Shader* gs_scan_shader = nullptr;
    Pipeline gs_scan_pipeline;

    // pass 2
    Shader* gs_scan_sum_shader = nullptr;
    Pipeline gs_scan_sum_pipeline;

    // pass 3
    Shader* gs_shuffle_shader = nullptr;
    Pipeline gs_shuffle_pipeline;

    uint32_t xr_gs_camera_buffer_stride;
    Uniform gs_camera_data_uniform;
    WGPUBindGroup gs_camera_data_bindgroup;

    sGSCameraData gs_camera_data;

public:

    GSRenderer();

    int initialize(GLFWwindow* window, bool use_mirror_screen = false) override;
    void clean() override;

    void update(float delta_time) override;
    void render() override;

    void sort_splats(GSNode* gs_node);
    void render_gs(WGPURenderPassEncoder render_pass, uint32_t camera_stride_offset = 0);

    Uniform& get_gs_camera_data_uniform() { return gs_camera_data_uniform; }
};
