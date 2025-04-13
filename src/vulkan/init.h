#ifndef INIT_H
#define INIT_H

/******************NOTES******************/
/* 1. Create the window with SDL3
 *	1.1. Set initial width, height and name
 *
 * 2. Create the instance
 *	2.1. Set application info
 *	2.2. Set all the need Extentions
 *
 * 3. Pick a physical device to use
 *	3.1. Checks if the device is suitable with our needs
 *	  3.1.1. Checks if the device extentions are supported
 *	  3.1.2. TO-DO check if swapchain needs are supported
 *
 * 4. Create the device
 *	4.1. Create the queues
 *
*/
/*****************************************/

/************Global Function Declarations**********/
int initVulkan(void);

void cleanupInitVulkan(void);

#endif
