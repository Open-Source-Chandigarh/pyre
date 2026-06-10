#include "helpers/SceneSerializer.h"
#include "core/Entity.h"
#include "core/LightManager.h"
#include "core/rendering/Material.h"
#include "scenes/Scene.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

#include "core/ResourceManager.h"

using json = nlohmann::json;

// TextureType <-> string conversion for serialization
static std::string TextureTypeToString(TextureType type)
{
    switch (type)
    {
    case TextureType::TEX_DIFFUSE:
        return "diffuse";
    case TextureType::TEX_SPECULAR:
        return "specular";
    case TextureType::TEX_NORMAL:
        return "normal";
    case TextureType::TEX_DISPLACEMENT:
        return "displacement";
    case TextureType::TEX_CUBEMAP:
        return "cubemap";
    case TextureType::TEX_ENVIRONMENT:
        return "environment";
    default:
        return "other";
    }
}

static TextureType StringToTextureType(const std::string &str)
{
    if (str == "diffuse")
        return TextureType::TEX_DIFFUSE;
    if (str == "specular")
        return TextureType::TEX_SPECULAR;
    if (str == "normal")
        return TextureType::TEX_NORMAL;
    if (str == "displacement")
        return TextureType::TEX_DISPLACEMENT;
    if (str == "cubemap")
        return TextureType::TEX_CUBEMAP;
    if (str == "environment")
        return TextureType::TEX_ENVIRONMENT;
    return TextureType::Other;
}

// Helper: Convert glm::vec3 to JSON array
static json Vec3ToJson(const glm::vec3 &v)
{
    return json::array({v.x, v.y, v.z});
}

// Helper: Convert JSON array to glm::vec3
static glm::vec3 JsonToVec3(const json &j, const glm::vec3 &defaultVal = glm::vec3(0.0f))
{
    if (!j.is_array() || j.size() < 3)
        return defaultVal;
    return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

// Serialize a single Entity's Transform
static json SerializeTransform(const Transform &t)
{
    json j;
    j["position"] = Vec3ToJson(t.position);
    j["rotation"] = Vec3ToJson(t.rotation);
    j["scale"] = Vec3ToJson(t.scale);
    return j;
}

// Deserialize Transform from JSON
static void DeserializeTransform(Transform &t, const json &j)
{
    if (j.contains("position"))
        t.position = JsonToVec3(j["position"]);
    if (j.contains("rotation"))
        t.rotation = JsonToVec3(j["rotation"]);
    if (j.contains("scale"))
        t.scale = JsonToVec3(j["scale"], glm::vec3(1.0f));
}

// CullMode <-> string conversion
static std::string CullModeToString(CullMode mode)
{
    switch (mode)
    {
    case CullMode::Front:
        return "front";
    case CullMode::None:
        return "none";
    default:
        return "back";
    }
}

static CullMode StringToCullMode(const std::string &str)
{
    if (str == "front")
        return CullMode::Front;
    if (str == "none")
        return CullMode::None;
    return CullMode::Back;
}

// Serialize MaterialOverride (only the editable properties)
static json SerializeMaterial(const std::shared_ptr<Material> &mat)
{
    if (!mat)
        return json::object();

    json j;

    // Floats
    for (const auto &[key, val] : mat->floats)
    {
        j["floats"][key] = val;
    }

    // Vec3s (colors)
    for (const auto &[key, val] : mat->vec3s)
    {
        j["vec3s"][key] = Vec3ToJson(val);
    }

    // Bools
    for (const auto &[key, val] : mat->bools)
    {
        j["bools"][key] = val;
    }

    // CullMode and transparency
    j["cullMode"] = CullModeToString(mat->cullMode);
    j["isTransparent"] = mat->isTransparent;

    // Textures (save path + type so they can be reloaded)
    // Always serialize textures array (even if empty) to handle removals correctly
    json texArray = json::array();
    for (const auto &[key, tex] : mat->textures)
    {
        json texJson;
        texJson["key"] = key;

        if (!tex)
        {
            texJson["path"] = "";
            texJson["type"] = "none";
        }
        else
        {
            // Skip cubemaps/environment maps (they are procedural, not file-based)
            if (tex->type == TextureType::TEX_CUBEMAP || tex->type == TextureType::TEX_ENVIRONMENT)
                continue;
            if (tex->path.empty())
                continue; // Can't serialize without a path

            texJson["path"] = tex->path;
            texJson["type"] = TextureTypeToString(tex->type);
        }
        texArray.push_back(texJson);
    }
    j["textures"] = texArray; // Always include textures array to handle removals

    return j;
}

// Deserialize MaterialOverride from JSON
static void DeserializeMaterial(std::shared_ptr<Material> &mat, const json &j)
{
    if (j.empty())
        return;
    if (!mat)
        mat = std::make_shared<Material>();

    if (j.contains("floats"))
    {
        for (auto &[key, val] : j["floats"].items())
        {
            mat->floats[key] = val.get<float>();
        }
    }

    if (j.contains("vec3s"))
    {
        for (auto &[key, val] : j["vec3s"].items())
        {
            mat->vec3s[key] = JsonToVec3(val);
        }
    }

    if (j.contains("bools"))
    {
        for (auto &[key, val] : j["bools"].items())
        {
            mat->bools[key] = val.get<bool>();
        }
    }

    // CullMode and transparency
    if (j.contains("cullMode"))
        mat->cullMode = StringToCullMode(j["cullMode"].get<std::string>());
    if (j.contains("isTransparent"))
        mat->isTransparent = j["isTransparent"].get<bool>();

    // Textures: reload from file paths
    if (j.contains("textures"))
    {
        // Clear existing serializable textures to handle removals correctly
        // Guard against null texture pointers to prevent crash
        auto it = mat->textures.begin();
        while (it != mat->textures.end())
        {
            if (!it->second ||
                (it->second->type != TextureType::TEX_CUBEMAP && it->second->type != TextureType::TEX_ENVIRONMENT))
                it = mat->textures.erase(it);
            else
                ++it;
        }

        for (const auto &texJson : j["textures"])
        {
            if (!texJson.contains("key") || !texJson.contains("path") || !texJson.contains("type"))
                continue;

            std::string key = texJson["key"].get<std::string>();
            std::string path = texJson["path"].get<std::string>();

            if (path.empty())
            {
                mat->textures[key] = nullptr;
                continue;
            }

            TextureType type = StringToTextureType(texJson["type"].get<std::string>());

            auto tex = ResourceManager::LoadTexture(path, type);
            if (tex)
            {
                mat->textures[key] = tex;
                std::cout << "[SceneSerializer] Loaded texture: " << key << " <- " << path << "\n";
            }
            else
            {
                std::cerr << "[SceneSerializer] Failed to load texture: " << path << "\n";
            }
        }
    }
}

// Serialize a single Entity
static json SerializeEntity(const std::shared_ptr<Entity> &e)
{
    json j;
    j["name"] = e->name;
    j["transform"] = SerializeTransform(e->transform);

    // Only serialize materialOverride if it exists
    if (e->renderComp)
    {
        if (e->renderComp->materialOverride)
            j["materialOverride"] = SerializeMaterial(e->renderComp->materialOverride);

        json baseMatsArray = json::array();
        for (const auto &node : e->renderComp->nodes)
        {
            if (node.baseMaterial)
                baseMatsArray.push_back(SerializeMaterial(node.baseMaterial));
            else
                baseMatsArray.push_back(json::object());
        }
        j["baseMaterials"] = baseMatsArray;
    }

    return j;
}

// Deserialize Entity data (applies to both new and existing entities)
static void DeserializeEntity(std::shared_ptr<Entity> &e, const json &j)
{
    if (j.contains("name"))
        e->name = j["name"].get<std::string>();
    if (j.contains("transform"))
        DeserializeTransform(e->transform, j["transform"]);

    if (e->renderComp)
    {
        if (j.contains("materialOverride"))
        {
            e->GetOverrideMaterial();
            DeserializeMaterial(e->renderComp->materialOverride, j["materialOverride"]);
        }

        if (j.contains("baseMaterials"))
        {
            auto baseMatsArray = j["baseMaterials"];
            for (size_t i = 0; i < baseMatsArray.size() && i < e->renderComp->nodes.size(); i++)
            {
                DeserializeMaterial(e->renderComp->nodes[i].baseMaterial, baseMatsArray[i]);
            }
        }
    }
}

// ============================================================================
// LIGHT SERIALIZATION
// ============================================================================

static json SerializePointLight(const PointLight &pl)
{
    json j;
    j["position"] = Vec3ToJson(pl.position);
    j["ambient"] = Vec3ToJson(pl.ambient);
    j["diffuse"] = Vec3ToJson(pl.diffuse);
    j["specular"] = Vec3ToJson(pl.specular);
    j["constant"] = pl.constant;
    j["linear"] = pl.linear;
    j["quadratic"] = pl.quadratic;
    j["enabled"] = pl.enabled;
    return j;
}

static void DeserializePointLight(PointLight &pl, const json &j)
{
    if (j.contains("position"))
        pl.position = JsonToVec3(j["position"]);
    if (j.contains("ambient"))
        pl.ambient = JsonToVec3(j["ambient"]);
    if (j.contains("diffuse"))
        pl.diffuse = JsonToVec3(j["diffuse"]);
    if (j.contains("specular"))
        pl.specular = JsonToVec3(j["specular"]);
    if (j.contains("constant"))
        pl.constant = j["constant"].get<float>();
    if (j.contains("linear"))
        pl.linear = j["linear"].get<float>();
    if (j.contains("quadratic"))
        pl.quadratic = j["quadratic"].get<float>();
    if (j.contains("enabled"))
        pl.enabled = j["enabled"].get<bool>();
}

// ============================================================================
// POST-PROCESSING SERIALIZATION
// ============================================================================

#include "core/postprocessing/PostProcessingPipeline.h"

static json SerializePostProcessing(PostProcessingPipeline *pipeline)
{
    json j;
    if (!pipeline)
        return j;

    j["bloomEnabled"] = pipeline->IsBloomEnabled();
    j["bloomIterations"] = pipeline->bloomIterations;

    json effectsArray = json::array();
    for (const auto &effect : pipeline->GetEffects())
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

static void DeserializePostProcessing(PostProcessingPipeline *pipeline, const json &j)
{
    if (!pipeline || j.empty())
        return;

    if (j.contains("bloomEnabled"))
        pipeline->IsBloomEnabled() = j["bloomEnabled"].get<bool>();
    if (j.contains("bloomIterations"))
        pipeline->bloomIterations = j["bloomIterations"].get<int>();

    if (j.contains("effects"))
    {
        auto &pipelineEffects = pipeline->GetEffects();
        for (const auto &effectJson : j["effects"])
        {
            if (!effectJson.contains("name"))
                continue;
            std::string name = effectJson["name"].get<std::string>();

            // Find matching effect in current pipeline
            for (auto &effect : pipelineEffects)
            {
                if (effect->name == name)
                {
                    if (effectJson.contains("enabled"))
                        effect->enabled = effectJson["enabled"].get<bool>();
                    if (effectJson.contains("intensity"))
                        effect->intensity = effectJson["intensity"].get<float>();
                    break;
                }
            }
        }
    }
}

static json SerializeSpotLight(const SpotLight &sl)
{
    json j;
    j["position"] = Vec3ToJson(sl.position);
    j["direction"] = Vec3ToJson(sl.direction);
    j["ambient"] = Vec3ToJson(sl.ambient);
    j["diffuse"] = Vec3ToJson(sl.diffuse);
    j["specular"] = Vec3ToJson(sl.specular);
    j["constant"] = sl.constant;
    j["linear"] = sl.linear;
    j["quadratic"] = sl.quadratic;
    j["innerCutOff"] = sl.innerCutOff;
    j["outerCutOff"] = sl.outerCutOff;
    j["enabled"] = sl.enabled;
    return j;
}

static void DeserializeSpotLight(SpotLight &sl, const json &j)
{
    if (j.contains("position"))
        sl.position = JsonToVec3(j["position"]);
    if (j.contains("direction"))
        sl.direction = JsonToVec3(j["direction"]);
    if (j.contains("ambient"))
        sl.ambient = JsonToVec3(j["ambient"]);
    if (j.contains("diffuse"))
        sl.diffuse = JsonToVec3(j["diffuse"]);
    if (j.contains("specular"))
        sl.specular = JsonToVec3(j["specular"]);
    if (j.contains("constant"))
        sl.constant = j["constant"].get<float>();
    if (j.contains("linear"))
        sl.linear = j["linear"].get<float>();
    if (j.contains("quadratic"))
        sl.quadratic = j["quadratic"].get<float>();
    if (j.contains("innerCutOff"))
        sl.innerCutOff = j["innerCutOff"].get<float>();
    if (j.contains("outerCutOff"))
        sl.outerCutOff = j["outerCutOff"].get<float>();
    if (j.contains("enabled"))
        sl.enabled = j["enabled"].get<bool>();
}

static json SerializeLights(LightManager *lm)
{
    json j;

    // Directional Light
    j["directional"]["direction"] = Vec3ToJson(lm->GetDirLightDirection());
    j["directional"]["ambient"] = Vec3ToJson(lm->GetDirLightAmbient());
    j["directional"]["diffuse"] = Vec3ToJson(lm->GetDirLightDiffuse());
    j["directional"]["specular"] = Vec3ToJson(lm->GetDirLightSpecular());

    // Point Lights
    json pointsArray = json::array();
    for (const auto &pl : lm->points)
    {
        pointsArray.push_back(SerializePointLight(pl));
    }
    j["points"] = pointsArray;

    // Spot Lights
    json spotsArray = json::array();
    for (const auto &sl : lm->spots)
    {
        spotsArray.push_back(SerializeSpotLight(sl));
    }
    j["spots"] = spotsArray;

    return j;
}

static void DeserializeLights(LightManager *lm, const json &j)
{
    // Directional Light
    if (j.contains("directional"))
    {
        const auto &dir = j["directional"];
        if (dir.contains("direction"))
            lm->GetDirLightDirection() = JsonToVec3(dir["direction"]);
        if (dir.contains("ambient"))
            lm->GetDirLightAmbient() = JsonToVec3(dir["ambient"]);
        if (dir.contains("diffuse"))
            lm->GetDirLightDiffuse() = JsonToVec3(dir["diffuse"]);
        if (dir.contains("specular"))
            lm->GetDirLightSpecular() = JsonToVec3(dir["specular"]);
    }

    // Point Lights - CLEAR & REBUILD (not index matching!)
    if (j.contains("points"))
    {
        lm->ClearPointLights(); // 1. Wipe existing lights

        for (const auto &lightJson : j["points"])
        {
            PointLight pl;
            pl.constant = 1.0f;
            pl.linear = 0.09f;
            pl.quadratic = 0.032f;

            DeserializePointLight(pl, lightJson);
            lm->AddPointLight(pl); // 2. Add new light from JSON
        }
    }

    // Spot Lights - CLEAR & REBUILD
    if (j.contains("spots"))
    {
        lm->ClearSpotLights();

        for (const auto &lightJson : j["spots"])
        {
            SpotLight sl;
            DeserializeSpotLight(sl, lightJson);
            lm->AddSpotLight(sl);
        }
    }
}

bool SceneSerializer::Serialize(Scene *scene, LightManager *lightManager, const std::string &filepath)
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
    for (const auto &e : scene->GetEntities())
    {
        if (!e)
            continue;
        if (e->skyboxComp)
            continue;
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

bool SceneSerializer::Deserialize(Scene *scene, LightManager *lightManager, const std::string &filepath)
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
    catch (const json::parse_error &e)
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
            std::cout << "[SceneSerializer] Warning: File version " << ver << " differs from current " << VERSION
                      << "\n";
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
        // Track which entities have been updated to handle duplicate names (e.g. multiple "Cube" objects)
        std::set<Entity *> consumedEntities;

        for (const auto &entityJson : root["entities"])
        {
            if (!entityJson.contains("name"))
                continue;
            std::string name = entityJson["name"].get<std::string>();

            // Find first entity that matches name AND hasn't been consumed yet
            std::shared_ptr<Entity> entity = nullptr;
            for (auto &e : scene->GetEntities())
            {
                if (e && e->name == name && consumedEntities.find(e.get()) == consumedEntities.end())
                {
                    entity = e;
                    break;
                }
            }

            if (!entity)
            {
                entity = scene->CreateEntity(name);
                std::cout << "[SceneSerializer] Spawning new entity: " << name << "\n";
            }

            consumedEntities.insert(entity.get());
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
