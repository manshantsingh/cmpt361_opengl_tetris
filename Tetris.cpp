// Two-Dimensional Sierpinski Gasket       
// Generated using randomly selected vertices and bisection

#include "include/Angel.h"

// remove these before submission
#include <iostream>
using namespace std;

const int NUM_ROWS = 20;
const int NUM_COLS = 10;
const int WINDOWS_SIZE_SCALE=35;
const int WINDOW_SIZE_X = WINDOWS_SIZE_SCALE * (NUM_COLS+2);
const int WINDOW_SIZE_Y = WINDOWS_SIZE_SCALE * (NUM_ROWS+2);
const int NUM_GRID_LINE_POINTS = 4 + 2*NUM_ROWS + 2*NUM_COLS;
const float diffX = 2.0/(NUM_COLS+1), diffY=2.0/(NUM_ROWS+1); // this should give half a cell border
const float cornerX = diffX*5, cornerY = diffY*10;

//----------------------------------------------------------------------------

GLuint grid_vao;

void init_grid() {
    glGenVertexArrays( 1, &grid_vao );
    glBindVertexArray( grid_vao );

    vec2 grid[NUM_GRID_LINE_POINTS] = {
        vec2(-cornerX, 0),
        vec2(cornerX, 0),
        vec2(0, -cornerY),
        vec2(0, cornerY)
    };

    float temp = 0.0;
    for(int i=4; i<4 + 2*NUM_COLS; i+=4){
        temp += diffX;
        grid[i] = vec2(temp, -cornerY);
        grid[i+1] = vec2(temp, cornerY);
        grid[i+2] = vec2(-temp, -cornerY);
        grid[i+3] = vec2(-temp, cornerY);
    }
    temp = 0.0;
    for(int i=4 + 2*NUM_COLS; i<NUM_GRID_LINE_POINTS; i+=4){
        temp += diffY;
        grid[i] = vec2(-cornerX, temp);
        grid[i+1] = vec2(cornerX, temp);
        grid[i+2] = vec2(-cornerX, -temp);
        grid[i+3] = vec2(cornerX, -temp);
    }

    const vec3 lineColor = vec3(0.5, 0.5, 0.5);
    vec3 gridColors[NUM_GRID_LINE_POINTS];
    for(int i=0; i<NUM_GRID_LINE_POINTS; i++) gridColors[i] = lineColor;

    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(grid) + sizeof(gridColors), grid, GL_STATIC_DRAW );

    // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(grid), sizeof(gridColors), gridColors);

    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram( program );

    // Initialize the vertex position attribute from the vertex shader
    GLuint loc = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( loc );
    glVertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(grid)) );
}

//----------------------------------------------------------------------------

void init() {
    init_grid();

    glClearColor( 0.0, 0.0, 0.0, 1.0 ); // white background
}

//----------------------------------------------------------------------------

void display() {
    glBindVertexArray( grid_vao );
    glDrawArrays( GL_LINES, 0, NUM_GRID_LINE_POINTS);
    glFlush();
}

//----------------------------------------------------------------------------
bool downPressed=false;
void keyboard(unsigned char key, int x, int y) {
    switch ( key ) {
    case 'w':
        if(!downPressed){
            downPressed=true;
            cout<<"going down\n";
        }
            
        break;
    case 033:
        exit( EXIT_SUCCESS );
        break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    switch ( key ) {
    case 'w':
        if(downPressed){
            downPressed=false;
            cout<<"keyboardUp"<<endl;
        }
        break;
    case 033:
        exit( EXIT_SUCCESS );
        break;
    }
}

//----------------------------------------------------------------------------

int main(int argc, char **argv) {
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA );
    glutInitWindowSize( WINDOW_SIZE_X, WINDOW_SIZE_Y );

    // If you are using freeglut, the next two lines will check if 
    // the code is truly 3.2. Otherwise, comment them out
    glutInitContextVersion( 3, 2 );
    glutInitContextProfile( GLUT_CORE_PROFILE );

    glutCreateWindow( "Tetris" );

    // Iff you get a segmentation error at line 34, please uncomment the line below
    glewExperimental = GL_TRUE; 
    glewInit();

    init();

    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );
    glutKeyboardUpFunc( keyboardUp );
    glutIgnoreKeyRepeat(true);

    glutMainLoop();
    return 0;
}
