// materialCommon.glsl  (NO #version here)
// Runtime material helpers. Requires TexCoords in fragment shader scope.

// Samplers (engine will bind textures and set *_present ints)
uniform sampler2D material_diffuse;

uniform sampler2D material_specular;

// Fallback / direct values
uniform vec3 material_diffuseColor;
uniform vec3 material_specularColor;

uniform float material_shininess;

#ifndef material_diffuse_present
uniform int material_diffuse_present;
#endif

#ifndef material_specular_present
uniform int material_specular_present;
#endif

#ifndef material_shininess_present
uniform int material_shininess_present;
#endif

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
    if (material_shininess_present == 1)
        return material_shininess;
    return 32.0;
}