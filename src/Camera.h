#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Classe responsável pela câmera sintética.
class Camera
{
public:
    // Posição inicial da câmera.
    glm::vec3 position = glm::vec3(0.0f, 0.70f, 4.0f);

    // Orientação em graus.
    float yaw = -90.0f;
    float pitch = -5.0f;

    // Parâmetros da projeção.
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    // Calcula a direção para onde a câmera está olhando.
    glm::vec3 getFront() const
    {
        glm::vec3 front;

        front.x =
            cos(glm::radians(yaw)) *
            cos(glm::radians(pitch));

        front.y =
            sin(glm::radians(pitch));

        front.z =
            sin(glm::radians(yaw)) *
            cos(glm::radians(pitch));

        return glm::normalize(front);
    }
};

#endif
