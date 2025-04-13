#ifndef SETUP_H
#define SETUP_H

/******************NOTES******************/
/* 1. Create the swapchain
 *	1.1. choose a usable image format, format and colorspace
 *	1.2. choose usable image extent, probably just the window sizeof
 *	1.3. choose usable present mode
 *	1.3. Choose usable swapchain image count
 *	1.4. Get the swapchain images
 *	1.5. Create imageviews from these images
 * 2. Create the renderpass
 * 3. Create graphics pipeline
*/
/*****************************************/

/******************TODO******************/
/* 1. Check if there is an dedicated transfer queue family and use that if available
 *  1.1. image sharing mode should be set to concurrent
 * 2. graphics pipeline cache
*/
/*****************************************/

/************Global Function Declarations**********/
int setupVulkan(void);

void cleanupSetupVulkan(void);

#endif
