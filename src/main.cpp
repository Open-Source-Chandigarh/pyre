#define STB_IMAGE_IMPLEMENTATION

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include "Helpers/shaderClass.h"
#include "Helpers/camera.h"

#include <iostream>
#include <vector>

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
unsigned int loadTexture(const char* path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool mouseCaptured = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// rotation
float rotationSpeed = 50.0f;
float rotationAngle = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
glm::vec3 lightColor(1.0f); // 

// background color options
std::vector<glm::vec3> bgColors = {
    glm::vec3(0.1f, 0.1f, 0.1f), // dark gray
    glm::vec3(0.0f, 0.0f, 0.0f), // black
    glm::vec3(0.0f, 0.0f, 1.0f), // blue
    glm::vec3(1.0f, 0.0f, 0.0f)  // red
};
static int colorIndex = 0;

// global wireframe state (toggled by key callback)
static bool g_wireframe = false;

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Graphics Engine", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // new: key callback to reliably detect single key presses (edge triggered)
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to capture our mouse
    // capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

     // ensure default fill mode initially
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader lightCubeShader(
        "shaders\\cubeLightVertexShader.vs",
        "shaders\\cubeLightFragmentShader.fs");

    Shader lightShader(
        "shaders\\ligthningVertexShader.vs",
        "shaders\\lightningFragmentShader.fs"
    );
    // cube vertex data
    float vertices[] = {
        // Positions          // Normals           // Texture Coords
        -0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,  0.0f,0.0f,
         0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,  1.0f,0.0f,
         0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,  1.0f,1.0f,
         0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,  1.0f,1.0f,
        -0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,  0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,  0.0f,0.0f,

        -0.5f,-0.5f, 0.5f,  0.0f,0.0f,1.0f,  0.0f,0.0f,
         0.5f,-0.5f, 0.5f,  0.0f,0.0f,1.0f,  1.0f,0.0f,
         0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,  1.0f,1.0f,
         0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,  1.0f,1.0f,
        -0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,  0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,  0.0f,0.0f,1.0f,  0.0f,0.0f,

        -0.5f, 0.5f, 0.5f,-1.0f,0.0f,0.0f,  1.0f,0.0f,
        -0.5f, 0.5f,-0.5f,-1.0f,0.0f,0.0f,  1.0f,1.0f,
        -0.5f,-0.5f,-0.5f,-1.0f,0.0f,0.0f,  0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,-1.0f,0.0f,0.0f,  0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,-1.0f,0.0f,0.0f,  0.0f,0.0f,
        -0.5f, 0.5f, 0.5f,-1.0f,0.0f,0.0f,  1.0f,0.0f,

         0.5f, 0.5f, 0.5f,1.0f,0.0f,0.0f, 1.0f,0.0f,
         0.5f, 0.5f,-0.5f,1.0f,0.0f,0.0f, 1.0f,1.0f,
         0.5f,-0.5f,-0.5f,1.0f,0.0f,0.0f, 0.0f,1.0f,
         0.5f,-0.5f,-0.5f,1.0f,0.0f,0.0f, 0.0f,1.0f,
         0.5f,-0.5f, 0.5f,1.0f,0.0f,0.0f, 0.0f,0.0f,
         0.5f, 0.5f, 0.5f,1.0f,0.0f,0.0f, 1.0f,0.0f,

        -0.5f,-0.5f,-0.5f,0.0f,-1.0f,0.0f,0.0f,1.0f,
         0.5f,-0.5f,-0.5f,0.0f,-1.0f,0.0f,1.0f,1.0f,
         0.5f,-0.5f, 0.5f,0.0f,-1.0f,0.0f,1.0f,0.0f,
         0.5f,-0.5f, 0.5f,0.0f,-1.0f,0.0f,1.0f,0.0f,
        -0.5f,-0.5f, 0.5f,0.0f,-1.0f,0.0f,0.0f,0.0f,
        -0.5f,-0.5f,-0.5f,0.0f,-1.0f,0.0f,0.0f,1.0f,

        -0.5f, 0.5f,-0.5f,0.0f,1.0f,0.0f,0.0f,1.0f,
         0.5f, 0.5f,-0.5f,0.0f,1.0f,0.0f,1.0f,1.0f,
         0.5f, 0.5f, 0.5f,0.0f,1.0f,0.0f,1.0f,0.0f,
         0.5f, 0.5f, 0.5f,0.0f,1.0f,0.0f,1.0f,0.0f,
        -0.5f, 0.5f, 0.5f,0.0f,1.0f,0.0f,0.0f,0.0f,
        -0.5f, 0.5f,-0.5f,0.0f,1.0f,0.0f,0.0f,1.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // tex coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // light cube VAO
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // textures
    unsigned int diffuseMap = loadTexture("resources\\container3.png");
    unsigned int specularMap = loadTexture("resources\\container3Spec.png");

    lightShader.use();
    lightShader.setInt("material.diffuse", 0);
    lightShader.setInt("material.specular", 1);
    lightShader.setFloat("material.shininess", 32.0f);
    lightShader.setVec3("lightPos", lightPos);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // clear screen with selected background color
        glm::vec3 color = bgColors[colorIndex];
        glClearColor(color.r, color.g, color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // animate light color using global lightColor
        float time = static_cast<float>(glfwGetTime());
        lightColor.r = sin(time * 2.0f) * 0.5f + 0.5f;
        lightColor.g = sin(time * 0.7f) * 0.5f + 0.5f;
        lightColor.b = sin(time * 1.3f) * 0.5f + 0.5f;

        // use light shader
        lightShader.use();
        lightShader.setVec3("lightCubeColor", lightColor);

        glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
        glm::vec3 ambientColor = lightColor * glm::vec3(0.2f);
        lightShader.setVec3("light.ambient", ambientColor);
        lightShader.setVec3("light.diffuse", diffuseColor);
        lightShader.setVec3("light.specular", lightColor);
        lightShader.setVec3("lightPos", lightPos);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);

        // rotation
        rotationAngle += rotationSpeed * deltaTime;
        glm::mat4 model = glm::rotate(glm::mat4(1.0f),
            glm::radians(rotationAngle),
            glm::vec3(0.0f, 1.0f, 0.0f));
        lightShader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        // render cube
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // render light cube
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        model = glm::translate(glm::mat4(1.0f), lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        lightCubeShader.setMat4("model", model);
        lightCubeShader.setVec3("lightCubeColor", lightColor);

        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        lightPos.x = sin(glfwGetTime());
        lightPos.z = cos(glfwGetTime());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

// --- texture loader ---
unsigned int loadTexture(const char* path)
{
    unsigned int texture;
    glGenTextures(1, &texture);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = (nrChannels == 1 ? GL_RED : (nrChannels == 3 ? GL_RGB : GL_RGBA));
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
        std::cerr << "\033[31m Failed to load texture \033[0m" << std::endl;

    stbi_image_free(data);
    return texture;
}

// --- input processing ---
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
        rotationSpeed += 10.0f;
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
        rotationSpeed -= 10.0f;
    if (rotationSpeed < 0.0f) rotationSpeed = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    static bool bKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bKeyPressed)
    {
        colorIndex = (colorIndex + 1) % bgColors.size();
        bKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
        bKeyPressed = false;

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        camera.Reset();
        std::cout << "Camera reset!\n";
    }

}
  // Key callback: toggles wireframe <-> fill on F key press (edge triggered)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
        std::cout << "Wireframe mode: " << (g_wireframe ? "ON" : "OFF") << std::endl;
    }

    // Allow ESC to release mouse even if used via key callback: keep previous behavior too
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        if (mouseCaptured)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouseCaptured = false;
        }
    }
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// --- callbacks --
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !mouseCaptured)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
        firstMouse = true;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!mouseCaptured) return;
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}