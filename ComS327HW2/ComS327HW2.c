#include<stdio.h>
#include<time.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>

#define bias 199
#define Row 21
#define Column 80

#define NULL ((void *)0)

typedef struct tile{
    char tile[Row][Column];
    int north_exit;
    int south_exit;
    int west_exit;
    int east_exit;
}tile_t;

//generate boulders border and clearings to start the map
void gen_boulders_clearings(tile_t *f){
    for(int i=0; i<21; i++){
        for(int j=0; j<80;j++){
            f->tile[i][j] = '.'; //clearings on entire grid
        }
    }
    //generate boulders on top and bottom of grid
    for(int i=0; i<80;i++){
        f->tile[0][i] = '%'; 
        f->tile[20][i] = '%';
    }
    //generate boulders on left and right of grid
    for(int i = 0;i<21;i++){
        f->tile[i][0] = '%';
        f->tile[i][79] = '%';
    }
}

void gen_grass(tile_t *f){ //generate tall grass on map
    srand(time(NULL));
    int x = rand()%7+1;
    for(int k=0;k<=x;k++){
        int i = rand()%19;
        int j = rand()%78;
        for(int m = i-5;m<i+5;m++){
            for(int n=j-5; n<j+5; n++){
                if(m>1 && n>1 && m<19 && n<75){
                    f->tile[m][n] = ':';
                }
            }
        }
    }
}

//generate paths - map to map feature
void gen_path(tile_t *World[399][399], int curY, int curX, tile_t *f){
    srand(time(NULL));
    if(curX -1 >= 0){
        if(World[curY][curX-1]){
            tile_t *west = World[curY][curX-1];
            f->west_exit = west->east_exit;
        }else{
            f->west_exit = rand()%14+3;
        }
    }else{
        f->west_exit = rand()%14+3;
    }
    if(curX+1<399){
        if(World[curY][curX+1]){
            tile_t *east=World[curY][curX+1];
            f->east_exit = east->west_exit;
        }else{
            f->east_exit = rand()%14+3;
        }
    }else{
        f->east_exit = rand()%14+3;
    }
    if(curY+1<399){
        if(World[curY+1][curX]){
            tile_t *south = World[curY+1][curX];
            f->south_exit = south->north_exit;
        }else{
            f->south_exit = rand()%74+3;
        }
    }else{
        f->south_exit = rand()%74+3;
    }
    if(curY-1>=0){
        if(World[curY-1][curX]){
            tile_t *north = World[curY-1][curX];
            f->north_exit = north->south_exit;
        }else{
            f->north_exit = rand()%74+3;
        }
    }else{
        f->north_exit = rand()%74+3;
    }

    int x_index;
    int y_index;
    int flag;
    while(1){
        x_index = rand()%60+10;
        y_index = rand()%12+4;
        if(y_index != f->east_exit && y_index != f->west_exit && x_index!= f->north_exit && x_index != f->south_exit){
            break;
        }
    }

    for(int i=0; i<=x_index;i++){ //path for west to east between maps
        f->tile[f->west_exit][i] = '#'; //create boulders based on previous map exit/entrance
    }
    flag = f->west_exit-y_index;
    if(flag>0){
        for(int i=f->west_exit; i>= y_index; i--){
            f->tile[i][x_index] = '#';
        }
    }else if(flag<0){
        for(int i=y_index; i>= f->west_exit; i--){
            f->tile[i][x_index] = '#';
        }
    }

    for(int i=80;i>=x_index;i--){ //path for east to west between maps
        f->tile[f->east_exit][i] = '#';
    }
    flag = f->east_exit - y_index;
    if(flag>0){
        for(int i = f->east_exit; i>= y_index; i--){
            f->tile[i][x_index] = '#'; 
        }
    }else if(flag<0){
        for(int i = f->east_exit; i<=y_index;i++){
            f->tile[i][x_index] = '#';
        }
    }

    for(int i=0; i<= y_index; i++){ //path for north to south between maps
        f->tile[i][f->north_exit] = '#';
    }
    flag = f->north_exit-x_index;
    if(flag>0){
        for(int i = f->north_exit; i>= x_index; i--){
            f-> tile[y_index][i] = '#';
        }
    }else if(flag<0){
        for(int i = f->north_exit; i<= x_index; i++){
            f->tile[y_index][i] = '#';
        }
    }

    for(int i=20;i>=y_index; i--){ //path for south to north between maps
        f-> tile[i][f->south_exit]='#';
    }
    flag = f->south_exit - x_index;
    if(flag>0){
        for(int i = f->south_exit; i >= x_index; i--){
            f->tile[y_index][i]='#';
        }
    }else if(flag<0){
        for(int i = f->south_exit; i <= x_index; i++){
            f->tile[y_index][i] = '#';
        }
    }
}

void gen_buildings(tile_t *f, float prob){ //generate PokeMart and centers
    int iprob = prob;
    int ran = rand()%100+1;
    if(ran <= iprob){
        f->tile[1][f->north_exit+1] = 'C';
        f->tile[2][f->north_exit+1] = 'C';
        f->tile[1][f->north_exit+2] = 'C';
        f->tile[2][f->north_exit+2] = 'C';

        f->tile[f->east_exit-1][77] = 'M';
        f->tile[f->east_exit-1][78] = 'M';
        f->tile[f->east_exit-2][77] = 'M';
        f->tile[f->east_exit-2][78] = 'M';
    }
}

void gen_trees(tile_t *f){ //generate trees on map
    for(int i=0;i<30;i++){
        int xrand = rand()%71+4;
        int yrand = rand()%12+4;
        if(f->tile[yrand][xrand] == '.'){
            f->tile[yrand][xrand] = '^';
        }
    }
}

void gen_rocks(tile_t *f){ //generate rocks throughout the map
    for(int i=0; i<30; i++){
        int xrand = rand()%71+4;
        int yrand = rand()%12+4;
        if(f->tile[yrand][xrand] == '.'){
            f->tile[yrand][xrand] = '%';
        }
    }
}

float probability(int curY, int curX){ //prob function of the distance
    float p;
    int d = sqrt((199-curY)*(199-curY)+(199-curX)*(199-curX));
    if(curX == 199 && curY == 199){
        p = 100;
    }else if(d>=200){
        p=5;
    }else{
        p=50 - ((45*d)/200);
    }
    return p;
}

void print_map(tile_t *f){ //print the entire map on console
    for(int i = 0; i<Row; i++){
        for(int j = 0; j<Column; j++){
            printf("%c",f->tile[i][j]);
        }
        printf("\n");
    }
}

void loc_output(int curY, int curX){ //print the map to map location
    printf("You are in (%d,%d)\n", curY-bias, curX-bias);
    printf("\n");
    printf("\n");
}

//generate the map using the struct defined in beginning
void gen_map(tile_t *World[399][399], int curY, int curX){
    if(World[curY][curX] == NULL){
        srand(time(NULL));
        tile_t *f;
        f = malloc(sizeof(tile_t));
        gen_boulders_clearings(f);
        gen_grass(f);
        gen_trees(f);
        gen_rocks(f);
        gen_path(World, curY,curX, f);
        float prob = probability(curY,curX);
        gen_buildings(f,prob);
        World[curY][curX] = f;
        print_map(World[curY][curX]);
        loc_output(curY,curX);
    }else{
        print_map(World[curY][curX]);
        loc_output(curY,curX);
    }
}

int main(){
    tile_t *World[399][399];
    for(int i = 0; i<399; i++){ //init
        for(int j = 0; j<399; j++){
            World[i][j] = NULL;
        }
    }
    
    char input;
    gen_map(World,199,199);
    int curX = 199;
    int curY = 199;

    while(1){
        printf("%s","Input your action(n,s,e,w,f,q):\n");
        scanf("%s", &input);

        switch(input){
            case 'n':
                if(curY-1>=0){
                    curY--;
                    gen_map(World,curY,curX);
                }else{
                    printf("Out of boundary!\n");
                }
                continue;
            case 's':
                if(curY+1<399){
                    curY++;
                    gen_map(World,curY,curX);
                }else{
                    printf("Out of boundary!\n");
                }
                continue;
            case 'e':
                if(curX+1<399){
                    curX++;
                    gen_map(World,curY,curX);
                }else{
                    printf("Out of boundary!\n");
                }
                continue;
            case 'w':
                if(curX-1>=0){
                    curX--;
                    gen_map(World,curY,curX);
                }else{
                    printf("Out of boundary!\n");
                }
                continue;
            case 'f':
                int temp1;
                int temp2;
                printf("%s", "Input X: ");
                scanf("%d", &temp2);
                if(temp2>= -199 && temp2<199){
                    curY = temp2+bias;
                }else{
                    printf("Invalid input, try again\n");
                    continue;
                }
                printf("%s", "Input Y: ");
                scanf("%d", &temp1);
                if(temp1>=-199 && temp1<199){
                    curX = temp1+bias;
                }else{
                    printf("Invalid input, try again\n");
                    continue;
                }
                gen_map(World,curY,curX);
                continue;
            case 'q':
                return 0;
            default:
                printf("%s", "Invalid input, try again.\n");
                continue;
        }
    }
    return 0;
}