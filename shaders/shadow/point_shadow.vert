#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 5) in mat4 instanceModel;

uniform mat4 model;
uniform bool useInstancing;

out vec4 FragPos; 

void main()
{
    if (useInstancing) {
        FragPos = instanceModel * vec4(aPos, 1.0);
    } else {
        FragPos = model * vec4(aPos, 1.0);
    }
    // Note: We do NOT set gl_Position here. 
    // The Geometry Shader takes 'FragPos', duplicates it 6 times, 
    // and applies the projection matrices there.
    gl_Position = FragPos;
}