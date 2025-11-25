// globalUbos.glsl  (NO #version here - entry shader must provide it)

// std140 UBO bound at binding = 0
layout(std140, binding = 0) uniform GlobalLights
{
    mat4 view;
    mat4 proj;
    vec4 viewPos; // vec3 + padding

    int numPointLights;
    int numSpotLights;
    int pad0;
    int pad1;

    // Directional light
    vec4 dir_direction;
    vec4 dir_ambient;
    vec4 dir_diffuse;
    vec4 dir_specular;

    // PointLights (MAX = 8)
    vec4 point_position[8];
    vec4 point_ambient[8];
    vec4 point_diffuse[8];
    vec4 point_specular[8];
    vec4 point_params[8]; // x=constant, y=linear, z=quadratic

    // SpotLights (MAX = 4)
    vec4 spot_position[4];
    vec4 spot_direction[4];
    vec4 spot_cutoffs[4]; // x=inner, y=outer
    vec4 spot_ambient[4];
    vec4 spot_diffuse[4];
    vec4 spot_specular[4];
    vec4 spot_params[4];
};
