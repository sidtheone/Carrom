/*!
* 
* Contains Defination of main function 
* Copyright (c) 2010 by Sidharth
*/
//#define GLUT_DISABLE_ATEXIT_HACK
#include "display.h"
#include "variables.h"
#include "engine.h"
#include "model.h"
#include "sound.h"
#include "interface.h"
#include <math.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
using namespace std;




/*!
* \brief
* Used to initilize things needed for glut , intilization of board and quadrics 
*/
void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);

	GLfloat mat_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] = { 1.0 };
	GLfloat light_position[] = { 5.0, 5.0, 10.0, 1.0 };
	GLfloat model_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
	
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	//setting up light
	
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glShadeModel (GL_SMOOTH);
	glEnable(GL_NORMALIZE);
	glEnable(GL_DEPTH_TEST);
	//calling function to initlize carrom board
	initCarromBoardModel();
	//calling function to initlize quadric models
	initQuadricModels();



}

/*!
* \brief
* Timer Function calls itself after an intervel and is used to show animation
* 
* \param extra
* 
* Write detailed description for Timer here.
* 
* \remarks
* Can also use glutidlefunction
* 
* \see
* Separate items with the '|' character.
*/
void Timer(int extra)
{
	carromEngine();
	glutPostRedisplay();
	glutTimerFunc(10, Timer, 0);
}

/*!
* \brief
* called when veiwport is resized
* 
* \param w
* 
* 
* \param h
* 
* 
* 
* Contains just a reshape call to reshape the viewport
*/
void reshape (int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);

}



void safeExit(void)
{
	for(unsigned int i=0;i< objectVector.size();i++)
			delete objectVector[i];
		objectVector.clear();
		KillALData();
}




/*!
* \brief
* Contains main glut loop and calls all other funtions or sets up listerners
* 
* \param argc
* Description of parameter argc.
* 
* \param argv
* Description of parameter argv.
* 
* \returns
* 0 if exit is okay other wise any other value
*/
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	w=600;
	h=600;
	glutInitWindowSize(w, h);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(argv[0]);
	alutInit(NULL, 0);
	//setting of audio properties
	alGetError();
	if(LoadALData() == AL_FALSE)
	{
	    cout<<"Error loading data.";
		return 0;
	}

	SetListenerValues();
	//
	init ();
	glutDisplayFunc(display);
	glutTimerFunc(30, Timer, 0);
	glutReshapeFunc(reshape);
	atexit(safeExit);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(processMouse);
	glutPassiveMotionFunc(processMouseActiveMotion);
	glutMainLoop();
	return 0;
}