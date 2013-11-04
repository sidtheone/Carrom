/*!
* Defination of interface methods 
* 
* Copyright (c) 2010 by sidharth
*/


#include "variables.h"

#include "model.h"
#include "interface.h"
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>



int strikerSet = NOT_SET;

/*!
* \brief
* Method to process mouse action to move from one screen to another 
* 
* \param button
* Tells which button is pressed
* 
* \param stateMouse
* Tells the state of the mouse button .
* 
* \param x
*  coordinate x of screen 
* 
* \param y
* coordinate y of screen 
* 
* 
* Method to process mouse action to move from one screen to another 
* 
*/
void processMouse(int button, int stateMouse, int x, int y) 
{

	if(state == STATE_BOARD_SIMULATION)
	{
		if (stateMouse == GLUT_DOWN) {

			state = STATE_BOARD_SIMULATION;
			glutPostRedisplay();

		}
	}

	if(state == STATE_BOARD_PLACE_STRIKER)
	{
		if(button == GLUT_LEFT_BUTTON)
		{

			int i = cdObjectVector.size();
			CarromMen *carromMen = (CarromMen *)cdObjectVector[i-1];
			carromMen->visible = true;
			x=x-h/2;
			y=y-w/2;
			if(((x > -130)&&(x< 130))&&((y>134)&&(y<154)))
			{if(x< -123 )
			x=-123;
			if( x > 123)
				x=123;
			carromMen->position.Set(x * 370 /186.0, -145 * 370 /186.0, 0);
			ALenum soundState;
			//check if there is a sound playing currently

			alGetSourcei(AudioSource[SOUND_CARROMMEN_COLLISION], AL_SOURCE_STATE, &soundState);
			//setting up board sound for placing the striker
			if(soundState != AL_PLAYING)
			{   
				alSourcePlay(AudioSource[SOUND_STRIKER_WALL]);
			}
			strikerSet = SET;
			}
		}
		if(button == GLUT_RIGHT_BUTTON && strikerSet == 1 && stateMouse == GLUT_UP)
		{
			state = STATE_BOARD_STRIKER_DIRECTION;
			strikerSet = NOT_SET;

		}
		glutPostRedisplay();
	}



	if(state == STATE_BOARD_STRIKER_DIRECTION)
	{  
		if (stateMouse == GLUT_DOWN) 
		{
			state = STATE_BOARD_STRIKER_POWER;
			strikerSpeed = 0.0;
			glutPostRedisplay();
			alSourcePlay(AudioSource[SOUND_POWER_BAR]);

		}
		return;
	}

	if(state == STATE_BOARD_STRIKER_POWER)
	{ 
		if (stateMouse == GLUT_UP) 
		{//code for giving velocity to the carrom striker 
			int i = cdObjectVector.size();
			alSourceStop(AudioSource[SOUND_POWER_BAR]);
			CarromMen *carromMen = (CarromMen *)cdObjectVector[i-1];
			carromMen->velocity.Set(-sin(angle * 3.141 /180), cos(angle * 3.141 /180), 0);
			carromMen->velocity = carromMen->velocity * strikerSpeed;
			state = STATE_BOARD_SIMULATION;
			glutPostRedisplay();

		}

	}

}




/*!
* \brief
* Function to process the mouse movement and convert it to the angle of the striker
* 
* \param x
* location x of mouse
* 
* \param y
* location y of mouse
* 
* 
* Function to process the mouse movement and convert it to the angle of the striker
* 
* \remarks
* none
* 
*/
void processMouseActiveMotion(int x, int y) 
{

	if(state ==  2)
	{
		x = x - w /2;
		angle = -x/300.0 * 75;
		glutPostRedisplay();

	}


}

/*!
* \brief
* Method used to process keyboard functionalities 
* 
* \param key
* which key is pressed
* 
* \param x
* Description of parameter x.
* 
* \param y
* Description of parameter y.
* 
* 
* Method used to process keyboard functionalities
* 
* \remarks
* none
*/
void keyboard (unsigned char key, int x, int y)
{
	switch (key) 
	{
		//reinitialize the board if state is place striker and key pressed is r
	case 'r':
		if(state == STATE_BOARD_PLACE_STRIKER)
		{
			//glutPostWindowRedisplay(1);
			for(unsigned int i=0;i< objectVector.size();i++)
				delete objectVector[i];
			objectVector.clear();
			initCarromBoardModel();

			glutPostRedisplay();
		}
		break;
		//handling esc key
	case 27 :

		exit(0);
		break;



	default:
		break;
	}
}