/*!
* Defination of various functions that create the view that is shown in the viewports
* 
* Copyright (c) 2010 by sidharth
*/
#include "display.h"
#include "variables.h"
#include "object.h"
#include <al/al.h>
#include <al/alc.h>
#include <al/alut.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

//ambient characterstics of various materials
GLfloat mat_ambient[] = { 0.7, 0., 0.0, 1.0 };
GLfloat mat_ambient_color[] = { 0.0, 0., 0.0, 1.0 };
GLfloat mat_ambient_red[] = { 1, 0.0, 0.0, 1.0 };
GLfloat mat_ambient_white[] = { 0.5, 0.5, 0.5, 1.0 };
GLfloat mat_ambient_black[] = { 0.0, 0.0, 0.0, 1.0 };
GLfloat mat_ambient_striker[] = { 0.0, 0.0, .6, 1.0 };

//diffuse characterstics of various materails
GLfloat mat_diffuse[] = { 1, 0.90, 0.0, .5 };
GLfloat mat_diffuse_red[] = { 1, 0.0, 0.0, .5 };
GLfloat mat_diffuse_white[] = { .7, .7, .7, .5 };
GLfloat mat_diffuse_black[] = { .01, .01, .01, 1 };
GLfloat mat_diffuse_striker[] = { .00, .00, .1, 1 };

//specular and misclenious characterstics
GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1};
GLfloat no_shininess[] = { 0.0 };
GLfloat low_shininess[] = { 10.0 };
GLfloat high_shininess[] = { 100.0 };
GLfloat mat_emission[] = {0.3, 0.2, 0.2, 0.0};
GLfloat no_mat[] = { 0.0, 0.0, 0.0, 1.0 };
GLfloat position[] = { 5.0, 5.0, 10.0, 1.0 };


/*!
* \brief
* Creates Boundary of the carromBoard
* 
* \param length
* Length of the boundary
* 
* \param breadth
* Length of the boundary
* 
* \param width
* Length of the boundary
*  
* Sets the material properties and draws the bounadary
* 
* \remarks
* Need to be called 4 times for the sides and 4 times for the corner to make the vertices alinged
* 
*/
void inline createBoardBoundary(float length, float breadth,float width)
{
	glScalef (length, breadth, width);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	//glHint( GL_POLYGON_SMOOTH_HINT,GL_NICEST );
	glutSolidCube (1.0);

}

/*!
* \brief
* creates the stirker box where stirker can be placed
* 
* 
* \param length
* Length of the boundary
* 
* \param breadth
* Length of the boundary
* 
* \param width
* Length of the boundary
*  
* Draws a glut wire cube 
* 
* \remarks
* Need to be called 4 times for the 4 visual boundaries can also be improved in design
* 
*/
void inline createBoardVisualStrikerBox(float length, float breadth, float width)
{
	glScalef (length, breadth, width);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);


	glutWireCube(1.0);

}

/*!
* \brief
* Used to create the pockets of the carromBoard
* 
* Creates a black disc that can be placed as a pocket
* 
* \remarks
* The disc needs to be just above the board for visibility
* 
*/
void inline createPocket()
{
	glShadeModel (GL_SMOOTH);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_black);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_black);
	glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	glCallList(startList+1);
}


/*!
* \brief
* Creates the visual outer circle 
* 
* 
* this creates a hollow disc as a circle
* 
* \remarks
* The circle is created and needs to be scaled to suit the board dimensions
* 
*/
void inline createOuterCircle()
{
	glShadeModel (GL_SMOOTH);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_black);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_black);
	glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	glCallList(startList+2);
}


/*!
* \brief
* Creates the board 
* 
* \param carromBoard
* CarromBoard pointer containing the dimensions of the board and othe properties
* 
* Function 1) Creates the playing board 2) Creates boundaries 3)creates pockets 4) creates visuals 
* 
* \remarks
* The whole board model needs to be scaled for the view
* 
*/
void  createBoard1(CarromBoard* carromBoard)
{

	glPushMatrix();// doing this for the board 
	//Making the board surface


	glPushMatrix();
	glScalef (carromBoard->boardSurface.length, carromBoard->boardSurface.breadth, carromBoard->boardSurface.height);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	glutSolidCube (1.0);

	glPopMatrix();


	//Making the board boundary

	glPushMatrix();
	glTranslatef(0.0, carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2, 0.0);
	createBoardBoundary(carromBoard->boardSurface.length, carromBoard->boardBoundary.breadth, carromBoard->boardBoundary.height);			
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0, -(carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), 0.0);
	createBoardBoundary(carromBoard->boardSurface.length, carromBoard->boardBoundary.breadth, carromBoard->boardBoundary.height);			
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-(carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), 0.0, 0.0);
	createBoardBoundary(carromBoard->boardBoundary.breadth, carromBoard->boardSurface.length  , carromBoard->boardBoundary.height);			

	glPopMatrix();

	glPushMatrix();
	glTranslatef((carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), 0.0, 0.0);
	createBoardBoundary(carromBoard->boardBoundary.breadth, carromBoard->boardSurface.length  ,carromBoard->boardBoundary.height);				
	glPopMatrix();	

	//creating board corners
	glPushMatrix();
	glTranslatef(-(carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), -(carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), 0.0);
	createBoardBoundary(carromBoard->boardBoundary.breadth,carromBoard->boardBoundary.breadth , carromBoard->boardBoundary.height);			

	glPopMatrix();

	glPushMatrix();
	glTranslatef((carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), -(carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), 0.0);
	createBoardBoundary(carromBoard->boardBoundary.breadth,carromBoard->boardBoundary.breadth , carromBoard->boardBoundary.height);			

	glPopMatrix();

	glPushMatrix();
	glTranslatef((carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), (carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), 0.0);
	createBoardBoundary(carromBoard->boardBoundary.breadth,carromBoard->boardBoundary.breadth , carromBoard->boardBoundary.height);			

	glPopMatrix();

	glPopMatrix();

	glPushMatrix();
	glTranslatef(-(carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), (carromBoard->boardSurface.length/2 + carromBoard->boardBoundary.breadth/2), 0.0);
	createBoardBoundary(carromBoard->boardBoundary.breadth,carromBoard->boardBoundary.breadth , carromBoard->boardBoundary.height);			

	glPopMatrix();

	//makingPockets

	glPushMatrix();
	glTranslatef((carromBoard->boardSurface.length /2 - carromBoard->boardPockets[0].radius),(carromBoard->boardSurface.length /2 - carromBoard->boardPockets[0].radius), carromBoard->boardSurface.height/2 + 0.005);				
	glScalef(carromBoard->boardPockets[0].radius, carromBoard->boardPockets[0].radius, 1.0);
	createPocket();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-(carromBoard->boardSurface.length /2 - carromBoard->boardPockets[0].radius),(carromBoard->boardSurface.length /2 - carromBoard->boardPockets[0].radius), carromBoard->boardSurface.height/2 + 0.005);				
	glScalef(carromBoard->boardPockets[0].radius, carromBoard->boardPockets[0].radius, 1.0);

	createPocket();
	glPopMatrix();

	glPushMatrix();
	glTranslatef((carromBoard->boardSurface.length /2 - carromBoard->boardPockets[0].radius),-(carromBoard->boardSurface.length /2 - carromBoard->boardPockets[0].radius), carromBoard->boardSurface.height/2 + 0.005);				
	glScalef(carromBoard->boardPockets[0].radius, carromBoard->boardPockets[0].radius, 1.0);

	createPocket();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-(carromBoard->boardSurface.length /2 - carromBoard->boardPockets[0].radius),-(carromBoard->boardSurface.length /2 - carromBoard->boardPockets[0].radius), carromBoard->boardSurface.height/2 + 0.005);				
	glScalef(carromBoard->boardPockets[0].radius, carromBoard->boardPockets[0].radius, 1.0);
	createPocket();
	glPopMatrix();

	// Creating board visuals 
	glPushMatrix();
	glTranslatef((carromBoard->boardSurface.length/2 - carromBoard->boardBoundary.breadth), 0.0, carromBoard->boardSurface.height/2 + 0.005);
	createBoardVisualStrikerBox(carromBoard->boardBoundary.breadth/2, carromBoard->boardSurface.length - 220  ,.5);				
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(-(carromBoard->boardSurface.length/2 - carromBoard->boardBoundary.breadth), 0.0, carromBoard->boardSurface.height/2 + 0.005);
	createBoardVisualStrikerBox(carromBoard->boardBoundary.breadth/2, carromBoard->boardSurface.length - 220  ,.5);				
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0.0, (carromBoard->boardSurface.length/2 - carromBoard->boardBoundary.breadth), carromBoard->boardSurface.height/2 + 0.005);
	createBoardVisualStrikerBox( carromBoard->boardSurface.length -220  ,carromBoard->boardBoundary.breadth/2,.5);				
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0.0, -(carromBoard->boardSurface.length/2 - carromBoard->boardBoundary.breadth), carromBoard->boardSurface.height/2 + 0.005);
	createBoardVisualStrikerBox( carromBoard->boardSurface.length - 220  ,carromBoard->boardBoundary.breadth/2,.5);				
	glPopMatrix();
	//creating central rings
	glPushMatrix();
	glTranslatef(0,0, carromBoard->boardSurface.height/2 + 0.005);				
	glScalef(carromBoard->boardPockets[0].radius*4, carromBoard->boardPockets[0].radius*4, 1.0);
	createOuterCircle();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0,0, carromBoard->boardSurface.height/2 + 0.005);				
	glScalef(carromBoard->boardPockets[0].radius*.5, carromBoard->boardPockets[0].radius*.5, 1.0);
	createPocket();
	glPopMatrix();

	/*glTranslatef(0,0, carromBoard->boardSurface.height/2 + 0.005);				
	glScalef(carromBoard->boardPockets[0].radius/2, carromBoard->boardPockets[0].radius/2, 1.0);
	createPocket();
	glPopMatrix();*/
	//glTranslatef(0,0, carromBoard->boardSurface.height/2 + 0.005);				
	////glScalef(carromBoard->boardPockets[0].radius/2, carromBoard->boardPockets[0].radius/2, 1.0);
	//createOuterCircle();
	//glPopMatrix();

	glPopMatrix();



}


/*!
* \brief
* Function call that displays the board as well as the carromMens
* 
* Function first creates and scales the board and then creates the carromMen and also handle the striker direction movement
* 
* \remarks
* Dont touch anything
* 
*/
void boardDisplay()
{/*	glEnable (GL_POINT_SMOOTH) ;
 glEnable (GL_LINE_SMOOTH) ;
 glEnable (GL_POLYGON_SMOOTH) ;*/



	glPushMatrix();
	//TODO Dice the board 

	CarromBoard * carrom;
	carrom = (CarromBoard *)objectVector[0];
	//making the board surface
	glPushMatrix();
	//glTranslatef(3.7,3.7,0);
	glScalef(.01,.01,.01);

	createBoard1(carrom);
	glPopMatrix();
	//making a CarromMens
	for(unsigned int i=1; i<objectVector.size(); i++)
	{	
		CarromMen *carromMen = (CarromMen *) objectVector[i];
		if (!carromMen->visible)
			continue;


		//drawing the carromMens
		glPushMatrix();//material
		//setting colours for various carrom men
		glShadeModel (GL_SMOOTH);
		if(carromMen->objectType == OBJECT_CARROM_MEN_BLACK)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_black);
		}
		if(carromMen->objectType == OBJECT_CARROM_MEN_RED)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_red);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_red);
		}
		if(carromMen->objectType == OBJECT_CARROM_MEN_WHITE)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_white);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_white);
		}
		if(carromMen->objectType == OBJECT_STRIKER)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_striker);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_striker);
		}

		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

		glPushMatrix();//carromMen
		//moving to the position of drawing

		glTranslatef(carromMen->position.x /100, carromMen->position.y /100, carrom->boardSurface.height /200);
		//glRotatef(270.0, 1.0, 0.0, 0.0);

		//scaling the carromMen according to the dimensions
		glScalef(carromMen->radius /100, carromMen->radius /100, carromMen->height /100);

		//drawing CarromMens 1) draw upper and lower discs 2) draw cylender 
		glPushMatrix();//lower disc

		glCallList(startList+1);

		//making direction if the state 2 ie shooting state is active
		if(carromMen->objectType == OBJECT_STRIKER && state == 2)
		{   
			glRotatef(-90+angle,0,0,1);
			glPushMatrix();

			glTranslatef(-2, 0, 0);
			glRotatef(270,0,1,0);

			glCallList(startList+3);
			glPopMatrix();
		}

		glPushMatrix();//upper disc

		glTranslatef(0, 0, 1);
		glCallList(startList+1);

		glPopMatrix();//upper disc
		//	glColor3f(1.0, 1.0, 0.0);
		glCallList(startList);

		glPopMatrix();//lower disc		

		glPopMatrix();//carromMen

		glPopMatrix();//material
	}

	glPopMatrix ();//closing basic


}

/*!
* \brief
* Display function to be called from main which draws on the screen
* 
* 
* The function firstly defines the viewport and the viewing way according to the state of the game and then draws the board
* 
* \remarks
* States are handled through this function, can be used for split screen 
* 
*/
void display()
{
	switch(state)
	{
	case STATE_BOARD_PLACE_STRIKER : glViewport (0, 0, (GLsizei) w, (GLsizei) h);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		glOrtho(-(BOARD_BREADTH /2 + 4 * BOARD_BOUNDARY_BREADTH) /100, (BOARD_BREADTH /2 + 4 * BOARD_BOUNDARY_BREADTH) /100,-(BOARD_BREADTH /2 + 4 * BOARD_BOUNDARY_BREADTH) /100,(BOARD_BREADTH /2 + 4 * BOARD_BOUNDARY_BREADTH) /100, -1, 10);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glPushMatrix();
		////glScalef(10,10,0);
		//glColor3f(1,1,1);
		//glRasterPos3f(0, 0,1);
		//for (char *c="Press Enter Or Click to Start Game"; *c != '\0'; c++)
		//{
		//	//glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);

		//	glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
		//}
		//glPopMatrix();
		glPushMatrix();
		boardDisplay();
		glPopMatrix();
		glFlush();
		glutSwapBuffers();
		break;

	case STATE_BOARD_SIMULATION : 
		glViewport (0, 0, (GLsizei) w, (GLsizei) h);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		gluPerspective(65.0, (GLfloat) w/(GLfloat) h, 1.0, 500.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt (0.0, -2.0,8.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		boardDisplay();

		glFlush();
		glutSwapBuffers();
		break;

	case STATE_BOARD_STRIKER_DIRECTION :
		glViewport (0, 0, (GLsizei) w, (GLsizei) h);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		gluPerspective(65.0, (GLfloat) w/(GLfloat) h, 1.0, 500.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt (0.0, -2.0,6.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		boardDisplay();
		//gluLookAt (15.0, 0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		glFlush();
		glutSwapBuffers();
		break;

	case STATE_BOARD_STRIKER_POWER :

		glViewport (0, 0, (GLsizei) w, (GLsizei) h);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		gluPerspective(65.0, (GLfloat) w/(GLfloat) h, 1.0, 500.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt (0.0, -1.0,10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glPushMatrix();//code for power bar
		/**/
		if(strikerSpeed >= 5)
		{   //setting speed of the striker 
			strikerSpeed = 0;
			//playing sound again form starting 
			alSourcePlay(AudioSource[SOUND_POWER_BAR]);
		}
		else strikerSpeed += 0.04;

		glTranslatef(5.5,-5+(strikerSpeed),0);
		createBoardBoundary(.5,strikerSpeed*2,1);
		glPopMatrix();//end code for power bar

		boardDisplay();
		//gluLookAt (15.0, 0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		glFlush();
		glutSwapBuffers();
		break;
	}

}
