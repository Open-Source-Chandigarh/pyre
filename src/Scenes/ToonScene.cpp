#include "scenes/ToonScene.h"
#include "core/rendering/GlobalUBO.h"
#include "core/ResourceManager.h"
#include "core/rendering/geometry/GeometryFactory.h"

ToonScene::ToonScene(Window& win) : win(win), toonShader(nullptr)
{

}

void ToonScene::init()
{
	CreateGlobalUBO();

	toonShader = ResourceManager::LoadShader("toon", "shaders/modularVertexShader.vs", "shaders/toon.fs");

	cube = GeometryFactory::CreateCube();
	sphere = GeometryFactory::CreateSphere();
	torus = GeometryFactory::CreateTorus();

	{
		std::shared_ptr<Entity> e = Entity::Create();
		e->type = Entity::Type::Mesh;
		e->meshRenderer.mesh = &cube;
		e->meshRenderer.shader = toonShader;
		auto mat = std::make_shared<Material>();
		mat->vec3s["material_diffuseColor"] = glm::vec3(0.9f, 0.2f, 0.1f);
		mat->floats["material_shininess"] = 32.0f;
		mat->outlineEnabled = true;
		mat->outlineColor = glm::vec3(0.0f, 0.0f, 0.0f); // Black outline
		e->meshRenderer.material = mat;

		e->transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
		e->transform.scale = glm::vec3(1.0f);

		entities.push_back(e);
	}

	{
		std::shared_ptr<Entity> e = Entity::Create();
		e->type = Entity::Type::Mesh;
		e->meshRenderer.mesh = &sphere;
		e->meshRenderer.shader = toonShader;
		auto mat = std::make_shared<Material>();
		mat->vec3s["material_diffuseColor"] = glm::vec3(1.5f, 1.5f, 0.0f);
		mat->floats["material_shininess"] = 32.0f;
		mat->outlineEnabled = true;
		mat->outlineColor = glm::vec3(0.0f, 0.0f, 0.0f); // Black outline
		e->meshRenderer.material = mat;

		e->transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
		e->transform.scale = glm::vec3(0.7f);

		entities.push_back(e);
	}

	{
		std::shared_ptr<Entity> e = Entity::Create();
		e->type = Entity::Type::Mesh;
		e->meshRenderer.mesh = &torus;
		e->meshRenderer.shader = toonShader;
		auto mat = std::make_shared<Material>();
		mat->vec3s["material_diffuseColor"] = glm::vec3(0.1f, 0.2f, 0.8f);
		mat->floats["material_shininess"] = 32.0f;
		mat->outlineEnabled = true;
		mat->outlineColor = glm::vec3(0.0f, 0.0f, 0.0f); // Black outline
		e->meshRenderer.material = mat;

		e->transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
		e->transform.scale = glm::vec3(0.7f);

		entities.push_back(e);
	}

	// -------------------------
// LIGHTS
// -------------------------
	lightManager.ClearPointLights();

	lightManager.SetDirectional(
		glm::vec3(-0.5f, -1.0f, -0.3f),
		glm::vec3(0.04f),
		glm::vec3(0.55f),
		glm::vec3(0.7f)
	);

	PointLight k;
	k.position = glm::vec3(1.5f, 2, 1.5f);
	k.ambient = glm::vec3(0.03f);
	k.diffuse = glm::vec3(1);
	k.specular = glm::vec3(0.5f);
	k.constant = 1; k.linear = 0.09f; k.quadratic = 0.032f;
	lightManager.AddPointLight(k);

	PointLight s;
	s.position = glm::vec3(-3.0f, 1.5f, 0.0f);
	s.ambient = glm::vec3(0.03f);
	s.diffuse = glm::vec3(0.8f, 0.9f, 0.2f);
	s.specular = glm::vec3(0.2f);
	s.constant = 1; s.linear = 0.09f; s.quadratic = 0.032f;
	lightManager.AddPointLight(s);

	PointLight f;
	f.position = glm::vec3(-1, 2, 1);
	f.ambient = glm::vec3(0.04f);
	f.diffuse = glm::vec3(0.7, 0.4, 0.1);
	f.specular = glm::vec3(0.4f);
	f.constant = 1; f.linear = 0.14f; f.quadratic = 0.07f;
	lightManager.AddPointLight(f);

	PointLight r;
	r.position = glm::vec3(-1, 2, -2);
	r.ambient = glm::vec3(0.01f);
	r.diffuse = glm::vec3(0.2, 0.5, 0.4);
	r.specular = glm::vec3(0.2f);
	r.constant = 1; r.linear = 0.09f; r.quadratic = 0.032f;
	lightManager.AddPointLight(r);
}

void ToonScene::update()
{
	glClearColor(0.5, 0.8, 0.9, 1.0);
}

void ToonScene::render()
{
	auto app = win.GetAppState();
	if (!app) return;

	glm::mat4 view = app->camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(
		glm::radians(app->camera.Zoom),
		(float)win.Width() / win.Height(),
		0.1f, 100.0f
	);

	renderer.BeginScene(view, proj, app->camera.Position);

	// Camera-driven spotlight if you use one
	if (!lightManager.spots.empty())
	{
		lightManager.spots[0].position = app->camera.Position;
		lightManager.spots[0].direction = app->camera.Front;
	}

	// Upload global UBO (lights + camera)
	lightManager.UploadToUBO(view, proj, app->camera.Position);

	// Draw everything
	renderer.RenderScene(entities, app->camera);

	renderer.EndScene();
}