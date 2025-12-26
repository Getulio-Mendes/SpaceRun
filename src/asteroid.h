#ifndef ASTEROID_H
#define ASTEROID_H

#include "libs/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstdlib>
#include <algorithm>

#include "engine/model.h"
#include "engine/shader.h"
#include "engine/primitives.h"

enum AsteroidType {
    SMALL,
    MEDIUM,
    LARGE
};

class Asteroid {
public:
    glm::vec3 Position;
    glm::vec3 Rotation;
    glm::vec3 Velocity;
    glm::vec3 RotationVelocity;
    float Scale;
    AsteroidType Type;
    int MeshIndex;
    unsigned int TextureID;
    bool hitable;
    glm::vec3 LocalCenter;
    float LocalRadius;

    Asteroid(AsteroidType type, glm::vec3 position, int meshIndex, unsigned int textureID, glm::vec3 velocityDir = glm::vec3(0.0f)) 
        : Type(type), Position(position), MeshIndex(meshIndex), TextureID(textureID), LocalCenter(0.0f), LocalRadius(1.0f) {
        
        // Random rotation
        Rotation = glm::vec3(rand() % 360, rand() % 360, rand() % 360);
        
        // Random rotation velocity
        RotationVelocity = glm::vec3(
            (rand() % 100 - 50) / 10.0f,
            (rand() % 100 - 50) / 10.0f,
            (rand() % 100 - 50) / 10.0f
        );

        float speedBase = 0.0f;

        // Set properties based on type
        switch (type) {
            case SMALL:
                Scale = (rand() % 20) / 100.0f + 0.1f; 
                speedBase = 4.0f;
                hitable = false;
                break;
            case MEDIUM:
                Scale = (rand() % 20) / 10.0f + 2.0f; 
                speedBase = 8.0f;
                hitable = true;
                break;
            case LARGE:
                Scale = (rand() % 20) / 5.0f + 10.0f; 
                speedBase = 8.0f;
                hitable = true;
                break;
        }

        // Randomize speed a bit
        float speed = speedBase * ((rand() % 50 + 75) / 100.0f); // 0.75 to 1.25 factor

        if (glm::length(velocityDir) > 0.001f) {
            Velocity = glm::normalize(velocityDir) * speed;
        } else {
             Velocity = glm::vec3(
                (rand() % 100 - 50) / 100.0f,
                (rand() % 100 - 50) / 100.0f,
                (rand() % 100 - 50) / 100.0f
            );
            Velocity = glm::normalize(Velocity) * speed;
        }
    }

    void Update(float deltaTime) {
        Position += Velocity * deltaTime;
        Rotation += RotationVelocity * deltaTime;
    }

    void Draw(Shader& shader, Model& model) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, Position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(Scale));
        shader.setMat4("model", modelMatrix);
        
        if (MeshIndex < model.meshes.size())
            model.meshes[MeshIndex].Draw(shader, TextureID);
    }

};

Asteroid GenerateAsteroid(glm::vec3 center, float minRadius, float maxRadius, float ySpread, Model* model, const std::vector<unsigned int>& textures, glm::vec3 direction = glm::vec3(0.0f)) {
    int typeRand = rand() % 100;
    AsteroidType type;
    if (typeRand < 80) type = SMALL;      // 80% small
    else if (typeRand < 90) type = MEDIUM; // 10% medium
    else type = LARGE;                     // 10% large

    float angle;

    // If direction is essentially zero, spawn in full circle (for initial field)
    if (glm::length(direction) < 0.1f) {
        angle = static_cast<float>(rand() % 360);
    } else {
        // Spawn in a cone in front of the direction
        float baseAngle = glm::degrees(atan2(direction.x, direction.z));
        float spread = 120.0f; // 120 degrees cone
        float angleOffset = (rand() % (int)spread) - (spread / 2.0f);
        angle = baseAngle + angleOffset;
    }
    
    float radAngle = glm::radians(angle);

    // Calculate distance: strictly between minRadius and maxRadius
    float dist = minRadius + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (maxRadius - minRadius);

    float x = sin(radAngle) * dist;
    float z = cos(radAngle) * dist;
    
    // Random height variation
    // Small asteroids have 5x more vertical spread
    float finalYSpread = (type == SMALL) ? ySpread * 5.0f : ySpread; 
    float y = ((rand() % 100) / 50.0f - 1.0f) * finalYSpread; 
    
    glm::vec3 pos = center + glm::vec3(x, y, z);

    // Calculate velocity direction
    glm::vec3 velocityDir;
    
    if (type == SMALL) {
        // Completely random direction for small asteroids
        velocityDir = glm::normalize(glm::vec3(
            (rand() % 100 - 50) / 10.0f,
            (rand() % 100 - 50) / 10.0f,
            (rand() % 100 - 50) / 10.0f
        ));
    } else {
        // Target towards player for Medium/Large
        glm::vec3 toPlayer = center - pos;
        glm::vec3 baseDir = glm::normalize(toPlayer);
        
        // Add randomness (approx 5 degrees)
        glm::vec3 randomDir = glm::vec3(
            (rand() % 100 - 50) / 100.0f,
            (rand() % 100 - 50) / 100.0f,
            (rand() % 100 - 50) / 100.0f
        );
        
        velocityDir = glm::normalize(baseDir + randomDir * 0.1f);
    }

    int meshIndex = 0;
    if (model && model->meshes.size() > 0)
        meshIndex = rand() % model->meshes.size();
    
    unsigned int textureID = 0;
    if (textures.size() > 0)
        textureID = textures[rand() % textures.size()];

    Asteroid ast(type, pos, meshIndex, textureID, velocityDir);
    if (model && meshIndex < model->meshes.size()) {
        ast.LocalCenter = model->meshes[meshIndex].Center;
        ast.LocalRadius = model->meshes[meshIndex].Radius;
    }
    return ast;
}

#endif
