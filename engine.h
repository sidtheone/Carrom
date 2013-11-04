/*!
* 2-D carrom engine
* 
* Copyright (c) 2010 by sidharth
*/
#ifndef ENGINE_H
#define ENGINE_H
#include "variables.h"
#include "sound.h"




#define STEPS 20
#define U_FRICTION 0.005

/*!
* \brief
* Carrom Engine Based on Spring Hooks law
*  
* Carrom Engine that works on the principle of spirngs when collisions happen
*/
void carromEngine( ){


	if(state != STATE_BOARD_SIMULATION)
		return;
	int wallCollisionFinal = 0;

	float k = .9;
	CarromBoard *board = (CarromBoard *)objectVector[0];
	for (int subStep = 0; subStep < STEPS; ++subStep)
	{
		int stateCheckFLag = 0;
		int wallCollision = 0;
		int carromMenCollision = 0;

		for (unsigned int i = 0; i < cdObjectVector.size(); i++)
		{
			CarromMen *cdObject1 = (CarromMen *)cdObjectVector[i];

			if(!cdObject1->visible)
				continue;

			//code for checking pockets intersection and settings sounds
			if((cdObject1->position - board->boardPockets[0].position).Magnitude() - 4<= (board->boardPockets[0].radius - cdObject1->radius))
			{	
				alSourcePlay(AudioSource[SOUND_POCKET]);
				cdObject1->visible = false;
				continue;
			}
			if((cdObject1->position - board->boardPockets[1].position).Magnitude() - 4<= (board->boardPockets[0].radius - cdObject1->radius))
			{	
				alSourcePlay(AudioSource[SOUND_POCKET]);
				cdObject1->visible = false;
				continue;
			}
			if((cdObject1->position - board->boardPockets[2].position).Magnitude() - 4<= (board->boardPockets[0].radius - cdObject1->radius))

			{
				alSourcePlay(AudioSource[SOUND_POCKET]);
				cdObject1->visible = false;
				continue;
			}
			if((cdObject1->position - board->boardPockets[3].position).Magnitude() - 4<= (board->boardPockets[0].radius - cdObject1->radius))
			{
				alSourcePlay(AudioSource[SOUND_POCKET]);
				cdObject1->visible = false;
				continue;
			}

			// collision with other carrom mens
			for (unsigned int j = i + 1; j < cdObjectVector.size(); ++j)
			{ 

				CarromMen *cdObject2 = (CarromMen *)cdObjectVector[j];
				if(	!cdObject2->visible)
					continue;

				float distance = (cdObject1->position -cdObject2->position).Magnitude();
				if (distance < (cdObject1->radius + cdObject2->radius))
				{
					if(!cdObject1->colliding || !cdObject2->colliding)
					{//play sound if any of the 2 objects are not colliding
						cdObject1->colliding = 1;
						cdObject2->colliding = 1;

						ALenum state;
						//check if there is a sound playing currently

						alGetSourcei(AudioSource[SOUND_CARROMMEN_COLLISION], AL_SOURCE_STATE, &state);
						if(state != AL_PLAYING)
						{   
							alSourcef (AudioSource[1], AL_GAIN,     0.5/(cdObject1->velocity-cdObject2->velocity).Magnitude());
							alSourcePlay(AudioSource[SOUND_CARROMMEN_COLLISION]);
						}

					}
					//float distance = sqrt(distanceSquared);
					Vector3D directionitoj = cdObject2->position - cdObject1->position;

					float directionitojLength = directionitoj.Magnitude();
					if (directionitojLength != 0)
					{
						directionitoj = directionitoj / directionitojLength;
						// Find the magnitude of the force aka x in F = -kx of hooke's law
						// displacement = currentLength - restLength
						float displacement = directionitojLength - (cdObject1->radius + cdObject2->radius);
						cdObject1->force += directionitoj * k *displacement;
						cdObject2->force -= directionitoj * k *displacement;

					}
				}
			}


		}
		for (unsigned int i = 0; i < cdObjectVector.size(); ++i)
		{
			CarromMen *cdObject1 = (CarromMen *)cdObjectVector[i];

			if(!cdObject1->visible)
				continue;

			// Wall collisions
			if (cdObject1->position.x < (- BOARD_BREADTH /2 + cdObject1->radius) )
			{ 
				cdObject1->force.x += k * ((- BOARD_BREADTH /2 + cdObject1->radius) - cdObject1->position.x); 
				wallCollision=1;
			}
			if (cdObject1->position.x > BOARD_BREADTH /2 - cdObject1->radius) 
			{ 
				cdObject1->force.x += k * ((BOARD_BREADTH /2 - cdObject1->radius) - cdObject1->position.x); 
				wallCollision=1;
			}
			if (cdObject1->position.y < -BOARD_BREADTH /2 + cdObject1->radius) 
			{
				cdObject1->force.y += k * ((-BOARD_BREADTH /2 + cdObject1->radius) - cdObject1->position.y); 
				wallCollision=1;
			}
			if (cdObject1->position.y > BOARD_BREADTH /2 - cdObject1->radius) 
			{ 
				cdObject1->force.y += k * ((BOARD_BREADTH /2 - cdObject1->radius) - cdObject1->position.y); 
				wallCollision=1;
			}
			if(wallCollision == 1)
			{
				if(cdObject1->objectType == OBJECT_STRIKER)
				{
					ALenum state;
					alSourcef (AudioSource[1], AL_GAIN,     0.5/(cdObject1->velocity.Magnitude()));
					alGetSourcei(AudioSource[SOUND_STRIKER_WALL], AL_SOURCE_STATE, &state);
					if(state != AL_PLAYING)
						alSourcePlay(AudioSource[SOUND_STRIKER_WALL]);

				}
				else
				{
					ALenum state;

					alGetSourcei(AudioSource[SOUND_CARROMMEN_WALL], AL_SOURCE_STATE, &state);
					if(state != AL_PLAYING)
						alSourcePlay(AudioSource[SOUND_CARROMMEN_WALL]);
				}
				wallCollision = 0;
			}

			//calculating friction
			if(cdObject1->velocity.Magnitude()!=0)
			{
				cdObject1->force -=  cdObject1->velocity * cdObject1->mass * (U_FRICTION /cdObject1->velocity.Magnitude()); 			
			}

			cdObject1->velocity += (cdObject1->force / cdObject1->mass)/STEPS;
			if(cdObject1->velocity.Magnitude() > 0.0005 )
			{
				stateCheckFLag++;
			}


			cdObject1->position += cdObject1->velocity *(0.99 / STEPS);
			cdObject1->force.SetZero();

		}

		if(stateCheckFLag == 0)
			//setting state to change when the objects stop moving
			state = STATE_BOARD_PLACE_STRIKER;

		//setting collision of all the objects as null for next processing of the frame
		for (unsigned int i = 0; i < cdObjectVector.size(); ++i)
		{
			CarromMen *cdObject1 = (CarromMen *)cdObjectVector[i];
			cdObject1->colliding=false;
		}
	}


}
#endif

