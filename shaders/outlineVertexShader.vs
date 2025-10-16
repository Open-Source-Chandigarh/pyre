#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTex; // optional if you keep same VAO layout

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float outlinePixels;   // desired thickness in pixels (e.g. 3.0)
uniform float screenHeight;    // viewport height in pixels
uniform float fovDegrees;      // vertical fov in degrees

// pass-through if needed
void main()
{
    // Transform position & normal into view space
    vec4 viewPos = view * model * vec4(aPos, 1.0);

    // Normal transform (use upper-left 3x3 of view*model)
    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    vec3 viewNormal = normalize(normalMatrix * aNormal);

    // positive depth
    float depth = -viewPos.z; // view space z is negative in right-handed camera

    // compute view-space offset that corresponds to outlinePixels
    float fovRad = radians(fovDegrees);
    float viewHeight = 2.0 * depth * tan(0.5 * fovRad);
    float offsetView = outlinePixels * (viewHeight / screenHeight);

    // move along the view-space normal
    vec3 offsetPosView = viewPos.xyz + viewNormal * offsetView;

    // project to clip space
    vec4 clip = projection * vec4(offsetPosView, 1.0);
    gl_Position = clip;
}
