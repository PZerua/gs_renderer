#pragma once

#include "includes.h"

#include "graphics/renderer.h"

class GSRenderer : public Renderer {

    Shader* gs_render_shader = nullptr;
    Pipeline gs_render_pipeline;

    Shader* gs_basis_shader = nullptr;
    Pipeline gs_basis_pipeline;

    Shader* gs_scan_shader = nullptr;
    Pipeline gs_scan_pipeline;

    Shader* gs_scan_sum_shader = nullptr;
    Pipeline gs_scan_sum_pipeline;

    Shader* gs_shuffle_shader = nullptr;
    Pipeline gs_shuffle_pipeline;

    uint32_t xr_gs_camera_buffer_stride;
    Uniform gs_camera_data_uniform;
    WGPUBindGroup gs_camera_data_bindgroup;

    struct sGSCameraData {
        glm::mat4x4 view_projection;
        glm::vec2   screen_size;
        glm::vec2   dummy;
    } gs_camera_data;

public:

    GSRenderer();

    int initialize(GLFWwindow* window, bool use_mirror_screen = false) override;
    void clean() override;

    void update(float delta_time) override;
    void render() override;

    void render_gs(WGPURenderPassEncoder render_pass, uint32_t camera_stride_offset = 0);
};
