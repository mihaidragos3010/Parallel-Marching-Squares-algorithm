#include "marchingSquares.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <math.h>

// Creates a map between the binary configuration (e.g. 0110_2) and the corresponding pixels
// that need to be set on the output image. An array is used for this map since the keys are
// binary numbers in 0-15. Contour images are located in the './contours' directory.
void init_contour_map(ppm_image ***contour_map, int id, int nr_threads) {

    int start = id * CONTOUR_CONFIG_COUNT / nr_threads;
    int end = std::min((id + 1) * CONTOUR_CONFIG_COUNT / nr_threads, CONTOUR_CONFIG_COUNT);
    for (int i = start; i < end; i++) {
        char filename[FILENAME_MAX_SIZE];
        sprintf(filename, "./contours/%d.ppm", i);
        (*contour_map)[i] = read_ppm(filename);
    }

    return;
}

// Creates a new image scaled in a predefined format.
void rescale_image(ppm_image **new_image, ppm_image *image, int id, int nr_threads) {
    uint8_t sample[3];

    // If the image is already within limits
    if (image->x <= RESCALE_X && image->y <= RESCALE_Y) {
        (*new_image)->x = image->x;
        (*new_image)->y = image->y;
        memmove((*new_image)->data, image->data, image->x * image->y * sizeof(ppm_pixel));
        return ;
    }

    (*new_image)->x = RESCALE_X;
    (*new_image)->y = RESCALE_Y;

    // Use bicubic interpolation for scaling
    int start = id * RESCALE_X / nr_threads;
    int end = std::min((id + 1) * RESCALE_X / nr_threads, RESCALE_X);
    for (int i = start; i < end; i++) {
        for (int j = 0; j < (*new_image)->y; j++) {
            float u = (float)i / (float)((*new_image)->x - 1);
            float v = (float)j / (float)((*new_image)->y - 1);
            sample_bicubic(image, u, v, sample);

            (*new_image)->data[i * (*new_image)->y + j].red = sample[0];
            (*new_image)->data[i * (*new_image)->y + j].green = sample[1];
            (*new_image)->data[i * (*new_image)->y + j].blue = sample[2];
        }
    }

    return;
}

// Corresponds to step 1 of the marching squares algorithm, which focuses on sampling the image.
// Builds a p x q grid of points with values which can be either 0 or 1, depending on how the
// pixel values compare to the `sigma` reference value. The points are taken at equal distances
// in the original image, based on the `step_x` and `step_y` arguments.
void sample_grid(unsigned char ***grid, ppm_image *image, int step_x, int step_y, unsigned char sigma, int id,int nr_threads) {
    int p = image->x / step_x;
    int q = image->y / step_y;

    int start = id * p / nr_threads;
    int end = std::min((id + 1) * p / nr_threads, p);

    for (int i = start; i < end; i++) {
        for (int j = 0; j < q; j++) {
            ppm_pixel curr_pixel = image->data[i * step_x * image->y + j * step_y];

            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > sigma) {
                (*grid)[i][j] = 0;
            } else {
                (*grid)[i][j] = 1;
            }
        }
    }

    // last sample points have no neighbors below / to the right, so we use pixels on the
    // last row / column of the input image for them
    for (int i = start; i < end; i++) {
        ppm_pixel curr_pixel = image->data[i * step_x * image->y + image->x - 1];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > sigma) {
            (*grid)[i][q] = 0;
        } else {
            (*grid)[i][q] = 1;
        }
    }
    for (int j = 0; j < q; j++) {
        ppm_pixel curr_pixel = image->data[(image->x - 1) * image->y + j * step_y];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > sigma) {
            (*grid)[p][j] = 0;
        } else {
            (*grid)[p][j] = 1;
        }
    }

    return;
}

// Updates a particular section of an image with the corresponding contour pixels.
// Used to create the complete contour image.
void update_image(ppm_image *image, ppm_image *contour, int x, int y) {
    for (int i = 0; i < contour->x; i++) {
        for (int j = 0; j < contour->y; j++) {
            int contour_pixel_index = contour->x * i + j;
            int image_pixel_index = (x + i) * image->y + y + j;

            image->data[image_pixel_index].red = contour->data[contour_pixel_index].red;
            image->data[image_pixel_index].green = contour->data[contour_pixel_index].green;
            image->data[image_pixel_index].blue = contour->data[contour_pixel_index].blue;
        }
    }
}

// Corresponds to step 2 of the marching squares algorithm, which focuses on identifying the
// type of contour which corresponds to each subgrid. It determines the binary value of each
// sample fragment of the original image and replaces the pixels in the original image with
// the pixels of the corresponding contour image accordingly.
void march(ppm_image *image, unsigned char **grid, ppm_image **contour_map, int id, int nr_threads) {
    int p = image->x / STEP;
    int q = image->y / STEP;

    int start = id * p / nr_threads;
    int end = std::min((id + 1) * p / nr_threads, p);
    for (int i = start; i < end; i++) {
        for (int j = 0; j < q; j++) {
            unsigned char k = 8 * grid[i][j] + 4 * grid[i][j + 1] + 2 * grid[i + 1][j + 1] + 1 * grid[i + 1][j];
            update_image(image, contour_map[k], i * STEP, j * STEP);
        }
    }
}

// Calls `free` method on the utilized resources.
void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x) {
    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        free(contour_map[i]->data);
        free(contour_map[i]);
    }
    free(contour_map);

    for (int i = 0; i <= image->x / step_x; i++) {
        free(grid[i]);
    }
    free(grid);

    free(image->data);
    free(image);
}



