#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in float Displacement;

uniform float time;
uniform vec3 color;

void main()
{
    // Gradient: Bright at base (0.0), transparent at tip (1.0)
    // We invert Displacement assuming cone base is at 0 and tip at 1
    float alpha = 1.0 - smoothstep(0.0, 1.0, Displacement);
    
    // Add a pulse to the alpha
    float pulse = (sin(time * 30.0) + 1.0) * 0.1;
    alpha += pulse;

    // Core is bluish, edges are colored
    vec3 finalColor = mix(color, vec3(0.2, 0.0, 0.8), alpha * 0.5);

    // Hard fade at the very end
    if (alpha <= 0.0) discard;

    FragColor = vec4(finalColor, alpha);
}