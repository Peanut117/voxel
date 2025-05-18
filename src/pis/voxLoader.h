#ifndef VOX_LOADER_H
#define VOX_LOADER_H

#include <stdint.h>

#include <cglm/cglm.h>

typedef struct Chunk {
    char tag[4];
    uint32_t size;
    uint32_t childSize;
} Chunk;

typedef struct Model {
    int32_t id;
    uint32_t size[3];
    uint32_t translation[3];
    uint8_t* data;
} Model;

typedef struct __attribute__((packed))
{
    uint8_t x;
    uint8_t z;
    uint8_t y;
    uint8_t colorIndex;
} VoxelData;

typedef struct __attribute__((packed))
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} VoxelColor;

typedef struct {
    ivec3 dimensions;
    size_t bufferSize;
    uint8_t* data;
    VoxelColor* palette;
} Vox;

Vox ReadVoxFile(char* fileName);
void CloseVoxFile(Vox vox);

#endif
