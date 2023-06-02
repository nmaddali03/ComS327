#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void setUp(int grid[23][23]);
void printGrid(int grid[23][23]);
void calculateSandPile(int grid[23][23], int y, int x);

void dropSand(int grid[23][23], int y, int x);
void dropAroundOrigin(int grid[23][23], int y, int x);

int main(int argc, char *argv[]) {
    int counter = 1;
    int grid[23][23];
    int fps = 60; // default fps
    setUp(grid);//setting up the grid

    if (argc >= 2) {
        if (!strcmp(argv[1], "--fps")) { // user entered fps
            counter = 3;
            fps = atoi(argv[2]);
        }
        while (counter < argc) {
            int y = atoi(argv[counter]);
            int x = atoi(argv[++counter]);
            int z = atoi(argv[++counter]);
            if(y == 11 && x == 11 && z == -1){
                printf("Sink cannot be placed in the center-cell\n");
                return 0;
            }
            grid[y][x] = z;
            counter++;
        }
    }
    //start computing the spaces on the grid for the sand
    while (1) {
        usleep(1000000 / fps);
        printGrid(grid);
        dropSand(grid, 11, 11);
        calculateSandPile(grid, 11, 11);
    }
}

void calculateSandPile(int grid[23][23], int y, int x) {
    //Check for out of bounds
    if (y >= 23 || x >= 23 || y <= -1 || x <= -1) { return; }
    if (grid[y][x] == -1) { return; }
    if (grid[y][x] >= 9) {
        grid[y][x] = 1;

        dropAroundOrigin(grid, y, x);

        calculateSandPile(grid, y - 1, x - 1);
        calculateSandPile(grid, y - 1, x);
        calculateSandPile(grid, y - 1, x + 1);
        calculateSandPile(grid, y, x - 1);
        calculateSandPile(grid, y, x + 1);
        calculateSandPile(grid, y + 1, x - 1);
        calculateSandPile(grid, y + 1, x);
        calculateSandPile(grid, y + 1, x + 1);
    }
}

void dropAroundOrigin(int grid[23][23], int y, int x) {
    dropSand(grid, y - 1, x - 1);
    dropSand(grid, y - 1, x);
    dropSand(grid, y - 1, x + 1);
    dropSand(grid, y, x - 1);
    dropSand(grid, y, x + 1);
    dropSand(grid, y + 1, x - 1);
    dropSand(grid, y + 1, x);
    dropSand(grid, y + 1, x + 1);
}

void dropSand(int grid[23][23], int y, int x) {
    //Check for out of bounds
    if (y >= 23 || x >= 23 || y <= -1 || x <= -1) { return; }
    if (grid[y][x] == -1) { return; }

    grid[y][x] = grid[y][x] + 1;
}

void setUp(int grid[23][23]) {
    int i, j;
    for (j = 0; j < 23; j++) {
        for (i = 0; i < 23; i++) {
            grid[j][i] = 0;
        }
    }
}

void printGrid(int grid[23][23]) {
    int x, y;
    for (y = 0; y < 23; y++) {
        for (x = 0; x < 23; x++) {
            if (grid[y][x] == -1) {
                printf("%3c ", '#');
            } else {
                printf("%3d ", grid[y][x]);
            }
        }
        printf("\n");
    }
    printf("\n");
}