#include "helpers/SceneSerializer.h"
#include "scenes/Scene.h"
#include "core/Entity.h"
#include "core/LightManager.h"
#include "core/rendering/Material.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;

// Helper: Convert glm::vec3 to JSON array
static json Vec3ToJson(const glm::vec3& v)
{
    return json::array({v.x, v.y, v.z});
}

// Helper: Convert JSON array to glm::vec3
static glm::vec3 JsonToVec3(const json& j, const glm::vec3& defaultVal = glm::vec3(0.0f))
{
    if (!j.is_array() || j.size() < 3) return defaultVal;
    return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

// Serialize a single Entity's Transform
static json SerializeTransform(const Transform& t)
{
    json j;
    j["position"] = Vec3ToJson(t.position);
    j["rotation"] = Vec3ToJson(t.rotation);
    j["scale"]    = Vec3ToJson(t.scale);
    return j;
}

// Deserialize Transform from JSON
static void DeserializeTransform(Transform& t, const json& j)
{
    if (j.contains("position")) t.position = JsonToVec3(j["position"]);
    if (j.contains("rotation")) t.rotation = JsonToVec3(j["rotation"]);
    if (j.contains("scale"))    t.scale    = JsonToVec3(j["scale"], glm::vec3(1.0f));
}

// Serialize MaterialOverride (only the editable properties)
static json SerializeMaterial(const std::shared_ptr<Material>& mat)
{
    if (!mat) return json::object();
    
    json j;
    
    // Floats
    for (const auto& [key, val] : mat->floats)
    {
        j["floats"][key] = val;
    }
    
    // Vec3s (colors)
    for (const auto& [key, val] : mat->vec3s)
    {
        j["vec3s"][key] = Vec3ToJson(val);
    }
    
    // Bools
    for (const auto& [key, val] : mat->bools)
    {
        j["bools"][key] = val;
    }
    
    return j;
}

// Deserialize MaterialOverride from JSON
static void DeserializeMaterial(std::shared_ptr<Material>& mat, const json& j)
{
    if (j.empty()) return;
    if (!mat) mat = std::make_shared<Material>();
    
    if (j.contains("floats"))
    {
        for (auto& [key, val] : j["floats"].items())
        {
            mat->floats[key] = val.get<float>();
        }
    }
    
    if (j.contains("vec3s"))
    {
        for (auto& [key, val] : j["vec3s"].items())
        {
            mat->vec3s[key] = JsonToVec3(val);
        }
    }
    
    if (j.contains("bools"))
    {
        for (auto& [key, val] : j["bools"].items())
        {
            mat->bools[key] = val.get<bool>();
        }
    }
}

// Serialize a single Entity
static json SerializeEntity(const std::shared_ptr<Entity>& e)
{
    json j;
    j["name"] = e->name;
    j["transform"] = SerializeTransform(e->transform);
    
    // Only serialize materialOverride if it exists
    if (e->renderComp && e->renderComp->materialOverride)
    {
        j["materialOverride"] = SerializeMaterial(e->renderComp->materialOverride);
    }
    
    return j;
}

// Deserialize Entity data (applies to both new and existing entities)
static void DeserializeEntity(std::shared_ptr<Entity>& e, const json& j)
{
    if (j.contains("name")) e->name = j["name"].get<std::string>();
    if (j.contains("transform")) DeserializeTransform(e->transform, j["transform"]);
    
    if (j.contains("materialOverride") && e->renderComp)
    {
        DeserializeMaterial(e->renderComp->materialOverride, j["materialOverride"]);
    }
}

bool SceneSerializer::Serialize(Scene* scene, const std::string& filepath)
{
    if (!scene)
    {
        std::cerr << "[SceneSerializer] Error: Scene is null\n";
        return false;
    }
    
    json root;
    root["version"] = VERSION;
    
    // Scene metadata
    root["scene"]["name"] = scene->Name();
    root["scene"]["clearColor"] = Vec3ToJson(scene->clearColor);
    
    // Entities
    json entitiesArray = json::array();
    for (const auto& e : scene->GetEntities())
    {
        if (!e) continue;
        // Skip skybox entities (they're procedural, not serializable)
        if (e->skyboxComp) continue;
        
        entitiesArray.push_back(SerializeEntity(e));
    }
    root["entities"] = entitiesArray;
    
    // Ensure directory exists
    std::filesystem::path path(filepath);
    if (path.has_parent_path())
    {
        std::filesystem::create_directories(path.parent_path());
    }

    // Write to file
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[SceneSerializer] Error: Could not open file for writing: " << filepath << "\n";
        return false;
    }
    
    file << root.dump(2); // Pretty print with 2-space indent
    file.close();
    
    std::cout << "[SceneSerializer] Scene saved to: " << filepath << "\n";
    return true;
}

bool SceneSerializer::Deserialize(Scene* scene, const std::string& filepath)
{
    if (!scene)
    {
        std::cerr << "[SceneSerializer] Error: Scene is null\n";
        return false;
    }
    
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[SceneSerializer] Error: Could not open file for reading: " << filepath << "\n";
        return false;
    }
    
    json root;
    try
    {
        file >> root;
    }
    catch (const json::parse_error& e)
    {
        std::cerr << "[SceneSerializer] JSON parse error: " << e.what() << "\n";
        return false;
    }
    file.close();
    
    // Version check (for future compatibility)
    if (root.contains("version"))
    {
        std::string ver = root["version"].get<std::string>();
        if (ver != VERSION)
        {
            std::cout << "[SceneSerializer] Warning: File version " << ver 
                      << " differs from current " << VERSION << "\n";
        }
    }
    
    // Load scene metadata
    if (root.contains("scene"))
    {
        if (root["scene"].contains("clearColor"))
        {
            scene->clearColor = JsonToVec3(root["scene"]["clearColor"]);
        }
    }
    
    // Load entity data: Find existing or Create new
    if (root.contains("entities"))
    {
        for (const auto& entityJson : root["entities"])
        {
            if (!entityJson.contains("name")) continue;
            std::string name = entityJson["name"].get<std::string>();
            
            // 1. Try to find existing entity (for state restoration)
            std::shared_ptr<Entity> entity = scene->FindEntityByName(name);
            
            // 2. If not found, CREATE it (for level loading)
            if (!entity)
            {
                entity = scene->CreateEntity(name);
                std::cout << "[SceneSerializer] Spawning new entity: " << name << "\n";
            }
            
            // 3. Apply deserialized data
            DeserializeEntity(entity, entityJson);
        }
    }
    
    std::cout << "[SceneSerializer] Scene loaded from: " << filepath << "\n";
    return true;
}

// ============================================================================
// LIGHT SERIALIZATION
// ============================================================================

static json SerializePointLight(const PointLight& pl)
{
    json j;
    j["position"] = Vec3ToJson(pl.position);
    j["ambient"]  = Vec3ToJson(pl.ambient);
    j["diffuse"]  = Vec3ToJson(pl.diffuse);
    j["specular"] = Vec3ToJson(pl.specular);
    j["constant"] = pl.constant;
    j["linear"]   = pl.linear;
    j["quadratic"] = pl.quadratic;
    j["enabled"]  = pl.enabled;
    return j;
}

static void DeserializePointLight(PointLight& pl, const json& j)
{
    if (j.contains("position"))  pl.position  = JsonToVec3(j["position"]);
    if (j.contains("ambient"))   pl.ambient   = JsonToVec3(j["ambient"]);
    if (j.contains("diffuse"))   pl.diffuse   = JsonToVec3(j["diffuse"]);
    if (j.contains("specular"))  pl.specular  = JsonToVec3(j["specular"]);
    if (j.contains("constant"))  pl.constant  = j["constant"].get<float>();
    if (j.contains("linear"))    pl.linear    = j["linear"].get<float>();
    if (j.contains("quadratic")) pl.quadratic = j["quadratic"].get<float>();
    if (j.contains("enabled"))   pl.enabled   = j["enabled"].get<bool>();
}

// ============================================================================
// POST-PROCESSING SERIALIZATION
// ============================================================================

#include "core/postprocessing/PostProcessingPipeline.h"

static json SerializePostProcessing(PostProcessingPipeline* pipeline)
{
    json j;
    if (!pipeline) return j;

    j["bloomEnabled"] = pipeline->IsBloomEnabled();
    j["bloomIterations"] = pipeline->bloomIterations;

    json effectsArray = json::array();
    for (const auto& effect : pipeline->GetEffects())
    {
        json ej;
        ej["name"] = effect->name;
        ej["enabled"] = effect->enabled;
        ej["intensity"] = effect->intensity;
        effectsArray.push_back(ej);
    }
    j["effects"] = effectsArray;

    return j;
}

static void DeserializePostProcessing(PostProcessingPipeline* pipeline, const json& j)
{
    if (!pipeline || j.empty()) return;

    if (j.contains("bloomEnabled")) pipeline->IsBloomEnabled() = j["bloomEnabled"].get<bool>();
    if (j.contains("bloomIterations")) pipeline->bloomIterations = j["bloomIterations"].get<int>();

    if (j.contains("effects"))
    {
        auto& pipelineEffects = pipeline->GetEffects();
        for (const auto& effectJson : j["effects"])
        {
            if (!effectJson.contains("name")) continue;
            std::string name = effectJson["name"].get<std::string>();

            // Find matching effect in current pipeline
            for (auto& effect : pipelineEffects)
            {
                if (effect->name == name)
                {
                    if (effectJson.contains("enabled")) effect->enabled = effectJson["enabled"].get<bool>();
                    if (effectJson.contains("intensity")) effect->intensity = effectJson["intensity"].get<float>();
                    break;
                }
            }
        }
    }
}

static json SerializeLights(LightManager* lm)
{
    json j;
    
    // Directional Light
    j["directional"]["direction"] = Vec3ToJson(lm->GetDirLightDirection());
    j["directional"]["ambient"]   = Vec3ToJson(lm->GetDirLightAmbient());
    j["directional"]["diffuse"]   = Vec3ToJson(lm->GetDirLightDiffuse());
    j["directional"]["specular"]  = Vec3ToJson(lm->GetDirLightSpecular());
    
    // Point Lights
    json pointsArray = json::array();
    for (const auto& pl : lm->points)
    {
        pointsArray.push_back(SerializePointLight(pl));
    }
    j["points"] = pointsArray;
    
    return j;
}

static void DeserializeLights(LightManager* lm, const json& j)
{
    // Directional Light
    if (j.contains("directional"))
    {
        const auto& dir = j["directional"];
        if (dir.contains("direction")) lm->GetDirLightDirection() = JsonToVec3(dir["direction"]);
        if (dir.contains("ambient"))   lm->GetDirLightAmbient()   = JsonToVec3(dir["ambient"]);
        if (dir.contains("diffuse"))   lm->GetDirLightDiffuse()   = JsonToVec3(dir["diffuse"]);
        if (dir.contains("specular"))  lm->GetDirLightSpecular()  = JsonToVec3(dir["specular"]);
    }
    
    // Point Lights - CLEAR & REBUILD (not index matching!)
    if (j.contains("points"))
    {
        lm->ClearPointLights(); // 1. Wipe existing lights
        
        for (const auto& lightJson : j["points"])
        {
            PointLight pl;
            // Set sensible defaults
            pl.constant = 1.0f;
            pl.linear = 0.09f;
            pl.quadratic = 0.032f;
            
            DeserializePointLight(pl, lightJson);
            lm->AddPointLight(pl); // 2. Add new light from JSON
        }
    }
}

bool SceneSerializer::Serialize(Scene* scene, LightManager* lightManager, const std::string& filepath)
{
    if (!scene)
    {
        std::cerr << "[SceneSerializer] Error: Scene is null\n";
        return false;
    }
    
    json root;
    root["version"] = VERSION;
    
    // Scene metadata
    root["scene"]["name"] = scene->Name();
    root["scene"]["clearColor"] = Vec3ToJson(scene->clearColor);
    
    // Entities
    json entitiesArray = json::array();
    for (const auto& e : scene->GetEntities())
    {
        if (!e) continue;
        if (e->skyboxComp) continue;
        entitiesArray.push_back(SerializeEntity(e));
    }
    root["entities"] = entitiesArray;
    
    // Lights
    if (lightManager)
    {
        root["lights"] = SerializeLights(lightManager);
    }

    // Post-Processing
    if (scene->GetPostPipeline())
    {
        root["postProcessing"] = SerializePostProcessing(scene->GetPostPipeline());
    }
    
    // Ensure directory exists
    std::filesystem::path path(filepath);
    if (path.has_parent_path())
    {
        std::filesystem::create_directories(path.parent_path());
    }

    // Write to file
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[SceneSerializer] Error: Could not open file for writing: " << filepath << "\n";
        return false;
    }
    
    file << root.dump(2);
    file.close();
    
    std::cout << "[SceneSerializer] Scene + Lights saved to: " << filepath << "\n";
    return true;
}

bool SceneSerializer::Deserialize(Scene* scene, LightManager* lightManager, const std::string& filepath)
{
    if (!scene)
    {
        std::cerr << "[SceneSerializer] Error: Scene is null\n";
        return false;
    }
    
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[SceneSerializer] Error: Could not open file for reading: " << filepath << "\n";
        return false;
    }
    
    json root;
    try
    {
        file >> root;
    }
    catch (const json::parse_error& e)
    {
        std::cerr << "[SceneSerializer] JSON parse error: " << e.what() << "\n";
        return false;
    }
    file.close();
    
    // Version check
    if (root.contains("version"))
    {
        std::string ver = root["version"].get<std::string>();
        if (ver != VERSION)
        {
            std::cout << "[SceneSerializer] Warning: File version " << ver 
                      << " differs from current " << VERSION << "\n";
        }
    }
    
    // Load scene metadata
    if (root.contains("scene"))
    {
        if (root["scene"].contains("clearColor"))
        {
            scene->clearColor = JsonToVec3(root["scene"]["clearColor"]);
        }
    }
    
    // Load entity data
    if (root.contains("entities"))
    {
        for (const auto& entityJson : root["entities"])
        {
            if (!entityJson.contains("name")) continue;
            std::string name = entityJson["name"].get<std::string>();
            
            std::shared_ptr<Entity> entity = scene->FindEntityByName(name);
            if (!entity)
            {
                entity = scene->CreateEntity(name);
                std::cout << "[SceneSerializer] Spawning new entity: " << name << "\n";
            }
            DeserializeEntity(entity, entityJson);
        }
    }
    
    // Load lights
    if (lightManager && root.contains("lights"))
    {
        DeserializeLights(lightManager, root["lights"]);
    }

    // Load Post-Processing
    if (scene->GetPostPipeline() && root.contains("postProcessing"))
    {
        DeserializePostProcessing(scene->GetPostPipeline(), root["postProcessing"]);
    }
    
    std::cout << "[SceneSerializer] Scene + Lights loaded from: " << filepath << "\n";
    return true;
}
