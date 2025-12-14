#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 uColor;
uniform bool useUniformColor;
uniform int digit; // -1 for none, 0-9 for digits

// Function to check if a point is inside a rectangle
float rect(vec2 uv, vec2 pos, vec2 size) {
    vec2 d = abs(uv - pos) - size;
    return 1.0 - step(0.0, max(d.x, d.y));
}

void main()
{
    if (digit >= 0) {
        // Procedural 7-segment display
        // UVs are 0..1. Let's map segments.
        // Segments:
        //   A
        // F   B
        //   G
        // E   C
        //   D
        
        float on = 0.0;
        float t = 0.08; // thickness
        float h = 0.4;  // height/width of segment area
        
        vec2 c = vec2(0.5, 0.5); // center
        
        // A (Top)
        if (digit!=1 && digit!=4) 
            on += rect(TexCoords, vec2(0.5, 0.9), vec2(h, t));
            
        // B (Top-Right)
        if (digit!=5 && digit!=6) 
            on += rect(TexCoords, vec2(0.9, 0.7), vec2(t, 0.2));
            
        // C (Bottom-Right)
        if (digit!=2) 
            on += rect(TexCoords, vec2(0.9, 0.3), vec2(t, 0.2));
            
        // D (Bottom)
        if (digit!=1 && digit!=4 && digit!=7) 
            on += rect(TexCoords, vec2(0.5, 0.1), vec2(h, t));
            
        // E (Bottom-Left)
        if (digit==0 || digit==2 || digit==6 || digit==8) 
            on += rect(TexCoords, vec2(0.1, 0.3), vec2(t, 0.2));
            
        // F (Top-Left)
        if (digit!=1 && digit!=2 && digit!=3 && digit!=7) 
            on += rect(TexCoords, vec2(0.1, 0.7), vec2(t, 0.2));
            
        // G (Middle)
        if (digit!=0 && digit!=1 && digit!=7) 
            on += rect(TexCoords, vec2(0.5, 0.5), vec2(h, t));
            
        if (on > 0.5) {
            FragColor = vec4(uColor, 1.0);
        } else {
            discard;
        }
    } else {
        // compass dots
        if (useUniformColor)
            FragColor = vec4(uColor, 1.0);
        else
            FragColor = vec4(1.0);
    }
}
