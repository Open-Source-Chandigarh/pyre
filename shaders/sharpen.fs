#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform float strength; // 0.0 = none, 1.0 = full

const float offset = 1.0 / 300.0;

void main()
{
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset),
        vec2( 0.0f,    offset),
        vec2( offset,  offset),
        vec2(-offset,  0.0f),
        vec2( 0.0f,    0.0f),
        vec2( offset,  0.0f),
        vec2(-offset, -offset),
        vec2( 0.0f,   -offset),
        vec2( offset, -offset)
    );

    // simple blur kernel (box blur)
    float blurKernel[9] = float[](
         1, 1, 1,
         1, 1, 1,
         1, 1, 1
    );

    vec3 sample[9];
    for (int i = 0; i < 9; ++i)
        sample[i] = texture(scene, TexCoords + offsets[i]).rgb;

    // compute blur (average)
    vec3 blurred = vec3(0.0);
    for (int i = 0; i < 9; ++i)
        blurred += sample[i] * blurKernel[i];
    blurred /= 9.0;

    vec3 original = texture(scene, TexCoords).rgb;

    float s = clamp(strength, 0.0, 1.0);

    // unsharp mask: original + amount * (original - blurred)
    vec3 result = original + s * (original - blurred);

    FragColor = vec4(result, 1.0);
}