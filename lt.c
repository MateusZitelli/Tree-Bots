#include <stdio.h>
#include <stdlib.h>
//SDL
#include <SDL.h>
#define BPP 4
#define DEPTH 32

#define INSTRUCTIONS 50
#define X_SIZE 500
#define Y_SIZE 80
#define OPCODES 8

struct block{
    int x;
    int y;
    int i;
    int ptr;
    int pos;
    int age;
    float energy;
    float nutrients;
    int * program;
    int die;
    int verified;
    int sustented;
    int loops;
    struct loop * loops_adr;
    int * memory;
    int moved;
    int lastRep;
    struct block * dad;
    struct block ** sustenting;
};

struct rep{
      struct block * b;
      int mode;  
};

struct mov{
    struct block * b;
    int x;
    int y;
};

struct loop{
        int progPos;
        int memAdr;
};

struct block *** grid;
struct block * blockList;
struct rep * toRep;
struct mov * toMov;
struct block ** tobeKiled;
int quantityKiled;
int quantityMoved;
int lastBlock = 0;
int repBlocks;
float energymean;

SDL_Surface *screen;
SDL_Event event;
int stop = 0;
int updateEach = 10;


void setpixel(int x, int y, int r, int g, int b)
{
    y = Y_SIZE - y;
    if (x < 0 || y < 0 || x >= X_SIZE || y >= Y_SIZE || r < 0 || g < 0 || b < 0)
        return;
    if (r > 255)
        r = 255;
    if (g > 255)
        g = 255;
    if (b > 255)
        b = 255;
    Uint32 *pixmem32;
    Uint32 colour;

    colour = SDL_MapRGB(screen->format, r, g, b);

    pixmem32 = (Uint32 *) screen->pixels + screen->w * y + x;
    *pixmem32 = colour;
}

void setBlock(int x, int y, int * prog, struct block * sust, int mode, float energy, float nutrients, int mut){
    int i;
    if(energy < 0) return;
    if(x < 0 || x > X_SIZE - 1 || y < 0 || y > Y_SIZE -1) return;
    if(grid[x][y] != NULL) return;
    if(blockList[lastBlock].i != -1){
        free(blockList[lastBlock].sustenting);
        free(blockList[lastBlock].program);
        free(blockList[lastBlock].loops_adr);
        free(blockList[lastBlock].memory);
    }
    blockList[lastBlock].i = lastBlock;
    blockList[lastBlock].sustenting = (struct block ** ) malloc(8 * sizeof(struct block *));
    blockList[lastBlock].program = (int *) malloc(INSTRUCTIONS * sizeof(int));
    blockList[lastBlock].loops_adr = (struct loop *) malloc(20 * sizeof(struct loop));
    blockList[lastBlock].memory = (int *) malloc(INSTRUCTIONS * sizeof(int));
    for(i = 0; i < 8 ; i++){
        blockList[lastBlock].sustenting[i] = NULL;
    }
    for(i = 0; i < INSTRUCTIONS; i++){
        blockList[lastBlock].program[i] = prog[i];
    }
    for(i = 0; i < INSTRUCTIONS; i++){
        blockList[lastBlock].memory[i] = 0;
    }
    while(1){
        if(rand() % 100 < 8)
            blockList[lastBlock].program[rand() % INSTRUCTIONS] = rand() % 8;
        else
            break;
    }
    blockList[lastBlock].dad = sust;
    if(sust != NULL){
        sust->sustenting[mode] = &blockList[lastBlock];
    }
    grid[x][y] = &blockList[lastBlock];
    blockList[lastBlock].x = x;
    blockList[lastBlock].y = y;
    blockList[lastBlock].pos = 0;
    blockList[lastBlock].ptr = 0;
    blockList[lastBlock].energy = energy;
    blockList[lastBlock].nutrients = nutrients;
    blockList[lastBlock].age = 0;
    blockList[lastBlock].loops = 0;
    blockList[lastBlock].verified = 0;
    blockList[lastBlock].moved = 0;
    blockList[lastBlock].lastRep = 20;
    blockList[lastBlock++].die = 0;
}

void setBlockByMode(int * prog, struct block * sust, int mode, float energy, float nutrients, int mut){
    int x = 0,y = 0;
    switch(mode){
        case 0:
            x = sust->x + 1;
            y = sust->y;
            break;
        case 1:
            x = sust->x + 1;
            y = sust->y + 1;
            break;
        case 2:
            x = sust->x;
            y = sust->y + 1;
            break;
        case 3:
            x = sust->x - 1;
            y = sust->y + 1;
            break;
        case 4:
            x = sust->x - 1;
            y = sust->y;
            break;
        case 5:
            x = sust->x - 1;
            y = sust->y - 1;
            break;
        case 6:
            x = sust->x;
            y = sust->y - 1;
            break;
        case 7:
            x = sust->x + 1;
            y = sust->y - 1;
            break;
        
    }
    setBlock(x, y, prog, sust, mode, energy, nutrients, mut);
}
int sustented(struct block * b){
     int i;
     if(b->verified){
        return(b->sustented);
     }
     b->verified = 1;
     b->sustented = 0;
     if(b->y == 0){
        b->sustented = 1;
    return(1);
     }else{
    if(grid[b->x][b->y - 1] != NULL){
        b->sustented = b->sustented || sustented(grid[b->x][b->y - 1]);
        return(b->sustented);
    }
    if(b->dad != NULL)
        b->sustented = b->sustented || sustented(b->dad);
        
    }
    return(b->sustented);
}

int moveBlockRelative(struct block * b, int vx, int vy, struct block * last){
    if(b->x + vx >= X_SIZE || b->x + vx < 0 || b->y + vy >= Y_SIZE || b->y + vy < 0) return(0);
    if(grid[b->x + vx][b->y + vy] != NULL) return(0);
    grid[b->x + vx][b->y + vy] = grid[b->x][b->y];
    grid[b->x][b->y] = NULL;
    b->x = b->x + vx;
    b->y = b->y + vy;
    return(1);
}


void move(struct block * b, int x, int y){
    if(x == 0 && y == 0) return;
    toMov[quantityMoved].b = b;
    toMov[quantityMoved].x = x;
    toMov[quantityMoved++].y = y;
}

void fall(){
    int i;
    for(i = 0; i < lastBlock; i++){
        if(!sustented(&blockList[i]))
            move(&blockList[i], 0, -1);
    }
}

void removeBlock(struct block * b){
    int i, j, pos;
    for(i = 0; i < 8; i++){
        if(b->sustenting[i] != NULL)
            b->sustenting[i]->dad = NULL;
        if(b->dad != NULL && b->dad->sustenting[i] == b){
            b->dad->sustenting[i] = NULL;
        }
    }
    grid[b->x][b->y] = NULL;
    pos = b->i;
    blockList[pos] = blockList[--lastBlock];
    blockList[pos].i = pos;
    grid[blockList[pos].x][blockList[pos].y] = &blockList[pos];
    for(i = 0; i < 8; i++){
        if(blockList[pos].sustenting[i] != NULL && blockList[pos].sustenting[i]->dad == &blockList[lastBlock]){
            blockList[pos].sustenting[i]->dad = &blockList[pos];
        }
        if(blockList[pos].dad != NULL && blockList[pos].dad->sustenting[i] == &blockList[lastBlock]){
            blockList[pos].dad->sustenting[i] = &blockList[pos];
        }
    }
}

void kill(struct block * b){
    tobeKiled[quantityKiled++] = b;
}

void die(struct block * b){
    b->die = 1;
}

void atack(struct block * b){
    if(b->x + 1 > X_SIZE - 1 || b->y + 1 > Y_SIZE - 1 || b->y - 1 < 0 || b->x - 1 < 0) return;
    float energy = 0;
    switch(b->program[b->pos]){ 
        case 0:
            if(grid[b->x + 1][b->y] == NULL) return;
            if(grid[b->x + 1][b->y]->die){
                kill(grid[b->x + 1][b->y]);
                b->energy += 1;
                b->nutrients += 1;
            }
            
            break;
        case 1:
            if(grid[b->x + 1][b->y + 1] == NULL) return;
            if(grid[b->x + 1][b->y + 1]->die){
                kill(grid[b->x + 1][b->y + 1]);
                b->energy += 1;
                b->nutrients += 1;
            }
            break;
        case 2:
            if(grid[b->x][b->y + 1] == NULL) return;
            if(grid[b->x][b->y + 1]->die){
                kill(grid[b->x][b->y + 1]);
                b->energy += 1;
                b->nutrients += 1;
            }
            break;
        case 3:
            if(grid[b->x - 1][b->y + 1] == NULL) return;
            if(grid[b->x - 1][b->y + 1]->die){
                kill(grid[b->x - 1][b->y + 1]);
                b->energy += 1;
                b->nutrients += 1;
            }
            break;
        case 4:
            if(grid[b->x - 1][b->y] == NULL) return;
            if(grid[b->x - 1][b->y]->die){
                kill(grid[b->x - 1][b->y]);
                b->energy += 1;
                b->nutrients += 1;
            }
            break;
        case 5:
            if(grid[b->x - 1][b->y - 1] == NULL) return;
            if(grid[b->x - 1][b->y - 1]->die){
                kill(grid[b->x - 1][b->y - 1]);
                b->energy += 1;
                b->nutrients += 1;
            }
            break;
        case 6:
            if(grid[b->x][b->y - 1] == NULL) return;
            if(grid[b->x][b->y - 1]->die){
                kill(grid[b->x][b->y - 1]);
                b->energy += 1;
                b->nutrients += 1;
            }
            break;
        case 7:
            if(grid[b->x + 1][b->y - 1] == NULL) return;
            if(grid[b->x + 1][b->y - 1]->die){
                kill(grid[b->x + 1][b->y - 1]);
                b->energy += 1;
                b->nutrients += 1;
            }
            break;
        
    }
}

void getEnegy(struct block * b){
    if(b->pos + 1 < INSTRUCTIONS - 1 && b->sustenting != NULL && b->sustenting[b->program[b->pos]] != NULL){
        b->sustenting[b->program[b->pos]]->energy /= 3.0;
        b->energy += b->sustenting[b->program[b->pos]]->energy * 2;
        b->pos += 1;
    }
}

void getNutrients(struct block * b){
    if(b->pos + 1 < INSTRUCTIONS - 1 && b->sustenting != NULL && b->sustenting[b->program[b->pos]] != NULL){
        b->nutrients = b->nutrients / 3.0;
        b->sustenting[b->program[b->pos]]->nutrients += b->nutrients * 2;
        b->pos += 1;
    }
}


void reproduce(struct block *b){
    b->energy /= 2.0;
    b->nutrients /= 2.0;
    b->lastRep = 20;
    toRep[repBlocks].b = b;
    toRep[repBlocks++].mode = b->program[++b->pos];
}

void resolveRep(){
    int i;
    for(i = 0; i < repBlocks; i++){
        setBlockByMode(toRep[i].b->program, toRep[i].b, toRep[i].mode, toRep[i].b->energy - 0.5, toRep[i].b->nutrients - 0.5, 1);
    }
}

void resolveKileds(){
    int i;
    for(i = 0; i < quantityKiled; i++){
        removeBlock(tobeKiled[i]);
    }
}

void resolveMoves(){
    int i;
    for(i = 0; i < quantityMoved; i++){
        if(toMov[i].b == NULL) continue;
        moveBlockRelative(toMov[i].b, toMov[i].x, toMov[i].y, NULL);
    }
}

void run(struct block * b){
    if(!b->die && (b->energy <= 0 || b->nutrients <= 0 || b->pos > INSTRUCTIONS - 1)){
        die(b);
        return;
    }
    energymean += b->energy;
    if(!b->die){
        switch(b->program[b->pos]){
            case 0:
                if(b->lastRep > 0) break;
                b->pos = (b->pos + 1) % INSTRUCTIONS;
                reproduce(b);
                break;
            case 1:
                getEnegy(b);
                break;
            case 2:
                getNutrients(b);
                break;
            case 3:
                break;
                b->pos = (b->pos + 1) % INSTRUCTIONS;
                switch(b->program[b->pos]){
                    case 0:
                          move(b, 1, 0);
                          b->energy -= 5;
                          break; 
                    case 1:
                          move(b, 1, 0);
                          b->energy -= 5;
                          break;
                    case 2:
                          b->energy -= 5;
                          break; 
                    case 3:
                          move(b, -1, 0);
                          b->energy -= 5;
                          break;
                    case 4:
                          move(b, -1, 0);
                          b->energy -= 5;
                          break; 
                    case 5:
                          move(b, -1, -1);
                          b->energy -= 2;
                          break;
                    case 6:
                          move(b, 0, -1);
                          b->energy += 1;
                          break; 
                    case 7:
                          move(b, 1, -1);
                          b->energy -= 2;
                          break;
                }
                if(b->y > Y_SIZE - 1) kill(b);
                break;
            case 4:
                atack(b);
                break;
            case 5: //Logic Operations
                b->pos = (b->pos + 1) % INSTRUCTIONS;
                switch(b->program[b->pos]){
                        case 0:
                                ++b->ptr;
                                if(b->ptr >= INSTRUCTIONS) die(b);
                                break;
                        case 1:
                                --b->ptr;
                                if(b->ptr < 0) die(b);
                                break;
                        case 2:
                                ++b->memory[b->ptr];
                                break;
                        case 3:
                                --b->memory[b->ptr];
                                if(b->memory[b->ptr] < 0) die(b);
                                break;
                        case 4:
                                if(b->loops >= 19) break;
                                b->loops_adr[b->loops].progPos = b->pos + 1;
                                b->loops_adr[b->loops++].memAdr = b->ptr;
                                break;
                        case 5:
                                if(b->loops > 0 && b->memory[b->loops_adr[b->loops - 1].memAdr]){
                                        b->pos = b->loops_adr[b->loops - 1].progPos;
                                }else if(b->loops > 0){
                                        --b->loops;
                                }else{
                                        die(b);
                                }
                                break;
                }
                break;
            case 6:
                die(b);
                break;
                
        }
    }else{
        switch(b->program[b->pos]){
            case 1:
                getEnegy(b);
                break;
            case 2:
                getNutrients(b);
                break;
        }
    }
    if(b->energy < 10){
        kill(b);
    }
    b->pos = (b->pos + 1) % INSTRUCTIONS;
    ++b->age;
    --b->lastRep;
    b->energy -= b->age / 800.0;
    b->nutrients -= b->age / 800.0;
}

void resoursesDistribuition(){
    int i, j;
    float t;
    for(i = 0; i < X_SIZE; i++){
        t = 1;
        for(j = Y_SIZE - 1; j >= 0; j--){
            if(grid[i][j] != NULL){
                //setpixel(i, j + 10, 255,0,0);
                if(!grid[i][j]->die)grid[i][j]->energy += 50/ t;
                t += 0.1;
            }else{
                setpixel(i, j, 230 / t,230 / t,175 / t);
            }
        }
    }
    for(i = 0; i < X_SIZE; i++){
        if(grid[i][0] != NULL)
                if(!grid[i][0]->die)grid[i][0]->nutrients += 5000.0;
    }
}

void step(){
    energymean = 0;
    int i;
    int prog[INSTRUCTIONS];
    repBlocks = 0;
    quantityMoved = 0;
    quantityKiled = 0;
    for(i = 0; i < INSTRUCTIONS; i++){
        prog[i] = rand() % 8;
    }
    setBlock(rand() % X_SIZE,0, prog, NULL, 0, 50, 50, 0);
    for(i = 0; i < X_SIZE; i++){
        memset(grid[i], NULL, sizeof(grid[i][0]) * Y_SIZE);
    }
    for(i = 0; i < lastBlock; i++){
        blockList[i].verified = 0;
        blockList[i].moved = 0;
        grid[blockList[i].x][blockList[i].y] = &blockList[i];
    }
    resoursesDistribuition();

    for(i = 0; i < lastBlock; i++){
        run(&blockList[i]);
    }
    fall();
    resolveKileds();
    resolveRep();
    resolveMoves();
}

void init(){
    int prog[INSTRUCTIONS];
    if (SDL_Init(SDL_INIT_VIDEO) < 0);
    if (!(screen = SDL_SetVideoMode(X_SIZE, Y_SIZE, DEPTH, SDL_HWSURFACE))) {
        exit(1);
        SDL_Quit();
    }
    srand(time(0));
    int i, j;
    blockList = (struct block *) malloc(X_SIZE * Y_SIZE * sizeof(struct block));
    grid = (struct block ***) malloc(X_SIZE * sizeof(struct block **));
    tobeKiled = (struct block **) malloc(X_SIZE * Y_SIZE * sizeof(struct block*));
    toRep = (struct rep *) malloc(X_SIZE * Y_SIZE * sizeof(struct rep));
    toMov = (struct mov *) malloc(X_SIZE * Y_SIZE * sizeof(struct mov));
    for(i = 0; i < X_SIZE; i++){
        grid[i] = (struct block **) malloc(Y_SIZE * sizeof(struct block *));
        for(j = 0; j < Y_SIZE; j++){
            grid[i][j] = NULL;
        }
    }
    for(i = 0; i < X_SIZE * Y_SIZE; i++){
        blockList[i].x = -1;
        blockList[i].y = -1;
        blockList[i].ptr = -1;
        blockList[i].age = -1;
        blockList[i].die = -1;
        blockList[i].i = -1;
        blockList[i].dad = NULL;
        blockList[i].sustenting = NULL;
        blockList[i].program = NULL;
        blockList[i].memory = NULL;
        blockList[i].loops_adr = NULL;
    }
}

void render(){
    int i, j;
    int color[3];
    int pos[2];

    for(i = 0; i < lastBlock; i++){
        color[0] = blockList[i].program[0] / 8.0 * 255;
        color[1] = blockList[i].program[INSTRUCTIONS / 2] / 8.0 * 255;
        color[2] = blockList[i].program[INSTRUCTIONS - 1] / 8.0 * 255;
        if(blockList[i].die){
            if(blockList[i].energy < 1){
                color[0] = color[0] - 70 < 0?0:color[0] - 70;
                color[1] = color[1] - 70 < 0?0:color[1] - 70;
                color[2] = color[2] - 70 < 0?0:color[2] - 70;
            }else{
                color[0] = color[0] - 90 < 0?0:color[0] - 70;
                color[1] = color[1] - 90 < 0?0:color[1] - 70;
                color[2] = color[2] - 90 < 0?0:color[2] - 70;
            }
        }
        pos[0] = blockList[i].x;
        pos[1] = blockList[i].y; 
        setpixel(pos[0], pos[1], color[0], color[1], color[2]);
    }
    SDL_Flip(screen);
    SDL_FillRect(screen, NULL, 0x000000);
}

void events(){
    int i, posx , posy;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            stop = 1;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_UP:
                ++updateEach;
                printf("Update Each %i frames\n", updateEach);
                break;
            case SDLK_DOWN:
                --updateEach;
                printf("Update Each %i frames\n", updateEach);
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == 1 && grid[event.button.x][Y_SIZE - event.button.y] != NULL){
                posx = event.button.x;
                posy = Y_SIZE - event.button.y;
                printf("Cell at %i, %i:\n", posx, posy);
                printf("\tProgram = {");
                for(i = 0; i < INSTRUCTIONS - 1; i++){
                    printf("%i, ", grid[posx][posy]->program[i]);
                }
                printf("%i}\n", grid[posx][posy]->program[INSTRUCTIONS - 1]);
                printf("\tMemory = {");
                for(i = 0; i < INSTRUCTIONS - 1; i++){
                    printf("%i, ", grid[posx][posy]->memory[i]);
                }
                printf("%i}\n", grid[posx][posy]->memory[INSTRUCTIONS - 1]);
            }else{
                printf("Cell %i, %i in NULL\n", event.button.x, event.button.y);
            }
            break;
        }
    }
}

void freeMemory(){
    int i;
    for(i = 0; i < X_SIZE; i++){
        free(grid[i]);
    }
    free(grid);
    for(i = 0; i < lastBlock; i++){
        free(blockList[i].program);
        free(blockList[i].sustented);
        free(blockList[i].memory);
        free(blockList[i].loops_adr);
    }
    free(blockList);
    free(toRep);
    free(toMov);
    free(tobeKiled);
}

int main(){
    init();
    int i = 0;
    while(!stop){
        events();
        step();
        if(i % updateEach == 0){
            render();

        }
        ++i;
    }
    freeMemory();
    return(0);
}
