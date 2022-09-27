#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmp_header.h"

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
    int i, j;

    readImage(&localBmpFileHeader, &localBmpInfoHeader, &localBmpPixelMatrix, path);
    
    for (i = 0; i < localBmpInfoHeader->height; i++) {
        for (j = 0; j < localBmpInfoHeader->width; j++) {

            /* Check if we are out-of-bounds in the main image */
            if ((*bmpInfoHeader)->height > i + x && (*bmpInfoHeader)->width > j + y) {
                (*bmpPixelMatrix)[i + x][j + y] = localBmpPixelMatrix[i][j];
            }
        }
    }

    /* Free the memory */
    for (i = 0; i < localBmpInfoHeader->height; i++) {
        free(localBmpPixelMatrix[i]);
    }
    free(localBmpPixelMatrix);
    free(localBmpInfoHeader);
    free(localBmpFileHeader);
    
    return;
}

/* Function to decide which interval is greater */
int max (int x, int y) {
    if (x > y) {
        return 0;
    }

    return 1;
}

/* Translation function that returns y */
int translateLineY(int xA, int xB, int yA, int yB, int x) {
    return (yB - yA) * (x - xA) / (xB - xA) + yA;
}

/* Translation function that returns x */
int translateLineX(int xA, int xB, int yA, int yB, int y) {
    return (y - yA) * (xB - xA) / (yB - yA) + xA;
}

/* Function used to draw when Oy interval is greater */
void drawY(bmp_infoheader *bmpInfoHeader, bmp_pixel ***bmpPixelMatrix, bmp_pixel *drawColor, int lineWidth, 
        int x1, int y1, int x2, int y2) {
    int i, j, x, y, start, end;

    if (y2 >= y1) {
        start = y1;
        end = y2;
    } else {
        start = y2;
        end = y1;
    }

    for (y = start; y <= end; y++) {
        x = translateLineX(x1, x2, y1, y2, y);

        /* Draw the adjacent pixels */ 
        if (lineWidth > 1) {
            for (i = x - lineWidth / 2; i <= x + lineWidth / 2; i++) {
                for (j = y - lineWidth / 2; j <= y + lineWidth / 2; j++) {

                    /* Check if out-of-bound */
                    if (i >= 0 && j >= 0 && i < bmpInfoHeader->height && j < bmpInfoHeader->width) {
                        (*bmpPixelMatrix)[i][j] = *drawColor;
                    }
                }
            }
        } else {
            (*bmpPixelMatrix)[x][y] = *drawColor;
        }
    }

    return;
}

/* Function used to draw when Ox interval is greater */
void drawX(bmp_infoheader *bmpInfoHeader, bmp_pixel ***bmpPixelMatrix, bmp_pixel *drawColor, int lineWidth, 
        int x1, int y1, int x2, int y2) {
    int i, j, x, y, start, end;

    if (x2 >= x1) {
        start = x1;
        end = x2;
    } else {
        start = x2;
        end = x1;
    }

    for (x = start; x <= end; x++) {
        y = translateLineY(x1, x2, y1, y2, x);

        /* Draw the adjacent pixels */
        if (lineWidth > 1) {
            for (i = x - lineWidth / 2; i <= x + lineWidth / 2; i++) {
                for (j = y - lineWidth / 2; j <= y + lineWidth / 2; j++) {

                    /* Check if out-of-bound */
                    if (i >= 0 && j >= 0 && i < bmpInfoHeader->height && j < bmpInfoHeader->width) {
                        (*bmpPixelMatrix)[i][j] = *drawColor;
                    }
                }
            }
        } else {
            (*bmpPixelMatrix)[x][y] = *drawColor;
        }
    }

    return;
}

/* Function used to draw a line */
void drawLine(bmp_infoheader *bmpInfoHeader, bmp_pixel ***bmpPixelMatrix, bmp_pixel *drawColor, int lineWidth, 
        int x1, int y1, int x2, int y2) {
    int aux;

    /* Oy interval is greater */
    if (max(abs(x2 - x1), abs(y2 - y1))) {

        /* Swap if we don't start in the "top left" corner */
        if (x1 > x2) {
            aux = x1;
            x1 = x2;
            x2 = aux;

            aux = y1;
            y1 = y2;
            y2 = aux;
        }

        drawY(bmpInfoHeader, bmpPixelMatrix, drawColor, lineWidth, x1, y1, x2, y2);
        
    /* Ox interval is greater */
    } else {
        drawX(bmpInfoHeader, bmpPixelMatrix, drawColor, lineWidth, x1, y1, x2, y2);   
    }

    return;
}

/* Function used to fill pixels */
void fill(bmp_pixel ***bmpPixelMatrix, bmp_infoheader *bmpInfoHeader, int y, int x, bmp_pixel *drawColor, unsigned char R, unsigned char G, unsigned char B) {
    if (x >= 0 && y >= 0 && x < bmpInfoHeader->height && y < bmpInfoHeader->width) {
        
        if ((*bmpPixelMatrix)[x][y].R == R && (*bmpPixelMatrix)[x][y].G == G && (*bmpPixelMatrix)[x][y].B == B) {

            (*bmpPixelMatrix)[x][y] = *drawColor;
        
            fill(bmpPixelMatrix, bmpInfoHeader, y, x + 1, drawColor, R, G, B);
            fill(bmpPixelMatrix, bmpInfoHeader, y + 1, x, drawColor, R, G, B);
            fill(bmpPixelMatrix, bmpInfoHeader, y, x - 1, drawColor, R, G, B);
            fill(bmpPixelMatrix, bmpInfoHeader, y - 1, x, drawColor, R, G, B);
        }
    
    }

    return;
}

int main(int argc, char const *argv[]) {
    bmp_fileheader *bmpFileHeader = malloc(sizeof(bmp_fileheader));
    bmp_infoheader *bmpInfoHeader = malloc(sizeof(bmp_infoheader));
    bmp_pixel **bmpPixelMatrix, *drawColor = calloc(1, sizeof(bmp_pixel));
    char *command = calloc(MAX_LENGTH, sizeof(char));
    char *path = calloc(MAX_LENGTH, sizeof(char));
    unsigned int lineWidth = 1, x1, y1, x2, y2, x3, y3, height, width, R, G, B, i;

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
            if (bmpFileHeader->fileMarker1 != 'B' || bmpFileHeader->fileMarker2 != 'M') {
                perror("Nothing is stored");
                exit(-1);
            }

            scanf("%s", path);
            writeImage(bmpFileHeader, bmpInfoHeader, bmpPixelMatrix, path);

        /* Insert command */
        } else if (!strcmp(command, "insert")) {
            scanf("%s", path);
            scanf("%u", &y1);
            scanf("%u", &x1);

            /* Check if the image is loaded  */
            if (bmpPixelMatrix == NULL) {
                perror("Nothing is stored");
                exit(-1);
            }

            insert(&bmpFileHeader, &bmpInfoHeader, &bmpPixelMatrix, path, x1, y1);
        
        /* Set command */
        } else if (!strcmp(command, "set")) {
            memset(command, '\0', MAX_LENGTH * sizeof(char));
            scanf("%s", command);

            /* Read the drawing color */
            if (!strcmp(command, "draw_color")) {
                scanf("%d %d %d", &R, &G, &B);

                drawColor->R = R;
                drawColor->G = G;
                drawColor->B = B;

            /* Read the drawing line width */
            } else if(!strcmp(command, "line_width")) {
                scanf("%d", &lineWidth);
            }

        /* Draw command */
        } else if (!strcmp(command, "draw")) {
            memset(command, '\0', MAX_LENGTH * sizeof(char));
            scanf("%s", command);

            /* Check if the image is loaded  */
            if (bmpPixelMatrix == NULL) {
                perror("Nothing is stored");
                exit(-1);
            }

            /* Draw line */
            if (!strcmp(command, "line")) {
                scanf("%d %d %d %d", &y1, &x1, &y2, &x2);

                drawLine(bmpInfoHeader, &bmpPixelMatrix, drawColor, lineWidth, x1, y1, x2, y2);

            /* Draw rectangle */
            } else if (!strcmp(command, "rectangle")) {
                scanf("%d %d %d %d", &y1, &x1, &width, &height);
                
                drawLine(bmpInfoHeader, &bmpPixelMatrix, drawColor, lineWidth, x1, y1, x1 + height, y1);
                drawLine(bmpInfoHeader, &bmpPixelMatrix, drawColor, lineWidth, x1, y1, x1, y1 + width);
                drawLine(bmpInfoHeader, &bmpPixelMatrix, drawColor, lineWidth, x1 + height, y1, x1 + height, y1 + width);
                drawLine(bmpInfoHeader, &bmpPixelMatrix, drawColor, lineWidth, x1, y1 + width, x1 + height, y1 + width);

            /* Draw triangle */
            } else if (!strcmp(command, "triangle")) {
                scanf("%d %d %d %d %d %d", &y1, &x1, &y2, &x2, &y3, &x3);

                drawLine(bmpInfoHeader, &bmpPixelMatrix, drawColor, lineWidth, x1, y1, x3, y3);
                drawLine(bmpInfoHeader, &bmpPixelMatrix, drawColor, lineWidth, x3, y3, x2, y2);
                drawLine(bmpInfoHeader, &bmpPixelMatrix, drawColor, lineWidth, x2, y2, x1, y1);
            } else {
                printf("Command not found\n");
            }
        
        /* Fill command */
        } else if (!strcmp(command, "fill")) {
            scanf("%d %d", &y1, &x1);


            if (bmpPixelMatrix == NULL) {
                perror("Nothing is stored");
                exit(-1);
            }

            R = bmpPixelMatrix[x1][y1].R;
            G = bmpPixelMatrix[x1][y1].G;
            B = bmpPixelMatrix[x1][y1].B;

            fill(&bmpPixelMatrix, bmpInfoHeader, y1, x1, drawColor, R, G, B);
        } else {
            printf("Command not found\n");
        }

        memset(command, '\0', MAX_LENGTH * sizeof(char));
        memset(path, '\0', MAX_LENGTH * sizeof(char));
        scanf("%s", command);
    }

    /* Free the memory */
    if (bmpPixelMatrix != NULL) {
        for (i = 0; i < bmpInfoHeader->height; i++) {
            free(bmpPixelMatrix[i]);
        }
        free(bmpPixelMatrix);
    }

    free(bmpInfoHeader);
    free(bmpFileHeader);
    free(command);
    free(path);
    
    return 0;
}
