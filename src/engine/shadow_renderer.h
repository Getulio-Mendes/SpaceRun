#ifndef SHADOW_RENDERER_H
#define SHADOW_RENDERER_H

#include "engine/shader.h"
#include "engine/primitives.h"
#include "asteroidField.h"
#include "player.h"
#include "game_item.h"
#include "camera.h"

#include <vector>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>

class ShadowRenderer {
public:
    // --- Configuration Thresholds ---
    // Asteroids smaller than this scale will NOT cast shadows (performance optimization)
    const float MIN_SHADOW_CASTER_SCALE = 1.0f; 
    // Max instances to batch per draw call (prevents reallocation)
    const size_t MAX_INSTANCE_COUNT = 10000;    

    // Constructor
    ShadowRenderer(unsigned int shadowW = 1024, unsigned int shadowH = 1024, unsigned int maxPoint = 20, float pointFar = 80.0f, float dir_cull_radius = 80.0f)
        : SHADOW_WIDTH(shadowW), SHADOW_HEIGHT(shadowH), MAX_POINT_SHADOWS(maxPoint), pointLightFarPlane(pointFar),
          dirDepthMap(0), dirDepthFBO(0), globalCullRadius(dir_cull_radius), sharedInstanceVBO(0)
    {
        pointDepthMaps.assign(MAX_POINT_SHADOWS, 0);
        pointDepthFBOs.assign(MAX_POINT_SHADOWS, 0);
    }

    ~ShadowRenderer() {
        if (dirDepthMap) glDeleteTextures(1, &dirDepthMap);
        if (dirDepthFBO) glDeleteFramebuffers(1, &dirDepthFBO);
        for (unsigned int i = 0; i < pointDepthMaps.size(); ++i) {
            if (pointDepthMaps[i]) glDeleteTextures(1, &pointDepthMaps[i]);
            if (pointDepthFBOs[i]) glDeleteFramebuffers(1, &pointDepthFBOs[i]);
        }
        if (spotDepthMap) glDeleteTextures(1, &spotDepthMap);
        if (spotDepthFBO) glDeleteFramebuffers(1, &spotDepthFBO);
        if (sharedInstanceVBO) glDeleteBuffers(1, &sharedInstanceVBO);
    }

    void Init() {
        // --- 1. Directional Light (2D Texture) ---
        glGenFramebuffers(1, &dirDepthFBO);
        glGenTextures(1, &dirDepthMap);
        glBindTexture(GL_TEXTURE_2D, dirDepthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindFramebuffer(GL_FRAMEBUFFER, dirDepthFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dirDepthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- 2. Point Lights (Cubemaps) ---
        for (unsigned int i = 0; i < MAX_POINT_SHADOWS; ++i) {
            glGenFramebuffers(1, &pointDepthFBOs[i]);
            glGenTextures(1, &pointDepthMaps[i]);
            glBindTexture(GL_TEXTURE_CUBE_MAP, pointDepthMaps[i]);
            for (unsigned int face = 0; face < 6; ++face) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glBindFramebuffer(GL_FRAMEBUFFER, pointDepthFBOs[i]);
            // Faces attached dynamically in render loop
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // --- 3. Spotlight (2D Texture) ---
        glGenFramebuffers(1, &spotDepthFBO);
        glGenTextures(1, &spotDepthMap);
        glBindTexture(GL_TEXTURE_2D, spotDepthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor2[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor2);
        glBindFramebuffer(GL_FRAMEBUFFER, spotDepthFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, spotDepthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- 4. Optimization: Single Persistent VBO ---
        glGenBuffers(1, &sharedInstanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, sharedInstanceVBO);
        // Pre-allocate memory for MAX_INSTANCE_COUNT matrices. usage = DYNAMIC_DRAW
        glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCE_COUNT * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // Reserve CPU side vector to avoid reallocations during frame
        tempMatrixBuffer.reserve(MAX_INSTANCE_COUNT);
    }

    // Main Render Function
    void RenderShadowPass(const glm::vec3& sunPos,
                          const std::vector<Item>& items,
                          AsteroidField& asteroidField,
                          Player& player,
                          Model& spaceshipModel,
                          Shader& simpleDepthShader,
                          Shader& pointDepthShader,
                          Camera& camera,
                          unsigned int screenWidth,
                          unsigned int screenHeight)
    {
        // 1. Save State
        GLint prevViewport[4];
        glGetIntegerv(GL_VIEWPORT, prevViewport);
        GLint prevFBO = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFBO);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT); // Peter-panning fix

        // 2. Render Directional Light (Sun)
        {
            float near_plane = 1.0f, far_plane = 500.0f;
            // Ortho box size matches globalCullRadius roughly
            glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, near_plane, far_plane);
            
            // Sun follows player on X/Z plane to keep shadows high resolution around player
            glm::vec3 sunTarget = player.Position;
            glm::vec3 sunPosAdjusted = sunTarget + glm::normalize(sunPos) * 100.0f; // Keep sun vector but centered relative to player
            
            glm::mat4 lightView = glm::lookAt(sunPosAdjusted, sunTarget, glm::vec3(0.0f, 0.0f, -1.0f));
            dirLightSpaceMatrix = lightProjection * lightView;

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, dirDepthFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(2.0f, 2.0f);
            
            simpleDepthShader.use();
            simpleDepthShader.setMat4("lightSpaceMatrix", dirLightSpaceMatrix);
         
            // Pass: Center = Player, Radius = globalCullRadius
            RenderSceneDepthInternal(simpleDepthShader, asteroidField, player, spaceshipModel, items, 
                                     player.Position, globalCullRadius);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // 3. Render Point Lights (Closest N lights)
        {
            // Gather valid lights
            std::vector<glm::vec3> pointLightPositions;
            for (const auto& it : items) {
                if (it.isLightSource && pointLightPositions.size() < MAX_POINT_SHADOWS)
                    pointLightPositions.push_back(it.position);
            }

            // Sort by distance to player
            activePointIndices.clear();
            if (!pointLightPositions.empty()) {
                std::vector<int> idx(pointLightPositions.size());
                for (size_t i = 0; i < idx.size(); ++i) idx[i] = (int)i;
                
                std::sort(idx.begin(), idx.end(), [&](int a, int b) {
                    return glm::distance(pointLightPositions[a], player.Position) < glm::distance(pointLightPositions[b], player.Position);
                });
                
                // Only render closest 3 lights
                int take = std::min((int)idx.size(), 1);
                for (int i = 0; i < take; ++i) activePointIndices.push_back(idx[i]);
            }

            // Projection for Cubemap
            float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, 1.0f, pointLightFarPlane);

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            
            for (int sel = 0; sel < (int)activePointIndices.size(); ++sel) {
                int i = activePointIndices[sel];
                glm::vec3 lightPos = pointLightPositions[i];

                std::vector<glm::mat4> shadowTransforms;
                shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)));

                glBindFramebuffer(GL_FRAMEBUFFER, pointDepthFBOs[i]);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthMaps[i], 0);
                glClear(GL_DEPTH_BUFFER_BIT);

                pointDepthShader.use();
                for (int m = 0; m < 6; ++m)
                    pointDepthShader.setMat4("shadowMatrices[" + std::to_string(m) + "]", shadowTransforms[m]);
                pointDepthShader.setVec3("lightPos", lightPos);
                pointDepthShader.setFloat("far_plane", pointLightFarPlane);

                // Pass: Center = Light, Radius = pointLightFarPlane
                RenderSceneDepthInternal(pointDepthShader, asteroidField, player, spaceshipModel, items, 
                                         lightPos, pointLightFarPlane);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }

        // 4. Render Spotlight (Flashlight)
        {
            float fov = 35.0f; // Slightly wider than visual cone to avoid clipping
            float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
            glm::mat4 spotProj = glm::perspective(glm::radians(fov), aspect, 1.0f, spotLightFarPlane);

            glm::vec3 lightPos = player.Position + player.GetForwardVector() * 2.0f;
            glm::vec3 lightTarget = lightPos + player.GetForwardVector();
            glm::mat4 spotView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
            spotLightSpaceMatrix = spotProj * spotView;

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, spotDepthFBO);
            glClear(GL_DEPTH_BUFFER_BIT);

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(2.0f, 2.0f);

            simpleDepthShader.use();
            simpleDepthShader.setMat4("lightSpaceMatrix", spotLightSpaceMatrix);

            // Pass: Center = Light Position, Radius = spotLightFarPlane
            RenderSceneDepthInternal(simpleDepthShader, asteroidField, player, spaceshipModel, items, 
                                     lightPos, spotLightFarPlane);

            glDisable(GL_POLYGON_OFFSET_FILL);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // 5. Restore State
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    }

    // Helper to bind textures to shader slots
    int BindForShader(Shader& shader, int startTextureUnit = 10) {
        int texUnit = startTextureUnit;

        shader.use();
        
        // 1. Bind Directional Shadow Map
        shader.setInt("dirShadowMap", texUnit);
        glActiveTexture(GL_TEXTURE0 + texUnit);
        glBindTexture(GL_TEXTURE_2D, dirDepthMap);
        shader.setMat4("dirLightSpaceMatrix", dirLightSpaceMatrix);
        texUnit++;

        // 2. Bind Point Shadow Maps
        // CRITICAL FIX: Initialize ALL shadow samplers to a safe unit first.
        // This prevents unused samplers from defaulting to Unit 0 (Diffuse) and breaking the render.
        int safeUnit = texUnit; // We will use this unit for all inactive/dummy shadows
        glActiveTexture(GL_TEXTURE0 + safeUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointDepthMaps[0]); // Bind any valid cubemap here so it's not empty

        for (int i = 0; i < (int)MAX_POINT_SHADOWS; ++i) {
            std::string name = "shadows[" + std::to_string(i) + "]";
            std::string farName = "pointShadowFarPlanes[" + std::to_string(i) + "]";

            // Default state: Point to the safe unit, disable shadow calculation (far_plane = 0)
            shader.setInt(name.c_str(), safeUnit);
            shader.setFloat(farName.c_str(), 0.0f);
        }

        // Now overwrite only the ACTIVE lights with their unique texture units
        for (int j = 0; j < (int)activePointIndices.size(); ++j) {
            int i = activePointIndices[j];
            std::string name = "shadows[" + std::to_string(i) + "]";
            std::string farName = "pointShadowFarPlanes[" + std::to_string(i) + "]";

            // Assign a NEW unique texture unit for this active light
            int specificUnit = safeUnit + 1 + j; // Start after the safe unit
            
            shader.setInt(name.c_str(), specificUnit);
            shader.setFloat(farName.c_str(), pointLightFarPlane);
            
            glActiveTexture(GL_TEXTURE0 + specificUnit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, pointDepthMaps[i]);
        }
        
        // Increment texUnit counter past all the used slots
        texUnit += (1 + (int)activePointIndices.size());

        // 3. Bind Spotlight Shadow Map
        if (spotDepthMap != 0) {
            shader.setInt("spotShadowMap", texUnit);
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, spotDepthMap);
            shader.setMat4("spotLightSpaceMatrix", spotLightSpaceMatrix);
            shader.setFloat("spotShadowFarPlane", spotLightFarPlane);
            texUnit++;
        }

        return texUnit;
    }

    // Public members to allow debug access if needed
    std::vector<unsigned int> pointDepthMaps;
    std::vector<unsigned int> pointDepthFBOs;

private:
    unsigned int SHADOW_WIDTH, SHADOW_HEIGHT, MAX_POINT_SHADOWS;
    float pointLightFarPlane;
    float globalCullRadius; 

    unsigned int dirDepthMap, dirDepthFBO;
    unsigned int spotDepthMap, spotDepthFBO;
    float spotLightFarPlane = 150.0f;

    glm::mat4 dirLightSpaceMatrix = glm::mat4(1.0f);
    glm::mat4 spotLightSpaceMatrix = glm::mat4(1.0f);

    std::vector<int> activePointIndices;

    // Optimization Members
    unsigned int sharedInstanceVBO;
    std::vector<glm::mat4> tempMatrixBuffer;

    // Internal Scene Renderer
    void RenderSceneDepthInternal(Shader &depthShader, 
                                  AsteroidField& asteroidField, 
                                  Player& player, 
                                  Model& spaceshipModel, 
                                  const std::vector<Item>& items, 
                                  const glm::vec3& cullCenter, 
                                  float currentPassRadius) 
    {
        depthShader.use();

        // 1. Render Player (Always)
        depthShader.setBool("useInstancing", false);
        depthShader.setMat4("model", player.GetModelMatrix());
        player.Draw(depthShader, spaceshipModel);

        // 2. Render Asteroids (Optimized Instanced Batching)
        if (asteroidField.asteroidModel && !asteroidField.asteroidModel->meshes.empty()) {
            
            // Bind the persistent VBO once
            glBindBuffer(GL_ARRAY_BUFFER, sharedInstanceVBO);

            for (size_t meshIdx = 0; meshIdx < asteroidField.asteroidModel->meshes.size(); ++meshIdx) {
                tempMatrixBuffer.clear();

                // CPU-Side Culling and Filtering
                for (const auto& ast : asteroidField.asteroids) {
                    // Optimization: Skip small debris shadows
                    if (ast.Scale < MIN_SHADOW_CASTER_SCALE) continue;

                    // Match Mesh
                    if (ast.MeshIndex != (int)meshIdx) continue;

                    // Distance Culling
                    float dist = glm::distance(ast.Position, cullCenter);
                    if (dist > (currentPassRadius + (ast.LocalRadius * ast.Scale))) continue;

                    tempMatrixBuffer.push_back(ast.GetModelMatrix());
                }

                if (tempMatrixBuffer.empty()) continue;

                // GPU Upload - BufferSubData (Fast)
                size_t count = std::min(tempMatrixBuffer.size(), MAX_INSTANCE_COUNT);
                glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(glm::mat4), tempMatrixBuffer.data());

                // Setup VAO
                unsigned int VAO = asteroidField.asteroidModel->meshes[meshIdx].VAO;
                glBindVertexArray(VAO);
                
                // Configure Instanced Attributes
                std::size_t vec4Size = sizeof(glm::vec4);
                for (unsigned int i = 0; i < 4; i++) {
                    glEnableVertexAttribArray(5 + i);
                    glVertexAttribPointer(5 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
                    glVertexAttribDivisor(5 + i, 1);
                }

                depthShader.setBool("useInstancing", true);
                
                // Draw Call
                glDrawElementsInstanced(GL_TRIANGLES, 
                                        asteroidField.asteroidModel->meshes[meshIdx].indices.size(), 
                                        GL_UNSIGNED_INT, 0, 
                                        (GLsizei)count);

                // Cleanup
                for (unsigned int i = 0; i < 4; i++) glVertexAttribDivisor(5 + i, 0);
                glBindVertexArray(0);
            }
            
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        // 3. Render Items (Simple Distance Cull)
        for (const auto& item : items) {
            if (glm::distance(item.position, cullCenter) > currentPassRadius + 2.0f) continue;
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, item.position);
            model = glm::scale(model, item.scale);
            depthShader.setMat4("model", model);
            renderSphere();
        }
    }
};

#endif // SHADOW_RENDERER_H