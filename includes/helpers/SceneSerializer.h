#pragma once
#include <string>
#include <memory>

class Scene;
class LightManager;

/**
 * @brief Handles JSON serialization for Scenes and Lights.
 * Format Version: 1.0
 */
class SceneSerializer
{
public:
    // --- Save ---
    static bool Serialize(Scene* scene, const std::string& filepath);
    static bool Serialize(Scene* scene, LightManager* lightManager, const std::string& filepath);

    // --- Load ---
    static bool Deserialize(Scene* scene, const std::string& filepath);
    static bool Deserialize(Scene* scene, LightManager* lightManager, const std::string& filepath);

private:
    static constexpr const char* VERSION = "1.0";
};
