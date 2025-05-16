#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "pis/engine.h"

UniformBufferObject ubo = {0};

bool processInput(PisEngine* pis)
{
    SDL_Event event;

    while(SDL_PollEvent(&event))
    {
        //User requests quit
        if(event.type == SDL_EVENT_QUIT)
            return false;

        if(event.type == SDL_EVENT_KEY_UP && event.key.scancode == SDL_SCANCODE_RETURN)
            ubo.time = !ubo.time;
    }

    return true;
}

int main(void)
{
    PisEngine* pis = calloc(1, sizeof(PisEngine));
    if(pis == NULL)
    {
        fprintf(stderr, "Failed to allocate\n");
        return -1;
    }

    pis->windowExtent.width = 1280;
    pis->windowExtent.height = 720;

    strcpy(pis->voxelFile, "/Users/nielsbil/Downloads/vox/#treehouse/#treehouse.vox");
    // strcpy(pis->voxelFile, "/Users/nielsbil/Downloads/vox/#odyssey/#odyssey_scene.vox");
    // strcpy(pis->voxelFile, "/Users/nielsbil/Downloads/vox/character/chr_fox.vox");
    // strcpy(pis->voxelFile, "/Users/nielsbil/Downloads/vox/castle.vox");
    // strcpy(pis->voxelFile, "/Users/nielsbil/Downloads/vox/Church_Of_St_Sophia.vox");
    // strcpy(pis->voxelFile, "/Users/nielsbil/Downloads/vox/nuke.vox");
    // strcpy(pis->voxelFile, "/Users/nielsbil/Downloads/vox/scan/dragon.vox");

    PisEngineInitialize(pis);

    const bool* keys = SDL_GetKeyboardState(NULL);

    glm_vec3((vec3){pis->voxelData.dimensions[0]/2.f,
                    pis->voxelData.dimensions[1]/2.f, -2}, ubo.position);

    SDL_SetWindowRelativeMouseMode(pis->window, true);
    float pitch = 0.f, yaw = 0.f;

    float moveSpeed = 0.1;
    ubo.fov = 90.f;

    while(processInput(pis))
    {
        float dx, dy, sensitivity = 0.002;
        SDL_GetRelativeMouseState(&dx, &dy);
        yaw -= dx * sensitivity;
        pitch -= dy * sensitivity;

        ubo.forward[0] = cos(pitch) * sin(yaw);
        ubo.forward[1] = sin(pitch);
        ubo.forward[2] = cos(pitch) * cos(yaw);
        glm_normalize(ubo.forward);

        glm_cross(ubo.forward, (vec3){0, 1, 0}, ubo.right);
        glm_normalize(ubo.right);

        glm_cross(ubo.right, ubo.forward, ubo.up);

        vec3 delta;
        glm_vec3_zero(delta);

        float velocity = moveSpeed; // + deltatime

        if (keys[SDL_SCANCODE_LSHIFT]) {
            velocity *= 3;
        }

        if (keys[SDL_SCANCODE_W]) {
            vec3 temp;
            glm_vec3_scale(ubo.forward, velocity, temp);
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_S]) {
            vec3 temp;
            glm_vec3_scale(ubo.forward, -velocity, temp);
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_A]) {
            vec3 temp;
            glm_vec3_scale(ubo.right, -velocity, temp);
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_D]) {
            vec3 temp;
            glm_vec3_scale(ubo.right, velocity, temp);
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_SPACE]) {
            vec3 temp = {0.0f, velocity, 0.0f};
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_LCTRL]) {
            vec3 temp = {0.0f, -velocity, 0.0f};
            glm_vec3_add(delta, temp, delta);
        }
        if(keys[SDL_SCANCODE_MINUS])    ubo.fov += 0.50;
        if(keys[SDL_SCANCODE_EQUALS])   ubo.fov -= 0.50;
        if(keys[SDL_SCANCODE_LSHIFT] && keys[SDL_SCANCODE_EQUALS])  ubo.fov = 90;

        glm_vec3_add(ubo.position, delta, ubo.position);

        ubo.time = (float)SDL_GetTicks() / 1000.f;

        UpdateUniformBuffer(pis, ubo);

        PisEngineDraw(pis);
    }

    PisEngineCleanup(pis);

    free(pis);

    printf("Program finished\n");

    return 0;
}
