#include "scenes/Scene.h"
#include "core/rendering/Framebuffer.h"
#include "core/postprocessing/PostProcessingPipeline.h"

Scene::Scene() {}
Scene::~Scene() {}

void Scene::Render(AppState &appState)
{
    Renderer *renderer = appState.renderer.get();
    
    renderer->BeginScene(appState.camera, 
                         *appState.lightManager, 
                         appState.GetAspectRatio());

    renderer->RenderScene(entities, 
                          appState.camera, 
                          *appState.lightManager, 
                          *sceneFBO, 
                          *postPipeline, 
                          appState.wireframeEnabled, 
                          this->clearColor);

    renderer->EndScene();
}

void Scene::BindWindow(Window *window)
{
    this->win = window;
    sceneFBO = std::make_unique<Framebuffer>(
        (unsigned int)win->Width(), (unsigned int)win->Height(), true, true);
        
    postPipeline = std::make_unique<PostProcessingPipeline>(
        (unsigned int)win->Width(), (unsigned int)win->Height());
    
    postPipeline->AddGammaCorrection(2.2f);
}

void Scene::OnResize(int w, int h) 
{
    if (sceneFBO) sceneFBO->Resize((unsigned int)w, (unsigned int)h);
    if (postPipeline) postPipeline->Resize((unsigned int)w, (unsigned int)h);
}