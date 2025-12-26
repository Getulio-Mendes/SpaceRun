#ifndef GAME_ITEM_H
#define GAME_ITEM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "engine/shader.h"
#include "engine/primitives.h"

struct Item {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
    bool isLightSource;
    bool isUnlit;
    float spawnTime;
    
    Item(glm::vec3 pos, glm::vec3 sc, glm::vec3 col, bool isLight, bool unlit = false, float time = 0.0f) 
        : position(pos), scale(sc), color(col), isLightSource(isLight), isUnlit(unlit), spawnTime(time) {
            if (isLightSource) isUnlit = true;
        }
};

inline void RenderItems(Shader& shader, const std::vector<Item>& items) {
    // Ensure Depth Test is enabled and configured correctly
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Enable Stencil Test
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF); // Enable writing to stencil buffer
    
    shader.setInt("hasDiffuse", 0); 
    
    for (const auto& item : items)
    {
        // 1st Pass: Draw object normally
        // Always pass stencil test, write 1 to stencil buffer
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        
        shader.setBool("isUnlit", item.isUnlit);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, item.position);
        model = glm::scale(model, item.scale);
        shader.setMat4("model", model);
        shader.setVec3("objectColor", item.color);
        
        renderSphere();
        
        // 2nd Pass: Draw outline
        // Only draw where stencil value is NOT 1 (i.e., outside the object)
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00); // Disable writing to stencil buffer

        shader.setBool("useSingleColor", true);
        shader.setVec3("singleColor", glm::vec3(1.0f, 0.5f, 0.0f)); // Orange highlight
        
        model = glm::mat4(1.0f);
        model = glm::translate(model, item.position);
        model = glm::scale(model, item.scale * 1.1f);
        shader.setMat4("model", model);
        
        renderSphere();
        
        shader.setBool("useSingleColor", false);
        shader.setBool("isUnlit", false);
    }

    // Restore global state
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glDisable(GL_STENCIL_TEST);
}

#endif
