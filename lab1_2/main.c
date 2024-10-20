#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <complex.h>
#include <stdbool.h>
#include "timer.h"

typedef struct {
    int start;
    int end;
    int max_iter;
    int npoints;
    double min_real;
    double max_real;
    double min_imag;
    double max_imag;
    bool* result;
} ThreadData;

bool is_in_mandelbrot(double complex c, int max_iter) {
    double complex z = 0 + 0 * I;
    for (int i = 0; i < max_iter; i++) {
        z = z * z + c;
        if (cabs(z) > 2.0) {
            return false;
        }
    }
    return true;
}

void* compute_mandelbrot(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start; i < data->end; i++) {
        int row = i / data->npoints;
        int col = i % data->npoints;
        double real = data->min_real + (double)col / data->npoints * (data->max_real - data->min_real);
        double imag = data->min_imag + (double)row / data->npoints * (data->max_imag - data->min_imag);
        double complex c = real + imag * I;
        data->result[i] = is_in_mandelbrot(c, data->max_iter);
    }
    return NULL;
}

void write_to_csv(const char* filename, bool* result, int npoints, double min_real, double max_real, double min_imag, double max_imag) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: unable to open file for writing\n");
        exit(EXIT_FAILURE);
    }

    // Write CSV header
    fprintf(file, "Real,Imag\n");

    for (int row = 0; row < npoints; row++) {
        for (int col = 0; col < npoints; col++) {
            if (result[row * npoints + col]) {
                double real = min_real + (double)col / npoints * (max_real - min_real);
                double imag = min_imag + (double)row / npoints * (max_imag - min_imag);
                fprintf(file, "%f,%f\n", real, imag);
            }
        }
    }
    fclose(file);
}

void write_to_txt(const char* filename, int nthreads, int npoints, double execution_time) {
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        fprintf(stderr, "Error: unable to open file for writing\n");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%d,%d,%f\n", nthreads, npoints, execution_time);
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <nthreads> <npoints>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int nthreads = atoi(argv[1]);
    int npoints = atoi(argv[2]);
    int max_iter = 1000;

    if (nthreads <= 0 || npoints <= 0) {
        fprintf(stderr, "Error: number of threads and points must be positive integers.\n");
        return EXIT_FAILURE;
    }

    double min_real = -2.0, max_real = 1.0;
    double min_imag = -1.5, max_imag = 1.5;

    bool* result = (bool*)malloc(npoints * npoints * sizeof(bool));
    if (result == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for result\n");
        return EXIT_FAILURE;
    }

    pthread_t threads[nthreads];
    ThreadData thread_data[nthreads];

    int points_per_thread = (npoints * npoints) / nthreads;
    for (int i = 0; i < nthreads; i++) {
        thread_data[i].start = i * points_per_thread;
        thread_data[i].end = (i == nthreads - 1) ? npoints * npoints : (i + 1) * points_per_thread;
        thread_data[i].max_iter = max_iter;
        thread_data[i].npoints = npoints;
        thread_data[i].min_real = min_real;
        thread_data[i].max_real = max_real;
        thread_data[i].min_imag = min_imag;
        thread_data[i].max_imag = max_imag;
        thread_data[i].result = result;
    }

    double start, finish;
    GET_TIME(start);

    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&threads[i], NULL, compute_mandelbrot, &thread_data[i]) != 0) {
            fprintf(stderr, "Error: unable to create thread %d\n", i);
            free(result);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    GET_TIME(finish);
    double execution_time = finish - start;

    write_to_csv("mandelbrot.csv", result, npoints, min_real, max_real, min_imag, max_imag);
    write_to_txt("mandelbrot_data.txt", nthreads, npoints, execution_time);

    printf("Execution time: %f seconds\n", execution_time);

    free(result);

    return EXIT_SUCCESS;
}
