#ifndef LIGHTING_H
#define LIGHTING_H

#include "shader.h"
#include "game_item.h"
#include "player.h"
#include <vector>
#include <string>

inline void SetupSceneLighting(Shader& shader, const std::vector<Item>& items, const glm::vec3& sunPos, Player& player) {
    // 1. Directional light (Sun)
    shader.setVec3("dirLight.direction", -sunPos);
    shader.setVec3("dirLight.ambient", 0.2f, 0.2f, 0.2f);
    shader.setVec3("dirLight.diffuse", 0.6f, 0.6f, 0.6f);
    shader.setVec3("dirLight.specular", 0.6f, 0.6f, 0.6f);

    // 2. Point lights
    int lightCount = 0;
    for(const auto& item : items) {
        if(item.isLightSource && lightCount < 20) {
            std::string number = std::to_string(lightCount);
            shader.setVec3("pointLights[" + number + "].position", item.position);
            
            // Use item color for light color, with increased intensity to cut through fog
            shader.setVec3("pointLights[" + number + "].ambient", item.color * 0.1f);
            shader.setVec3("pointLights[" + number + "].diffuse", item.color * 1.5f); 
            shader.setVec3("pointLights[" + number + "].specular", item.color * 2.0f);
            
            shader.setFloat("pointLights[" + number + "].constant", 1.0f);
            shader.setFloat("pointLights[" + number + "].linear", 0.007f);
            shader.setFloat("pointLights[" + number + "].quadratic", 0.0002f);
            lightCount++;
        }
    }
    shader.setInt("nPointLights", lightCount);

    // 3. SpotLight (Flashlight)
    player.SetSpotlight(shader);
}

#endif
