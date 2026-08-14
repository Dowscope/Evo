#pragma once

#include "systems/System.hpp"

#include "rendering/Scene.hpp"

class RenderSystem : public System {
public:
    using System::System;
    ~RenderSystem() override = default;

    virtual void render(const Scene& scene) = 0;
};
