#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include "engine.h"

void CreateSwapchain(PisEngine* pis, uint32_t width, uint32_t height);
void CreateDrawImages(PisEngine* pis, uint32_t width, uint32_t height);

#endif
