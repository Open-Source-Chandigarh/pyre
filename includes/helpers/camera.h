#pragma once

#include <thirdparty/glad/glad.h>
#include <thirdparty/glm/glm.hpp>
#include <thirdparty/glm/gtc/matrix_transform.hpp>

#include "../core/Constants.h"

// Defines several possible options for camera movement
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// An abstract camera class
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // euler Angles
    float Yaw;
    float Pitch;

    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    float Near;
    float Far;

    // constructor with vectors
    Camera(
        glm::vec3 position = CameraDefaults::POSITION,
        glm::vec3 up       = CameraDefaults::UP,
        float yaw          = CameraDefaults::YAW,
        float pitch        = CameraDefaults::PITCH
    )
        : Front(CameraDefaults::FRONT),
          MovementSpeed(CameraDefaults::SPEED),
          MouseSensitivity(CameraDefaults::SENSITIVITY),
          Zoom(CameraDefaults::ZOOM),
          Near(CameraDefaults::NEAR_PLANE),
          Far(CameraDefaults::FAR_PLANE)
    {
        Position = position;
        WorldUp  = up;
        Yaw      = yaw;
        Pitch    = pitch;
        updateCameraVectors();
    }

    // constructor with scalar values
    Camera(
        float posX, float posY, float posZ,
        float upX,  float upY,  float upZ,
        float yaw,  float pitch
    )
        : Front(CameraDefaults::FRONT),
          MovementSpeed(CameraDefaults::SPEED),
          MouseSensitivity(CameraDefaults::SENSITIVITY),
          Zoom(CameraDefaults::ZOOM),
          Near(CameraDefaults::NEAR_PLANE),
          Far(CameraDefaults::FAR_PLANE)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp  = glm::vec3(upX, upY, upZ);
        Yaw      = yaw;
        Pitch    = pitch;
        updateCameraVectors();
    }

    void ResetProjection()
    {
        Zoom = CameraDefaults::ZOOM;
        Near = CameraDefaults::NEAR_PLANE;
        Far  = CameraDefaults::FAR_PLANE;
    }

    void Reset()
    {
        Position = CameraDefaults::POSITION;
        Front    = CameraDefaults::FRONT;
        Up       = CameraDefaults::UP;
        Yaw      = CameraDefaults::YAW;
        Pitch    = CameraDefaults::PITCH;
        Zoom     = CameraDefaults::ZOOM;

        updateCameraVectors();
        ResetProjection();
    }

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)  Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT)     Position -= Right * velocity;
        if (direction == RIGHT)    Position += Right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        if (constrainPitch)
        {
            if (Pitch > 89.0f)  Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)  Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }

private:
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};
