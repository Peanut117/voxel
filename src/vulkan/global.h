#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL3/SDL_events.h>
#include "define.h"

/********************Global Variables*******************/
extern UniformBufferObject ubo;

static const int WINDOW_WIDTH = 800;
static const int WINDOW_HEIGHT = 600;

/********************Functions*******************/
bool processInput(SDL_Event event);

#endif
