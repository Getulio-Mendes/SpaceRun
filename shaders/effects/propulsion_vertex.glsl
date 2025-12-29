#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out float Displacement;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float thrustLevel; // 0.0 to 1.0

void main()
{
    TexCoords = aTexCoords;
    vec3 pos = aPos;

    // Wobble effect: Only affects the tip of the cone (where y > 0.5 approx)
    // We assume the cone grows along the Y axis in model space
    if (pos.y > 0.0) {
        float wave = sin(time * 20.0 + pos.y * 5.0) * 0.1;
        pos.x += wave * pos.y * thrustLevel; // More wobble at the tip
        pos.z += cos(time * 15.0) * 0.1 * pos.y * thrustLevel;
    }

    Displacement = pos.y; // Pass height to fragment for gradient
    gl_Position = projection * view * model * vec4(pos, 1.0);
}