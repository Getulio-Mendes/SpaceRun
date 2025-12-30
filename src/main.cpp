#include <libs/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

#include "engine/window.h"
#include "engine/shader.h"
#include "engine/model.h"
#include "engine/primitives.h"
#include "engine/skybox.h"
#include "engine/lighting.h"
#include "engine/ui.h"
#include "engine/shadow_renderer.h"
#include "player.h"
#include "asteroid.h"
#include "asteroidField.h"
#include "game_item.h"

// Configurações da janela
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int MAX_POINT_LIGHTS = 20;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const float spawnRadius = 200.0f;
const float despawnRadius = 300.0f;


int main(int argc, char ** argv)
{

    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " Shadows (1 for True or 0 for False)" << std::endl;
        return -1;
    }

    GLFWwindow* window = createWindow(SCR_WIDTH, SCR_HEIGHT, "Space Run", nullptr);
    if (!window) return -1;

    Player player;
    Camera camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);

    WindowContext ctx;
    ctx.player = &player;
    ctx.camera = &camera;
    ctx.firstMouse = true;
    ctx.lastX = SCR_WIDTH / 2.0;
    ctx.lastY = SCR_HEIGHT / 2.0;
    glfwSetWindowUserPointer(window, &ctx);
   
    // Main
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    // Effects
    Shader uiShader("shaders/ui_vertex.glsl", "shaders/ui_fragment.glsl");
    Shader shieldShader("shaders/effects/shield_vertex.glsl", "shaders/effects/shield_fragment.glsl");
    Shader propulsionShader("shaders/effects/propulsion_vertex.glsl", "shaders/effects/propulsion_fragment.glsl");
    // Shadow shaders 
    Shader simpleDepthShader("shaders/shadow/shadow_depth.vert", "shaders/shadow/shadow_depth.frag");
    Shader pointDepthShader("shaders/shadow/point_shadow.vert", "shaders/shadow/point_shadow.frag", "shaders/shadow/point_shadow.geom");    
 
    // Carregar Skybox
    Skybox skybox;

    // Asteroid Field Setup
    AsteroidField asteroidField = AsteroidField(2000, spawnRadius, despawnRadius);
    std::vector<Item> items;

    // Shadow renderer 
    ShadowRenderer shadowRenderer(2048, 2048, 20, 80.0f);
    if(argv[1][0] == '1'){
        shadowRenderer.Init();
    }

    // Directional Light Source 
    glm::vec3 sunPos(-50.0f, 100.0f, -50.0f); 

    float lastItemSpawnTime = 0.0f;
    int score = 0;

    // Loop de renderização
    while (!glfwWindowShouldClose(window))
    {
        // Cálculo do deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Item Spawning Logic (Every 5 seconds)
        if (currentFrame - lastItemSpawnTime > 5.0f) {
            lastItemSpawnTime = currentFrame;
            spawnItem(items, player.Position, player.CorridorWidth, currentFrame);
        }

        processInput(window);
        player.ProcessInput(window, deltaTime);
        
        // Physics Update
        player.Update(deltaTime, camera);
        asteroidField.UpdateAsteroidField(deltaTime, player.Position, player.GetForwardVector(), currentFrame);
        
        // Depth test para que a ordem de draw não importe
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
       
        // Clear
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 2000.0f);
        glm::mat4 view = camera.GetViewMatrix();

         // --- RENDER SKYBOX (First) ---
        skybox.Draw(view, projection);

        // Set default view
        shader.use();
        shader.setBool("useShadows", false);
        shader.setBool("useInstancing", false);
        shader.setBool("useSingleColor", false);
        shader.setVec3("viewPos", camera.Position);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        // Fog Configuration
        shader.setBool("useFog", true);
        shader.setVec3("fogColor", glm::vec3(0.0f, 0.0f, 0.0f)); 
        shader.setFloat("fogStart", spawnRadius);
        shader.setFloat("fogEnd", despawnRadius);

        // --- Light Configuration ---
        SetupSceneLighting(shader, items, sunPos, player);

        if(argv[1][0] == '1'){
            // Bind shadow maps (handled by ShadowRenderer)
            shadowRenderer.BindForShader(shader, 10); 
            // Use the ShadowRenderer to perform the depth passes lights
            shadowRenderer.RenderShadowPass(sunPos, items, asteroidField, player, player.spaceshipModel, simpleDepthShader, pointDepthShader, camera, SCR_WIDTH, SCR_HEIGHT);

        }
        else{
            shader.setInt("dirShadowMap", 10);
            shader.setInt("spotShadowMap", 11);

            for (int i = 0; i < MAX_POINT_LIGHTS; ++i) { 
               std::string name = "pointShadows[" + std::to_string(i) + "]";
               shader.setInt(name.c_str(), 12 + i); 
            }
        }

        // Renderizar modelo da nave espacial
        player.Draw(shader, player.spaceshipModel);
 
        shader.setBool("useInstancing", true);
        asteroidField.DrawAsteroidFieldInstanced(shader);
        shader.setBool("useInstancing", false);

        // Check Collision
        float playerRadius = player.HitboxSize.x * player.ShieldScaleMultiplier;
        int hitIndex = asteroidField.CheckAsteroidCollision(player.Position, playerRadius);
        if (hitIndex != -1) {
            bool gameOver = player.OnAsteroidCollision(asteroidField.asteroids[hitIndex].Position);
            if (gameOver) {
                glfwSetWindowShouldClose(window, true);
                std::cout << "GAME OVER! " << "Score: " << score << std::endl;
            }
        }

        // Check Item Collection and Expiration
        if(checkItems(items, player.Position, currentFrame)) {
            score++;
            std::cout << "Collected Item! Score: " << score << std::endl;
        }

        // Renderizar itens
        RenderItems(shader, items);

        // Draw Engines
        propulsionShader.use();
        propulsionShader.setMat4("projection", projection);
        propulsionShader.setMat4("view", view);
        player.DrawEngines(propulsionShader, currentFrame);

        // Draw Shield, last for transparency
        shieldShader.use();
        shieldShader.setMat4("projection", projection);
        shieldShader.setMat4("view", view);
        player.DrawShield(shieldShader, camera.Position, currentFrame);

        // Draw UI Compass
        // Get current framebuffer size for correct viewport handling
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        
        RenderCompass(uiShader, camera, player, items, fbWidth, fbHeight);

        // Clear depth again for score overlay
        glClear(GL_DEPTH_BUFFER_BIT); 
        RenderUI(uiShader, score, player.Lives, fbWidth, fbHeight);

        // Trocar buffers e verificar eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza
    glfwTerminate();
    return 0;
}

