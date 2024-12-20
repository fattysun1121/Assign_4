/*
 * Skeleton code for COSE436 Fall 2024
 *
 * Won-Ki Jeong, wkjeong@korea.ac.kr
 *
 */

#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include <iostream>
#include <assert.h>
#include "textfile.h"
#include "tfeditor.h"
#include "Angel.h"


#define WIN_WIDTH 600
#define WIN_HEIGHT 600


//#define FILE_NAME "../data/CThead_512_512_452.raw"
//#define W 512
//#define H 512
//#define D 452


//#define FILE_NAME "../data/tooth_100_90_160.raw"
//#define W 100
//#define H 90
//#define D 160


//#define FILE_NAME "../data/bonsai_256_256_256.raw"
//#define W 256
//#define H 256
//#define D 256



//#define FILE_NAME "../data/Bucky_32_32_32.raw"
//#define W 32
//#define H 32
//#define D 32



#define FILE_NAME "../data/lung_256_256_128.raw"
#define W 256
#define H 256
#define D 128


// Trackball parameters initialization 
// Rotation
float angle = 0.0, axis[3];
bool leftHold = false;
float lastPos[3] = { 0.0, 0.0, 0.0 };

// Scaling
float zoom = 1.0f;
mat4 zoomMat = Scale(vec3(1.0, 1.0, 1.0));


// Glut windows
int volumeRenderingWindow;
int transferFunctionWindow;

// Shader programs
GLuint p;
GLuint backFaceShader;


// Coordinate spaces initialization
mat4 proj = Perspective(60, 1.0, 0.1, 20);
mat4 model = RotateY(90) * RotateX(90) * Translate(-0.5, -0.5, -0.5);  // centers and faces left
mat4 viewMatrix = LookAt(
    vec4(0.0, 0.0, 2.0, 1.0),
    vec4(0.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 0.0));

// Texture objects
GLuint objectTex;
GLuint backFaceTex;  // blank 2D texture used to render back faces into
GLuint transferFuncTex;

// Other...
unsigned int VAO;

enum class Face {
    FRONT,
    BACK
};

enum class RenderModes {
    MIP = 1,
    ISOSURFACE,
    ALPHA_COMBO
};

enum RenderModes mode = RenderModes::MIP;
unsigned int framebuffer;

mat4 RotateAroundAxis(float ang, float x, float y, float z)
{
    if (ang == 0) {
        return mat4(1.0);
    }

    vec3 naxis = normalize(vec3(x, y, z));
    mat4 result = mat4(1.0);
    result[0][0] = pow(naxis[0], 2) * (1 - cos(ang)) + cos(ang);
    result[0][1] = naxis[0] * naxis[1] * (1 - cos(ang)) - naxis[2] * sin(ang);
    result[0][2] = naxis[0] * naxis[2] * (1 - cos(ang)) + naxis[1] * sin(ang);

    result[1][0] = naxis[0] * naxis[1] * (1 - cos(ang)) + naxis[2] * sin(ang);
    result[1][1] = pow(naxis[1], 2) * (1 - cos(ang)) + cos(ang);
    result[1][2] = naxis[1] * naxis[2] * (1 - cos(ang)) - naxis[0] * sin(ang);

    result[2][0] = naxis[0] * naxis[2] * (1 - cos(ang)) - naxis[1] * sin(ang);
    result[2][1] = naxis[1] * naxis[2] * (1 - cos(ang)) + naxis[0] * sin(ang);
    result[2][2] = pow(naxis[2], 2) * (1 - cos(ang)) + cos(ang);

    return result;
}
//
// Loading volume file, create 3D texture and its histogram
//
void load3Dfile(char *filename,int w,int h,int d) {

    // loading volume data
    FILE *f = fopen(filename, "rb");
    unsigned char *data = new unsigned char[w*h*d];
    fread(data, 1, w*h*d, f);
    fclose(f);

    // generate 3D texture
    glGenTextures(1, &objectTex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, objectTex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, w, h, d, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_3D, 0);
    // create histogram
    for (int i = 0; i<256; i++) {
        histogram[i] = 0;
    }
    for (int i = 0; i < w*h*d; i++) {
        histogram[data[i]]++;
    }
    for (int i = 0; i<256; i++) {
        histogram[i] /= w*h*d;
    }

    delete[]data;
}


void changeSize(int w, int h) {

    // Prevent a divide by zero, when window is too short
    // (you cant make a window of zero WIN_WIDTH).
    if(h == 0) h = 1;
    float ratio = 1.0f * (float) w / (float)h;

    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);
}


void keyboard(unsigned char key, int x, int y)
{
    if (key == '1') {
        mode = RenderModes::MIP;
        std::cout << "MIP" << std::endl;
    }
    else if (key == '2') {
        mode = RenderModes::ISOSURFACE;
        std::cout << "ISOSURFACE" << std::endl;
    }
    else if (key == '3') {
        mode = RenderModes::ALPHA_COMBO;
        std::cout << "ALPHA_COMBO" << std::endl;
    }

    glutPostRedisplay();
}

// Parameter face determines which group of faces (front or back) to render
void drawBoundingBox(enum Face face) {
    glEnable(GL_CULL_FACE);
    if (face == Face::FRONT) {
        glCullFace(GL_BACK);
    }
    else {
        glCullFace(GL_FRONT);
    }
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (GLuint*)NULL);
    glBindVertexArray(0);
    glDisable(GL_CULL_FACE);
}

void renderScene(void) 
{
    zoomMat = Scale(vec3(zoom, zoom, zoom));
    model = zoomMat * RotateAroundAxis(angle, axis[0], axis[1], axis[2]) * model;
    mat4 Mvp = proj * viewMatrix * model;

    // Render back faces
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);  // The framebuffer we are drawing into
    glUseProgram(backFaceShader);
    glUniformMatrix4fv(glGetUniformLocation(backFaceShader, "Mvp"), 1, GL_TRUE, Mvp);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawBoundingBox(Face::BACK);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);  // Back to default

    
    // Render front faces
    glUseProgram(p);
    mat4 Mv = viewMatrix * model;
    glUniformMatrix4fv(glGetUniformLocation(p, "Mvp"), 1, GL_TRUE, Mvp);
    glUniformMatrix4fv(glGetUniformLocation(p, "Mv"), 1, GL_TRUE, Mv);
    glUniformMatrix4fv(glGetUniformLocation(p, "V"), 1, GL_TRUE, viewMatrix);
    glUniform3fv(glGetUniformLocation(p, "lightSrc"), 1, vec3(0.0, 7.0, 0.0));
    glUniform1i(glGetUniformLocation(p, "mode"), (GLint) mode);
    glUniform1i(glGetUniformLocation(p, "transparency"), (GLint)10);
    GLint volumeTexIdx = glGetUniformLocation(p, "tex");
    if (volumeTexIdx >= 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, objectTex);
        glUniform1i(volumeTexIdx, 0);
  
    }
    else
    {
        std::cout << "objectTex"
            << "is not bound to the shader"
            << std::endl;
    }
    GLint backFaceTexIdx = glGetUniformLocation(p, "backFaceTex");
    if (backFaceTexIdx >= 0)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, backFaceTex);
        glUniform1i(backFaceTexIdx, 1);
    }
    else
    {
        std::cout << "backFaceTex"
            << "is not bound to the shader"
            << std::endl;
    }
    GLint transferFuncTexIdx = glGetUniformLocation(p, "transferFuncTex");
    if (transferFuncTexIdx >= 0)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_1D, transferFuncTex);
        glUniform1i(transferFuncTexIdx, 2);
    }
    else
    {
        std::cout << "transferFuncTex"
            << "is not bound to the shader"
            << std::endl;
    }
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawBoundingBox(Face::FRONT);

    angle = 0.0f;
    zoom = 1.0f;
    glutSwapBuffers();
}


void idle()
{
    if (transferFunctionChanged) {
        glutSetWindow(volumeRenderingWindow);
        // Dynamically changes transfer function texture
        glBindTexture(GL_TEXTURE_1D, transferFuncTex);
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_FLOAT, transferFunction);
        glBindTexture(GL_TEXTURE_1D, 0);
        transferFunctionChanged = false;
        glutPostRedisplay();
    }
}


void init() 
{
    load3Dfile(FILE_NAME, W, H, D);
        
    // Transfer function configuration
    // -------------------------------
    for (int i = 0; i < 256; i++) {
        transferFunction[i * 4 + 0] = float(i) / 255.0;
        transferFunction[i * 4 + 1] = float(i) / 255.0;
        transferFunction[i * 4 + 2] = float(i) / 255.0;
        transferFunction[i * 4 + 3] = float(i) / 255.0;
    }	

    glGenTextures(1, &transferFuncTex);
    glBindTexture(GL_TEXTURE_1D, transferFuncTex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, 256, 0, GL_RGBA, GL_FLOAT, transferFunction);
    glBindTexture(GL_TEXTURE_1D, 0);

    // Framebuffer configuration
    // -------------------------
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenTextures(1, &backFaceTex);
    glBindTexture(GL_TEXTURE_2D, backFaceTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // width and height should be screen size since we want the framebuffer to be drawn as default screen
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backFaceTex, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
  
    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete: " << fboStatus << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);  // Back to default framebuffer

    // Bounding box VAO configuration
    // ------------------------------
    GLfloat vertices[24] = {
        0.0, 0.0, 0.0,
        0.0, 0.0, 1.0,
        0.0, 1.0, 0.0,
        0.0, 1.0, 1.0,
        1.0, 0.0, 0.0,
        1.0, 0.0, 1.0,
        1.0, 1.0, 0.0,
        1.0, 1.0, 1.0
    };

    GLuint indices[36] = {
        1,5,7,
        7,3,1,
        0,2,6,
        6,4,0,
        0,1,3,
        3,2,0,
        7,5,4,
        4,6,7,
        2,3,7,
        7,6,2,
        1,0,4,
        4,5,1
    };
    unsigned int VBO, IBO;

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    // used in glDrawElement()
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLuint), indices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0); // for vertexloc
    glEnableVertexAttribArray(1); // for vertexcol

    // the vertex location is the same as the vertex color
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat*)NULL);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat*)NULL);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBindVertexArray(0);

}


void mouseMove(int x, int y)
{
    if (leftHold) //If the left button has been clicked
    {
        float curPos[3], dx, dy, dz;
        float d, norm;
        curPos[0] = (2.0f * x - WIN_WIDTH) / WIN_WIDTH; // Calculate x component for vector at mouse's current position
        curPos[1] = (WIN_HEIGHT - 2.0f * y) / WIN_HEIGHT; // Calculate y component for vector at mouse's current position
        d = sqrtf(curPos[0] * curPos[0] + curPos[1] * curPos[1]); // Calculate z component
        d = (d < 1.0f) ? d : 1.0f; // Project vector onto surface of trackball
        curPos[2] = sqrtf(1.001f - d * d); //Calculate z component
        norm = 1.0 / sqrt(curPos[0] * curPos[0] + curPos[1] * curPos[1] + curPos[2] * curPos[2]);
        curPos[0] *= norm; // Normalize vecor
        curPos[1] *= norm;
        curPos[2] *= norm;
        dx = curPos[0] - lastPos[0]; // Check if mouse has moved from last position
        dy = curPos[1] - lastPos[1];
        dz = curPos[2] - lastPos[2];
        if (dx || dy || dz) // If mouse has moved
        {
            angle = 90.0 * sqrt(dx * dx + dy * dy + dz * dz); // Calculate rotation angle
            axis[0] = lastPos[1] * curPos[2] - lastPos[2] * curPos[1]; // Calculate rotation axis
            axis[1] = lastPos[2] * curPos[0] - lastPos[0] * curPos[2];
            axis[2] = lastPos[0] * curPos[1] - lastPos[1] * curPos[0];
            lastPos[0] = curPos[0]; // Set the last position to the current
            lastPos[1] = curPos[1];
            lastPos[2] = curPos[2];
        }
    }

    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y)
{
    // Detect mouse click
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        leftHold = true;
        float d, norm;
        lastPos[0] = (2.0f * x - WIN_WIDTH) / WIN_WIDTH; // Calculate the x and y components of the initial movement vector
        lastPos[1] = (WIN_HEIGHT - 2.0f * y) / WIN_HEIGHT;
        d = sqrtf(lastPos[0] * lastPos[0] + lastPos[1] * lastPos[1]); // Calculate the z component of the vector using pythagorean theorem
        d = (d < 1.0f) ? d : 1.0f; // If the z component is not on the surface of the trackball, set it to 1: the radius of the trackball
        lastPos[2] = sqrtf(1.001f - d * d);
        norm = 1.0 / sqrt(lastPos[0] * lastPos[0] + lastPos[1] * lastPos[1] + lastPos[2] * lastPos[2]); // Calculate vector length
        lastPos[0] *= norm; // Normalize vector
        lastPos[1] *= norm;
        lastPos[2] *= norm;
    }
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
            zoom -= 0.1f;
        }
        else {
            zoom += 0.1f;
        }
        
        leftHold = false;
    }
    else {
        leftHold = false;
    }
}
int main(int argc, char **argv) 
{
    glutInit(&argc, argv);

    //
    // 1. Transfer Function Editor Window
    //
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 700);
    glutInitWindowSize(600, 300);
    transferFunctionWindow = glutCreateWindow("Transfer Function");

    // register callbacks
    glutDisplayFunc(renderScene_transferFunction);
    glutReshapeFunc(changeSize_transferFunction);

    glutMouseFunc(mouseClick_transferFunction);
    glutMotionFunc(mouseMove_transferFunction);
    glutIdleFunc(idle);

    init_transferFunction();

    //
    // 2. Main Volume Rendering Window
    //
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(WIN_WIDTH,WIN_HEIGHT);
    volumeRenderingWindow = glutCreateWindow("Volume Rendering");

    // register callbacks
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(keyboard);

    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMove);

    glutIdleFunc(idle);

    glewInit();
    if (glewIsSupported("GL_VERSION_3_3"))
        printf("Ready for OpenGL 3.3\n");
    else {
        printf("OpenGL 3.3 is not supported\n");
        exit(1);
    }

    // Create shader program
    p = createGLSLProgram( "../volumeRendering.vert", NULL, "../volumeRendering.frag" );
    backFaceShader = createGLSLProgram("../face.vert", NULL, "../face.frag");
    init();

    // enter GLUT event processing cycle
    glutMainLoop();

    return 1;
}