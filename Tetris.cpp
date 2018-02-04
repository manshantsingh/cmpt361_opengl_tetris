// Two-Dimensional Sierpinski Gasket       
// Generated using randomly selected vertices and bisection

#include "include/Angel.h"

#include <cstdlib>
#include <ctime>
#include <vector>

using namespace std;

const int NUM_ROWS = 20;
const int NUM_COLS = 10;
const int WINDOWS_SIZE_SCALE=35;
const int WINDOW_SIZE_X = WINDOWS_SIZE_SCALE * (NUM_COLS+2);
const int WINDOW_SIZE_Y = WINDOWS_SIZE_SCALE * (NUM_ROWS+2);
const int NUM_GRID_LINE_POINTS = 4 + 2*NUM_ROWS + 2*NUM_COLS;
const float diffX = 2.0/(NUM_COLS+1), diffY=2.0/(NUM_ROWS+1); // this should give half a cell border
const float cornerX = diffX*5, cornerY = diffY*10;

const float gravity_time = 1000.0;

//----------------------------------------------------------------------------

enum Rotation_mode { NONE, SEMI, FULL};

class Shape{
public:
    Shape(vector<vec2> positions, vec2 center, Rotation_mode rmode)
        : _pos(positions), _center(center), _rmode(rmode) {
            for(vec2& v: _pos){
                v *= _SCALE;
            }
            _center *= _SCALE;
        }

    Shape(const Shape& s)
        : _pos(s._pos), _center(s._center), _rmode(s._rmode) {
            move(_START_POS_OFFSET);
        }

    void rotate() {
        if(_rmode == NONE) return;

        if(_straight) {
            for(vec2& v: _pos) {
                v = _ROTATE * (v - _center) + _center;
            }
        }
        else{
            for(vec2& v: _pos) {
                v = _ROTATE_REVERSE * (v - _center) + _center;
            }
        }
            
        if(_rmode == SEMI) _straight = !_straight;
    }

    const vector<vec2>& getPos(){
        return _pos;
    }

    const vector<vec3>& getColor(){
        return _color;
    }

    void setColor(vec3 val){
        _color = vector<vec3>(_pos.size(), val);
    }

    bool moveAndTestOutOfScreen(){
        move(_GRAVITY);
        return _center.y < -cornerY;
    }
private:
    static float _ANGLE;
    static mat2 _ROTATE;
    static mat2 _ROTATE_REVERSE;
    static vec2 _SCALE;
    static vec2 _GRAVITY;
    static vec2 _START_POS_OFFSET;

    vector<vec2> _pos;
    vector<vec3> _color;
    vec2 _center;
    Rotation_mode _rmode;
    bool _straight = true;

    void move(vec2 offset){
        for(vec2& v: _pos){
            v += offset;
        }
        _center += offset;
    }
};

float Shape::_ANGLE = M_PI/2;
mat2 Shape::_ROTATE = mat2 ( cos(_ANGLE), sin(_ANGLE), -sin(_ANGLE), cos(_ANGLE) );
mat2 Shape::_ROTATE_REVERSE = mat2 ( cos(-_ANGLE), sin(-_ANGLE), -sin(-_ANGLE), cos(-_ANGLE) );
vec2 Shape::_SCALE = vec2(diffX, diffY);
vec2 Shape::_GRAVITY = vec2(0, -diffY);
vec2 Shape::_START_POS_OFFSET = vec2(0, cornerY);

template<typename T>
int vecSize(const vector<T>& v){
    return v.size() * sizeof(T);
}

const int NUM_SHAPES = 7;
Shape shapes[NUM_SHAPES] = {
    // O
    Shape({ vec2(-1, 0), vec2(1, 0), vec2(-1, -2), vec2(1, -2) },
        vec2(0, -1),
        NONE
    ),
    // I
    Shape({ vec2(-2, 0), vec2(2, 0), vec2(-2, -1), vec2(2, -1) },
        vec2(0.5, -0.5),
        SEMI
    ),
    // S
    Shape({ vec2(2, 0), vec2(0, 0), vec2(2, -1), vec2(-0, -1),
            vec2(1, -1), vec2(-1, -1), vec2(1, -2), vec2(-1, -2) },
        vec2(0.5, -0.5),
        SEMI
    ),
    // Z
    Shape({ vec2(-1, 0), vec2(1, 0), vec2(-1, -1), vec2(1, -1),
            vec2(0, -1), vec2(2, -1), vec2(0, -2), vec2(2, -2) },
        vec2(0.5, -0.5),
        SEMI
    ),
    // L
    Shape({ vec2(2, 0), vec2(2, -1), vec2(-1, 0), vec2(0, -1), vec2(-1, -2), vec2(0, -2) },
        vec2(0.5, -0.5),
        FULL
    ),
    // J
    Shape({ vec2(-1, 0), vec2(-1, -1), vec2(2, 0), vec2(1, -1), vec2(2, -2), vec2(1, -2) },
        vec2(0.5, -0.5),
        FULL
    ),
    // T
    Shape({ vec2(0, -2), vec2(1, -2), vec2(0, -1), vec2(1, -1),
            vec2(-1, -1), vec2(2, -1), vec2(-1, 0), vec2(2, 0) },
        vec2(0.5, -0.5),
        FULL
    )
};

Shape* curr;


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

void display_single() {
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );

    auto& pos = curr->getPos();
    auto& color = curr->getColor();

    glBufferData( GL_ARRAY_BUFFER, vecSize(pos) + vecSize(color), &pos[0], GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, vecSize(pos), &pos[0] );
    glBufferSubData( GL_ARRAY_BUFFER, vecSize(pos), vecSize(color), &color[0] );

    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram( program );

    GLuint loc = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( loc );
    glVertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(vecSize(pos)) );

    glClear( GL_COLOR_BUFFER_BIT );     // clear the window

    glDrawArrays( GL_TRIANGLE_STRIP, 0, pos.size() );
}

void display() {
    display_single();

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
        case 'q':
            exit( EXIT_SUCCESS );
            break;
    }
}

void keyboardSpecial( int key, int x, int y )
{
    switch(key){
        case GLUT_KEY_DOWN:
            if(!downPressed){
                downPressed=true;
                cout<<"going down special\n";
            }
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
    }
}

void keyboardSpecialUp( int key, int x, int y )
{
    switch(key){
        case GLUT_KEY_DOWN:
            if(downPressed){
                downPressed=false;
                cout<<"keyboardUp special"<<endl;
            }
            break;
    }
}

//----------------------------------------------------------------------------

void setNewCurr(){
    if(curr != nullptr){
        delete curr;
    }
    curr = new Shape(shapes[rand()%NUM_SHAPES]);
    curr->setColor(vec3(1.0, 0.0, 0.0));
}

void gravity(int){
    if(curr == nullptr || curr->moveAndTestOutOfScreen()){
        setNewCurr();
    }

    glutPostRedisplay();
    glutTimerFunc(gravity_time, gravity, 0);
}

//----------------------------------------------------------------------------

int main(int argc, char **argv) {
    srand(time(nullptr));
    setNewCurr();

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

    glutSpecialFunc( keyboardSpecial );
    glutSpecialUpFunc( keyboardSpecialUp );

    glutIgnoreKeyRepeat(true);

    glutTimerFunc(gravity_time, gravity, 0);

    glutMainLoop();
    return 0;
}
