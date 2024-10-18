#pragma once

#include "includes.h"

#include "graphics/renderer.h"

class GSNode;

class GSRenderer : public Renderer {


public:

    GSRenderer();

    void clean() override;

    void update(float delta_time) override;
    void render() override;
};
