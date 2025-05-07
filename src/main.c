#include <malloc/_malloc_type.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "pis/engine.h"

bool processInput(PisEngine* pis)
{
    SDL_Event event;

    while(SDL_PollEvent(&event))
    {
        //User requests quit
        if(event.type == SDL_EVENT_QUIT)
            return false;
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

    PisEngineInitialize(pis);

    const bool* keys = SDL_GetKeyboardState(NULL);

    glm_vec3((vec3){0.5, 0.5, -2}, pis->ubo.position);

    SDL_SetWindowRelativeMouseMode(pis->window, true);
    float pitch = 0.f, yaw = 0.f;

    pis->ubo.fov = 90.0f;

    float moveSpeed = 0.075;
    float velocity = moveSpeed; // + deltatime

    while(processInput(pis))
    {
        float dx, dy, sensitivity = 0.002;
        SDL_GetRelativeMouseState(&dx, &dy);
        yaw -= dx * sensitivity;
        pitch += dy * sensitivity;

        pis->ubo.forward[0] = cos(pitch) * sin(yaw);
        pis->ubo.forward[1] = sin(pitch);
        pis->ubo.forward[2] = cos(pitch) * cos(yaw);
        glm_normalize(pis->ubo.forward);

        glm_cross(pis->ubo.forward, (vec3){0, 1, 0}, pis->ubo.right);
        glm_normalize(pis->ubo.right);

        glm_cross(pis->ubo.right, pis->ubo.forward, pis->ubo.up);

        vec3 delta;
        glm_vec3_zero(delta);

        if (keys[SDL_SCANCODE_W]) {
            vec3 temp;
            glm_vec3_scale(pis->ubo.forward, velocity, temp);
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_S]) {
            vec3 temp;
            glm_vec3_scale(pis->ubo.forward, -velocity, temp);
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_A]) {
            vec3 temp;
            glm_vec3_scale(pis->ubo.right, -velocity, temp);
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_D]) {
            vec3 temp;
            glm_vec3_scale(pis->ubo.right, velocity, temp);
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_LSHIFT]) {
            vec3 temp = {0.0f, -velocity, 0.0f};
            glm_vec3_add(delta, temp, delta);
        }
        if (keys[SDL_SCANCODE_LCTRL]) {
            vec3 temp = {0.0f, velocity, 0.0f};
            glm_vec3_add(delta, temp, delta);
        }
        if(keys[SDL_SCANCODE_MINUS])    pis->ubo.fov -= 0.50;
        if(keys[SDL_SCANCODE_EQUALS])   pis->ubo.fov += 0.50;
        if(keys[SDL_SCANCODE_LSHIFT] && keys[SDL_SCANCODE_EQUALS])  pis->ubo.fov = 90;

        glm_vec3_add(pis->ubo.position, delta, pis->ubo.position);

        pis->ubo.time = (float)SDL_GetTicks() / 1000.f;

        PisEngineDraw(pis);
    }

    PisEngineCleanup(pis);

    free(pis);

    printf("Program finished\n");

    return 0;
}
