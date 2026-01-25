#include <iostream>
#include "scenes/test.h"
#include "core/Camera.h"

class TestScene {
    private:
        float printInterval = 2.0f;
        float timeSinceLastPrint = 0.0f;
        bool printCameraEnabled= true;
        Camera* camera = nullptr;
    public:
        explicit TestScene(Camera* cam)
        : camera(cam) {}
        void Update(float deltaTime) {
            if (!printCameraEnabled || !camera) {
                return;
            }

            timeSinceLastPrint += deltaTime;
            if (timeSinceLastPrint >= printInterval) {
                PrintCameraInfo();
                timeSinceLastPrint = -printInterval;
            }
        }
        void PrintCameraInfo() const{
            const glm::vec3& pos = camera->GetPosition();
            const glm::vec3& dir = camera->GetFront();
            std::cout << "Camera Position: ("
                      << pos.x << ", "
                      << pos.y << ", "
                      << pos.z << ")\n";
            std::cout << "Camera Direction: ( "
                      << dir.x << ", "
                      << dir.y << ",  "
                      << dir.z << ")\n";
        }
        void ToggleCameraDebug(){
            printCameraEnabled = !printCameraEnabled;
        }
}