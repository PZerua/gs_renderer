#pragma once

#include "engine/engine.h"

class GSEngine : public Engine {

public:

	int initialize(Renderer* renderer, sEngineConfiguration configuration = {}) override;
    void clean() override;

	void update(float delta_time) override;
	void render() override;

    void render_gui();
};
