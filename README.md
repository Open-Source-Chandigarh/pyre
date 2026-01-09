<div align="center">
  <img src="resources/textures/icon.png" alt="Pyre Logo" width="220"/>
  <h1>Pyre Graphics Engine</h1>

  <p>
    <strong>A high-performance, modular 3D rendering engine architecture built with Modern OpenGL 4.6 and C++20.</strong>
  </p>

  <p>
    <a href="#visual-showcase">Showcase</a> •
    <a href="#technical-features">Features</a> •
    <a href="#build--installation">Build</a> •
    <a href="#controls">Controls</a> •
    <a href="LICENSE">License</a>
  </p>

  ![Badge](https://img.shields.io/badge/Language-C%2B%2B20-00599C?style=for-the-badge&logo=c%2B%2B)
  ![Badge](https://img.shields.io/badge/API-OpenGL%204.6-5586A4?style=for-the-badge&logo=opengl)
  ![Badge](https://img.shields.io/badge/Build-CMake-064F8C?style=for-the-badge&logo=cmake)
  ![Badge](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)
</div>

<br />

Pyre is a research-oriented graphics engine designed to explore advanced real-time rendering techniques. It implements a modern deferred-style architecture within a forward renderer, utilizing Uniform Buffer Objects (UBOs) for efficient data transport and Geometry Shaders for complex shadow generation.

The project features a self-contained CMake build system that automatically manages dependencies including GLFW, Assimp, and GLAD, ensuring a streamlined cross-platform compilation process.

---

## Visual Showcase

| **Omni-Directional Point Shadows** | **Cascaded Shadow Maps (CSM)** |
| :---: | :---: |
| <img src="resources/gifs/point shadows.gif" alt="Point Shadows Demo" width="100%"> | <img src="resources/gifs/csm.gif" alt="CSM Demo" width="100%"> |
| *Dynamic omni-directional shadows using Geometry Shaders.* | *High-resolution directional shadows with cascade splits.* |

| **Environment Mapping Reflections** | **Non-Photorealistic Rendering (Toon)** |
| :---: | :---: |
| <img src="resources/screenshots/Environment mapping reflections.jpg" alt="Reflections Demo" width="100%"> | <img src="resources/screenshots/toon scene.png" alt="Toon Shading Demo" width="100%"> |
| *Real-time reflections using skybox environment mapping.* | *Stylized rendering with discretized lighting bands and rim highlights.* |

| **Hardware Instancing (1M+ Entities)** | **Post-Processing Pipeline** |
| :---: | :---: |
| <img src="resources/screenshots/moon.png" alt="Instancing Demo" width="100%"> | <img src="resources/screenshots/post_inversion.png" alt="Post Processing Demo" width="100%"> |
| *High-throughput rendering using vertex attribute divisors.* | *Framebuffer-based effects chain (Inversion).* |

---

## Technical Features

### Rendering Pipeline
* **Advanced Shadow Mapping:**
    * **Cascaded Shadow Maps (CSM):** Implemented for directional lights to maintain high-resolution shadows across large view distances using texture arrays.
    * **Omnidirectional Shadow Mapping:** Utilizes Geometry Shaders to clone geometry onto Cubemap Arrays in a single pass, significantly reducing CPU draw call overhead.
    * **Percentage-Closer Filtering (PCF):** Applied to both shadow techniques for soft-edge filtering.
* **Modern Data Architecture:** Extensive use of **Uniform Buffer Objects (UBOs)** with strict `std140` memory layout for efficient global state management (Camera, Lights, Shadow Matrices).
* **Hardware Instancing:** Optimized rendering path for high-density scenes (e.g., asteroid belts) capable of pushing millions of polygons at interactive framerates.
* **Geometry Shaders:** Utilized for point light layer selection and debug visualizations (vertex normals).

### Engine Architecture
* **Entity-Component System:** Flexible object composition using `Entity`, `MeshComponent`, `ModelComponent`, and `SkyboxComponent`.
* **Modular GLSL Preprocessor:** Custom shader compilation pipeline supporting `#include` directives to modularize lighting and UBO definitions.
* **Asset Management:** Centralized `ResourceManager` providing thread-safe loading, caching, and reference counting for textures, models, and shaders.
* **Scene Management:** Abstract scene system allowing for hot-swapping between different rendering environments and logic states.
* **Post-Processing Stack:** Extensible `PostProcessingPipeline` class managing ping-pong framebuffers for screen-space effects.

---

## Prerequisites

Ensure the following tools are available in your environment:

* **C++20 Compliant Compiler** (MSVC 19.28+, GCC 10+, or Clang 10+)
* **CMake 3.16** or higher
* **Git**

*Note: Dependencies such as GLFW, Assimp, GLM, and GLAD are fetched automatically via CMake.*

---

## Build & Installation

### Option A – Visual Studio 2022 (Recommended)
1.  Clone the repository.
2.  Open Visual Studio.
3.  Select **Open a Local Folder** and target the `pyre` directory.
4.  Allow CMake to configure the project cache.
5.  Select **Pyre.exe** as the startup target and run.

### Option B – Command Line

```bash
# 1. Clone the repository
git clone [https://github.com/Open-Source-Chandigarh/pyre.git](https://github.com/Open-Source-Chandigarh/pyre.git)
cd pyre

# 2. Generate build files
mkdir build && cd build
cmake ..

# 3. Compile
cmake --build . --config Debug


### **Run the Engine**



```bash

# Windows

.\/Debug\/Pyre.exe



# Linux / macOS

./Pyre

```



---



## Controls



```

Key             Action

W, A, S, D       Move Camera

Mouse            Look Around

Scroll           Adjust FOV

Right Arrow      Next Scene

Left Arrow       Previous Scene

F                Toggle Wireframe

R                Reset Camera

ESC              Lock/Unlock Mouse

```



---



## 📂 Project Structure



```

Pyre/

├── CMakeLists.txt            # Build configuration

├── .gitignore                # Ignore build artifacts

├── src/                      # Engine source

│   ├── main.cpp              # Entry point & game loop

│   ├── core/                 # Engine internals

│   │   ├── rendering/        # Renderer, Mesh, Model, FBO

│   │   │   └── geometry/     # Procedural shapes (Cube, Sphere, Torus)

│   │   ├── Window.cpp        # GLFW window wrapper

│   │   └── InputManager.cpp  # Input processing

│   └── scenes/               # Example scenes

├── includes/                 # Public headers

│   ├── core/                 # Engine interfaces

│   ├── scenes/               # Scene declarations

│   └── thirdparty/           # GLAD, stb_image, etc.

├── shaders/                  # GLSL programs

│   ├── modularVertex.vs      # Vertex uber-shader

│   ├── modularFragment.fs    # Lighting uber-shader

│   ├── includes/             # Shared GLSL modules

│   └── postprocessing/       # Screen effects

└── resources/                # Assets

    ├── models/               # OBJ files

    └── textures/             # Diffuse/specular maps, skyboxes

```



---



## Contributing



Want to implement **Shadow Mapping**, **Normal Mapping**, or optimizations?



```bash

git checkout -b feature/AmazingFeature

# implement your changes

git commit -m "Add AmazingFeature"

git push origin feature/AmazingFeature

```



Then open a **Pull Request**.



If you’d like to contribute, please read the [CONTRIBUTING.md](https://github.com/Open-Source-Chandigarh/pyre?tab=contributing-ov-file).



---



## Credits



* **Joey de Vries** – Creator of LearnOpenGL

* **GLFW, GLAD, GLM, Assimp, stb_image** – The tech stack powering Pyre



---



## 📄 License



Licensed under the **MIT License**. See the `LICENSE` file for details.
