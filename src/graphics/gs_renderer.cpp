#include "gs_renderer.h"

#include "graphics/shader.h"
#include "graphics/renderer_storage.h"
#include "engine/gs_engine.h"

#include "framework/nodes/gs_node.h"

#include "framework/camera/camera.h"
#include "framework/camera/orbit_camera.h"

#include "engine/scene.h"

GSRenderer::GSRenderer() : Renderer()
{

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
