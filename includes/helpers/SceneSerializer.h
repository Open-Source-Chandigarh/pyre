#pragma once
#include <string>
#include <memory>

class Scene;

/**
 * @brief Handles saving and loading scenes to/from JSON files.
 * 
 * This class follows the "Dedicated Serializer" pattern, keeping
 * serialization logic separate from the Entity/Scene classes.
 * 
 * JSON Format Version: 1.0
 * - Entities: name, transform, materialOverride
 * - Lights: directional + point lights
 * - Scene: name, clearColor
 */
class SceneSerializer
{
public:
    /**
     * @brief Serialize a scene to a JSON file.
     * @param scene The scene to serialize
     * @param filepath Path to save the JSON file (e.g., "scenes/myScene.json")
     * @return true if successful, false otherwise
     */
    static bool Serialize(Scene* scene, const std::string& filepath);

    /**
     * @brief Deserialize a JSON file into a scene.
     * @param scene The scene to populate (should be initialized)
     * @param filepath Path to the JSON file to load
     * @return true if successful, false otherwise
     */
    static bool Deserialize(Scene* scene, const std::string& filepath);

private:
    static constexpr const char* VERSION = "1.0";
};
