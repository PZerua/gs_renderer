#pragma once

#include "includes.h"

#include "graphics/uniform.h"

#include "framework/nodes/node_3d.h"

struct sGSData {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 color;
    glm::vec3 scale;
    float opacity;
};

class GSNode : public Node3D {

    uint32_t gs_count = 0;
    WGPUBuffer gs_buffer = nullptr;

    Uniform model_uniform;
    WGPUBindGroup model_bindgroup;

public:

    bool is_skinned = false;

    GSNode();
    ~GSNode();

    virtual void render() override;
    virtual void update(float delta_time) override;

    void render_gui() override;

    WGPUBuffer get_gs_buffer() {
        return gs_buffer;
    }

    WGPUBindGroup get_model_bindgroup() {
        return model_bindgroup;
    }

    void create_gs_data_buffer(const std::vector<sGSData> gs_data);

    uint32_t get_gs_count();
    uint32_t get_gs_bytes_size();

};
