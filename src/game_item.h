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
    float spawnTime;
    
    Item(glm::vec3 pos, glm::vec3 sc, glm::vec3 col, float time = 0.0f) 
        : position(pos), scale(sc), color(col), spawnTime(time) {
        }
};

void RenderItems(Shader& shader, const std::vector<Item>& items) {
    // Ensure Depth Test is enabled and configured correctly
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Enable Stencil Test
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF); // Enable writing to stencil buffer
    
    shader.setInt("hasDiffuse", 0);
    // By default render items with their object color; enable singleColor only
    // for the outline pass below.
    shader.setBool("useSingleColor", false);
    
    for (const auto& item : items)
    {
        // 1st Pass: Draw object normally and write 1 to stencil where fragments pass
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        // Use the object's own color for the first pass
        shader.setBool("useSingleColor", false);
        shader.setBool("isUnlit", true);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, item.position);
        model = glm::scale(model, item.scale);
        shader.setMat4("model", model);
        shader.setVec3("objectColor", item.color);

        renderSphere();

        // 2nd Pass: Draw outline. Only draw where stencil value is NOT 1 (i.e., outside the object)
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        // Disable writing to stencil buffer 
        glStencilMask(0x00); 

        // Render the scaled object with front faces culled so back faces (the rim)
        // are drawn as an outline. This avoids depth-fighting and makes the
        // outline visible without disabling depth testing.
        shader.setBool("useSingleColor", true);
        // Orange highlight
        shader.setVec3("singleColor", glm::vec3(1.0f, 0.5f, 0.0f)); 

        // Cull front faces so only back faces of the slightly larger model draw
        glEnable(GL_CULL_FACE);
        GLint prevCull = 0;
        // query to preserve state (returns 0/1)
        glGetIntegerv(GL_CULL_FACE, &prevCull); 
        glCullFace(GL_FRONT);

        model = glm::mat4(1.0f);
        model = glm::translate(model, item.position);
        model = glm::scale(model, item.scale * 1.1f);
        shader.setMat4("model", model);

        renderSphere();

        glCullFace(GL_BACK);
        if (!prevCull) glDisable(GL_CULL_FACE);

        shader.setBool("useSingleColor", false);
        shader.setBool("isUnlit", false);
    }

    // Restore global state
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glDisable(GL_STENCIL_TEST);
}

void spawnItem(std::vector<Item>& items, const glm::vec3& playerPos, float corridorWidth, float currentTime) {
    // Spawn ahead of player (X-)
    float spawnDist = 200.0f;
    float x,y,z;
    x = playerPos.x - spawnDist;
    
    // Random Z within corridor
    float minDistance;
    // rand() % 1000 gives 0-999. Divided by 500 gives 0-2. Minus 1 gives -1 to 1.
    do {

        x += (((rand() % 1000) / 500.0f) - 1.0f) * 30.0f;

        z = (((rand() % 1000) / 500.0f) - 1.0f) * corridorWidth * 0.9f; 
        
        // Random Y
        y = (((rand() % 1000) / 500.0f) - 1.0f) * 20.0f;
        minDistance = 0;
        for(const auto& item : items) {
            float dist = glm::distance(glm::vec3(x, y, z), item.position);
            if (minDistance == 0 || dist < minDistance) {
                minDistance = dist;
            }
        }
    } while(minDistance < 100.0f && items.size() > 1);

    // Random Color
    glm::vec3 color(
        (rand() % 100) / 100.0f,
        (rand() % 100) / 100.0f,
        (rand() % 100) / 100.0f
    );

    items.push_back(Item(glm::vec3(x, y, z), glm::vec3(1.5f), color, currentTime));
    std::cout << "Spawned Item at: " << x << ", " << y << ", " << z << std::endl;

}

bool checkItems(std::vector<Item>& items, const glm::vec3& playerPos, float currentFrame) {
    bool captured = false;
    for (auto it = items.begin(); it != items.end(); ) {
        // Expiration Check (10 seconds)
        if (currentFrame - it->spawnTime > 20.0f) {
            it = items.erase(it);
            continue;
        }

        float distance = glm::distance(playerPos, it->position);
        // use player radius approx 2.5 for easier collection
        if (distance < (2.5f + it->scale.x)) {
            captured = true;
            
            it = items.erase(it);
        } else {
            ++it;
        }
    }
    return captured;
}

#endif
