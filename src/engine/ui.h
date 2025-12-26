#ifndef UI_H
#define UI_H

#include "libs/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include "engine/shader.h"
#include "primitives.h"
#include "camera.h"
#include "player.h"
#include "game_item.h"

inline void RenderDigit(Shader& shader, int digit, glm::vec3 position, float size) {
    if (digit < 0 || digit > 9) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    // width is usually less than height for digits. 0.6 is good aspect ratio.
    model = glm::scale(model, glm::vec3(size * 0.6f, size, 1.0f)); 
    
    shader.setMat4("model", model);
    shader.setInt("digit", digit);
    
    renderQuad();
}

inline void RenderCompass(Shader& shader, Camera& camera, const Player& player, const std::vector<Item>& items, int scrWidth, int scrHeight) {
    // Clear depth buffer so compass is drawn on top
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Set viewport to top-right corner
    int compassSize = 150;
    glViewport(scrWidth - compassSize - 10, scrHeight - compassSize - 10, compassSize, compassSize);
    
    shader.use();
    
    // Create view matrix with only rotation
    glm::mat4 uiView = glm::mat4(glm::mat3(camera.GetViewMatrix())); 
    // Move camera back a bit to see the items
    uiView = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f)) * uiView;
    
    glm::mat4 uiProjection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    
    shader.setMat4("view", uiView);
    shader.setMat4("projection", uiProjection);
    
    // Draw Items
    shader.setBool("useUniformColor", true);
    for (const auto& item : items) {
        glm::vec3 dir = item.position - player.Position;
        // Project onto the compass sphere (radius 1.5)
        if (glm::length(dir) > 0.1f) {
            glm::vec3 pos = glm::normalize(dir) * 1.5f;
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::scale(model, glm::vec3(0.15f));
            
            shader.setVec3("uColor", item.color);
            shader.setMat4("model", model);
            renderSphere();
        }
    }
    
    shader.setBool("useUniformColor", false);
    
    // Restore viewport
    glViewport(0, 0, scrWidth, scrHeight);
}

inline void RenderUI(Shader& shader, int score, int lives, int scrWidth, int scrHeight) {
    // Setup Orthographic Projection for 2D overlay
    // Use -100 to 100 for Z range to ensure cubes are not clipped
    glm::mat4 projection = glm::ortho(0.0f, (float)scrWidth, 0.0f, (float)scrHeight, -100.0f, 100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    
    // Disable depth test for UI overlay to ensure it draws on top
    glDisable(GL_DEPTH_TEST);

    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    
    // --- Render Score (Yellow, Top-Left) ---
    shader.setBool("useUniformColor", true);
    shader.setVec3("uColor", glm::vec3(1.0f, 1.0f, 0.0f)); 

    std::string scoreStr = std::to_string(score);
    float size = 40.0f; 
    float spacing = size * 0.7f;
    float startX = 50.0f;
    float startY = scrHeight - 50.0f;

    for (size_t i = 0; i < scoreStr.length(); i++) {
        int digit = scoreStr[i] - '0';
        RenderDigit(shader, digit, glm::vec3(startX + i * spacing, startY, 0.0f), size);
    }

    // --- Render Lives (Red, Top-Left, below Score) ---
    shader.setVec3("uColor", glm::vec3(1.0f, 0.0f, 0.0f));
    
    std::string livesStr = std::to_string(lives);
    // Below score
    float livesStartY = startY - 60.0f; 

    for (size_t i = 0; i < livesStr.length(); i++) {
        int digit = livesStr[i] - '0';
        RenderDigit(shader, digit, glm::vec3(startX + i * spacing, livesStartY, 0.0f), size);
    }

    shader.setBool("useUniformColor", false);
    // Reset digit mode
    shader.setInt("digit", -1); 
    
    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);
}

#endif
