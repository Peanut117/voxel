#include <malloc/_malloc_type.h>
#include <stdio.h>
#include <stdlib.h>

#include "pis/engine.h"

bool processInput(void)
{
	SDL_Event event;

	while(SDL_PollEvent(&event) != 0)
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

    while(processInput())
    {
        PisEngineDraw(pis);
    }

    PisEngineCleanup(pis);

    free(pis);

    printf("Program finished\n");

	return 0;
}
