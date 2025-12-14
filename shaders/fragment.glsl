#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform bool useSingleColor;
uniform vec3 singleColor;
uniform vec3 objectColor;
uniform float alpha = 1.0;

uniform bool isUnlit;
uniform int hasDiffuse;
uniform float brightness = 1.0;

// Material properties
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform float materialShininess = 32.0;

// Directional Light
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

// Point Light
struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
#define MAX_POINT_LIGHTS 20
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int nPointLights;

// Spot Light
struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform SpotLight spotLight;

// Fog
uniform bool useFog = false;
uniform vec3 fogColor = vec3(0.0, 0.0, 0.0);
uniform float fogStart = 50.0;
uniform float fogEnd = 100.0;

// Function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffColor, vec3 specColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor);

void main()
{

    if(useSingleColor)
    {
        FragColor = vec4(singleColor, alpha);
        return;
    }

    // Unlit mode for Skybox (Just return texture color)
    if(isUnlit)
    {
        vec3 color;
        if(hasDiffuse == 1)
            color = texture(texture_diffuse1, TexCoords).rgb;
        else
            color = objectColor;
            
        FragColor = vec4(color, alpha);
        return;
    }

    // Determine material colors
    vec3 diffColor;
    vec3 specColor;
    if (hasDiffuse == 1)
    {
        diffColor = vec3(texture(texture_diffuse1, TexCoords));
        specColor = vec3(texture(texture_specular1, TexCoords).r);
    }
    else
    {
        diffColor = objectColor;
        specColor = vec3(0.5); // Default specular
    }

    // Properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Phase 1: Directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir, diffColor, specColor);
    
    // Phase 2: Point lights
    for(int i = 0; i < nPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, diffColor, specColor);    
    
    // Phase 3: Spot light
    norm = -norm;
    vec3 spotLightResult = CalcSpotLight(spotLight, norm, FragPos, viewDir, diffColor, specColor);    
    
    result *= brightness;

    // Apply Fog
    if (useFog) {
        float dist = length(viewPos - FragPos);
        float fogFactor = (fogEnd - dist) / (fogEnd - fogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        result = mix(fogColor, result, fogFactor);
    }

    // Add spotlight AFTER fog to cut through it
    result += spotLightResult * brightness;

    FragColor = vec4(result, alpha);
}

// Calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3 lightDir = normalize(-light.direction);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    // Combine results
    vec3 ambient = light.ambient * diffColor;
    vec3 diffuse = light.diffuse * diff * diffColor;
    vec3 specular = light.specular * spec * specColor;
    return (ambient + diffuse + specular);
}

// Calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // Combine results
    vec3 ambient = light.ambient * diffColor;
    vec3 diffuse = light.diffuse * diff * diffColor;
    vec3 specular = light.specular * spec * specColor;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// Calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // Combine results
    vec3 ambient = light.ambient * diffColor;
    vec3 diffuse = light.diffuse * diff * diffColor;
    vec3 specular = light.specular * spec * specColor;
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}
