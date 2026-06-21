#version 420 core
out vec2 FragColor;
in vec2 TexCoords;

const float PI = 3.14159265359;

float VanDerCorput(uint bits)
{
    // reverse the bits
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    // divide by max possible 32 bit unsigned int value to convert in the float range 0.0 to 1.0
    return float(bits) * 2.3283064365386963e-10; 
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), VanDerCorput(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;

    // azimuth angle (spin)
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // convert from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent space to world space
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

// Geometry Function (Schlick-GGX)
// n dot v / n dot v * (1 - k) + k
// a = roughness
// k = (a * a) / 2.0
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0; // k for ambient lighting

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

// Geomtery function (Smith)
// to acccount for both shadowing and masking
// GeometryGGX(NdotV, k) *  GeometryGGX(NdotL, k) V is camera direction and L is light direction
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) 
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness); // view masking
    float ggx1 = GeometrySchlickGGX(NdotL, roughness); // light shadowing
    
    return ggx1 * ggx2;
}

vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV); // sin^2 + cos^2 = 1;
    // because ggx mirrors are isotropic (perfectly symmetrical) 
    // spinning the camera around the vertical axis does not change the lighting math at all
    V.y = 0.0;
    V.z = NdotV; // the dot product is cosine of the angle 

    float A = 0.0;
    float B = 0.0;
    // we assume the surface is pointing straight up
    // light only cares about the angle between normal and view vector
    // so we fake the normal direction
    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        // generate a random hammersley coordinate
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);

        // bend the 2d random vector into specular lobe of a micro mirror (H) based on roughness
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        // bounce the camera (V) off the micro mirror (H) to get the light direction (L)
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0); // because N is (0,0,1), dot(N,L) is literally just L.z
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // only add light if the ray is above the surface of the object
        if (NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            // we get this when we divide our integrals variable terms by PDF in monte carlo integration
            float G_Vis = (G * VdotH) / (NdotH * NdotV);

            // fresnel alpha variable (1 - v.h)^5)
            float Fc = pow(1.0 - VdotH, 5.0);
            // add the results to our red (scale) and green (bias)
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }   
    }

    // calculate the monte carlo average
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    
    return vec2(A, B);
}

void main() 
{
    vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);
    FragColor = integratedBRDF;
}