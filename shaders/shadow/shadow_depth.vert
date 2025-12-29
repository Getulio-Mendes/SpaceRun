#version 330 core
layout (location = 0) in vec3 aPos;

// Optional instanced model (locations 5..8 match instance attributes set by the app)
layout (location = 5) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform bool useInstancing = false;

void main()
{
    if (useInstancing) {
        gl_Position = lightSpaceMatrix * instanceModel * vec4(aPos, 1.0);
    } else {
        gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    }
}
