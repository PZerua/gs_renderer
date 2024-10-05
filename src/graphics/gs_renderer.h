#pragma once

#include "includes.h"

#include "graphics/renderer.h"

class GSRenderer : public Renderer {

    Shader* gs_shader = nullptr;
    Pipeline gs_pipeline;

public:

    GSRenderer();

    int initialize(GLFWwindow* window, bool use_mirror_screen = false) override;
    void clean() override;

    void update(float delta_time) override;
    void render() override;

    void render_gs(WGPURenderPassEncoder render_pass, uint32_t camera_stride_offset = 0);
};
