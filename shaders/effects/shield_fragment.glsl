#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 viewPos;
uniform float time;
uniform bool isInvulnerable;

void main()
{
    // 1. Fresnel Effect (Rim Lighting)
    // Calculates how "perpendicular" the view direction is to the surface normal
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 norm = normalize(Normal);
    float dotProduct = dot(viewDir, norm);
    
    // Invert dot product: 0.0 at center (facing camera), 1.0 at edges
    float fresnel = 1.0 - max(dotProduct, 0.0);
    
    // Sharpen the rim (power of 3 makes the center more transparent)
    fresnel = pow(fresnel, 2.5);

    // 2. Base Color
    vec3 shieldColor;
    if (isInvulnerable) {
        // Flashing Red/Orange when hit
        float flash = abs(sin(time * 10.0)); 
        shieldColor = mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 0.5, 0.0), flash);
    } else {
        // Cyan/Blue pulsing energy - Closer to original (0.0, 0.5, 1.0)
        float pulse = (sin(time * 2.0) + 1.0) * 0.5; // 0 to 1
        shieldColor = mix(vec3(0.0, 0.5, 1.0), vec3(0.0, 0.6, 1.0), pulse);
    }

    // 3. Combine
    // Add a small base alpha so the center isn't completely invisible
    float alpha = fresnel + 0.1; 
    
    // Clamp alpha
    alpha = clamp(alpha, 0.0, 0.8);

    FragColor = vec4(shieldColor, alpha);
}