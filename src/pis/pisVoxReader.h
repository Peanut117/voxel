#ifndef PIS_VOX_READER_H
#define PIS_VOX_READER_H

#include <stdint.h>

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} Size;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

typedef struct {
    Color color;
} Material;

typedef struct PisVox {
    Size size;
    uint8_t* voxels;
    Material materials[256];
} PisVox;

PisVox PisVoxReadFromFile(char* fileName);
void DestroyPisVox(PisVox pisV);

#endif
