#version 450

layout(binding = 0) uniform UniformBufferObject {
    ivec2 resolution;
    float time;
} ubo;

layout(location = 0) out vec4 outColor;

//===================================================//

const int MAX_RAY_STEPS = 128;

const float PI = 3.14159265359;
const vec3 CAM_POS = {0.0, 0.0, 0.0};

const ivec3 VOXEL = {0, 0, -5};

void main()
{
    float aspectRatio = float(ubo.resolution.x) / ubo.resolution.y;
    const float fov = 90.0;
    float viewportWidth = 2 * aspectRatio;
    float Px = (2 * ((gl_FragCoord.x + 0.5) / (ubo.resolution.x)) - 1) * aspectRatio;
    float Py = (1 - 2 * (gl_FragCoord.y + 0.5) / ubo.resolution.y);
    const vec3 rayOrigin = CAM_POS;
    vec3 rayDir = vec3(Px, Py, -1) - rayOrigin;
    rayDir = normalize(rayDir);

    // Ik weet niet of deze cast werkt
    ivec3 currentVoxel = ivec3(floor(rayOrigin));
    ivec3 step = ivec3(sign(rayDir));

    vec3 nextVoxelBoundary = currentVoxel + step;

    // Could check if it can't be infinity
    vec3 tMax = (nextVoxelBoundary - rayOrigin) / rayDir;
    vec3 tDelta = step / rayDir;

    for(int i = 0; i < MAX_RAY_STEPS; i++){
        // DO
        if(currentVoxel == VOXEL)
        {
            outColor = vec4(1.0, 0.0, 0.0, 1.0);
            return;
        }

        // Calc
        if(tMax.x < tMax.y)
        {
            if(tMax.x < tMax.z)
            {
                // X-axis
                currentVoxel.x += step.x;
                tMax.x += tDelta.x;
            } else {
                // Z-axis
                currentVoxel.z += step.z;
                tMax.z += tDelta.z;
            }
        } else {
            if(tMax.y < tMax.z)
            {
                // Y-axis
                currentVoxel.y += step.y;
                tMax.y += tDelta.y;
            } else {
                // Z-axis
                currentVoxel.z += step.z;
                tMax.z += tDelta.z;
            }
        }
    }

    float a = 0.5 * (rayDir.y + 1);

    vec3 aColor = {1.0, 1.0, 1.0};
    vec3 bColor = {0.5, 0.7, 1.0};
    vec3 color = aColor + (bColor - aColor) * a;
    outColor = vec4(color, 1.0);
}
