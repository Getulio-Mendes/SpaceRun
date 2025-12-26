#include <libs/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

#include "engine/shader.h"
#include "engine/model.h"
#include "engine/primitives.h"
#include "engine/skybox.h"
#include "engine/lighting.h"
#include "engine/ui.h"
#include "player.h"
#include "asteroid.h"
#include "asteroidField.h"
#include "game_item.h"

// Configurações da janela
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Câmera
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Player
Player player;
const float spawnRadius = 200.0f;
const float despawnRadius = 300.0f;

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

int main()
{
    // Inicialização do GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Criação da janela
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Trabalho GC", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Captura do mouse para controlar a visão
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Carregar funções OpenGL com GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }


    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader uiShader("shaders/ui_vertex.glsl", "shaders/ui_fragment.glsl");
    Shader shieldShader("shaders/shield_vertex.glsl", "shaders/shield_fragment.glsl");
    Shader propulsionShader("shaders/propulsion_vertex.glsl", "shaders/propulsion_fragment.glsl");

    // Carregar modelo da nave espacial (GLTF)
    Model spaceshipModel("../models/scene.gltf");
 
    // Carregar Skybox
    Skybox skybox;

    // Asteroid Field Setup
    Model asteroidModel("../models/asteriods/asteroid_03_01.obj", true, true);
    std::vector<unsigned int> asteroidTextures;
    asteroidTextures.push_back(TextureFromFile("space_asteroids_02_l_0001.jpg", "../models/asteriods"));
    asteroidTextures.push_back(TextureFromFile("space_asteroids_02_l_0002.jpg", "../models/asteriods"));
    asteroidTextures.push_back(TextureFromFile("space_asteroids_02_l_0003.jpg", "../models/asteriods"));
    asteroidTextures.push_back(TextureFromFile("space_asteroids_02_l_0004.jpg", "../models/asteriods"));
    asteroidTextures.push_back(TextureFromFile("space_asteroids_02_l_0005.jpg", "../models/asteriods"));
    asteroidTextures.push_back(TextureFromFile("space_asteroids_02_l_0006.jpg", "../models/asteriods"));
    asteroidTextures.push_back(TextureFromFile("space_asteroids_02_l_0007.jpg", "../models/asteriods"));
    asteroidTextures.push_back(TextureFromFile("space_asteroids_02_l_0008.jpg", "../models/asteriods"));

    AsteroidField asteroidField = AsteroidField(&asteroidModel, asteroidTextures, 2000, spawnRadius, despawnRadius);
    std::vector<Item> items;

    // Directional Light Source 
    glm::vec3 sunPos(0.0f, 100.0f, 80.0f); 

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
            
            // Spawn ahead of player (X-)
            float spawnDist = 200.0f;
            float x = player.Position.x - spawnDist;
            
            // Random Z within corridor
            // rand() % 1000 gives 0-999. Divided by 500 gives 0-2. Minus 1 gives -1 to 1.
            float z = (((rand() % 1000) / 500.0f) - 1.0f) * player.CorridorWidth * 0.9f; 
            
            // Random Y
            float y = (((rand() % 1000) / 500.0f) - 1.0f) * 20.0f;

            // Random Color
            glm::vec3 color(
                (rand() % 100) / 100.0f,
                (rand() % 100) / 100.0f,
                (rand() % 100) / 100.0f
            );

            items.push_back(Item(glm::vec3(x, y, z), glm::vec3(1.5f), color, true,false, currentFrame));
            std::cout << "Spawned Item at: " << x << ", " << y << ", " << z << std::endl;
        }

        // Input
        processInput(window);
        
        // Physics Update
        player.Update(deltaTime, camera);
        
        // Depth test para que a ordem de draw não importe
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
 
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glStencilMask(0xFF); // Ensure we can clear the stencil buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Ativar shader
        shader.use();
        shader.setBool("useSingleColor", false);
        shader.setFloat("brightness", 1.0f);

        // --- RENDER SKYBOX (First) ---
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
 
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 2000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        skybox.Draw(view, projection);

        // Reactivate main shader
        shader.use();

        // --- Light Configuration ---
        SetupSceneLighting(shader, items, sunPos, player);
        
        // Fog Configuration
        shader.setBool("useFog", true);
        shader.setVec3("fogColor", glm::vec3(0.0f, 0.0f, 0.0f)); 
        shader.setFloat("fogStart", 100.0f);
        shader.setFloat("fogEnd", 150.0f);

        // --- Draw Objects ---
        shader.setVec3("viewPos", camera.Position);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // Renderizar modelo da nave espacial
        player.Draw(shader, spaceshipModel);

        // Draw Asteroids
        asteroidField.UpdateAsteroidField(deltaTime, player.Position, player.GetForwardVector(), currentFrame);
        asteroidField.DrawAsteroidField(shader);

        // Check Collision
        float playerRadius = player.HitboxSize.x * player.ShieldScaleMultiplier;
        int hitIndex = asteroidField.CheckAsteroidCollision(player.Position, playerRadius);
        if (hitIndex != -1) {
            if (player.InvulnerabilityTimer <= 0.0f) {
                player.Lives--;
                // 2 seconds invulnerability
                player.InvulnerabilityTimer = 2.0f; 
                if (player.Lives <= 0) {
                    glfwSetWindowShouldClose(window, true);
                    std::cout << "GAME OVER! " << "Score: " << score << std::endl;
                }
            }

            // Simple bounce effect
            glm::vec3 pushDir = glm::normalize(player.Position - asteroidField.asteroids[hitIndex].Position);
            player.Velocity += pushDir * 10.0f; 
        }

        // Check Item Collection and Expiration
        for (auto it = items.begin(); it != items.end(); ) {
            // Expiration Check (10 seconds)
            if (currentFrame - it->spawnTime > 20.0f) {
                it = items.erase(it);
                continue;
            }

            float distance = glm::distance(player.Position, it->position);
            // use player radius approx 2.5 for easier collection
            if (distance < (2.5f + it->scale.x)) {
                score++;
                std::cout << "Collected Item! Score: " << score << std::endl;
                
                it = items.erase(it);
            } else {
                ++it;
            }
        }

        // Renderizar itens (Luzes e Cubos)
        RenderItems(shader, items);

        // Draw Engines
        propulsionShader.use();
        propulsionShader.setMat4("projection", projection);
        propulsionShader.setMat4("view", view);
        player.DrawEngines(propulsionShader, currentFrame);

        // Draw Hitbox (Shield), last for transparency
        shieldShader.use();
        shieldShader.setMat4("projection", projection);
        shieldShader.setMat4("view", view);
        player.DrawHitbox(shieldShader, camera.Position, currentFrame);

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

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    player.ProcessInput(window, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    player.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    player.ProcessScroll(yoffset);
}
