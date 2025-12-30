
#include <GLFW/glfw3.h>
#include <libs/glad.h>
#include <iostream>

#include "../player.h"


// Small struct to hold per-window input state and pointers to objects
struct WindowContext {
    Player* player = nullptr;
    Camera* camera = nullptr;
    bool firstMouse = true;
    double lastX = 0.0;
    double lastY = 0.0;
};

// Forward declarations of callbacks so they can be passed to glfwSet*Callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

GLFWwindow* createWindow(int height, int width, const char* title, WindowContext* Ctx){
    // Inicialização do GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Criação da janela
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwWindowHint(GLFW_SAMPLES,4);
    if (window == NULL)
    {
        std::cout << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);
    if (Ctx) {
        glfwSetWindowUserPointer(window, Ctx);
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // Captura do mouse para controlar a visão
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Carregar funções OpenGL com GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Falha ao inicializar GLAD" << std::endl;
        return NULL;
    }

    glEnable(GL_MULTISAMPLE);
    return window;
}


void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->player) return;

    if (ctx->firstMouse)
    {
        ctx->lastX = xpos;
        ctx->lastY = ypos;
        ctx->firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - ctx->lastX);
    float yoffset = static_cast<float>(ctx->lastY - ypos);

    ctx->lastX = xpos;
    ctx->lastY = ypos;

    ctx->player->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->player) return;

    ctx->player->ProcessScroll(static_cast<float>(yoffset));
}
