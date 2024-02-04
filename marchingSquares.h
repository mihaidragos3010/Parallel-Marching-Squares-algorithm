#ifndef THREAD_FUNCTIONS_H
#define THREAD_FUNCTIONS_H

#include "helpers.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define RGB_COMPONENT_COLOR     255
#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048

// The structure of marchingSquares function arguments
typedef struct {
    int id;
    int nr_threads;
    ppm_image *image;
    ppm_image *scaled_image;
    ppm_image **contour_map;
    unsigned char **grid;
    pthread_barrier_t *barrier;
}arguments_thread;

void init_contour_map(ppm_image ***contour_map, int id, int nr_threads);
void rescale_image(ppm_image **new_image, ppm_image *image, int id, int nr_threads);
void sample_grid(unsigned char ***grid, ppm_image *image, int step_x, int step_y, unsigned char sigma, int id, int nr_threads);
void update_image(ppm_image *image, ppm_image *contour, int x, int y);
void march(ppm_image *image, unsigned char **grid, ppm_image **contour_map, int id, int nr_threads);
void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x);
#endif