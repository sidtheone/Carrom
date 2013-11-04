/*!
 * Header file containg methods to handle sounds
 * 
 * Copyright (c) 200x by <your name/ organization here>
 */
#ifndef SOUND_H
#define SOUND_H
#include <al/al.h>
#include <al/alc.h>
#include <al/alut.h>
#include "variables.h"
#include <iostream>

using namespace std;

// Maximum data buffers 
#define NUM_BUFFERS 5

// Maximum sources 
#define NUM_SOURCES 5




// Buffers to hold sound data.
ALuint Buffer;

// Buffers hold sound data.
ALuint Buffers[NUM_BUFFERS];

// Sources are points of emitting sound.
ALuint AudioSource[NUM_SOURCES];


// Position of the AudioSource sound.
ALfloat AudioSourcePos[NUM_SOURCES][3] = { {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };

// Velocity of the AudioSource sound.
ALfloat AudioSourceVel[NUM_SOURCES][3]= { {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},{0.0, 0.0, 0.0} };


// Position of the Listener.
ALfloat ListenerPos[] = { 0.0, 0.0, 0.0 };

// Velocity of the Listener.
ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };

// Orientation of the Listener. (first 3 elements are "at", second 3 are "up")
// Also note that these should be units of '1'.
ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };

/*
* ALboolean LoadALData()
*
*	This function will loads sounds files from the disk using the Alut
*	utility and send the data into OpenAL as a buffer. 
*/
ALboolean LoadALData()
{
	// Variables to load into.

	ALenum format;
	ALsizei size;
	ALvoid* data;
	ALsizei freq;
	ALboolean loop = AL_FALSE;

	// Load wav data into a buffer.

	alGenBuffers(NUM_BUFFERS, Buffers);
	
	if(alGetError() != AL_NO_ERROR)
		return AL_FALSE;

	alutLoadWAVFile("carrom_carrommen_cd.wav", &format, &data, &size, &freq, &loop);
	alBufferData(Buffers[0], format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);
	alutLoadWAVFile("carrom_striker_wall.wav", &format, &data, &size, &freq, &loop);
	alBufferData(Buffers[1], format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);
	alutLoadWAVFile("carrom_carrommen_wall.wav", &format, &data, &size, &freq, &loop);
	alBufferData(Buffers[2], format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);
	alutLoadWAVFile("carrom_pot_sound.wav", &format, &data, &size, &freq, &loop);
	alBufferData(Buffers[3], format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);
	alutLoadWAVFile("carrom_power_bar.wav", &format, &data, &size, &freq, &loop);
	alBufferData(Buffers[4], format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);


	// Bind the buffer with the AudioSource.


	alGenSources(NUM_SOURCES, AudioSource);

	if(alGetError() != AL_NO_ERROR)
		return AL_FALSE;


	alSourcei (AudioSource[SOUND_CARROMMEN_COLLISION], AL_BUFFER,   Buffers [SOUND_CARROMMEN_COLLISION]  );
	alSourcef (AudioSource[SOUND_CARROMMEN_COLLISION], AL_PITCH,    1.0      );
	alSourcef (AudioSource[SOUND_CARROMMEN_COLLISION], AL_GAIN,     0.3    );
	alSourcefv(AudioSource[SOUND_CARROMMEN_COLLISION], AL_POSITION, AudioSourcePos[SOUND_CARROMMEN_COLLISION]);
	alSourcefv(AudioSource[SOUND_CARROMMEN_COLLISION], AL_VELOCITY, AudioSourceVel[SOUND_CARROMMEN_COLLISION]);
	alSourcei (AudioSource[SOUND_CARROMMEN_COLLISION], AL_LOOPING,  loop     );

	alSourcei (AudioSource[SOUND_STRIKER_WALL], AL_BUFFER,   Buffers[SOUND_STRIKER_WALL]   );
	alSourcef (AudioSource[SOUND_STRIKER_WALL], AL_PITCH,    1.0      );
	alSourcef (AudioSource[SOUND_STRIKER_WALL], AL_GAIN,     0.7    );
	alSourcefv(AudioSource[SOUND_STRIKER_WALL], AL_POSITION, AudioSourcePos[SOUND_STRIKER_WALL]);
	alSourcefv(AudioSource[SOUND_STRIKER_WALL], AL_VELOCITY, AudioSourceVel[SOUND_STRIKER_WALL]);
	alSourcei (AudioSource[SOUND_STRIKER_WALL], AL_LOOPING,  loop     );

	alSourcei (AudioSource[SOUND_CARROMMEN_WALL], AL_BUFFER,   Buffers[SOUND_CARROMMEN_WALL]   );
	alSourcef (AudioSource[SOUND_CARROMMEN_WALL], AL_PITCH,    1.0      );
	alSourcef (AudioSource[SOUND_CARROMMEN_WALL], AL_GAIN,     0.4   );
	alSourcefv(AudioSource[SOUND_CARROMMEN_WALL], AL_POSITION, AudioSourcePos[SOUND_CARROMMEN_WALL]);
	alSourcefv(AudioSource[SOUND_CARROMMEN_WALL], AL_VELOCITY, AudioSourceVel[SOUND_CARROMMEN_WALL]);
	alSourcei (AudioSource[SOUND_CARROMMEN_WALL], AL_LOOPING,  loop     );

	alSourcei (AudioSource[SOUND_POCKET], AL_BUFFER,   Buffers[SOUND_POCKET]   );
	alSourcef (AudioSource[SOUND_POCKET], AL_PITCH,    1.0      );
	alSourcef (AudioSource[SOUND_POCKET], AL_GAIN,     0.7    );
	alSourcefv(AudioSource[SOUND_POCKET], AL_POSITION, AudioSourcePos[SOUND_POCKET]);
	alSourcefv(AudioSource[SOUND_POCKET], AL_VELOCITY, AudioSourceVel[SOUND_POCKET]);
	alSourcei (AudioSource[SOUND_POCKET], AL_LOOPING,  loop     );

	alSourcei (AudioSource[SOUND_POWER_BAR], AL_BUFFER,   Buffers[SOUND_POWER_BAR]   );
	alSourcef (AudioSource[SOUND_POWER_BAR], AL_PITCH,    1.0      );
	alSourcef (AudioSource[SOUND_POWER_BAR], AL_GAIN,     0.7    );
	alSourcefv(AudioSource[SOUND_POWER_BAR], AL_POSITION, AudioSourcePos[SOUND_POWER_BAR]);
	alSourcefv(AudioSource[SOUND_POWER_BAR], AL_VELOCITY, AudioSourceVel[SOUND_POWER_BAR]);
	alSourcei (AudioSource[SOUND_POWER_BAR], AL_LOOPING,  AL_TRUE     );

	if(alGetError() == AL_NO_ERROR)
		return AL_TRUE;

	return AL_FALSE;
}



/*
* void SetListenerValues()
*
*	Sets OpenAL listner Data
*	
*/
void SetListenerValues()
{
	alListenerfv(AL_POSITION,    ListenerPos);
	alListenerfv(AL_VELOCITY,    ListenerVel);
	alListenerfv(AL_ORIENTATION, ListenerOri);
}


/*
* void KillALData()
*
*	dealocates the buffers and sources on exit 
*	
*/
void KillALData()
{
	alDeleteBuffers(NUM_BUFFERS, Buffers);
	alDeleteSources(NUM_SOURCES, AudioSource);
	alutExit();
}


#endif SOUND_H

