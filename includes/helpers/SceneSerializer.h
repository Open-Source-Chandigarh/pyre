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
    /**
     * @brief Serialize a scene's state to a JSON file.
     * @param scene The scene to serialize.
     * @param lightManager Optional light manager to serialize global lighting state.
     * @param filepath The destination file path.
     * @return true on success, false otherwise.
     */
    static bool Serialize(Scene* scene, LightManager* lightManager, const std::string& filepath);

    /**
     * @brief Deserialize a scene's state from a JSON file.
     * @param scene The scene to update or populate.
     * @param lightManager Optional light manager to restore global lighting state.
     * @param filepath The source file path.
     * @return true on success, false otherwise.
     */
    static bool Deserialize(Scene* scene, LightManager* lightManager, const std::string& filepath);

private:
    static constexpr const char* VERSION = "1.0";
};
