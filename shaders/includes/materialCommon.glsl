// materialCommon.glsl 
// Runtime material helpers. Requires TexCoords in fragment shader scope.

// Samplers (engine will bind textures and set *_present ints)
uniform sampler2D material_diffuse;
uniform samplerCube material_skybox;
uniform sampler2D material_specular;
uniform sampler2D material_normal;
uniform sampler2D material_displacement;

// Fallback / direct values
uniform vec3 material_diffuseColor;
uniform vec3 material_specularColor;

uniform float material_shininess;
uniform float material_reflectivity;
uniform int material_diffuse_present;
uniform int material_specular_present;
uniform int material_skybox_present;
uniform int material_normal_present;
uniform int material_displacement_present;
uniform float material_heightScale;

// Helper accessors used by lighting_common.glsl
vec3 GetDiffuseColor(vec2 uv)
{
    if (material_diffuse_present == 1)
        return texture(material_diffuse, uv).rgb;
    return material_diffuseColor;
}

vec3 GetSpecularColor(vec2 uv)
{
    if (material_specular_present == 1)
        return texture(material_specular, uv).rgb;
    return material_specularColor;
}

float GetShininess()
{
    return material_shininess;
}

float GetReflectivity()
{
    return material_reflectivity;
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * (material_heightScale > 0.0 ? material_heightScale : 0.1); 
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(material_displacement, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(material_displacement, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material_displacement, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    return finalTexCoords;
}