# Space Run - Trabalho de Computação Gráfica

Aplicação gráfica 3D interativa desenvolvida em C++ e OpenGL, simulando uma nave espacial navegando por um campo de asteroides.

## Funcionalidades Implementadas

### Gráficos e Renderização
- **Iluminação Avançada**: Modelo Phong com múltiplas fontes de luz (Sol direcional, luzes pontuais coloridas, spotlight da nave).
- **Efeitos Visuais**:
  - **Fog (Neblina)**: Para profundidade atmosférica.
  - **Stencil Buffer**: Highlight em itens coletáveis.
  - **Blending**: Transparência para escudos e propulsores.
  - **Skybox**: Fundo espacial imersivo.
- **Shaders Customizados**:
  - Propulsão animada.
  - Escudo de energia.
  - UI procedural (Display de 7 segmentos).

### Gameplay e Física
- **Sistema de Voo**: Física com inércia, aceleração e atrito.
- **Colisão**: Detecção de colisão esférica entre nave e asteroides.
- **Sistema de Vidas e Pontuação**: Coleta de orbs de luz e dano por impacto.
- **Campo de Asteroides**: Geração procedural e gerenciamento de instâncias.

### Interface (UI)
- **HUD**: Pontuação e Vidas renderizados via shaders.
- **Bússola 3D**: Indica a direção dos objetivos

## Controles

| Tecla | Ação |
|-------|------|
| **W / S** | Acelerar / Desacelerar |
| **A / D** | Guinada (Yaw) - Esquerda/Direita |
| **Q / E** | Arfagem (Pitch) - Cima/Baixo |
| **Z / X** | Rolagem (Roll) |
| **Mouse** | Orientação da Câmera |
| **Scroll** | Distância da Câmera |
| **ESC** | Sair |

## Requisitos

- **OpenGL 3.3+**
- **GLFW**
- **GLAD**
- **GLM**
- **Assimp**
- **CMake**

### Instalação no Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install libglfw3-dev
sudo apt-get install libglew-dev
sudo apt-get install libglm-dev
sudo apt-get install libassimp-dev
```

### Para GLAD
O GLAD precisa ser gerado manualmente em: https://glad.dav1d.de/
1. Selecione OpenGL versão 3.3 ou superior
2. Profile: Core
3. Gere e baixe os arquivos
4. Copie `glad.c` para `src/` e `glad.h` para `include/`

## Compilação e Execução

Este projeto utiliza CMake para build.

### Linux (Build)

```bash
mkdir build
cd build
cmake ..
make
```

### Executar

Dentro da pasta `build`:

```bash
./trabalho_gc
```

## Estrutura do Projeto

- `src/`: Código fonte C++ (.cpp, .h)
- `shaders/`: Shaders GLSL (Vertex e Fragment)
- `models/`: Assets 3D e texturas
- `include/`: Bibliotecas de cabeçalho (GLAD, STB, etc.)

## Créditos e Referências

- Baseado nos tutoriais de [LearnOpenGL](https://learnopengl.com/).
- [OpenGL Tutorial](http://www.opengl-tutorial.org/)
- [Assimp Documentation](https://assimp.sourceforge.net/lib_html/index.html)
