#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp_header.h"

#define MAX_LENGTH 100

/* Function used to read a BMP image from a file */
void readImage(bmp_fileheader **bmpFileHeader, bmp_infoheader **bmpInfoHeader, bmp_pixel ***bmpPixelMatrix, char *path) {
    int i, padding;
    FILE *in = fopen(path, "rb");
    
    if (!in) {
        perror("fopen");
        exit(-1);
    }

    fread(*bmpFileHeader, sizeof(bmp_fileheader), 1, in);
    fread(*bmpInfoHeader, sizeof(bmp_infoheader), 1, in);

    /* Padding for every line */
    padding = (4 - ((*bmpInfoHeader)->width % 4)) % 4;

    *bmpPixelMatrix = malloc((*bmpInfoHeader)->height * sizeof(bmp_pixel *));
    for (i = 0; i < (*bmpInfoHeader)->height; i++) {
        (*bmpPixelMatrix)[i] = malloc((*bmpInfoHeader)->width * sizeof(bmp_pixel));

        fread((*bmpPixelMatrix)[i], sizeof(bmp_pixel), (*bmpInfoHeader)->width, in);

        /* Do NOT read the padding */
        fseek(in, padding, SEEK_CUR);
    }

    fclose(in);
    return;
}

/* Function used to write a BMP image to a file */
void writeImage(bmp_fileheader *bmpFileHeader, bmp_infoheader *bmpInfoHeader, bmp_pixel **bmpPixelMatrix, char *path) {
    int i, padding;
    FILE *out = fopen(path, "wb");
    
    if (!out) {
        perror("fopen");
        exit(-1);
    }

    fwrite(bmpFileHeader, sizeof(bmp_fileheader), 1, out);
    fwrite(bmpInfoHeader, sizeof(bmp_infoheader), 1, out);

    /* Padding for every line */
    padding = (4 - (bmpInfoHeader->width % 4)) % 4;

    for (i = 0; i < bmpInfoHeader->height; i++) {
        fwrite(bmpPixelMatrix[i], sizeof(bmp_pixel), bmpInfoHeader->width, out);

        /* Write '0' bytes for the padding */
        fwrite("0", sizeof(char), padding, out);
    }

    fclose(out);
    return;
}

/* Function used to overlap an image over the one already stored */
void insert(bmp_fileheader **bmpFileHeader, bmp_infoheader **bmpInfoHeader, bmp_pixel ***bmpPixelMatrix, char *path, int x, int y) {
    bmp_fileheader *localBmpFileHeader = malloc(sizeof(bmp_fileheader));
    bmp_infoheader *localBmpInfoHeader = malloc(sizeof(bmp_infoheader));
    bmp_pixel **localBmpPixelMatrix;
    int i, j, padding;

    readImage(&localBmpFileHeader, &localBmpInfoHeader, &localBmpPixelMatrix, path);
    
    padding = (4 - (localBmpInfoHeader->width % 4)) % 4;

    for (i = 0; i < localBmpInfoHeader->height; i++) {
        for (j = 0; j < localBmpInfoHeader->width; j++) {

            /* Check if we are out-of-bounds in the main image */
            if ((*bmpInfoHeader)->height > i + x && (*bmpInfoHeader)->width > j + y) {
                (*bmpPixelMatrix)[i + x][j + y] = localBmpPixelMatrix[i][j];
            }
        }
    }
    return;
}

int main(int argc, char const *argv[]) {
    bmp_fileheader *bmpFileHeader = malloc(sizeof(bmp_fileheader));
    bmp_infoheader *bmpInfoHeader = malloc(sizeof(bmp_infoheader));
    bmp_pixel **bmpPixelMatrix;
    char *command = calloc(MAX_LENGTH, sizeof(char));
    char *path = calloc(MAX_LENGTH, sizeof(char));
    unsigned int x, y, i;

    /* Interactive console */
    scanf("%s", command);
    while (strcmp(command, "quit")) {

        /* Edit command */
        if (!strcmp(command, "edit")) {
            scanf("%s", path);
            readImage(&bmpFileHeader, &bmpInfoHeader, &bmpPixelMatrix, path);
        
        /* Save command */
        } else if (!strcmp(command, "save")) {
            
            /* Check if the image is loaded */
            if (bmpFileHeader->fileMarker1 != 'B') {
                continue;
            }

            scanf("%s", path);
            writeImage(bmpFileHeader, bmpInfoHeader, bmpPixelMatrix, path);

        /* Insert command */
        } else if (!strcmp(command, "insert")) {
            scanf("%s", path);
            scanf("%u", &y);
            scanf("%u", &x);

            insert(&bmpFileHeader, &bmpInfoHeader, &bmpPixelMatrix, path, x, y);
        }

        memset(command, '0', MAX_LENGTH * sizeof(char));
        memset(path, '0', MAX_LENGTH * sizeof(char));
        scanf("%s", command);
    }

    for (i = 0; i < bmpInfoHeader->height; i++) {
        free(bmpPixelMatrix[i]);
    }
    free(bmpPixelMatrix);
    
    return 0;
}
