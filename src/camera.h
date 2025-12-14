#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Valores padrão da câmera
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  5.0f;  
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

class Camera
{
public:
    // Atributos da câmera
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // Ângulos de Euler
    float Yaw;
    float Pitch;
    

    // Construtor com vetores
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = YAW, 
           float pitch = PITCH) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
          MovementSpeed(SPEED), 
          MouseSensitivity(SENSITIVITY), 
          Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // Retorna a matriz view calculada usando ângulos de Euler e a LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Faz a câmera seguir um alvo em terceira pessoa
    void FollowTarget(glm::vec3 targetPosition, float targetYaw, float distance, float height, float yawOffset = 0.0f, float pitchOffset = 0.0f)
    {
        float yawRad = glm::radians(targetYaw);
        
        
        glm::vec3 backward;
        backward.x = sin(yawRad);  
        backward.y = 0.0f;
        backward.z = cos(yawRad); 
        
        // Posicionar câmera atrás e levemente acima do alvo
        Position = targetPosition + backward * distance + glm::vec3(0.0f, height, 0.0f);
        
        // Calcular para onde a câmera deve olhar (alvo + offsets do mouse)
        glm::vec3 toTarget = glm::normalize(targetPosition - Position);
        
        // Calcular os ângulos base para olhar para o alvo
        float baseYaw = glm::degrees(atan2(toTarget.z, toTarget.x)) - 90.0f;
        float basePitch = glm::degrees(asin(toTarget.y));
        
        // Aplicar offsets do mouse
        Yaw = baseYaw + yawOffset;
        Pitch = basePitch + pitchOffset;
        
        // Limitar pitch para evitar problemas
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
        
        // Atualizar vetores da câmera
        updateCameraVectors();
    }

private:
    // Calcula o vetor frontal a partir dos ângulos de Euler (atualizados) da câmera
    void updateCameraVectors()
    {
        // Calcular o novo vetor Front
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        
        // Recalcular também os vetores Right e Up
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
