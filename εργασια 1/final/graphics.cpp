#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <time.h>
#include <png.h>
#include <math.h>

#define C_WHITE 0
#define C_SCISSORS 1
#define C_ROCK 2
#define C_PAPER 3
#define C_RED 4
#define C_BLUE 5
#define C_NONE 6

#define N 15
#define CENTER 7
#define MAX_MOVES 30

char FILE_ROCK[11] = "./rock.png";
char FILE_PAPER[12] = "./paper.png";
char FILE_SCISSORS[15] = "./scissors.png";

int movesMade = 0;
int points = 0;

struct Cube {
    int x;
    int y;
    int type;
    float fakeColor;
};

Cube allCubes[N][N];

float camerax, cameray, camerazoom;

GLubyte *textureScissors;
GLubyte *texturePaper;
GLubyte *textureRock;

// Function Prorotypes
bool loadPngImage(char *name, int &outWidth, int &outHeight, bool &outHasAlpha, GLubyte **outData);
void loadTexture(int type);
void initializeTextures();
void initializeCubes();
void startGame();
void drawCube(Cube *cb, bool texture);
void drawCubeFakeColor(Cube *cb);
void drawCubes();
void drawCubesFake();
static void renderFakeScene(void);
static void renderScene(void);
bool areNeighbours(Cube cube1, Cube cube2);
bool eats(Cube cube1, Cube cube2);
void destroyEatingNeighbours(Cube cube);
void doArea1(bool isHorizontal, Cube middle);
void doArea2and3(bool isHorizontal, Cube middle);
void destroyTriad(Cube *cube1, Cube *cube2, Cube *cube3);
void checkTriad(Cube cube);
void swapCubes(Cube *cube1, Cube *cube2);
void checkAndSwapCubes(Cube *cube1, Cube *cube2);
void keyboardFunc(int key, int x, int y);
void menuOptions(int i);
void initializeMenu(void);
void mouseFunc(int btn, int state, int x, int y);
void initializeScene();

bool loadPngImage(char *name, int &outWidth, int &outHeight, bool &outHasAlpha, GLubyte **outData) {
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    int color_type, interlace_type;
    FILE *fp;
 
    if ((fp = fopen(name, "rb")) == NULL)
        return false;
 
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                     NULL, NULL, NULL);
 
    if (png_ptr == NULL) {
        fclose(fp);
        return false;
    }
 
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return false;
    }
 
    if (setjmp(png_jmpbuf(png_ptr))) {
        /* Free all of the memory associated
         * with the png_ptr and info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        /* If we get here, we had a
         * problem reading the file */
        return false;
    }
 
    /* Set up the output control if
     * you are using standard C streams */
    png_init_io(png_ptr, fp);
 
    /* If we have already
     * read some of the signature */
    png_set_sig_bytes(png_ptr, sig_read);
 
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);
 
    png_uint_32 width, height;
    int bit_depth;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 &interlace_type, NULL, NULL);
    outWidth = width;
    outHeight = height;
 
    unsigned int row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    *outData = (unsigned char*) malloc(row_bytes * outHeight);
 
    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
 
    for (int i = 0; i < outHeight; i++) {
        memcpy(*outData+(row_bytes * (outHeight-1-i)), row_pointers[i], row_bytes);
    }
 
    /* Clean up after the read,
     * and free any memory allocated */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
 
    /* Close the file */
    fclose(fp);
 
    return true;
}

void loadTexture(int type) {
    int width=45, height=45;
    bool hasAlpha = true;
    if (type == C_SCISSORS) {
        glTexImage2D(GL_TEXTURE_2D, 0, hasAlpha ? 4 : 3, width,
                 height, 0, hasAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE,
                 textureScissors);
    }
    else if (type == C_PAPER) {
        glTexImage2D(GL_TEXTURE_2D, 0, hasAlpha ? 4 : 3, width,
                 height, 0, hasAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE,
                 texturePaper);
    }
    else if (type == C_ROCK) {
        glTexImage2D(GL_TEXTURE_2D, 0, hasAlpha ? 4 : 3, width,
                 height, 0, hasAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE,
                 textureRock);
    }
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    
}

void initializeTextures() {
    int width=45, height=45;
    bool hasAlpha = true;
    
    bool success = loadPngImage(FILE_PAPER, width, height, hasAlpha, &textureScissors);
    if (!success) {
       printf("Unable to load paper image.\n");
        return;
    }

    success = loadPngImage(FILE_ROCK, width, height, hasAlpha, &texturePaper);
    if (!success) {
       printf("Unable to load rock image.\n");
        return;
    }

    success = loadPngImage(FILE_SCISSORS, width, height, hasAlpha, &textureRock);
    if (!success) {
       printf("Unable to load scissors image.\n");
        return;
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void initializeCubes() {
    int i, j;
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            allCubes[i][j].x = i;
            allCubes[i][j].y = j;
            allCubes[i][j].type = C_WHITE;
            allCubes[i][j].fakeColor = (i*N+j) / 254.0; // total 225 unique colors, one for each cube.
        }
    }
}

void startGame() {
    int i,j;
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            allCubes[i][j].type = rand() % 5 + 1;
        }
    }
    movesMade = 0;
    points = 0;
    printf("Game Started!\n");
    printf("SCORE: %d\n", points);
}

void drawCube(Cube *cb, bool texture) {
    int x = (*cb).x;
    int y = (*cb).y;
    int z = 0;

    if (texture) {
        //BACK
        glBegin(GL_POLYGON);
        glTexCoord2f(0.0, 0.0); 
        glVertex3f(  x+0.4f,  y-0.4f, z+0.4f );
        glTexCoord2f(0.0, 1.0); 
        glVertex3f(  x+0.4f,  y+0.4f, z+0.4f );
        glTexCoord2f(1.0, 1.0); 
        glVertex3f(  x-0.4f,  y+0.4f, z+0.4f );
        glTexCoord2f(1.0, 0.0); 
        glVertex3f(  x-0.4f,  y-0.4f, z+0.4f );
        glEnd();

        //FRONT
        glBegin(GL_POLYGON);
        glTexCoord2f(0.0, 0.0); 
        glVertex3f(  x+0.4f,  y-0.4f, z-0.4f );
        glTexCoord2f(0.0, 1.0); 
        glVertex3f(  x+0.4f,  y+0.4f, z-0.4f );
        glTexCoord2f(1.0, 1.0); 
        glVertex3f(  x-0.4f,  y+0.4f, z-0.4f );
        glTexCoord2f(1.0, 0.0); 
        glVertex3f(  x-0.4f,  y-0.4f, z-0.4f );
        glEnd();
    }
    else {
        //BACK
        glBegin(GL_POLYGON); 
        glVertex3f(  x+0.4f,  y-0.4f, z+0.4f ); 
        glVertex3f(  x+0.4f,  y+0.4f, z+0.4f ); 
        glVertex3f(  x-0.4f,  y+0.4f, z+0.4f ); 
        glVertex3f(  x-0.4f,  y-0.4f, z+0.4f );
        glEnd();

        //FRONT
        glBegin(GL_POLYGON); 
        glVertex3f(  x+0.4f,  y-0.4f, z-0.4f ); 
        glVertex3f(  x+0.4f,  y+0.4f, z-0.4f ); 
        glVertex3f(  x-0.4f,  y+0.4f, z-0.4f ); 
        glVertex3f(  x-0.4f,  y-0.4f, z-0.4f );
        glEnd();
    }
    
    //RIGHT
    glBegin(GL_POLYGON);
    glVertex3f( x+0.4f,  y-0.4f,  z-0.4f );
    glVertex3f( x+0.4f,  y+0.4f,  z-0.4f );
    glVertex3f( x+0.4f,  y+0.4f,  z+0.4f );
    glVertex3f( x+0.4f,  y-0.4f,  z+0.4f );
    glEnd();
    
    //LEFT
    glBegin(GL_POLYGON);
    glVertex3f(  x-0.4f,  y-0.4f,  z+0.4f );
    glVertex3f(  x-0.4f,  y+0.4f,  z+0.4f );
    glVertex3f(  x-0.4f,  y+0.4f,  z-0.4f );
    glVertex3f(  x-0.4f,  y-0.4f,  z-0.4f );
    glEnd();
    
    //TOP
    glBegin(GL_POLYGON);
    glVertex3f(  x+0.4f,  y+0.4f,  z+0.4f );
    glVertex3f(  x+0.4f,  y+0.4f,  z-0.4f );
    glVertex3f(  x-0.4f,  y+0.4f,  z-0.4f );
    glVertex3f(  x-0.4f,  y+0.4f,  z+0.4f );
    glEnd();

    //BOTTOM
    glBegin(GL_POLYGON);
    glVertex3f(  x+0.4f,  y-0.4f,  z-0.4f );
    glVertex3f(  x+0.4f,  y-0.4f,  z+0.4f );
    glVertex3f(  x-0.4f,  y-0.4f,  z+0.4f );
    glVertex3f(  x-0.4f,  y-0.4f,  z-0.4f );
    glEnd();
}

void drawCubeFakeColor(Cube *cb) {
    int x = (*cb).x;
    int y = (*cb).y;
    int z = 0;

    glColor3f((*cb).fakeColor, (*cb).fakeColor, (*cb).fakeColor);

    //BACK
    glBegin(GL_POLYGON); 
    glVertex3f(  x+0.4f,  y-0.4f, z+0.4f ); 
    glVertex3f(  x+0.4f,  y+0.4f, z+0.4f ); 
    glVertex3f(  x-0.4f,  y+0.4f, z+0.4f ); 
    glVertex3f(  x-0.4f,  y-0.4f, z+0.4f );
    glEnd();

    //FRONT
    glBegin(GL_POLYGON); 
    glVertex3f(  x+0.4f,  y-0.4f, z-0.4f ); 
    glVertex3f(  x+0.4f,  y+0.4f, z-0.4f ); 
    glVertex3f(  x-0.4f,  y+0.4f, z-0.4f ); 
    glVertex3f(  x-0.4f,  y-0.4f, z-0.4f );
    glEnd();
    
    //RIGHT
    glBegin(GL_POLYGON);
    glVertex3f( x+0.4f,  y-0.4f,  z-0.4f );
    glVertex3f( x+0.4f,  y+0.4f,  z-0.4f );
    glVertex3f( x+0.4f,  y+0.4f,  z+0.4f );
    glVertex3f( x+0.4f,  y-0.4f,  z+0.4f );
    glEnd();
    
    //LEFT
    glBegin(GL_POLYGON);
    glVertex3f(  x-0.4f,  y-0.4f,  z+0.4f );
    glVertex3f(  x-0.4f,  y+0.4f,  z+0.4f );
    glVertex3f(  x-0.4f,  y+0.4f,  z-0.4f );
    glVertex3f(  x-0.4f,  y-0.4f,  z-0.4f );
    glEnd();
    
    //TOP
    glBegin(GL_POLYGON);
    glVertex3f(  x+0.4f,  y+0.4f,  z+0.4f );
    glVertex3f(  x+0.4f,  y+0.4f,  z-0.4f );
    glVertex3f(  x-0.4f,  y+0.4f,  z-0.4f );
    glVertex3f(  x-0.4f,  y+0.4f,  z+0.4f );
    glEnd();

    //BOTTOM
    glBegin(GL_POLYGON);
    glVertex3f(  x+0.4f,  y-0.4f,  z-0.4f );
    glVertex3f(  x+0.4f,  y-0.4f,  z+0.4f );
    glVertex3f(  x-0.4f,  y-0.4f,  z+0.4f );
    glVertex3f(  x-0.4f,  y-0.4f,  z-0.4f );
    glEnd();

}

void drawCubes() {
    int i, j, k;
    bool isTexture = false;
    
    for (k=0; k<6; k++) { //for each type (did it like this to avoid multiple texture loading on each frame)
        if (k == C_WHITE)           {glColor3f(  1.0,  1.0,  1.0 ); isTexture = false;}
        else if (k == C_RED)        {glColor3f(  1.0,  0.0,  0.0 ); isTexture = false;}
        else if (k == C_BLUE)       {glColor3f(  0.0,  0.0,  1.0 ); isTexture = false;}
        else if (k == C_ROCK)       {loadTexture(C_ROCK);           isTexture =  true;}
        else if (k == C_PAPER)      {loadTexture(C_PAPER);          isTexture =  true;}
        else if (k == C_SCISSORS)   {loadTexture(C_SCISSORS);       isTexture =  true;}
        for (i=0; i<N; i++) {
            for (j=0; j<N; j++) {
                if (allCubes[i][j].type == k) {
                    drawCube(&allCubes[i][j], isTexture);
                }
            }
        }
    }
}

void drawCubesFake() {
    int i, j;
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            drawCubeFakeColor(&allCubes[i][j]);
        }
    }
}

static void renderFakeScene(void) {
    glClearColor(1.0f,1.0f,1.0f,1); // background -> white
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    gluLookAt(
        camerax, cameray, camerazoom,
        CENTER, CENTER, 0.0f,
        0.0f, 1.0f, 0.0f
            );
    
    glDisable(GL_LIGHTING);
    
    drawCubesFake();
    
    glEnable(GL_LIGHTING);
    
    glClearColor(0.0f,0.0f,0.0f,1); // background back to black
}

static void renderScene(void) {
    GLfloat light_position[] = { 7.0f, 7.0f, 5.0f, 1.0};
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    gluLookAt(
        camerax, cameray, camerazoom,
        CENTER, CENTER, 0.0f,
        0.0f, 1.0f, 0.0f
            );
    
    glPushMatrix();
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    
    glPopMatrix();

    drawCubes();
    

    //glDisable(GL_LIGHTING);
    
    //drawCubesFake();
    
    //glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

bool areNeighbours(Cube cube1, Cube cube2) {
    return (cube1.x+1 == cube2.x && cube1.y == cube2.y || 
            cube1.x-1 == cube2.x && cube1.y == cube2.y || 
            cube1.y+1 == cube2.y && cube1.x == cube2.x || 
            cube1.y-1 == cube2.y && cube1.x == cube2.x);
}

bool eats(Cube cube1, Cube cube2) {
    return (cube1.type == C_PAPER && cube2.type == C_ROCK || 
            cube1.type == C_ROCK && cube2.type == C_SCISSORS || 
            cube1.type == C_SCISSORS && cube2.type == C_PAPER);
}

void destroyEatingNeighbours(Cube cube) {
    if (cube.x+1 < N) {
        if (eats(cube, allCubes[cube.x+1][cube.y])) {
            allCubes[cube.x+1][cube.y].type = C_NONE;
            points+=2;
            printf("SCORE: %d\n", points);
        }
    }
    if (cube.x-1 >= 0) {
        if (eats(cube, allCubes[cube.x-1][cube.y])) {
            allCubes[cube.x-1][cube.y].type = C_NONE;
            points+=2;
            printf("SCORE: %d\n", points);
        }
    }
    if (cube.y+1 < N) {
        if (eats(cube, allCubes[cube.x][cube.y+1])) {
            allCubes[cube.x][cube.y+1].type = C_NONE;
            points+=2;
            printf("SCORE: %d\n", points);
        }
    }
    if (cube.y-1 >= 0) {
        if (eats(cube, allCubes[cube.x][cube.y-1])) {
            allCubes[cube.x][cube.y-1].type = C_NONE;
            points+=2;
            printf("SCORE: %d\n", points);
        }
    }
}

void doArea1(bool isHorizontal, Cube middle) {
    int i;
    int array1[12] = {-2, -2, -2, -1, -1,  0,  0,  1,  1,  2,  2,  2};
    int array2[12] = {-1,  0,  1, -1,  1, -1,  1, -1,  1, -1,  0,  1};

    if (isHorizontal) {
        for (i=0; i<12; i++) {
            if (middle.x + array1[i]<0 || middle.x + array1[i]>N-1 || middle.y + array2[i]<0 || middle.y + array2[i]>N-1) {
                continue;
            }
            if (!eats(middle, allCubes[middle.x + array1[i]][middle.y + array2[i]])) {
                points-=1;
                continue;
            }
            allCubes[middle.x + array1[i]][middle.y + array2[i]].type = C_NONE;
            points+=2;
        }
    } else {
        for (i=0; i<12; i++) {
            if (middle.x + array2[i]<0 || middle.x + array2[i]>N-1 || middle.y + array1[i]<0 || middle.y + array1[i]>N-1) {
                continue;
            }
            if (!eats(middle, allCubes[middle.x + array2[i]][middle.y + array1[i]])) {
                points-=1;
                continue;
            }
            allCubes[middle.x + array2[i]][middle.y + array1[i]].type = C_NONE;
            points+=2;
        }
    }
}

void doArea2and3(bool isHorizontal, Cube middle) {
    int i;
    int array1area2[20] = {-3, -3, -3, -3, -3, -2, -2, -1, -1,  0,  0,  1,  1,  2,  2,  3,  3,  3,  3,  3};
    int array2area2[20] = {-2, -1,  0,  1,  2, -2,  2, -2,  2, -2,  2, -2,  2, -2,  2, -2, -1,  0,  1,  2};

    int array1area3[28] = {-4, -4, -4, -4, -4, -4, -4, -3, -3, -2, -2, -1, -1,  0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  4,  4,  4,  4,  4};
    int array2area3[28] = {-3, -2, -1,  0,  1,  2,  3, -3,  3, -3,  3, -3,  3, -3,  3, -3,  3, -3,  3, -3,  3, -3, -2, -1,  0,  1,  2,  3};

    if (isHorizontal) {
        for (i=0; i<20; i++) {
            if (middle.x + array1area2[i]<0 || middle.x + array1area2[i]>N-1 || middle.y + array2area2[i]<0 || middle.y + array2area2[i]>N-1) {
                continue;
            }
            if (eats(middle, allCubes[middle.x + array1area2[i]][middle.y + array2area2[i]])) {
                allCubes[middle.x + array1area2[i]][middle.y + array2area2[i]].type = C_NONE;
                points+=3;
            }
        }
        for (i=0; i<28; i++) {
            if (middle.x + array1area3[i]<0 || middle.x + array1area3[i]>N-1 || middle.y + array2area3[i]<0 || middle.y + array2area3[i]>N-1) {
                continue;
            }
            if (eats(middle, allCubes[middle.x + array1area3[i]][middle.y + array2area3[i]])) {
                allCubes[middle.x + array1area3[i]][middle.y + array2area3[i]].type = C_NONE;
                points+=3;
            }
        }
    } else {
        for (i=0; i<20; i++) {
            if (middle.x + array2area2[i]<0 || middle.x + array2area2[i]>N-1 || middle.y + array1area2[i]<0 || middle.y + array1area2[i]>N-1) {
                continue;
            }
            if (eats(middle, allCubes[middle.x + array2area2[i]][middle.y + array1area2[i]])) {
                allCubes[middle.x + array2area2[i]][middle.y + array1area2[i]].type = C_NONE;
                points+=3;
            }
        }
        for (i=0; i<28; i++) {
            if (middle.x + array2area3[i]<0 || middle.x + array2area3[i]>N-1 || middle.y + array1area3[i]<0 || middle.y + array1area3[i]>N-1) {
                continue;
            }
            if (eats(middle, allCubes[middle.x + array2area3[i]][middle.y + array1area3[i]])) {
                allCubes[middle.x + array2area3[i]][middle.y + array1area3[i]].type = C_NONE;
                points+=3;
            }
        }
    }
}

void destroyTriad(Cube *cube1, Cube *cube2, Cube *cube3) {
    // while calling this func, order matters! cube2 is always the middle of the triad !!
    if ((*cube1).type == C_NONE || (*cube2).type == C_NONE || (*cube3).type == C_NONE) {return;}

    bool isHorizontal;
    int min, max;

    if ((*cube1).x == (*cube3).x)   {isHorizontal = false;}
    else                            {isHorizontal = true;}

    if ((*cube1).type != C_BLUE && (*cube1).type != C_RED) {
        doArea1(isHorizontal, *cube2);
        doArea2and3(isHorizontal, *cube2);
    }

    (*cube1).type = C_NONE;
    (*cube2).type = C_NONE;
    (*cube3).type = C_NONE;

    points+= 10;
    printf("SCORE: %d\n", points);
}

void checkTriad(Cube cube) {
    int x = cube.x;
    int y = cube.y;
    if (x-2 >= 0) {
        if (allCubes[x-2][y].type == allCubes[x-1][y].type && allCubes[x-1][y].type == allCubes[x][y].type) {
            destroyTriad(&(allCubes[x-2][y]), &(allCubes[x-1][y]), &(allCubes[x][y]));
        }
    }
    if (x-1 >= 0 && x+1 < N) {
        if (allCubes[x-1][y].type == allCubes[x][y].type && allCubes[x][y].type == allCubes[x+1][y].type) {
            destroyTriad(&(allCubes[x-1][y]), &(allCubes[x][y]), &(allCubes[x+1][y]));
        }
    }
    if (x+2 < N) {
        if (allCubes[x][y].type == allCubes[x+1][y].type && allCubes[x+1][y].type == allCubes[x+2][y].type) {
            destroyTriad(&(allCubes[x][y]), &(allCubes[x+1][y]), &(allCubes[x+2][y]));
        }
    }
    if (y-2 >= 0) {
        if (allCubes[x][y-2].type == allCubes[x][y-1].type && allCubes[x][y-1].type == allCubes[x][y].type) {
            destroyTriad(&(allCubes[x][y-2]), &(allCubes[x][y-1]), &(allCubes[x][y]));
        }
    }
    if (y-1 >= 0 && y+1 < N) {
        if (allCubes[x][y-1].type == allCubes[x][y].type && allCubes[x][y].type == allCubes[x][y+1].type) {
            destroyTriad(&(allCubes[x][y-1]), &(allCubes[x][y]), &(allCubes[x][y+1]));
        }
    }
    if (y+2 < N) {
        if (allCubes[x][y].type == allCubes[x][y+1].type && allCubes[x][y+1].type == allCubes[x][y+2].type) {
            destroyTriad(&(allCubes[x][y]), &(allCubes[x][y+1]), &(allCubes[x][y+2]));
        }
    }
}

void swapCubes(Cube *cube1, Cube *cube2) {
    int temp = (*cube1).type;
    (*cube1).type = (*cube2).type;
    (*cube2).type = temp;
    movesMade++;
    printf("Moves Left: %d\n", MAX_MOVES - movesMade);
    if (movesMade == 29) {
        printf("GAME OVER !\n");
    }
}

void checkAndSwapCubes(Cube *cube1, Cube *cube2) {
    if (areNeighbours(*cube1, *cube2) && ((*cube1).type != C_NONE || (*cube2).type != C_NONE)) {
        swapCubes(cube1, cube2);
        if ((*cube1).type == C_NONE) {
            destroyEatingNeighbours(*cube2);
        }
        else {
            checkTriad((*cube2));
            checkTriad((*cube1));
        }
        renderScene();
    }
}

void keyboardFunc(int key, int x, int y) {
    switch (key)
    {
        case GLUT_KEY_UP:
            cameray += 0.5f;
            break;

        case GLUT_KEY_DOWN:
            cameray -= 0.5f;
            break;
        
        case GLUT_KEY_LEFT:
            camerax -= 0.5f;
            break;
        
        case GLUT_KEY_RIGHT:
            camerax += 0.5f;
            break;
    }
    renderScene();
}

void menuOptions(int i) {
    switch (i) {
    case 0: // Exit
        exit(0);
        break;
    
    case 1: // Start Game
        startGame();
        break;
    }
}

void initializeMenu(void) {
    glutCreateMenu(menuOptions);
    glutAddMenuEntry("Exit", 0);
    glutAddMenuEntry("Start Game", 1);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

bool nowHolding = false;
Cube *cubeClicked;
Cube *cubeClickRelease;

void mouseFunc(int btn, int state, int x, int y) {
    //printf("%d, %d, %d, %d\n", btn, state, x, y);
    if (movesMade > MAX_MOVES -1 ) {return;}
    int i, j;
    
    if (btn==GLUT_LEFT_BUTTON) {
        
        unsigned char res[4];
        GLint viewport[4]; 

        renderFakeScene();

        glGetIntegerv(GL_VIEWPORT, viewport);
        glReadPixels(x, viewport[3] - y, 1,1,GL_RGBA, GL_UNSIGNED_BYTE, &res);
        
        //printf("%d, %d, %d\n", res[0], res[1], res[2]);

        i = res[0] / N;
        j = res[0] % N;

        //printf("(%d, %d)\n", i, j);
        
        renderScene();

        if (state == GLUT_DOWN) {
            if (res[0]==255 || allCubes[i][j].type == C_NONE) {return;}
            nowHolding = true;
            cubeClicked = &(allCubes[i][j]);
            //allCubes[i][j].type = C_WHITE;
        }
        else if (state == GLUT_UP) {
            if (res[0]==255) {
                nowHolding = false;
                return;
            }
            if (!nowHolding) {
                return;
            }
            checkAndSwapCubes(cubeClicked, &allCubes[i][j]);
        }
        //allCubes[i][j].type = C_WHITE;
    }
    else if (btn==3) { // Mouse Wheel Up
        camerazoom -= 0.5f;
        renderScene();
    }
    else if (btn==4) { // Mouse Wheel Down
        camerazoom += 0.5f;
        renderScene();
    }
}



void initializeScene() {
    camerax = 7.0f;
    cameray = 7.0f;
    camerazoom = 20.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, 600, 500);
    gluPerspective(45.0f, 600.0 / 500.0, 1.0f, 100.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    GLfloat diffuseMaterial[4] = { 0.5, 0.5, 0.5, 1.0 };
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };

    glClearColor(0.0f,0.0f,0.0f,1);
    glShadeModel (GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseMaterial);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 25.0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glColorMaterial(GL_FRONT, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glMatrixMode(GL_MODELVIEW);
    
    glLoadIdentity();
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    glutInit(&argc, argv);
    glutInitWindowSize(600,600);
    glutInitWindowPosition(100,100);
    glutCreateWindow("BraXaPsa III");
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutDisplayFunc(renderScene);
    //glutIdleFunc(renderScene);
    
    glutSpecialFunc(keyboardFunc);
	glutMouseFunc(mouseFunc);

    initializeScene();
    initializeMenu();

    initializeCubes();

    initializeTextures();
	
    glutMainLoop();

    return 0;
}