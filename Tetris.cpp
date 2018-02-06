// Two-Dimensional Sierpinski Gasket       
// Generated using randomly selected vertices and bisection

#include "include/Angel.h"

#include <cstdlib>
#include <ctime>
#include <vector>
#include <map>

using namespace std;

const int NUM_ROWS = 20;
const int NUM_COLS = 10;
const int WINDOWS_SIZE_SCALE=35;
const int WINDOW_SIZE_X = WINDOWS_SIZE_SCALE * (NUM_COLS+2);
const int WINDOW_SIZE_Y = WINDOWS_SIZE_SCALE * (NUM_ROWS+2);
const int NUM_GRID_LINE_POINTS = 4 + 2*NUM_ROWS + 2*NUM_COLS;
const float diffX = 2.0/(NUM_COLS+1), diffY=2.0/(NUM_ROWS+1); // this should give half a cell border
const float cornerX = diffX*5, cornerY = diffY*10;

const float UPDATE_INTERVAL = 10.0;
const int REGULAR_GRAVITY_FACTOR = 5;

const int NUM_COLORS = 6;
vec3 SHAPE_COLORS[NUM_COLORS] = {
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0),
    vec3(0.0, 1.0, 1.0),
    vec3(1.0, 0.0, 1.0)
};


GLuint grid_vao;

vector<vec2> ground_points;
vector<vec3> ground_colors;
vector<vector<vec3*>> cell_colors;

bool downPressed = false;
bool gameOver = false;
int updateCounter = 0;

//----------------------------------------------------------------------------

enum Rotation_mode { NONE, SEMI, FULL};

struct coord{
    int x, y;
    coord(){}
    coord(int a, int b): x(a), y(b) {}
    coord(const coord& another): x(another.x), y(another.y) {}
};

class Shape{
public:
    Shape(vector<coord> pos, Rotation_mode rmode) : _pos(pos), _rmode(rmode) {}
    Shape(const Shape& s) : _pos(s._pos), _rmode(s._rmode) {}

    void rotate() {
        if(_rmode == NONE) return;
        vector<coord> backup_pos = _pos; 
        coord backup_center=_center;

        if(_straight) {
            for(coord& v: _pos) {
                v = coord(-v.y , v.x);
            }
        }
        else{
            for(coord& v: _pos) {
                v = coord(v.y , -v.x);
            }
        }
            
        if(_rmode == SEMI) _straight = !_straight;

        // check rotation moved out of bounds
        int minX=NUM_COLS-1, minY=NUM_ROWS-1, maxX=0, maxY=0;
        for(auto v: getPos()){
            minX = min(v.x, minX);
            minY = min(v.y, minY);
            maxX = max(v.x, maxX);
            maxY = max(v.y, maxY);
        }

        if(minX<0) _center.x-=minX;
        else if(maxX>=NUM_COLS) _center.x -= NUM_COLS - maxX - 1;
        if(minY<0) _center.y-=minY;
        else if(maxY>=NUM_ROWS) _center.y -= NUM_ROWS - maxY - 1;

        if(hasCollision()){
            _pos = backup_pos;
            _center = backup_center;
            if(_rmode == SEMI) _straight = !_straight;
        }
    }

    vector<coord> getPos(){
        vector<coord> v(_pos.size());
        for(unsigned int i=0;i<v.size();i++){
            v[i] = coord(_pos[i].x + _center.x, _pos[i].y + _center.y);
        }
        return v;
    }

    vec3* getColor(){
        return _color;
    }

    void setColor(vec3* val){
        _color = val;
    }

    void moveHorizontal(bool right){
        _center.x += right ? 1 : -1;
        if(hasCollision()){
            _center.x -= right ? 1 : -1;
        }
    }

    bool moveDown(){
        _center.y--;
        if(hasCollision()){
            _center.y++;
            return true;
        }
        return false;
    }

    bool hasCollision(){
        auto pos = getPos();
        for(auto& v: pos){
            if(v.x<0 || v.x>=NUM_COLS || v.y<0 || v.y>=NUM_ROWS || cell_colors[v.y][v.x]!=nullptr){
                return true;
            }
        }
        return false;
    }
private:
    static coord _START_POS;

    vector<coord> _pos;
    vec3* _color;
    coord _center = _START_POS;
    Rotation_mode _rmode;
    bool _straight = true;
};
coord Shape::_START_POS = coord(NUM_COLS/2, NUM_ROWS-1);

Shape* curr;

const int NUM_SHAPES = 7;
Shape shapes[NUM_SHAPES] = {
    // O
    Shape({ coord(0, 0), coord(0, -1), coord(1, 0), coord(1, -1) }, NONE),
    // I
    Shape({ coord(-2, 0), coord(-1, 0), coord(0, 0), coord(1, 0) }, SEMI),
    // S
    Shape({ coord(0, 0), coord(1, 0), coord(-1, -1), coord(0, -1) }, SEMI),
    // Z
    Shape({ coord(-1, 0), coord(0, 0), coord(0, -1), coord(1, -1) }, SEMI),
    // L
    Shape({ coord(-1, 0), coord(0, 0), coord(1, 0), coord(-1, -1) }, FULL),
    // J
    Shape({ coord(-1, 0), coord(0, 0), coord(1, 0), coord(1, -1) }, FULL),
    // T
    Shape({ coord(-1, 0), coord(0, 0), coord(1, 0), coord(0, -1) }, FULL)
};

template<typename T>
int vecSize(const vector<T>& v){
    return v.size() * sizeof(T);
}

void appendPoints(const vector<coord>& pos, vector<vec2>& points, vec3 color, vector<vec3>& colors){
    for(coord v: pos){
        vec2 temp(v.x * diffX - cornerX, v.y * diffY - cornerY);

        if(points.size()){
            points.push_back(points.back());
            points.push_back(temp);
            colors.push_back(color);
            colors.push_back(color);
        }

        points.push_back(temp);
        temp.x += diffX;
        points.push_back(temp);
        temp += vec2(-diffX, diffY);
        points.push_back(temp);
        temp.x += diffX;
        points.push_back(temp);

        colors.push_back(color);
        colors.push_back(color);
        colors.push_back(color);
        colors.push_back(color);
    }
}

void recomputePoints(){
    ground_points.clear();
    ground_colors.clear();

    for(int i=0; i<NUM_ROWS; i++){
        for(int j=0; j<NUM_COLS; j++){
            vec3* v = cell_colors[i][j];
            if(v==nullptr) continue;
            vec2 temp(j * diffX - cornerX, i * diffY - cornerY);

            if(ground_points.size()){
                ground_points.push_back(ground_points.back());
                ground_points.push_back(temp);
                ground_colors.push_back(*v);
                ground_colors.push_back(*v);
            }

            ground_points.push_back(temp);
            temp.x += diffX;
            ground_points.push_back(temp);
            temp += vec2(-diffX, diffY);
            ground_points.push_back(temp);
            temp.x += diffX;
            ground_points.push_back(temp);

            ground_colors.push_back(*v);
            ground_colors.push_back(*v);
            ground_colors.push_back(*v);
            ground_colors.push_back(*v);
        }
    }
}


//----------------------------------------------------------------------------

void setNewCurr(){
    if(curr != nullptr){
        auto pos=curr->getPos();
        auto* color=curr->getColor();
        for(auto& v: pos){
            if(v.x<0||v.y<0) {
                cout<<"should not get here\n";
                cout<<v.x<<" while limit: "<<NUM_COLS<<"\n"<<v.y<<" while limit: "<<NUM_ROWS<<endl<<endl;
                continue;
            }
            cell_colors[v.y][v.x]=color;
        }
        auto it=cell_colors.begin();
        bool somethingChanged=false;
        while(it!=cell_colors.end()){
            bool rowFull=true;
            for(auto x: *it){
                if(x==nullptr){
                    rowFull=false;
                    break;
                }
            }
            if(rowFull){
                somethingChanged=true;
                it = cell_colors.erase(it);
            }
            else it++;
        }
        if(somethingChanged){
            for(int i=cell_colors.size();i<NUM_ROWS;i++) cell_colors.emplace_back(NUM_COLS, nullptr);
            recomputePoints();
        }
        else appendPoints(pos, ground_points, *color, ground_colors);
        delete curr;
    }
    curr = new Shape(shapes[rand()%NUM_SHAPES]);
    curr->setColor(&SHAPE_COLORS[rand()%NUM_COLORS]);
    if(curr->hasCollision()){
        gameOver=true;
        cout<<"\n\nYOU LOST\n\n";
    }
}

//----------------------------------------------------------------------------

void init_grid_lines() {
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
    ground_points.clear();
    ground_colors.clear();
    cell_colors = vector<vector<vec3*>>(NUM_ROWS, vector<vec3*>(NUM_COLS, nullptr));

    curr = nullptr;
    gameOver = false;
    updateCounter = 0;
    setNewCurr();
    srand(time(nullptr));

    glClearColor( 0.0, 0.0, 0.0, 1.0 ); // black background
}

// void update_ground() {
//     glGenVertexArrays( 1, &ground_vao );
//     glBindVertexArray( ground_vao );

//     GLuint buffer;
//     glGenBuffers( 1, &buffer );
//     glBindBuffer( GL_ARRAY_BUFFER, buffer );

//     glBufferData( GL_ARRAY_BUFFER, vecSize(ground_points) + vecSize(ground_colors), &ground_points[0], GL_STATIC_DRAW );
//     glBufferSubData( GL_ARRAY_BUFFER, vecSize(ground_points), vecSize(ground_colors), &ground_colors[0] );

//     GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
//     glUseProgram( program );

//     GLuint loc = glGetAttribLocation( program, "vPosition" );
//     glEnableVertexAttribArray( loc );
//     glVertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

//     GLuint vColor = glGetAttribLocation( program, "vColor" );
//     glEnableVertexAttribArray( vColor );
//     glVertexAttribPointer( vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(vecSize(ground_points)) );
// }

//----------------------------------------------------------------------------

void display_curr() {
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );

    vector<vec2> points;
    vector<vec3> colors;
    appendPoints(curr->getPos(), points, *curr->getColor(), colors);

    glBufferData( GL_ARRAY_BUFFER, vecSize(points) + vecSize(colors), &points[0], GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, vecSize(points), vecSize(colors), &colors[0] );

    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram( program );

    GLuint loc = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( loc );
    glVertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(vecSize(points)) );

    glDrawArrays( GL_TRIANGLE_STRIP, 0, points.size() );
}

void display_ground() {
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );

    glBufferData( GL_ARRAY_BUFFER, vecSize(ground_points) + vecSize(ground_colors), &ground_points[0], GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, vecSize(ground_points), vecSize(ground_colors), &ground_colors[0] );

    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram( program );

    GLuint loc = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( loc );
    glVertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(vecSize(ground_points)) );

    glDrawArrays( GL_TRIANGLE_STRIP, 0, ground_points.size() );
}

void display() {
    glClear( GL_COLOR_BUFFER_BIT );     // clear the window

    display_curr();
    display_ground();

    glBindVertexArray( grid_vao );
    glDrawArrays( GL_LINES, 0, NUM_GRID_LINE_POINTS);

    glFlush();
}

void gravity(){
    if(curr == nullptr || curr->moveDown()){
        setNewCurr();
    }
}

void update(int){
    if(downPressed || ++updateCounter > REGULAR_GRAVITY_FACTOR) {
        updateCounter = 0;
        gravity();
        glutPostRedisplay();

        if(gameOver) return; 
    }

    glutTimerFunc(UPDATE_INTERVAL, update, 0);
}

void reset(){
    init();
    glutPostRedisplay();
    glutTimerFunc(UPDATE_INTERVAL, update, 0);
}

//----------------------------------------------------------------------------
void keyboard(unsigned char key, int x, int y) {
    switch ( key ) {
        case 'r':
            cout<<"\n\n\RESTART\n\n\n";
            reset();
            break;
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
            }
            break;
        case GLUT_KEY_UP:
            curr->rotate();
            glutPostRedisplay();
            break;
        case GLUT_KEY_LEFT:
            curr->moveHorizontal(false);
            glutPostRedisplay();
            break;
        case GLUT_KEY_RIGHT:
            curr->moveHorizontal(true);
            glutPostRedisplay();
            break;
    }
}

void keyboardSpecialUp( int key, int x, int y )
{
    switch(key){
        case GLUT_KEY_DOWN:
            if(downPressed){
                downPressed=false;
            }
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
    init_grid_lines();

    glutDisplayFunc( display );

    glutKeyboardFunc( keyboard );

    glutSpecialFunc( keyboardSpecial );
    glutSpecialUpFunc( keyboardSpecialUp );

    glutIgnoreKeyRepeat(true);

    glutTimerFunc(UPDATE_INTERVAL, update, 0);

    glutMainLoop();
    return 0;
}
