#pragma once

#include <string>

class Scene {
public:
    virtual ~Scene() {}

    // called once when the scene is loaded
    virtual void init() = 0;

    // called every frame
    virtual void update(float deltaTime) = 0;

    // called every frame after update
    virtual void render() = 0;

    // optional: scene name
    virtual std::string name() const = 0;
};
