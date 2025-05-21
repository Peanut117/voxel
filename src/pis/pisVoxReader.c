#include "pisVoxReader.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PisVox PisVoxReadFromFile(char* fileName)
{
    PisVox pisV;

    // Open file
    FILE* fptr = fopen(fileName, "rb");
    if(fptr == NULL)
    {
        fprintf(stderr, "Failed to read file: %s\n", fileName);
        exit(-1);
    }

    // Read header to check if this is a PisV file and version check
    char header[8];
    fread(&header, 1, 8, fptr);

    if(strncmp(header, "PISV 001", 8) != 0)
    {
        fprintf(stderr, "Wrong file type\n");
        fclose(fptr);
        fptr = NULL;
        exit(-1);
    }

    // Read the dimensions of the voxel world
    fread(&pisV.size, sizeof(Size), 1, fptr);

    // Calculate the array size X*Y*Z
    uint32_t arraySize = pisV.size.x * pisV.size.y * pisV.size.z;

    // Read the byte count and add bytes
    uint32_t dataPtr = 0;
    pisV.voxels = malloc(arraySize);
    while(dataPtr < arraySize)
    {
        uint16_t byteCount = 0;
        fread(&byteCount, 2, 1, fptr);
        if(byteCount == 0)
        {
            fprintf(stderr, "Data not read right, 0 before end of file\n");
            free(pisV.voxels);
            pisV.voxels = NULL;
            fclose(fptr);
            fptr = NULL;
            exit(-1);
        }

        uint8_t byte = 0;
        fread(&byte, 1, 1, fptr);
        for(uint32_t i = 0; i < byteCount; i++)
        {
            pisV.voxels[dataPtr + i] = byte;
        }

        dataPtr += byteCount;
    }

    // Read materials tag
    char materialsTag[4];
    fread(&materialsTag, 1, 4, fptr);

    fread(pisV.materials, sizeof(Material), 256, fptr);

    fclose(fptr);

    return pisV;
}

void DestroyPisVox(PisVox pisV)
{
    free(pisV.voxels);
    pisV.voxels = NULL;
}
