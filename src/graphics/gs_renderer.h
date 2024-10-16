#pragma once

#include "includes.h"

#include "graphics/renderer.h"

class GSNode;

class GSRenderer : public Renderer {

    Shader* gs_render_shader = nullptr;
    Pipeline gs_render_pipeline;

public:

    GSRenderer();

    int initialize(GLFWwindow* window, bool use_mirror_screen = false) override;
    void clean() override;

    void update(float delta_time) override;
    void render() override;

    void render_gs(WGPURenderPassEncoder render_pass, uint32_t camera_stride_offset = 0);
};
