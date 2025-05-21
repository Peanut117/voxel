#include <stdio.h>
#include <stdlib.h>

#include "misc.h"

void ExitError(char* message)
{
    if(message != NULL)
        fprintf(stderr, "%s\n", message);

    exit(-1);
}

