#include "application/Application.h"
#include "application/AppState.h"
#include "core/Window.h"
#include "core/InputManager.h"
#include "core/ResourceManager.h"
#include "scenes/Scene.h"
#include "scenes/FactoryScene.h"
#include "scenes/Backpack.h"
#include "scenes/Space.h"
#include "scenes/ToonScene.h"
#include "scenes/Test.h"
#include "helpers/SceneSerializer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

Application::Application(const std::string& title, int width, int height)
{
    window = std::make_unique<Window>(width, height, title);
    appState = std::make_unique<AppState>();
}

Application::~Application()
{
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    ResourceManager::Clear(); 
    
    // appState and window will be destroyed automatically by unique_ptr
    // appState first (because it was declared 2nd), then window
    // this is needed because AppState components might need GL context to delete buffers
}

void Application::Init()
{
    inputMapper.LoadConfig("resources/config/input.json");

    for (auto& scene : appState->scenes)
        scene->BindWindow(window.get());

    for (auto& scene : appState->scenes) 
        scene->Init(*appState);

    if (!appState->scenes.empty()) {
        appState->scenes[appState->currentSceneIndex]->OnActivate(*appState);
        appState->camera.SetDefault();
    }

    ConfigureInput();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window->GetNative(), true);
    ImGui_ImplOpenGL3_Init("#version 420");
    glfwSwapInterval(0);
}

void Application::ConfigureInput()
{
    InputManager *input = window->GetInputManager();
    AppState *app = appState.get();
    Window *winPtr = window.get();

    // camera Movement
    input->BindKeyContinuous(inputMapper.GetKey("MoveForward", GLFW_KEY_W), [app](float dt) { 
        app->camera.ProcessKeyboard(FORWARD, dt); 
    });
    input->BindKeyContinuous(inputMapper.GetKey("MoveBackward", GLFW_KEY_S), [app](float dt) { 
        app->camera.ProcessKeyboard(BACKWARD, dt); 
    });
    input->BindKeyContinuous(inputMapper.GetKey("MoveLeft", GLFW_KEY_A), [app](float dt) { 
        app->camera.ProcessKeyboard(LEFT, dt); 
    });
    input->BindKeyContinuous(inputMapper.GetKey("MoveRight", GLFW_KEY_D), [app](float dt) { 
        app->camera.ProcessKeyboard(RIGHT, dt); 
    });

    // mouse
    input->BindMouseMove([app](double x, double y) {
        app->camera.ProcessMouseMovement((float)x, (float)y);
    });
    input->BindScroll([app](double y) {
        app->camera.ProcessMouseScroll((float)y);
    });
    // Note: Mouse capture now uses RMB hold-to-look (handled in InputManager)

    // scene switching
    auto SwitchScene = [app, winPtr](int offset) {
        int n = (int)app->scenes.size();
        if (n == 0) return;
        app->currentSceneIndex = (app->currentSceneIndex + offset + n) % n;
        glfwSetWindowTitle(winPtr->GetNative(), app->scenes[app->currentSceneIndex]->Name().c_str());
        app->camera.Reset(); // Clear any extreme movement/rotation
        app->selectedEntity = nullptr; // Reset selection on scene change
        app->scenes[app->currentSceneIndex]->OnActivate(*app);
        app->camera.SetDefault(); // Lock in the scene's starting camera state
    };

    input->BindKeyEvent(GLFW_KEY_RIGHT, GLFW_RELEASE, 
        [SwitchScene](){ SwitchScene(1); });
    input->BindKeyEvent(GLFW_KEY_LEFT, GLFW_RELEASE, 
        [SwitchScene](){ SwitchScene(-1); });

    // camera reset
    input->BindKeyEvent(inputMapper.GetKey("ResetCamera", GLFW_KEY_R), GLFW_RELEASE, [app]() {
        app->camera.Reset();
    });

    // debug options
    input->BindKeyEvent(inputMapper.GetKey("ToggleWireframe", GLFW_KEY_F), GLFW_RELEASE, [app]() {
        app->wireframeEnabled = !app->wireframeEnabled;
        glPolygonMode(GL_FRONT_AND_BACK, 
            app->wireframeEnabled ? GL_LINE : GL_FILL);
    });
}

void Application::Update(float dt)
{
    // Update Input
    window->GetInputManager()->Update(dt);

    // Update Current Scene
    if (!appState->scenes.empty()) {
        appState->scenes[appState->currentSceneIndex]->Update(*appState);
    }
}

void Application::Render()
{
    if (appState->scenes.empty()) return;

    Scene *activeScene = appState->scenes[appState->currentSceneIndex].get();
    glm::vec3 col = activeScene->clearColor;

    glClearColor(col.r, col.g, col.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    activeScene->Render(*appState);
}

void Application::Run()
{
    Init();
    // Track resizing logic
    int lastW = window->Width();
    int lastH = window->Height();

    while (!window->ShouldClose())
    {
        // time step
        float currentFrame = static_cast<float>(glfwGetTime());
        appState->deltaTime = currentFrame - appState->lastFrame;
        appState->lastFrame = currentFrame;

        // handle Resize
        if (window->Width() != lastW || window->Height() != lastH) {
            lastW = window->Width();
            lastH = window->Height();
            glViewport(0, 0, lastW, lastH);
            appState->width = lastW;
            appState->height = lastH;
            for (auto& s : appState->scenes) s->OnResize(lastW, lastH);
        }

        Update(appState->deltaTime);
        Render();

        // Render UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        Scene* currentScene = appState->scenes[appState->currentSceneIndex].get();

        // Shortcuts
        if (ImGui::GetIO().KeyCtrl)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_S))
            {
                std::string filepath = "scenes/" + currentScene->Name() + ".json";
                SceneSerializer::Serialize(currentScene, appState->lightManager.get(), filepath);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_O))
            {
                std::string filepath = "scenes/" + currentScene->Name() + ".json";
                SceneSerializer::Deserialize(currentScene, appState->lightManager.get(), filepath);
            }
        }

       // 1. Scene Hierarchy Panel (positioned on the right side)
        float panelWidth = 280.0f;
        float panelHeight = 350.0f;
        float padding = 10.0f;
        
        ImGui::SetNextWindowPos(ImVec2(appState->width - panelWidth - padding, padding), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
        ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        
        static int selectedEntityIndex = -1;
        auto& sceneEntities = currentScene->GetEntities();

        // Display scene name header
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Scene: %s", currentScene->Name().c_str());
        
        // Save/Load buttons (includes entities + lights)
        if (ImGui::Button("Save"))
        {
            std::string filepath = "scenes/" + currentScene->Name() + ".json";
            SceneSerializer::Serialize(currentScene, appState->lightManager.get(), filepath);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            std::string filepath = "scenes/" + currentScene->Name() + ".json";
            SceneSerializer::Deserialize(currentScene, appState->lightManager.get(), filepath);
        }
        ImGui::Separator();

        for (int i = 0; i < sceneEntities.size(); i++)
        {
            ImGui::PushID(i);
            Entity* entity = sceneEntities[i].get();
            std::string nameText = entity->name.empty() ? "Entity #" + std::to_string(i) : entity->name;
            
            bool isSelected = (selectedEntityIndex == i);
            
            // selection highlighting with background color
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));        // Blue background when selected
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f)); // Lighter blue on hover
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));  // Darker blue when clicked
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));          // White text
            }

            if (ImGui::Selectable(nameText.c_str(), isSelected))
            {
                if (isSelected)
                {
                    selectedEntityIndex = -1;
                    appState->selectedEntity = nullptr;
                }
                else
                {
                    selectedEntityIndex = i;
                    appState->selectedEntity = sceneEntities[i];
                }
            }

            if (isSelected) {
                ImGui::PopStyleColor(4);
            }

            ImGui::PopID();
        }
        ImGui::End();

        // 2. Inspector Panel (positioned below hierarchy)
        float inspectorY = padding + panelHeight + padding;
        float inspectorHeight = appState->height - inspectorY - padding - 50.0f; // Leave space for stats
        
        ImGui::SetNextWindowPos(ImVec2(appState->width - panelWidth - padding, inspectorY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, inspectorHeight), ImGuiCond_Always);
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        
        if (selectedEntityIndex >= 0 && selectedEntityIndex < sceneEntities.size())
        {
            Entity* entity = sceneEntities[selectedEntityIndex].get();
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s", entity->name.c_str());
            ImGui::Separator();
            
            // Transform Component
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Position", &entity->transform.position.x, 0.1f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("World-space position (X, Y, Z)");
                
                ImGui::DragFloat3("Rotation", &entity->transform.rotation.x, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Euler rotation in degrees");
                
                ImGui::DragFloat3("Scale",    &entity->transform.scale.x,    0.05f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale multiplier per axis");
            }

            // Render Component (Material Editing)
            if (entity->renderComp && ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Material* mat = entity->GetUniqueMaterial().get();
                if (mat)
                {
                    // Initialize properties with sensible defaults if they don't exist
                    // (operator[] would create black/zero values otherwise)
                    if (mat->floats.find("material_shininess") == mat->floats.end())
                        mat->floats["material_shininess"] = 32.0f;
                    if (mat->vec3s.find("material_diffuseColor") == mat->vec3s.end())
                        mat->vec3s["material_diffuseColor"] = glm::vec3(1.0f); // White (neutral)
                    if (mat->vec3s.find("material_specularColor") == mat->vec3s.end())
                        mat->vec3s["material_specularColor"] = glm::vec3(1.0f); // White (neutral)

                    // Standard Properties
                    ImGui::DragFloat("Shininess", &mat->floats["material_shininess"], 1.0f, 1.0f, 256.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Specular exponent: Higher = smaller, sharper highlights");
                    
                    ImGui::ColorEdit3("Diffuse", &mat->vec3s["material_diffuseColor"].x);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Base color of the material");
                    
                    ImGui::ColorEdit3("Specular", &mat->vec3s["material_specularColor"].x);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color of specular reflections");
                    
                    // Toggles
                    bool wireframe = mat->GetBool("wireframe");
                    if (ImGui::Checkbox("Wireframe", &wireframe)) mat->SetWireframe(wireframe);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Render this specific object in wireframe mode");

                    bool shadows = mat->GetBool("castShadows", true);
                    if (ImGui::Checkbox("Cast Shadows", &shadows)) mat->SetShadows(shadows);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Whether this object casts shadows onto other surfaces");

                    // Outline / Bloom
                    bool outline = mat->GetBool("outlineEnabled");
                    if (ImGui::Checkbox("Outline / Glow", &outline)) mat->bools["outlineEnabled"] = outline;
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enable bloom outline effect for this object");
                    
                    if (outline)
                    {
                        ImGui::ColorEdit3("Glow Color", &mat->vec3s["outlineColor"].x);
                        ImGui::DragFloat("Bloom Factor", &mat->floats["bloomFactor"], 0.1f, 0.0f, 10.0f);
                    }

                    // Texture Preview & Management
                    if (ImGui::TreeNode("Textures"))
                    {
                        std::string keyToRemove; // Deferred removal to avoid iterator invalidation
                        
                        for (auto& [name, tex] : mat->textures)
                        {
                            if (!tex) continue;
                            if (tex->type == TextureType::TEX_CUBEMAP || tex->type == TextureType::TEX_ENVIRONMENT) continue;

                            ImGui::PushID(name.c_str());
                            
                            // Image Preview
                            ImGui::Image((void*)(intptr_t)tex->ID, ImVec2(48, 48), ImVec2(0, 1), ImVec2(1, 0));
                            
                            // Tooltip
                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::Text("ID: %u (%dx%d)", tex->ID, tex->width, tex->height);
                                ImGui::Image((void*)(intptr_t)tex->ID, ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
                                ImGui::EndTooltip();
                            }

                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", name.c_str());
                            ImGui::TextDisabled("%s", tex->path.c_str());
                            
                            // Remove button
                            if (ImGui::SmallButton("Remove"))
                            {
                                keyToRemove = name;
                            }
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove this texture from the material");
                            
                            ImGui::EndGroup();
                            
                            ImGui::PopID();
                            ImGui::Separator();
                        }
                        
                        // Deferred removal
                        if (!keyToRemove.empty())
                        {
                            mat->textures.erase(keyToRemove);
                        }

                        // Add/Replace Texture Logic
                        if (ImGui::Button("Load Texture...")) ImGui::OpenPopup("LoadTexturePopup");
                        
                        if (ImGui::BeginPopup("LoadTexturePopup"))
                        {
                            static char keyBuf[64] = "material_diffuse";
                            static char pathBuf[128] = "resources/textures/container2.png";
                            static int typeIdx = 0;
                            const char* typeNames[] = { "Diffuse", "Specular", "Normal", "Displacement" };
                            
                            ImGui::InputText("Slot (Key)", keyBuf, 64);
                            ImGui::InputText("File Path", pathBuf, 128);
                            ImGui::Combo("Type", &typeIdx, typeNames, 4);
                            
                            TextureType selectedType = TextureType::TEX_DIFFUSE;
                            if (typeIdx == 1) selectedType = TextureType::TEX_SPECULAR;
                            else if (typeIdx == 2) selectedType = TextureType::TEX_NORMAL;
                            else if (typeIdx == 3) selectedType = TextureType::TEX_DISPLACEMENT;
                            
                            if (ImGui::Button("Load & Assign"))
                            {
                                auto newTex = ResourceManager::LoadTexture(pathBuf, selectedType);
                                if (newTex) {
                                    mat->textures[keyBuf] = newTex;
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::TreePop();
                    }
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Select an entity to inspect.");
        }
        ImGui::End();

        // 4. Lights Panel
        ImGui::SetNextWindowPos(ImVec2(appState->width - panelWidth * 2.0f - padding * 2.0f, padding), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight + inspectorHeight + padding), ImGuiCond_Always);
        ImGui::Begin("Lighting", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        
        LightManager* lm = appState->lightManager.get();
        if (lm)
        {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Direction", &lm->GetDirLightDirection().x, 0.05f, -1.0f, 1.0f);
                ImGui::ColorEdit3("Ambient##Dir", &lm->GetDirLightAmbient().x);
                ImGui::ColorEdit3("Diffuse##Dir", &lm->GetDirLightDiffuse().x);
                ImGui::ColorEdit3("Specular##Dir", &lm->GetDirLightSpecular().x);
            }

            if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (int i = 0; i < lm->points.size(); i++)
                {
                    ImGui::PushID(i);
                    bool& enabled = lm->points[i].enabled;
                    
                    // Dim the tree node text if the light is disabled
                    if (!enabled) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    
                    std::string lightLabel = "Point Light " + std::to_string(i);
                    if (ImGui::TreeNode(lightLabel.c_str()))
                    {
                        if (!enabled) ImGui::PopStyleVar();

                        ImGui::Checkbox("Enabled", &enabled);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle this light on/off without deleting it");
                        
                        ImGui::DragFloat3("Position", &lm->points[i].position.x, 0.1f);
                        ImGui::ColorEdit3("Ambient", &lm->points[i].ambient.x);
                        ImGui::ColorEdit3("Diffuse", &lm->points[i].diffuse.x);
                        ImGui::ColorEdit3("Specular", &lm->points[i].specular.x);
                        
                        ImGui::Separator();
                        ImGui::Text("Attenuation");
                        ImGui::DragFloat("Constant", &lm->points[i].constant, 0.01f, 0.0f, 2.0f);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Base intensity (usually 1.0)");
                        
                        ImGui::DragFloat("Linear", &lm->points[i].linear, 0.001f, 0.0f, 1.0f);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How fast light fades over distance");
                        
                        ImGui::DragFloat("Quadratic", &lm->points[i].quadratic, 0.0001f, 0.0f, 1.0f);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How fast light fades over distance squared (realistic falloff)");
                        
                        ImGui::Separator();
                        if (ImGui::Button("Delete Light"))
                        {
                            lm->points.erase(lm->points.begin() + i);
                            ImGui::TreePop();
                            ImGui::PopID();
                            break;
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Permanently remove this light from the scene");

                        ImGui::TreePop();
                    }
                    else if (!enabled) 
                    {
                        ImGui::PopStyleVar();
                    }
                    ImGui::PopID();
                }

                if (ImGui::Button("+ Add Point Light"))
                {
                    PointLight pl;
                    pl.position = appState->camera.Position + appState->camera.Front * 2.0f;
                    pl.diffuse = glm::vec3(1.0f);
                    pl.ambient = glm::vec3(0.1f);
                    pl.specular = glm::vec3(1.0f);
                    pl.constant = 1.0f;
                    pl.linear = 0.09f;
                    pl.quadratic = 0.032f;
                    lm->AddPointLight(pl);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Spawn a new light in front of the camera");
            }
        }
        ImGui::End();

        // Performance Stats (bottom-left corner)
        ImGui::SetNextWindowPos(ImVec2(padding, appState->height - 40.0f), ImGuiCond_Always);
        ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        ImGui::End();

        // 3. Global Settings Panel (positioned at top-left)
        ImGui::SetNextWindowPos(ImVec2(padding, padding), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_Always);
        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Wireframe Mode (F)", &appState->wireframeEnabled))
            {
                glPolygonMode(GL_FRONT_AND_BACK, appState->wireframeEnabled ? GL_LINE : GL_FILL);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Global toggle for wireframe rendering");

            Renderer* renderer = appState->renderer.get();
            if (renderer)
            {
                ImGui::Checkbox("Show Vertex Normals", &renderer->showNormals);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Visualize surface normals as yellow lines (Bypasses post-processing)");
                
                ImGui::Checkbox("Force Outlines", &renderer->forceOutlines);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enable selection glow for all objects globally");

                ImGui::Separator();
                
                // MSAA Toggle (Informational / Toggle bit)
                static bool msaa = true;
                if (ImGui::Checkbox("Anti-Aliasing (MSAA)", &msaa))
                {
                    if (msaa) glEnable(GL_MULTISAMPLE);
                    else glDisable(GL_MULTISAMPLE);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Smooth jagged edges using hardware multisampling. (Applies to main scene FBO)");
            }
        }

        if (ImGui::CollapsingHeader("Post-Processing", ImGuiTreeNodeFlags_DefaultOpen))
        {
            PostProcessingPipeline* pipeline = currentScene->GetPostPipeline();
            if (pipeline)
            {
                ImGui::Checkbox("Bloom", &pipeline->IsBloomEnabled());
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enable high-intensity glow effect for bright surfaces");
                
                if (pipeline->IsBloomEnabled())
                {
                    ImGui::Indent();
                    ImGui::SliderInt("Intensity (Blur)", &pipeline->bloomIterations, 1, 20);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Number of blur passes. Higher = smoother, wider glow (impacts performance)");
                    ImGui::Unindent();
                }

                ImGui::Separator();

                auto& effects = pipeline->GetEffects();
                for (auto& effect : effects)
                {
                    ImGui::PushID(effect->name.c_str());
                    ImGui::Checkbox(effect->name.c_str(), &effect->enabled);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip(("Toggle " + effect->name + " effect").c_str());
                    
                    if (effect->enabled)
                    {
                        ImGui::Indent();
                        if (effect->name == "Tone Mapping") {
                            ImGui::SliderFloat("Exposure", &effect->intensity, 0.1f, 5.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Overall brightness of the scene (HDR)");
                        }
                        else if (effect->name == "Gamma Correction") {
                            ImGui::SliderFloat("Gamma", &effect->intensity, 1.0f, 3.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Brightness curve adjustment (usually 2.2)");
                        }
                        else if (effect->name == "Sharpen") {
                            ImGui::SliderFloat("Strength", &effect->intensity, 0.0f, 5.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Edge enhancement strength");
                        }
                        ImGui::Unindent();
                    }
                    ImGui::PopID();
                }
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window->SwapBuffers();
        window->PollEvents();
    }
}

void Application::AddScene(Scene *scene)
{
    // take ownership convert raw pointer to unique_ptr
    appState->scenes.push_back(std::unique_ptr<Scene>(scene));
}