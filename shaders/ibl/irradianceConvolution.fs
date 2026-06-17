#version 420 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{
    // the coordinate of the fragment we are painting acts as our normal
    vec3 N = normalize(localPos);

    // fake up vector to calculate right vector
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    vec3 irradiance = vec3(0.0);

    // we take a sample of 0.025 across the whole hemisphere 
    // where the loop goes from 0 to 2PI for the horizontal azimuth angle (phi)
    // and 0 to PI/2 for the vertical zenith angle (theta)
    // this allows for a total of 15562 samples per fragment
    float sampleDelta = 0.025;
    float nrSamples = 0.0;

    // reinmann sum
    // Azimuth (Yaw)
    for (float phi = 0; phi < 2.0 * PI; phi += sampleDelta)
    {
        // Zenith (Pitch)
        for (float theta = 0; theta < PI / 2; theta += sampleDelta)
        {
            // spherical to cartesian
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

            // tangent to world space using our right, up and normal vectors
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }


    irradiance = PI * irradiance * (1.0 / nrSamples); // take the average of all the samples

    FragColor = vec4(irradiance, 1.0);
}