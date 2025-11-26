// materialCommon.glsl  (NO #version here)
// Runtime material helpers. Requires TexCoords in fragment shader scope.

// Samplers (engine will bind textures and set *_present ints)
uniform sampler2D material_diffuse;

uniform sampler2D material_specular;

// Fallback / direct values
uniform vec3 material_diffuseColor;
uniform vec3 material_specularColor;

uniform float material_shininess;

uniform int material_diffuse_present;

uniform int material_specular_present;

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