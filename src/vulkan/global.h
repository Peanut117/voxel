#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL3/SDL_events.h>
#include "define.h"

/********************Global Variables*******************/
extern UniformBufferObject ubo;

/********************Functions*******************/
bool processInput(SDL_Event event);

#endif
