// materialCommon.glsl 
// Runtime material helpers. Requires TexCoords in fragment shader scope.

// Samplers (engine will bind textures and set *_present ints)
uniform sampler2D material_diffuse;
uniform samplerCube material_skybox;
uniform sampler2D material_specular;
uniform sampler2D material_normal;

// Fallback / direct values
uniform vec3 material_diffuseColor;
uniform vec3 material_specularColor;

uniform float material_shininess;
uniform float material_reflectivity;
uniform int material_diffuse_present;
uniform int material_specular_present;
uniform int material_skybox_present;
uniform int material_normal_present;

// Helper accessors used by lighting_common.glsl
vec3 GetDiffuseColor()
{
    if (material_diffuse_present == 1)
        return texture(material_diffuse, TexCoords).rgb;
    return material_diffuseColor;
}

vec3 GetSpecularColor()
{
    if (material_specular_present == 1)
        return texture(material_specular, TexCoords).rgb;
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