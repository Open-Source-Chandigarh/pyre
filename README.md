# Pyre – Just Another Graphics Engine

**Pyre** is a lightweight graphics engine built with **OpenGL, GLFW, GLAD, GLM, and Assimp**, inspired by the concepts and techniques taught in the [LearnOpenGL](https://learnopengl.com/) book.
It aims to provide a clear, modular, and educational implementation of modern rendering techniques, making it a great project for learning and experimenting with real-time graphics.

<div align="center">
  <img src="resources/textures/icon.png" alt="Pyre Logo" width="200"/>
</div>

---

## ✨ Features

* **Core Rendering**: Modern OpenGL pipeline with GLFW for window/input, GLAD for loading, and GLM for math.
* **Lighting System**: Fully functional **Phong lighting model** with **directional, point, and spotlights**.
* **Texture Mapping**: Supports **diffuse** and **specular maps** for realistic materials.
* **Model Loading**: Integrated **Assimp** support for loading external 3D models (OBJ, FBX, etc.).
* **Primitives**: Built-in generation of cubes, spheres, planes, and more through a geometry factory.
* **Sandbox Demo Scene**: Walk freely using **WASD**, look around with the **mouse**, toggle **wireframe mode (F)**, **reset camera (R)**, and **switch scenes (arrow keys)**.
* **Scene System**: Modular architecture allowing multiple demos/scenes to be easily loaded and extended.
* **Educational Focus**: Developed step-by-step alongside LearnOpenGL concepts for clarity and understanding.

> 🕐 **Note:** When you first run Pyre, the screen may remain black for a few seconds (5–10s) while assets and shaders load — this is normal.

---

## 📂 Folder Structure

```
Pyre/
├── includes/                 # Header files and third-party includes
├── libs/                     # External libraries (GLFW, GLAD, Assimp, etc.)
├── resources/                # Assets used by the engine
│   ├── models/               # 3D model files (e.g., backpack.obj)
│   ├── textures/             # Diffuse/specular/normal maps and icons
│   └── shaders/              # GLSL shader files
│       ├── modularFragmentShader.fs
│       └── modularVertexShader.vs
├── src/                      # Core source code
│   ├── core/
│   │   ├── rendering/
│   │   │   ├── geometry/     # Procedural geometry generators
│   │   │   │   └── GeometryFactory.cpp
│   │   │   ├── Mesh.cpp
│   │   │   ├── Model.cpp
│   │   │   └── Renderer.cpp
│   │   ├── Entity.cpp
│   │   ├── InputManager.cpp
│   │   ├── LightManager.cpp
│   │   ├── ResourceManager.cpp
│   │   └── Window.cpp
│   ├── helpers/
│   │   └── shaderClass.cpp   # Shader compilation/loading utilities
│   ├── Scenes/               # Demo/test scenes
│   │   ├── backpack.cpp
│   │   └── factoryScene.cpp
│   └── main.cpp              # Entry point
└── Pyre.sln                  # Visual Studio solution file
```

---

## 🔧 Installation

Clone the repository:

```bash
git clone https://github.com/Open-Source-Chandigarh/pyre.git
cd pyre
```

---

## 🚀 Usage

1. Open the project in **Visual Studio**.
2. Double-click the **Pyre.sln** file.
3. Select **Build > Build Solution** or press `Ctrl + Shift + B`.
4. Make sure configuration is set to **Debug** or **Release** as needed.
5. Run **main.cpp**:

   * `F5` → Start debugging
   * `Ctrl + F5` → Run without debugging

You’ll start in a **sandbox scene** where you can walk around, toggle rendering modes, and explore the engine.

---

## 🤝 Contributing

Contributions, bug reports, and suggestions are welcome!
If you’d like to contribute, please read the [CONTRIBUTING.md](https://github.com/Open-Source-Chandigarh/pyre?tab=contributing-ov-file).

---

## 📚 Learning Resource

This project is being developed alongside the **LearnOpenGL** guide, serving as a practical, code-based implementation of its concepts.

---

## 🙌 Credits

* **LearnOpenGL by Joey de Vries** – Primary educational resource inspiring this engine.
* **OpenGL, GLFW, GLAD, GLM, Assimp** – Core libraries powering Pyre.

---

## 📄 License

This project is licensed under the **MIT License** – see the LICENSE file for details.
