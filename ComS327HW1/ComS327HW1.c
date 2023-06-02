//CREATE CHANGE LOG BEFORE SUBMITTING

#include<stdio.h>
#include<time.h>
#include<string.h>
#include<stdlib.h>

int Column = 80; //Column = width
int Row = 21; //Row = height
char Map[24][80];

void printMap(){
    char pos;
    for (int i = 0; i < Row; i++) {
        for (int j = 0; j < Column; j++) {
            printf("%c", Map[i][j]);
            
            if((j+1) % Column == 0){
                printf("\n");
            }
        }
    }    
}

void gen_map(){
    //create map 24x80 map grid with boulders as the borders
    for(int i = 0; i < Row; i++){
        for(int j = 0; j < Column; j++){
            if(i == 0 || i == Row-1){
                Map[i][j] = '%';
            }else if(j == 0 || j == Column-1){
                Map[i][j] = '%';
            }else{
                Map[i][j] = ' '; //create empty spaces for everything but the border
            }
        }
    }
}

void gen_roads(){
    int north_exit = rand()%40+40; //generate random exit on north side of grid
    if(north_exit <= 5){
        north_exit+=5; //finding the exits
    }else if(north_exit >75){
        north_exit -= 5;
    }
    int west_exit = rand()%10+10; // generate random exit on west side of grid
    if(west_exit <= 5){
        west_exit += 5;
    }else if(west_exit >17){
        west_exit-= 5;
    }
    int NE = north_exit; //rename variable with new exit location value
    int WE = west_exit;

    Map[0][NE] = '#'; //give the exit space a road symbol
    Map[WE][0] = '#';

    for(int i = 0; i<NE; i++){
        Map[WE][i] = '#';
    }
    for(int i = 0; i<=WE; i++){
        Map[i][NE] = '#';
    }

    int south_exit = rand()%40; //generate random exit on south side of grid
    if(south_exit <=5){
        south_exit+=5;
    }else if(south_exit >75){
        south_exit-=5;
    }
    int SE = south_exit; //generate random exit on east side of grid
    int east_exit = rand()%10;
    if(east_exit < 5){
        east_exit +=5;
    }else if(east_exit > 17){
        east_exit-= 5;
    }
    int EE = east_exit;

    Map[20][SE] = '#';
    Map[EE][79] = '#';

    for(int i = SE; i<80; i++){
        Map[EE][i] = '#';
    }
    for(int i = EE; i<21; i++){
        Map[i][SE] = '#';
    }
        
    //generate the pokecenters reachable from the paths
    Map[EE-1][SE] = 'C';
    Map[EE-2][SE] = 'C';
    Map[EE-1][SE+1] = 'C';
    Map[EE-2][SE+1] = 'C';
    //generate the pokemarts reachable from the paths
    Map[WE+1][NE] = 'M';
    Map[WE+2][NE] = 'M';
    Map[WE+1][NE+1] = 'M';
    Map[WE+2][NE+1] = 'M';
}

void addGrass(){
    int X = rand()%75+2;
    int Y = rand()%17+2;
    int Xgrass = rand()%30 + 3;
    int Ygrass = rand()%10 + 3;
    for(int i=0; i< Ygrass; i++){
        for(int j=0; j<Xgrass; j++){
            if(Map[Y + i][X + j] == ' '){
                Map[Y + i][X + j] = ':';
            }else{
                continue;
            }        
        }
    }    
}
void gen_grass(){
    int num = rand()%10 + 4; //generate random number for grass count
    for(int i = 0 ; i < num; i++){
        addGrass();
    } 
}

void gen_clearing(){
    for(int i = 0; i< 21 ; i++){
        for(int j = 0; j<80; j++){
            if(Map[i][j] == ' '){
                Map[i][j] = '.';
            }else{
                continue;
            }
        }
    }
}

void addTree(){
    int X = rand()%78+1;
    int Y = rand()%19+1;
    if(Map[Y][X] != '#' && Map[Y][X] != 'M' && Map[Y][X] != 'C'){
        Map[Y][X] = '^';
    }else{
        addTree();
    }
}

void gen_tree(){
    int num = rand()%50 + 20; //generate random num to generate trees
    for(int i = 0 ; i< num ;i++){
        addTree();
    }
}

void addRock(){
    int X = rand()%78+1;
    int Y = rand()%19+1;
    if(Map[Y][X] != '#' && Map[Y][X] != 'M' && Map[Y][X] != 'C' && Map[Y][X] != '^'){
        Map[Y][X] = '%'; //boulder and rock share the same symbol %
    }else{
        addRock();
    }
}
void gen_rock(){
    int num = rand()%60 + 10; //generate random num to generate rocks
    for(int i = 0; i< num;i++){
        addRock();
    }
}

int main(){
    srand(time(NULL));
    gen_map();
    gen_roads();
    gen_grass();
    gen_tree();
    gen_rock();
    gen_clearing(); //generate clearings after all the 'obstacles' have been generate
    printMap();
    return 0;
}