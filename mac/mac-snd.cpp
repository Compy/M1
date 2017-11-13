//
// M1 by R. Belmont
//
// Macintosh version by Richard Bannister
// © 2004-2010, All Rights Reserved
//
// No part of this code may be reused in any other project without the
// express written permission of Richard Bannister.
//
// http://www.bannister.org/software/
//

// mac-snd.cpp

#include "m1ui.h"
#include "m1snd.h"
#include "wavelog.h"

#include "mac-m1.h"
#include "mac-snd.h"
#include "mac-window.h"

// Exported globals
void						(*m1sdr_Callback)(unsigned long dwUser, short *samples);
int							nDSoundSegLen = 0;

// Private defines
#define kMaxBuffers			120

// Private globals
static ExtSoundHeaderPtr	header = nil;
static SndCallBackUPP		callback = nil;
static SndCommand			playCmd, callbackCmd;
static SndChannelPtr		channel = nil;
static void					*buffer[kMaxBuffers], *playBuffer;
static volatile int			buffers_used = 0, buffers_read = 0, buffers_write = 0;
static volatile int			sdr_pause = 0;
static volatile Boolean		playing = false;
static int					playtime = 0;
static EventLoopTimerUPP	gPlayTimerUPP = nil;
static EventLoopTimerRef	gPlayTimerRef = nil;

// Private prototypes
static pascal void		m1sdr_PlayCallback(SndChannelPtr chan, SndCommand *cmd);
static pascal void		m1sdr_GenerationCallback(EventLoopTimerRef timer, void *snd);
static void				m1sdr_ClearBuffers(void);

//
// m1sdr_Init
// Initialisation function for sound driver. We're using old fashioned Carbon sound callbacks because they work!
//

INT16 m1sdr_Init(int sample_rate)
{
	if (header != nil)
	{
		m1sdr_Exit();
	}
	
	// Allocate header
	header = (ExtSoundHeaderPtr)NewPtrClear(sizeof(ExtSoundHeader));
	header->loopStart = 0;
	header->loopEnd = 0;
	header->encode = extSH;
	header->baseFrequency = kMiddleC;
	header->sampleRate = sample_rate << 16;
	header->sampleSize = 16;
	header->numChannels = 2;

	// Callback function
	callback = NewSndCallBackUPP(m1sdr_PlayCallback);
	if (!callback)
		return -1;
		
	// Make a channel
	SndNewChannel(&channel, sampledSynth, initStereo, callback);
	if (!channel)
		return -1;
		
	// Create our two commands
	memset(&playCmd, 0, sizeof(SndCommand));
	playCmd.cmd = bufferCmd;
	playCmd.param1 = 0;
	playCmd.param2 = (long)header;	
	memset(&callbackCmd, 0, sizeof(SndCommand));
	callbackCmd.cmd = callBackCmd;
	callbackCmd.param1 = 0;
	callbackCmd.param2 = 0;
	
	// Successful initialisation
	return 0;
}


//
// m1sdr_Exit
// Free up allocated memory
//

void m1sdr_Exit(void)
{	
	m1sdr_RemoveGenerationCallback();
	
	if (channel != nil)
	{
		SndDisposeChannel(channel, true);
		channel = nil;
	}

	if (callback != nil)
	{
		DisposeSndCallBackUPP(callback);
		callback = nil;
	}

	if (header != nil)
	{
		DisposePtr((Ptr)header);
		header = nil;
	}	
}


//
// m1sdr_PlayCallback
// Copies pregenerated sound buffers to output device
//

static pascal void m1sdr_PlayCallback(SndChannelPtr chan, SndCommand *cmd)
{
	if (buffers_used > 0)
	{
		memcpy(playBuffer, buffer[buffers_read], nDSoundSegLen * 2 * sizeof(UINT16));
		if (++buffers_read >= kMaxBuffers) buffers_read = 0;
		buffers_used--;
		playtime++;
	}
	else
	{
		memset(header->samplePtr, 0, header->numFrames * 2 * sizeof(UINT16));
	}
	
	SndDoCommand(chan, &playCmd, true);
	SndDoCommand(chan, &callbackCmd, true);
}


//
// m1sdr_GetPlayTime
// Returns time of current playback in seconds
//

int m1sdr_GetPlayTime(void)
{
	return (playtime / 60);
}


//
// m1sdr_GetPlayTimeStr
// Returns string of current playback time in minutes and seconds
//

void m1sdr_GetPlayTimeStr(char *buf)
{
	int seconds, minutes;
	
	seconds = m1sdr_GetPlayTime();
	minutes = (seconds / 60);
	seconds -= minutes * 60;
	
	sprintf(buf, "    %i:%.2i", minutes, seconds);
}


//
// m1sdr_Pause
// Temporarily halt playback
//

void m1sdr_Pause(int p)
{
	sdr_pause = p;
}


//
// m1sdr_GetPlayBufferPtr
// Returns a buffer to the current playback buffer. This may change at any time
// and is only recommended for oscilloscope generation or the like. Do not attempt
// to record the output of this function!
//

INT16 *m1sdr_GetPlayBufferPtr(void) 
{
	return (INT16 *)playBuffer;
}


//
// m1sdr_InstallGenerationCallback
// Install event loop timer for playback
//

void m1sdr_InstallGenerationCallback(void)
{
	if (gPlayTimerUPP || gPlayTimerRef)
		m1sdr_RemoveGenerationCallback();
		
	gPlayTimerUPP = NewEventLoopTimerUPP(m1sdr_GenerationCallback);
	InstallEventLoopTimer(GetMainEventLoop(), 0, kEventDurationSecond / kMaxBuffers, gPlayTimerUPP, nil, &gPlayTimerRef);
}


//
// m1sdr_RemoveGenerationCallback
// Remove the above
//

void m1sdr_RemoveGenerationCallback(void)
{
	if (gPlayTimerRef)
	{
		RemoveEventLoopTimer(gPlayTimerRef);
		gPlayTimerRef = nil;
	}
	if (gPlayTimerUPP)
	{
		DisposeEventLoopTimerUPP(gPlayTimerUPP);
		gPlayTimerUPP = nil;
	}
}


//
// m1sdr_GenerationCallback
// Routine which actually generates audio which our play code will get hold of later
//

static pascal void m1sdr_GenerationCallback(EventLoopTimerRef timer, void *snd)
{
	static unsigned long last_draw;
	unsigned long t;

	if (sdr_pause)
		return;
	
	// Render the oscilloscope approximately 30fps
	t = TickCount();
	if ((TickCount() - last_draw) >= 2)
	{
		last_draw = TickCount();
		window_draw_scope();
	}
	
	// Buffer some more audio
	while (playing && (buffers_used < (kMaxBuffers - 1)))
	{
		m1sdr_Callback(nDSoundSegLen, (short *)buffer[buffers_write]);
		waveLogFrame((unsigned char *)buffer[buffers_write], nDSoundSegLen << 2);
		
		if (++buffers_write >= kMaxBuffers) buffers_write = 0;
		buffers_used++;
		
		if ((TickCount() - t < 60) && (buffers_used < (kMaxBuffers >> 1)))
		{
			double firetime = 0;
			
			SetEventLoopTimerNextFireTime(timer, firetime);
			return;
		}
	}
}


//
// m1sdr_CurrentlyPlaying
// Returns a boolean indicating if we're operational or not
//

Boolean m1sdr_CurrentlyPlaying(void)
{
	return playing;
}


//
// m1sdr_ClearBuffers
// Reset the sound buffers to their empty state
//

static void m1sdr_ClearBuffers(void)
{
	int i;
	
	for (i = 0; i < kMaxBuffers; i++)
	{
		memset(buffer[i], 0, nDSoundSegLen * 2 * sizeof(UINT16));
	}
	
	memset(playBuffer, 0, nDSoundSegLen * 2 * sizeof(UINT16));
	
	buffers_used = buffers_read = buffers_write = 0;
}


//
// m1sdr_PlayStart
// Start the callback
//

void m1sdr_PlayStart(void)
{
	// Stop any existing sound
	m1sdr_PlayStop();
	
	// Clear buffers
	m1sdr_ClearBuffers();
	header->samplePtr = (char *)playBuffer;
	playtime = 0;

	// Fire off a callback
	SndDoCommand(channel, &callbackCmd, true);
	
	playing = true;
}


//
// m1sdr_PlayStart
// Stop the callback
//

void m1sdr_PlayStop(void)
{
	SndCommand cmd;
	
	// Stop playing
	playing = false;
	
	// Set up bits
	cmd.param1 = 0;
	cmd.param2 = 0;
	cmd.cmd = quietCmd;
	SndDoImmediate(channel, &cmd);
	cmd.cmd = flushCmd;
	SndDoImmediate(channel, &cmd);
}


//
// m1sdr_FlushAudio
// Push all the sound currently in the buffer
//

void m1sdr_FlushAudio(void)
{
	m1sdr_PlayStart();
}


//
// m1sdr_SetSamplesPerTick
// Push all the sound currently in the buffer
//

void m1sdr_SetSamplesPerTick(UINT32 spf)
{
	int i;
	
	nDSoundSegLen = spf;

	for (i = 0; i < kMaxBuffers; i++)
	{
		if (buffer[i])
		{
			free(buffer[i]);
			buffer[i] = nil;
		}

		buffer[i] = malloc(nDSoundSegLen * 2 * sizeof(UINT16));
		if (!buffer[i])
		{
			m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to allocate memory for sound buffer.", 0);
			ExitToShell();
		}
		
		memset(buffer[i], 0, nDSoundSegLen * 2 * sizeof(UINT16));
	}	

	playBuffer = malloc(nDSoundSegLen * 2 * sizeof(UINT16));
	if (!playBuffer)
	{
		m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to allocate memory for playback buffer.", 0);
		ExitToShell();
	}
	
	memset(playBuffer, 0, nDSoundSegLen * 2 * sizeof(UINT16));

	header->samplePtr = (char *)playBuffer;
	header->numFrames = nDSoundSegLen;
}


//
// m1sdr_TimeCheck
// This function is not needed on the Mac
//

void m1sdr_TimeCheck(void)
{
}


//
// m1sdr_SetCallback
// Set the callback function
//

void m1sdr_SetCallback(void *fn)
{
	m1sdr_Callback = (void (*)(unsigned long, signed short *))fn;
}