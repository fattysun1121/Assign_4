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

// Texture object
GLuint objectTex;


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


void renderScene(void) 
{
	glClearColor(0, 0, 0, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(p);
	
	// Debug transfer function values
	std::cout << "R,G,B,A: " << transferFunction[0] << "," << transferFunction[1] << "," << transferFunction[2] << "," << transferFunction[3] << std::endl;

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
	load3Dfile(FILE_NAME, W, H, D);
	glUseProgram(p);
		
	for (int i = 0; i < 256; i++) {
		transferFunction[i * 4 + 0] = float(i) / 255.0;
		transferFunction[i * 4 + 1] = float(i) / 255.0;
		transferFunction[i * 4 + 2] = float(i) / 255.0;
		transferFunction[i * 4 + 3] = float(i) / 255.0;
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

	init();

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}