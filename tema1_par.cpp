// Author: APD team, except where source was noted

#include "helpers.h"
#include "marchingSquares.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048

void init_images_grid(ppm_image *image,ppm_image **scaled_image, ppm_image ***contour_map, 
                unsigned char ***grid, pthread_barrier_t *barrier, arguments_thread *arg, int nr_threads){

    (*contour_map) = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
    if (!(*contour_map)) {
        fprintf(stderr, "Unable to allocate memory for contour map\n");
        exit(1);
    }

    int p = image->x / STEP;
    int q = image->y / STEP;
    (*grid) = (unsigned char **)malloc((p + 1) * sizeof(unsigned char*));
    if (!(*grid)) {
        fprintf(stderr, "Unable to allocate memory for grid\n");
        exit(1);
    }

    for (int i = 0; i <= p; i++) {
        (*grid)[i] = (unsigned char *)malloc((q + 1) * sizeof(unsigned char));
        if (!(*grid)[i]) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }
    }

    (*scaled_image) = (ppm_image *)malloc(sizeof(ppm_image));
    if (!(*scaled_image)) {
        fprintf(stderr, "Unable to allocate memory for scaled image\n");
        exit(1);
    }

    (*scaled_image)->x = RESCALE_X;
    (*scaled_image)->y = RESCALE_Y;
    (*scaled_image)->data = (ppm_pixel*)malloc(RESCALE_X * RESCALE_Y * sizeof(ppm_pixel));
    if (!(*scaled_image)) {
        fprintf(stderr, "Unable to allocate memoryfor data in scaled image\n");
        exit(1);
    }

    for(int id=0; id < nr_threads;id++){
        arg[id].id = id;
        arg[id].nr_threads = nr_threads;
        arg[id].image = image;
        arg[id].scaled_image = (*scaled_image);
        arg[id].contour_map = (*contour_map);
        arg[id].grid = (*grid);
        arg[id].barrier = barrier;
    }

    return;
}

void *marchingSquares(void* args){

    int id = ((arguments_thread*)args)->id;
    int nr_threads = ((arguments_thread*)args)->nr_threads;
    pthread_barrier_t *barrier = ((arguments_thread*)args)->barrier;
    ppm_image *image = ((arguments_thread*)args)->image;
    ppm_image *scaled_image = ((arguments_thread*)args)->scaled_image;
    ppm_image **contour_map = ((arguments_thread*)args)->contour_map;
    unsigned char **grid = ((arguments_thread*)args)->grid;
    int err;

    // 0. Initialize contour map
    init_contour_map(&contour_map, id, nr_threads);

    // 1. Rescale the image
    rescale_image(&scaled_image, image, id, nr_threads);

    err = pthread_barrier_wait(barrier);
    if(err == EINVAL){
        std::cout << "Eroare la functia barierei" << std::endl;
            exit(-1);
    }
    
    // 2. Sample the grid
    sample_grid(&grid, scaled_image, STEP, STEP, SIGMA, id, nr_threads);

    err = pthread_barrier_wait(barrier);
    if(err == EINVAL){
        std::cout << "Eroare la functia barierei" << std::endl;
            exit(-1);
    }

    // 3. March the squares
    march(scaled_image, grid, contour_map, id, nr_threads);

    return NULL;
    
}

int main(int argc, char *argv[]) {
    
    if (argc < 4) {
        fprintf(stderr, "Usage: ./tema1 <in_file> <out_file> <P>\n");
        return 1;
    }

    int nr_threads = atoi(argv[3]);
    pthread_t threads[nr_threads];
    arguments_thread arg[nr_threads];
    int err;

    ppm_image *image = read_ppm(argv[1]); 

    pthread_barrier_t barrier;
    err = pthread_barrier_init(&barrier, NULL, nr_threads);
    if (err) {
            std::cout << "Eroare la initializarea barierei" << std::endl;
            exit(-1);
    } 

    ppm_image *scaled_image;
    ppm_image **contour_map;
    unsigned char **grid; 
    init_images_grid(image, &scaled_image, &contour_map, &grid, &barrier, arg, nr_threads);

    for (int id = 0; id < nr_threads; id++) {

        err = pthread_create(&threads[id], NULL, marchingSquares, &arg[id]);
 
        if (err) {
            std::cout << "Eroare la crearea thread-ului" << id << std::endl;
            exit(-1);
        }
    }
 
    void *status;
    for (int id = 0; id < nr_threads; id++) {
        err = pthread_join(threads[id], &status);
 
        if (err) {
            std::cout << "Eroare la asteptarea thread-ului" << id << std::endl;
            exit(-1);
        }
    }

    // 4. Write output
    write_ppm(scaled_image, argv[2]);

    // 5. Freeing used memory
    free_resources(scaled_image, contour_map, grid, STEP);
    free(image->data);
    free(image);
    
    return 0;
}
