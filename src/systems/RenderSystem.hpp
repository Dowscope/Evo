#pragma once

#include "systems/System.hpp"

class RenderSystem : public System {
public:
    using System::System;
    ~RenderSystem() override = default;

    virtual void render() = 0;
};
