#ifndef ASTEROID_H
#define ASTEROID_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstdlib>
#include <algorithm>

#include "model.h"
#include "shader.h"
#include "primitives.h"

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
    bool HasHitbox;
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
                HasHitbox = false;
                break;
            case MEDIUM:
                Scale = (rand() % 20) / 10.0f + 2.0f; 
                speedBase = 8.0f;
                HasHitbox = true;
                break;
            case LARGE:
                Scale = (rand() % 20) / 5.0f + 10.0f; 
                speedBase = 8.0f;
                HasHitbox = true;
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

    void DrawHitbox(Shader& shader) {
        if (!HasHitbox) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        shader.setBool("useSingleColor", true);
        shader.setVec3("singleColor", glm::vec3(1.0f, 0.0f, 0.0f)); // Red hitbox
        shader.setFloat("alpha", 0.3f); 
        shader.setBool("isUnlit", true);

        // Calculate transformation
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, Position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(Scale));
        
        glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(LocalCenter, 1.0f));
        float worldRadius = LocalRadius * Scale;

        glm::mat4 sphereModel = glm::mat4(1.0f);
        sphereModel = glm::translate(sphereModel, worldCenter);
        sphereModel = glm::scale(sphereModel, glm::vec3(worldRadius));
        
        shader.setMat4("model", sphereModel);
        
        renderSphere();
        
        shader.setBool("useSingleColor", false);
        shader.setBool("isUnlit", false);
        shader.setFloat("alpha", 1.0f);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
};

struct AsteroidField {
    std::vector<Asteroid> asteroids;
    Model* asteroidModel;
    std::vector<unsigned int> textures;
    float lastSpawnCheckTime;
    float spawnRadius;
    float despawnRadius;
    unsigned int maxAsteroids;
    float offset; // Store offset for generation
};

inline Asteroid GenerateAsteroid(glm::vec3 center, float minRadius, float maxRadius, float heightLimit, Model* model, const std::vector<unsigned int>& textures, glm::vec3 direction = glm::vec3(0.0f)) {
    int typeRand = rand() % 100;
    AsteroidType type;
    if (typeRand < 80) type = SMALL;      // 80% small
    else if (typeRand < 90) type = MEDIUM; // 10% medium
    else type = LARGE;                     // 10% large

    // Random angle in radians
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
    float ySpread = (type == SMALL) ? 1.5f : 0.4f; // Small asteroids have 5x more vertical spread
    float y = ((rand() % 100) / 50.0f - 1.0f) * heightLimit * ySpread; 
    
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

inline AsteroidField CreateAsteroidField(Model* model, const std::vector<unsigned int>& texs, int amount, float radius, float offset) {
    AsteroidField field;
    field.asteroidModel = model;
    field.textures = texs;
    field.lastSpawnCheckTime = 0.0f;
    field.spawnRadius = radius;
    // Ensure despawn radius is large enough to contain the initial offset band + buffer
    field.despawnRadius = radius + offset * 2.0f + 50.0f;
    field.maxAsteroids = amount;
    field.offset = offset;
    
    for(unsigned int i = 0; i < amount; i++)
    {
        // Initial generation: 360 degrees, distance [radius, radius + offset*2]
        field.asteroids.push_back(GenerateAsteroid(glm::vec3(0.0f), radius, radius + offset * 2.0f, offset, model, texs));
    }
    return field;
}

inline int CheckAsteroidCollision(AsteroidField& field, glm::vec3 playerPos, float playerRadius) {
    for (size_t i = 0; i < field.asteroids.size(); i++) {
        Asteroid& ast = field.asteroids[i];
        
        // Only Medium and Large asteroids have hitboxes/collision
        if (!ast.HasHitbox) continue;
        if (ast.Type == SMALL) continue; // Extra safety check

        // Calculate World Radius of the asteroid
        float astWorldRadius = ast.LocalRadius * ast.Scale;

        // Calculate World Center correctly using the transformation matrix
        // This ensures the hitbox matches the visual mesh even if the mesh is offset or rotated
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, ast.Position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(ast.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(ast.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(ast.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(ast.Scale));
        
        glm::vec3 astWorldCenter = glm::vec3(modelMatrix * glm::vec4(ast.LocalCenter, 1.0f));

        // Reduce the hitbox slightly (0.85) to be forgiving to the player
        float distance = glm::distance(playerPos, astWorldCenter);
        if (distance < (playerRadius + astWorldRadius * 0.75f)) {
            return (int)i; // Return index of the asteroid hit
        }
    }
    return -1; // No collision
}

inline void UpdateAsteroidField(AsteroidField& field, float deltaTime, glm::vec3 playerPos, glm::vec3 playerDir, float currentTime) {
    for (auto& asteroid : field.asteroids) {
        asteroid.Update(deltaTime);
    }

    // Lifecycle Check (Once per second)
    if (currentTime - field.lastSpawnCheckTime > 1.0f) {
        field.lastSpawnCheckTime = currentTime;

        // Remove far asteroids
        auto it = std::remove_if(field.asteroids.begin(), field.asteroids.end(), 
            [&](const Asteroid& a) {
                return glm::distance(a.Position, playerPos) > field.despawnRadius;
            });
        field.asteroids.erase(it, field.asteroids.end());

        // Spawn new ones if needed
        while (field.asteroids.size() < field.maxAsteroids) {
            // Spawn strictly between spawnRadius and despawnRadius (minus buffer)
            // And in the direction the player is facing
            field.asteroids.push_back(GenerateAsteroid(playerPos, field.spawnRadius, field.despawnRadius - 10.0f, field.offset, field.asteroidModel, field.textures, playerDir));
        }
    }
}

inline void DrawAsteroidField(AsteroidField& field, Shader& shader) {
    shader.setBool("isUnlit", false);
    shader.setFloat("brightness", 2.0f); 
    
    for (auto& asteroid : field.asteroids) {
        asteroid.Draw(shader, *field.asteroidModel);
    }
    
    shader.setFloat("brightness", 1.0f);

    // Draw Hitboxes
    for (auto& asteroid : field.asteroids) {
        if (asteroid.HasHitbox) {
            //asteroid.DrawHitbox(shader);
        }
    }
}

#endif
