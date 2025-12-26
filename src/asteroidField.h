#ifndef ASTEROID_FIELD_H
#define ASTEROID_FIELD_H

#include "libs/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstdlib>
#include <algorithm>

#include "engine/model.h"
#include "engine/shader.h"
#include "engine/primitives.h"
#include "asteroid.h"

struct AsteroidField {
    std::vector<Asteroid> asteroids;
    Model* asteroidModel;
    std::vector<unsigned int> textures;
    float lastSpawnCheckTime;
    float spawnRadius;
    float despawnRadius;
    unsigned int maxAsteroids;
    float ySpan;


    AsteroidField(Model* model, const std::vector<unsigned int>& texs, int amount, float spawnRadius, float despawnRadius) {
        this->asteroidModel = model;
        this->textures = texs;
        this->lastSpawnCheckTime = 0.0f;
        this->spawnRadius = spawnRadius;
        this->despawnRadius = despawnRadius;
        this->maxAsteroids = amount;
        this->ySpan = 50.0f;
        for(unsigned int i = 0; i < amount; i++) {
            // Initial generation: 360 degrees, distance [radius, radius + offset*2]
            this->asteroids.push_back(GenerateAsteroid(glm::vec3(0.0f), spawnRadius, despawnRadius, this->ySpan, model, texs));
        }
    }

    int CheckAsteroidCollision(glm::vec3 playerPos, float playerRadius) {
        for (size_t i = 0; i < this->asteroids.size(); i++) {
            Asteroid& ast = this->asteroids[i];
            
            // Only Medium and Large asteroids have hitboxes/collision
            if (!ast.hitable) continue;
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

    void UpdateAsteroidField(float deltaTime, glm::vec3 playerPos, glm::vec3 playerDir, float currentTime) {
        for (auto& asteroid : this->asteroids) {
            asteroid.Update(deltaTime);
        }

        // Lifecycle Check (Once per second)
        if (currentTime - this->lastSpawnCheckTime > 1.0f) {
            this->lastSpawnCheckTime = currentTime;

            // Remove far asteroids
            auto it = std::remove_if(this->asteroids.begin(), this->asteroids.end(), 
                [&](const Asteroid& a) {
                    return glm::distance(a.Position, playerPos) > this->despawnRadius;
                });
            this->asteroids.erase(it, this->asteroids.end());
            // Spawn new ones if needed
            while (this->asteroids.size() < this->maxAsteroids) {
                // Spawn strictly between spawnRadius and despawnRadius (minus buffer)
                // And in the direction the player is facing
                this->asteroids.push_back(GenerateAsteroid(playerPos, this->spawnRadius, this->despawnRadius, this->ySpan, this->asteroidModel, this->textures, playerDir));
            }
        }
    }

    // Instanced rendering for all asteroids
    void DrawAsteroidFieldInstanced(Shader& shader) {
        shader.setBool("isUnlit", false);

        // For each mesh in the model, draw all asteroids that use it
        for (size_t meshIdx = 0; meshIdx < asteroidModel->meshes.size(); ++meshIdx) {
            // Collect model matrices for asteroids using this mesh
            std::vector<glm::mat4> meshModelMatrices;
            for (const auto& asteroid : this->asteroids) {
                if (asteroid.MeshIndex == (int)meshIdx) {
                    meshModelMatrices.push_back(asteroid.GetModelMatrix());
                }
            }
            if (meshModelMatrices.empty()) continue;

            // Setup instance buffer
            unsigned int instanceVBO;
            glGenBuffers(1, &instanceVBO);
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, meshModelMatrices.size() * sizeof(glm::mat4), &meshModelMatrices[0], GL_STATIC_DRAW);

            unsigned int VAO = asteroidModel->meshes[meshIdx].VAO;
            glBindVertexArray(VAO);
            std::size_t vec4Size = sizeof(glm::vec4);
            for (unsigned int i = 0; i < 4; i++) {
                glEnableVertexAttribArray(5 + i);
                glVertexAttribPointer(5 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
                glVertexAttribDivisor(5 + i, 1);
            }

            asteroidModel->meshes[meshIdx].BindTextures(shader, 0);
            // Draw instanced
            //std::cout << "Drawing " << meshModelMatrices.size() << " instances of mesh " << meshIdx << std::endl;
            glDrawElementsInstanced(GL_TRIANGLES, asteroidModel->meshes[meshIdx].indices.size(), GL_UNSIGNED_INT, 0, meshModelMatrices.size());

            // Cleanup
            glBindVertexArray(0);
            glDeleteBuffers(1, &instanceVBO);
        }
    }
};


#endif // ASTEROID_FIELD_H