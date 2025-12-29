#ifndef LIGHTING_H
#define LIGHTING_H

#include "engine/shader.h"
#include "game_item.h"
#include "player.h"
#include <vector>
#include <string>

inline void SetupSceneLighting(Shader& shader, const std::vector<Item>& items, const glm::vec3& sunPos, Player& player) {
    shader.setFloat("brightness", 1.0f);

    // 1. Directional light (Sun)
    shader.setVec3("dirLight.direction", -sunPos);
    shader.setVec3("dirLight.ambient", 0.28f, 0.24f, 0.16f);   // softer warm yellowish ambient
    shader.setVec3("dirLight.diffuse", 0.26f, 0.22f, 0.12f);   // less intense yellow sunlight
    shader.setVec3("dirLight.specular", 0.5f, 0.44f, 0.26f);  // softer yellowish highlights

    // 2. Point lights
    int lightCount = 0;
    for(const auto& item : items) {
        if(item.isLightSource && lightCount < 20) {
            std::string number = std::to_string(lightCount);
            shader.setVec3("pointLights[" + number + "].position", item.position);
            
            // Use item color for light color, with increased intensity to cut through fog
            shader.setVec3("pointLights[" + number + "].ambient", item.color * 0.0f);
            shader.setVec3("pointLights[" + number + "].diffuse", item.color * 5.0f); 
            shader.setVec3("pointLights[" + number + "].specular", item.color * 5.0f);
            
            shader.setFloat("pointLights[" + number + "].constant", 1.0f);
            shader.setFloat("pointLights[" + number + "].linear", 0.001f);
            shader.setFloat("pointLights[" + number + "].quadratic", 0.001f);
            lightCount++;
        }
    }
    shader.setInt("nPointLights", lightCount);

    // 3. SpotLight (Flashlight)
    player.SetSpotlight(shader);
}

#endif
