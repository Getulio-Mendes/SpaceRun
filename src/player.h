#ifndef PLAYER_H
#define PLAYER_H

#include "libs/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "engine/model.h"
#include "engine/shader.h"
#include "engine/primitives.h"

class Player {
public:
    // Spaceship state
    glm::vec3 Position;
    glm::vec3 Rotation; // pitch, yaw, roll
    glm::vec3 Velocity;
    glm::vec3 AngularVelocity;

    // Physics constants
    float Acceleration;
    float MaxSpeed;
    float Friction;
    float RotationAcceleration;
    float MaxRotationSpeed;
    float RotationFriction;

    // Camera settings
    float CameraDistance;
    float CameraHeight;
    float CameraYawOffset;
    float CameraPitchOffset;
    float MouseSensitivity;

    // Hitbox settings
    glm::vec3 HitboxSize;
    float ShieldScaleMultiplier;
    
    // Game constraints
    float CorridorWidth;
    int Lives;
    float InvulnerabilityTimer;

    // Model adjustments
    glm::vec3 ModelScale;
    glm::vec3 ModelRotationCorrection;

    // Constructor
    Player(glm::vec3 startPos = glm::vec3(0.0f)) 
        : Position(startPos), 
          Rotation(0.0f, 90.0f, 0.0f), 
          Velocity(0.0f), 
          AngularVelocity(0.0f),
          Acceleration(35.0f),
          MaxSpeed(200.0f),
          Friction(2.0f),
          RotationAcceleration(200.0f),
          MaxRotationSpeed(180.0f),
          RotationFriction(3.0f),
          CameraDistance(8.0f),
          CameraHeight(2.0f),
          CameraYawOffset(90.0f),
          CameraPitchOffset(0.0f),
          MouseSensitivity(0.30f),
          HitboxSize(1.0f, 0.4f, 1.0f),
          ShieldScaleMultiplier(1.3f),
          CorridorWidth(40.0f),
          Lives(3),
          InvulnerabilityTimer(0.0f),
          ModelScale(0.001f), // Adjusted scale
          ModelRotationCorrection(0.0f, -90.0f, 0.0f) // Adjusted rotation
    {
    }

    void ProcessInput(GLFWwindow* window, float deltaTime) {
        // W/S - Move para frente/trás
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            glm::vec3 forward = GetForwardVector();
            Velocity += forward * Acceleration * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            glm::vec3 forward = GetForwardVector();
            Velocity -= forward * Acceleration * deltaTime;
        }
        
        // A/D - Rotaciona para esquerda/direita (Yaw)
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            AngularVelocity.y += RotationAcceleration * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            AngularVelocity.y -= RotationAcceleration * deltaTime;
        
        // Q/E - Rotaciona para cima/baixo (Pitch)
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            AngularVelocity.x += RotationAcceleration * 3.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            AngularVelocity.x -= RotationAcceleration * 3.0f * deltaTime;
        
        // Z/X - Roll
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            AngularVelocity.z -= RotationAcceleration * 3.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            AngularVelocity.z += RotationAcceleration * 3.0f * deltaTime;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;
        
        CameraYawOffset += xoffset;
        CameraPitchOffset += yoffset;
        
        if (CameraPitchOffset > 45.0f)
            CameraPitchOffset = 45.0f;
        if (CameraPitchOffset < -45.0f)
            CameraPitchOffset = -45.0f;
    }

    void ProcessScroll(float yoffset) {
        CameraDistance -= (float)yoffset * 0.5f;
        if (CameraDistance < 2.0f)
            CameraDistance = 2.0f;
        if (CameraDistance > 12.0f)
            CameraDistance = 12.0f;
    }

    void Update(float deltaTime, Camera& camera) {
        // Update Invulnerability Timer
        if (InvulnerabilityTimer > 0.0f) {
            InvulnerabilityTimer -= deltaTime;
        }

        // Physics Update
        Position += Velocity * deltaTime;
        Rotation += AngularVelocity * deltaTime;

        // Apply friction
        Velocity -= Velocity * Friction * deltaTime;
        AngularVelocity -= AngularVelocity * RotationFriction * deltaTime;

        // Clamp velocity
        if (glm::length(Velocity) > MaxSpeed) {
            Velocity = glm::normalize(Velocity) * MaxSpeed;
        }
        // Clamp angular velocity
        if (glm::length(AngularVelocity) > MaxRotationSpeed) {
            AngularVelocity = glm::normalize(AngularVelocity) * MaxRotationSpeed;
        }

        // Constraint Z axis (Corridor)
        if (Position.z > CorridorWidth) {
            Position.z = CorridorWidth;
            if (Velocity.z > 0) Velocity.z = 0.0f; // Stop velocity against wall
        }
        if (Position.z < -CorridorWidth) {
            Position.z = -CorridorWidth;
            if (Velocity.z < 0) Velocity.z = 0.0f; // Stop velocity against wall
        }

        // Update camera
        camera.FollowTarget(Position, Rotation.y, CameraDistance, CameraHeight, CameraYawOffset, CameraPitchOffset);
    }

    void SetSpotlight(Shader& shader) {
        glm::vec3 forward = GetForwardVector();
        shader.setVec3("spotLight.position", Position + forward * 1.5f);
        shader.setVec3("spotLight.direction", forward);
        shader.setVec3("spotLight.ambient", 2.0f, 1.0f, 1.0f);
        shader.setVec3("spotLight.diffuse", 2.5f, 1.5f, 1.5f);
        shader.setVec3("spotLight.specular", 2.0f, 1.0f, 1.0f);
        shader.setFloat("spotLight.constant", 1.0f);
        shader.setFloat("spotLight.linear", 0.0014f);
        shader.setFloat("spotLight.quadratic", 0.000007f);
        shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
    }

    void Draw(Shader& shader, Model& model) {
        shader.setMat4("model", GetModelMatrix());
        model.Draw(shader);
        
        // Update spotlight position and direction
        shader.setVec3("spotLight.position", Position + GetForwardVector() * 8.0f);
        shader.setVec3("spotLight.direction", GetForwardVector());
    }

    glm::mat4 GetHitboxModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, Position);
        model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        
        model = glm::scale(model, HitboxSize * ShieldScaleMultiplier);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.4f));
        return model;
    }

    void DrawHitbox(Shader& shader, glm::vec3 viewPos, float time) {
        shader.use();
        
        // Enable Blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Disable Depth Mask: The shield is transparent, so we don't want it 
        // writing to the depth buffer and hiding things behind it.
        glDepthMask(GL_FALSE);
        
        // Enable Back-face culling to see only the front of the shield
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Use the original hitbox model matrix to match the shape
        shader.setMat4("model", GetHitboxModelMatrix());
        shader.setVec3("viewPos", viewPos);
        shader.setFloat("time", time);
        shader.setBool("isInvulnerable", InvulnerabilityTimer > 0.0f);

        renderSphere();

        // Restore OpenGL states
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
    }

    void DrawEngines(Shader& shader, float time) {
        // Calculate thrust level based on Velocity magnitude relative to MaxSpeed
        float thrust = (glm::length(Velocity) / MaxSpeed) * 3.0f;
        thrust = glm::clamp(thrust, 0.0f, 1.0f); 

        if (thrust < 0.1f) return;

        shader.use();
        
        // Enable additive blending for glowing effect
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
        glDepthMask(GL_FALSE);

        shader.setFloat("time", time);
        shader.setFloat("thrustLevel", thrust);
        shader.setVec3("color", glm::vec3(1.3f, 0.2f, 0.0f)); 

        // Engine position relative to ship
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, Position);
        model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.2f)); 
        // Ship faces -Z, so back is +Z
        // Rotate cone to point backward (+Z)
        // Cone default is Y-up. Rotate 90 deg around X.
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Scale the engine plume
        float width = 0.3f + (0.3f * thrust);
        float length = thrust * 3.5f;
        model = glm::scale(model, glm::vec3(width, length, width)); 

        shader.setMat4("model", model);

        renderCone();

        // Restore states
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }

    glm::mat4 GetModelMatrix() const {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, Position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        
        modelMatrix = glm::scale(modelMatrix, ModelScale);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(ModelRotationCorrection.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(ModelRotationCorrection.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(ModelRotationCorrection.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return modelMatrix;
    }

    glm::vec3 GetForwardVector() const {
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    }
};

#endif
