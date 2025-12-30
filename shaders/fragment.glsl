#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

const float bias = 0.05;

uniform vec3 viewPos;
uniform bool useSingleColor;
uniform vec3 singleColor;
uniform vec3 objectColor;
uniform float alpha = 1.0;

uniform bool isUnlit;
uniform int hasDiffuse;
uniform float brightness = 1.0;

uniform bool useShadows;

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

// Directional shadow map (2D)
uniform sampler2D dirShadowMap;
uniform mat4 dirLightSpaceMatrix;


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
#define MAX_POINT_LIGHTS_SHADOWS 1

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int nPointLights;
uniform bool FORCE_WHITE;

uniform samplerCube pointShadows[MAX_POINT_LIGHTS];
uniform float pointShadowFarPlanes[MAX_POINT_LIGHTS];


// Forward declarations for shadow helper functions (using index-based sampling)
float CalcShadowCubeIndex(vec3 fragPos, vec3 lightPos, float far_plane, int shadowIndex);
vec3 CalcPointLightWithShadow(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor, int shadowIndex, float far_plane);

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

// Spotlight shadow map (2D)
uniform sampler2D spotShadowMap;
uniform mat4 spotLightSpaceMatrix;
uniform float spotShadowFarPlane;

// Fog
uniform bool useFog = false;
uniform vec3 fogColor = vec3(0.0, 0.0, 0.0);
uniform float fogStart = 50.0;
uniform float fogEnd = 100.0;

// Function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffColor, vec3 specColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor);
// Shadow helper for 2D shadow maps (directional and spot)
float CalcShadow2D(sampler2D shadowMap, mat4 lightSpaceMatrix, vec3 fragPos, vec3 normal, vec3 lightDir);

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

  
    vec3 result = CalcDirLight(dirLight, norm, viewDir, diffColor, specColor);
    
    vec3 pointLightsResult = vec3(0.0);
    for(int i = 0; i < nPointLights; i++) {
        if (i < MAX_POINT_LIGHTS_SHADOWS && useShadows) {
            float farp = pointShadowFarPlanes[i];
            pointLightsResult += CalcPointLightWithShadow(pointLights[i], norm, FragPos, viewDir, diffColor, specColor, i, farp);
        } else {
            pointLightsResult += CalcPointLight(pointLights[i], norm, FragPos, viewDir, diffColor, specColor);
        }
    }
    
    vec3 spotLightResult = CalcSpotLight(spotLight, norm, FragPos, viewDir, diffColor, specColor);    
    
    result *= brightness;

    // Apply Fog
    if (useFog) {
        float dist = length(viewPos - FragPos);
        float fogFactor = (fogEnd - dist) / (fogEnd - fogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        result = mix(fogColor, result, fogFactor);
    }

    // Add spotlight and pointLights AFTER fog to cut through it
    result += spotLightResult * brightness;
    result += pointLightsResult * brightness;

    FragColor = vec4(result, alpha);
}

// Calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3 lightDir = normalize(-light.direction);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    // Combine results
    vec3 ambient = light.ambient * diffColor;
    vec3 diffuse = light.diffuse * diff * diffColor;
    vec3 specular = light.specular * spec * specColor;

    float shadow = 0.0;
    if(useShadows){
        shadow = CalcShadow2D(dirShadowMap, dirLightSpaceMatrix, FragPos, normal, lightDir);
    }
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

// Calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    
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

// Simple omnidirectional shadow for point lights using a cubemap (single sample)
float CalcShadowCube(vec3 fragPos, vec3 lightPos, float far_plane, samplerCube shadowMap)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float closestDepth = texture(shadowMap, fragToLight).r;
    closestDepth *= far_plane; // undo [0,1]
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

// Point light calculation with shadow sampling (index-based)
vec3 CalcPointLightWithShadow(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor, int shadowIndex, float far_plane)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient = light.ambient * diffColor;
    vec3 diffuse = light.diffuse * diff * diffColor;
    vec3 specular = light.specular * spec * specColor;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    float shadow = 0.0;
    if (far_plane > 0.0) {
        shadow = CalcShadowCubeIndex(fragPos, light.position, far_plane, shadowIndex);
    }

    // Apply shadow to diffuse and specular only
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    return lighting;
}

// Sample and compare using sampler index
float CalcShadowCubeIndex(vec3 fragPos, vec3 lightPos, float far_plane, int shadowIndex) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float closestDepth = 0.0;
    // Manually select the correct sampler to avoid dynamic sampler-array indexing
    if (shadowIndex == 0) closestDepth = texture(pointShadows[0], fragToLight).r;
    else if (shadowIndex == 1) closestDepth = texture(pointShadows[1], fragToLight).r;
    else if (shadowIndex == 2) closestDepth = texture(pointShadows[2], fragToLight).r;
    else if (shadowIndex == 3) closestDepth = texture(pointShadows[3], fragToLight).r;
    else if (shadowIndex == 4) closestDepth = texture(pointShadows[4], fragToLight).r;
    else if (shadowIndex == 5) closestDepth = texture(pointShadows[5], fragToLight).r;
    else if (shadowIndex == 6) closestDepth = texture(pointShadows[6], fragToLight).r;
    else if (shadowIndex == 7) closestDepth = texture(pointShadows[7], fragToLight).r;
    else if (shadowIndex == 8) closestDepth = texture(pointShadows[8], fragToLight).r;
    else if (shadowIndex == 9) closestDepth = texture(pointShadows[9], fragToLight).r;
    else if (shadowIndex == 10) closestDepth = texture(pointShadows[10], fragToLight).r;
    else if (shadowIndex == 11) closestDepth = texture(pointShadows[11], fragToLight).r;
    else if (shadowIndex == 12) closestDepth = texture(pointShadows[12], fragToLight).r;
    else if (shadowIndex == 13) closestDepth = texture(pointShadows[13], fragToLight).r;
    else if (shadowIndex == 14) closestDepth = texture(pointShadows[14], fragToLight).r;
    else if (shadowIndex == 15) closestDepth = texture(pointShadows[15], fragToLight).r;
    else if (shadowIndex == 16) closestDepth = texture(pointShadows[16], fragToLight).r;
    else if (shadowIndex == 17) closestDepth = texture(pointShadows[17], fragToLight).r;
    else if (shadowIndex == 18) closestDepth = texture(pointShadows[18], fragToLight).r;
    else if (shadowIndex == 19) closestDepth = texture(pointShadows[19], fragToLight).r;
    else closestDepth = texture(pointShadows[0], fragToLight).r;
    closestDepth *= far_plane;   // undo [0;1]
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

float CalcShadow2D(sampler2D shadowMap, mat4 lightSpaceMatrix, vec3 fragPos, vec3 normal, vec3 lightDir) {
    vec4 fragPosLight = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5; // Map to [0, 1]

    if (projCoords.z > 1.0) return 0.0;
    
    // 1. Adaptive Bias: Increases bias based on slope to prevent "Triangular Acne"
    float currentBias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

    float currentDepth = projCoords.z;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    // 2. PCF (Percentage-Closer Filtering): Samples 9 pixels to smooth edges
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - currentBias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

// Calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
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

    float shadow = 0.0;
    if(useShadows){
        shadow = CalcShadow2D(spotShadowMap, spotLightSpaceMatrix, fragPos, normal, lightDir);
    }

    return (ambient + (1.0 - shadow) * (diffuse + specular));
}
