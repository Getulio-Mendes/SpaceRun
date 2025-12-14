#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "stb_image.h"

#include "mesh.h"
#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
   

unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);

class Model 
{
public:
    // Dados do modelo
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    bool flipNormals;

    // Construtor, espera um caminho de arquivo para um modelo 3D
    Model(std::string const &path, bool flipNormals = false) : flipNormals(flipNormals)
    {
        loadModel(path);
    }

    // Desenha o modelo e todas as suas meshes
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
    
private:
    // Carrega um modelo com extensões suportadas pelo ASSIMP do arquivo e armazena as meshes resultantes
    void loadModel(std::string const &path)
    {
        // Ler arquivo via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        
        // Verificar erros
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
        {
            std::cout << "ERRO::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }
        
        // Recuperar o diretório do caminho do arquivo
        directory = path.substr(0, path.find_last_of('/'));

        // Processar o nó raiz do ASSIMP recursivamente
        processNode(scene->mRootNode, scene);
    }

    // Processa um nó de forma recursiva. Processa cada mesh individual localizada no nó
    // e repete esse processo em seus filhos
    void processNode(aiNode *node, const aiScene *scene)
    {
        // Processar cada mesh no nó atual
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // O nó contém apenas índices para indexar os objetos reais na cena
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        
        // Após processar todas as meshes (se houver), processar recursivamente cada um dos nós filhos
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // Dados para preencher
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // Percorrer cada um dos vértices da mesh
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            
            // Posições
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            
            // Normais
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                if (flipNormals)
                    vector = -vector;
                vertex.Normal = vector;
            }
            
            // Coordenadas de textura
            if(mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                
                // Debug: Print first few texture coordinates
                if(i < 3)
                {
                    std::cout << "Vértice " << i << " TexCoords: (" << vec.x << ", " << vec.y << ")" << std::endl;
                }
                
                // Tangente
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                
                // Bitangente
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
            {
                std::cout << "AVISO: Mesh não tem coordenadas de textura!" << std::endl;
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }
        
        // Agora percorrer cada uma das faces da mesh (uma face é um triângulo da mesh) e recuperar os índices de vértices correspondentes
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // Recuperar todos os índices da face e armazená-los no vetor indices
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        
        // Processar materiais
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    

        std::cout << "Material tem " << material->GetTextureCount(aiTextureType_NONE) << " texturas NONE" << std::endl;
        std::cout << "Material tem " << material->GetTextureCount(aiTextureType_BASE_COLOR) << " texturas BASE_COLOR" << std::endl;
        std::cout << "Material tem " << material->GetTextureCount(aiTextureType_NORMALS) << " texturas NORMALS" << std::endl;
        std::cout << "Material tem " << material->GetTextureCount(aiTextureType_UNKNOWN) << " texturas UNKNOWN" << std::endl;

        // 1. Mapas diffuse
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        // Try BASE_COLOR for PBR materials
        std::vector<Texture> baseColorMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse");
        textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());
        
        // 2. Mapas specular
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        
        // 3. Mapas normal
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        
        std::vector<Texture> normalMaps2 = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
        textures.insert(textures.end(), normalMaps2.begin(), normalMaps2.end());
        
        // 4. Mapas height
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        // Se nenhuma textura foi encontrada, vamos carregar manualmente as texturas da pasta
        if (textures.empty())
        {
            std::cout << "AVISO: Nenhuma textura encontrada no material, tentando carregar manualmente..." << std::endl;
            
            // Tentar carregar a textura base color manualmente
            Texture tex;
            tex.id = TextureFromFile("DefaultMaterial_Base_Color.jpeg", "models/textures", false);
            tex.type = "texture_diffuse";
            tex.path = "models/textures/DefaultMaterial_Base_Color.jpeg";
            textures.push_back(tex);
            textures_loaded.push_back(tex);
        }
        
        // Retornar um objeto mesh criado a partir dos dados da mesh extraídos
        return Mesh(vertices, indices, textures);
    }

    // Verifica todos os materiais de textura de um determinado tipo e carrega as texturas se ainda não foram carregadas
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
    {
        std::vector<Texture> textures;
        std::cout << "Procurando texturas do tipo: " << typeName << " (total: " << mat->GetTextureCount(type) << ")" << std::endl;
        
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            // Verificar se a textura já foi carregada antes e, se sim, continuar para a próxima iteração
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            
            if(!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    std::cout << "Tentando carregar textura: " << filename << std::endl;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        std::cout << "Textura carregada com sucesso! " << width << "x" << height 
                  << " com " << nrComponents << " componentes" << std::endl;
        
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "ERRO: Falha ao carregar textura: " << filename << std::endl;
        std::cout << "Motivo: " << stbi_failure_reason() << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

#endif
