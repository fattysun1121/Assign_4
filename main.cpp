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

#define FILE_NAME "../data/CThead_512_512_452.raw"
#define W 512
#define H 512
#define D 452

/*
#define FILE_NAME "../data/tooth_100_90_160.raw"
#define W 100
#define H 90
#define D 160
*/


// Glut windows
int volumeRenderingWindow;
int transferFunctionWindow;

// Shader programs
GLuint p;
GLuint backFaceShader;

mat4 proj = Perspective(60, 1.0, 0.1, 20);
//mat4 proj = Ortho(-2.0, 2.0, -2.0, 2.0, 0.0 ,5.0);
mat4 model = mat4(1.0);
mat4 viewMatrix = LookAt(
    vec4(0.0, 0.0, 2.0, 1.0),
    vec4(0.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 0.0));

// Texture object
GLuint objectTex;

// Bounding box 
unsigned int VAO;

enum class Face {
    FRONT,
    BACK
};

// Framebuffers for front face and back face
unsigned int framebuffer;


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
    glBindTexture(GL_TEXTURE_3D, objectTex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, w, h, d, 0, GL_RED, GL_UNSIGNED_BYTE, data);

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
    // (you cant make a window of zero width).
    if(h == 0) h = 1;
    float ratio = 1.0f * (float) w / (float)h;

    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);
}


void keyboard(unsigned char key, int x, int y)
{
    if(key == 'p') {
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
    
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (GLuint*)NULL);
    glDisable(GL_CULL_FACE);
}

void renderScene(void) 
{
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glBindVertexArray(VAO);
    mat4 mvp = proj * viewMatrix * model;
    glUniformMatrix4fv(glGetUniformLocation(backFaceShader, "Mvp"), 1, GL_FALSE, mvp);
    glUniformMatrix4fv(glGetUniformLocation(p, "Mvp"), 1, GL_FALSE, mvp);

    // Render back faces
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);  // The framebuffer we are drawing into
    glUseProgram(backFaceShader);
    drawBoundingBox(Face::BACK);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);  // Back to default
    // Render front faces
    glUseProgram(p);
    drawBoundingBox(Face::FRONT);



    // Debug transfer function values
    std::cout << "R,G,B,A: " << transferFunction[0] << "," << transferFunction[1] << "," << transferFunction[2] << "," << transferFunction[3] << std::endl;
    
    glBindVertexArray(0);
    glutSwapBuffers();
}


void idle()
{
    if (transferFunctionChanged) {
        glutSetWindow(volumeRenderingWindow);
        transferFunctionChanged = false;
        glutPostRedisplay();
    }
}


void init() 
{
    //load3Dfile(FILE_NAME, W, H, D);
        
    /*for (int i = 0; i < 256; i++) {
        transferFunction[i * 4 + 0] = float(i) / 255.0;
        transferFunction[i * 4 + 1] = float(i) / 255.0;
        transferFunction[i * 4 + 2] = float(i) / 255.0;
        transferFunction[i * 4 + 3] = float(i) / 255.0;
    }	*/

    // Framebuffer configuration
    // -------------------------

    unsigned int texture;  // blank 2D texture used to render back faces into

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 600, 300, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
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
    glutInitWindowSize(600,600);
    volumeRenderingWindow = glutCreateWindow("Volume Rendering");

    // register callbacks
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(keyboard);

    //glutMouseFunc(mouseClick);
    //glutMotionFunc(mouseMove);

    glutIdleFunc(idle);

    glEnable(GL_DEPTH_TEST);

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