#include "gs_engine.h"

#include "framework/nodes/environment_3d.h"
#include "framework/parsers/parse_ply.h"

#include "graphics/gs_renderer.h"
#include "graphics/renderer_storage.h"

#include "engine/scene.h"

#include "shaders/mesh_grid.wgsl.gen.h"

#include "framework/utils/tinyfiledialogs.h"

int GSEngine::initialize(Renderer* renderer, sEngineConfiguration configuration)
{
    int error = Engine::initialize(renderer);

    if (error) return error;

    main_scene = new Scene("main_scene");


	return error;
}

int GSEngine::post_initialize()
{
    // Create skybox
    {
        MeshInstance3D* skybox = new Environment3D();
        main_scene->add_node(skybox);
    }

    {
        std::vector<Node*> entities;
        parse_ply("data/plys/ContainerCity.ply", entities);
        main_scene->add_nodes(entities);
    }

    // Create grid
    {
        MeshInstance3D* grid = new MeshInstance3D();
        grid->set_name("Grid");
        grid->add_surface(RendererStorage::get_surface("quad"));
        grid->set_position(glm::vec3(0.0f));
        grid->rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        grid->scale(glm::vec3(10.f));
        grid->set_frustum_culling_enabled(false);

        // NOTE: first set the transparency and all types BEFORE loading the shader
        //Material* grid_material = new Material();
        //grid_material->set_transparency_type(ALPHA_BLEND);
        //grid_material->set_cull_type(CULL_NONE);
        //grid_material->set_type(MATERIAL_UNLIT);
        //grid_material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_grid::source, shaders::mesh_grid::path, grid_material));
        //grid->set_surface_material_override(grid->get_surface(0), grid_material);

        //main_scene->add_node(grid);
    }

    return 0;
}

void GSEngine::clean()
{
    Engine::clean();
}

void GSEngine::update(float delta_time)
{
    Engine::update(delta_time);

    main_scene->update(delta_time);
}

void GSEngine::render()
{
#ifndef __EMSCRIPTEN__
    if (show_imgui) {
        render_gui();
    }
#endif

    main_scene->render();

    Engine::render();
}

void GSEngine::render_gui()
{
    render_default_gui();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open GS scene (.ply)"))
            {
                std::vector<const char*> filter_patterns = { "*.ply" };
                char const* open_file_name = tinyfd_openFileDialog(
                    "Ply loader",
                    "",
                    filter_patterns.size(),
                    filter_patterns.data(),
                    "Ply format",
                    0
                );

                if (open_file_name) {
                    std::vector<Node*> entities;
                    parse_ply(open_file_name, entities);
                    main_scene->add_nodes(entities);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
