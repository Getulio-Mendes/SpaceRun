#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"

#include <string>
#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    // Dados da mesh
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;
    glm::vec3 Center;
    float Radius;

    // Construtor
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        calculateBounds();
        setupMesh();
    }

    void calculateBounds() {
        if (vertices.empty()) {
            Center = glm::vec3(0.0f);
            Radius = 0.0f;
            return;
        }
        glm::vec3 min = vertices[0].Position;
        glm::vec3 max = vertices[0].Position;
        for (const auto& v : vertices) {
            min = glm::min(min, v.Position);
            max = glm::max(max, v.Position);
        }
        Center = (min + max) * 0.5f;
        
        float maxDist = 0.0f;
        for (const auto& v : vertices) {
            float dist = glm::length(v.Position - Center);
            if (dist > maxDist) maxDist = dist;
        }
        Radius = maxDist;
    }

    // Renderizar a mesh
    void Draw(Shader &shader, unsigned int overrideTextureID = 0) 
    {
        // Bind texturas apropriadas
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        bool hasDiffuse = false;
        bool hasSpecular = false;
        
        if (overrideTextureID != 0)
        {
            glActiveTexture(GL_TEXTURE0);
            shader.setInt("texture_diffuse1", 0);
            glBindTexture(GL_TEXTURE_2D, overrideTextureID);
            hasDiffuse = true;
        }
        else
        {
            for(unsigned int i = 0; i < textures.size(); i++)
            {
                glActiveTexture(GL_TEXTURE0 + i);
                
                std::string number;
                std::string name = textures[i].type;
                
                if(name == "texture_diffuse") {
                    number = std::to_string(diffuseNr++);
                    hasDiffuse = true;
                }
                else if(name == "texture_specular") {
                    number = std::to_string(specularNr++);
                    hasSpecular = true;
                }
                else if(name == "texture_normal")
                    number = std::to_string(normalNr++);
                else if(name == "texture_height")
                    number = std::to_string(heightNr++);

                shader.setInt((name + number).c_str(), i);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }
        }
        
        shader.setInt("hasDiffuse", hasDiffuse ? 1 : 0);
        shader.setInt("hasSpecular", hasSpecular ? 1 : 0);
        
        // Desenhar mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
    }

private:
    unsigned int VBO, EBO;

    void setupMesh()
    {
        // Criar buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        
        // Carregar dados nos vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Configurar ponteiros de atributos de vértices
        // Posições dos vértices
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        
        // Normais dos vértices
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        
        // Coordenadas de textura dos vértices
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        
        // Tangente dos vértices
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        
        // Bitangente dos vértices
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        glBindVertexArray(0);
    }
};

#endif
