//
// M1 by R. Belmont
//
// Macintosh version by Richard Bannister
// © 2005-2010, All Rights Reserved
//
// No part of this code may be reused in any other project without the
// express written permission of Richard Bannister.
//
// http://www.bannister.org/software/
//

// mac-m1.cpp

#include <CoreFoundation/CoreFoundation.h>
#include "m1ui.h"
#include "m1snd.h"
#include "wavelog.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "mac-m1.h"
#include "mac-menus.h"
#include "mac-roms.h"
#include "mac-snd.h"
#include "mac-window.h"

// Exported globals
int				gTotalGames;				// Total number of supported games

// Private defines
#define SUPPORTED_EVENTS	1
static EventTypeSpec	events_supported[SUPPORTED_EVENTS] =
{
	{ kEventClassCommand, kEventCommandProcess },
};

// Private globals
static int		gCurrentGame = -1;


//
// mac_switch_game
// Change to a different game (duh)
//

void mac_switch_game(int game)
{
	if (gCurrentGame != game)
	{
		char buffer[128];
		
		// Stop playback
		m1sdr_PlayStop();

		// Set current game variable
		gCurrentGame = game;

		// Update display info
		sprintf(buffer, "%s.zip", m1snd_get_info_str(M1_SINF_ROMNAME, gCurrentGame));
		//window_set_game_name(m1snd_get_info_str(M1_SINF_VISNAME, gCurrentGame));
		//window_set_song_title(nil);
		//window_set_song_number(nil);
		//window_set_driver_name(nil);
		//window_set_hardware_desc(nil);
		//window_set_rom_name(buffer);

		// Kick it off
		if (m1snd_run(M1_CMD_GAMEJMP, gCurrentGame) == 0)
		{
			waveLogStart();
		}
		else
		{
            // There was some old SysBeep code here
        }
	}
}


//
// mac_switch_song
// Guess what?
//

void mac_switch_song(int which)
{
	int	current;
	
	if (!m1sdr_CurrentlyPlaying())
	{
		return;
	}
	
	current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
	if (which == 1)
	{
		if (current < m1snd_get_info_int(M1_IINF_MAXSONG, 0))
		{
			current++;
		}
		else
		{
			current = m1snd_get_info_int(M1_IINF_MINSONG, 0);
		}
	}
	else if (which == -1)
	{
		if (current > m1snd_get_info_int(M1_IINF_MINSONG, 0))
		{
			current--;
		}
		else
		{
			current = m1snd_get_info_int(M1_IINF_MAXSONG, 0);
		}
	}
	
	// Stop, jump, and start
	m1sdr_PlayStop();
	m1snd_run(M1_CMD_SONGJMP, current);
	m1sdr_PlayStart();
}


//
// mac_msg_callback
// Handle messages coming to us from the core
//

int mac_msg_callback(void *unused, int message, char *txt, int iparm)
{	
	switch (message)
	{
		case M1_MSG_DRIVERNAME:
		{
			//window_set_driver_name(txt);
		}
		break;
		
		case M1_MSG_HARDWAREDESC:
		{
			//window_set_hardware_desc(txt);
		}
		break;
		
		case M1_MSG_ROMLOADERR:
		{
			//window_set_song_number((char *)"<unable to load roms>");
			//window_set_song_title((char *)"<unable to load roms>");
		}
		break;
		
		case M1_MSG_GETWAVPATH:
		{
			*txt = 0;
		}
		break;
				
		case M1_MSG_STARTINGSONG:
		{
			char buffer[512];
			
			gCurrentGame = m1snd_get_info_int(M1_IINF_CURGAME, 0);
			sprintf(buffer, "Song %i", m1snd_get_info_int(M1_IINF_CURSONG, 0));
			//window_set_song_number(buffer);
			//window_set_song_title(m1snd_get_info_str(M1_SINF_TRKNAME, (iparm<<16) | gCurrentGame));
		}
		break;
		
		case M1_MSG_MATCHPATH:
		{
			int i;
			
			for (i = 0; i < gTotalGames; i++)
			{
				if (!strcmp(txt, path_roms[i].zipname))
				{
					strcpy(txt, path_roms[i].path);
					m1snd_set_info_str(M1_SINF_SET_ROMPATH, txt, 0, 0, 0);
					return 1;
				}
				if (!strcmp(txt, path_pars[i].zipname))
				{
					strcpy(txt, path_pars[i].path);
					m1snd_set_info_str(M1_SINF_SET_ROMPATH, txt, 0, 0, 0);
					return 1;
				}
			}
			
			// Not found
			return 0;
		}
		break;

		case M1_MSG_BOOTFINISHED:
		{
			// Update the channel configuration stuff
			//window_channel_browser_update();
		}
		break;
		
		//default:
		case M1_MSG_ERROR:
		{
			short itemHit;
			Str255 pstr;
            printf("M1_MSG_ERROR: %s\n", txt);
		}
		break;
	}
	
	return 0;
}


// 
// change_to_app_directory
// Alters the current directory
//

int change_to_app_directory(void)
{
	CFURLRef		bundle_url, parent_url;
	static UInt8	path[PATH_MAX+1];
	
	// Create URL for resources dir
	bundle_url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	if (!bundle_url) return 0;

	// Parent URL
	parent_url = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, bundle_url);

	// Get path
	CFURLGetFileSystemRepresentation(parent_url, TRUE, path, PATH_MAX);
	
	// Release URL
	CFRelease(bundle_url);
	CFRelease(parent_url);

	// Switch to it
	if (chdir((char *)path) != 0)
	{
		return 0;
	}
	
	// Success
	return 1;
}


// 
// change_to_resources_directory
// Alters the current directory
//

static int change_to_resources_directory(void)
{
	CFURLRef		bundle_url;
	UInt8			path[PATH_MAX+1];
	
	// Create URL for resources dir
	bundle_url = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
	if (!bundle_url) return 0;

	// Get path
	CFURLGetFileSystemRepresentation(bundle_url, TRUE, path, PATH_MAX);
	
	// Release URL
	CFRelease(bundle_url);

	// Switch to it
	if (chdir((char *)path) != 0)
	{
		return 0;
	}
	
	// Success
	return 1;
}


// 
// mac_events_handler
// Dispatcher for application events. Menus only at the moment. The window code handles most things.
//

static pascal OSStatus mac_events_handler(EventHandlerCallRef unused1, EventRef event, void *unused2)
{
	OSStatus	result;
	UInt32		whatClass, whatHappened;

	// Default result is failure
	result = eventNotHandledErr;
	
	// Determine what type of event we've been given and bounce to the appropriate handler
	whatClass = GetEventClass(event);
	whatHappened = GetEventKind(event);
	switch (whatClass)
	{
		case kEventClassCommand:
		{
			switch (whatHappened)
			{
				case kEventCommandProcess:
				{
					result = menus_handle(event);
				}
				break;
			}
		}
		break;
	}
	
	// Kick result back
	return result;
}


//
// main
// Main entry point
//

int main(void)
{
	long							result;
	EventHandlerRef					dispatcher_ref;
		
	// Menus
	menus_open();

	// Check for compatible version of Mac OS X
    if (floor(kCFCoreFoundationVersionNumber) < kCFCoreFoundationVersionNumber10_9) {
        fprintf(stderr, "[CRIT] You must be on at least OSX 10.9 to use M1.\n");
        return 0;
    }

	// Set directory to resources directory
	// so we can find m1.xml
	if (!change_to_resources_directory())
	{
		printf("Unable to locate resources - fatal error\n");
		return 0;
	}
	
	// Kick off the core
	m1snd_init(NULL, mac_msg_callback);

	// Set up some settings
	m1snd_setoption(M1_OPT_LANGUAGE, M1_LANG_EN);
	m1snd_setoption(M1_OPT_WAVELOG, 0);
	m1snd_setoption(M1_OPT_RESETNORMALIZE, 0);

	// Now lets try to find some games
	gTotalGames = m1snd_get_info_int(M1_IINF_TOTALGAMES, 0);
	mac_roms_build_paths();

	// Open window. If it fails an error will have been shown already.
	//if (!window_open())
	//	return -1;
	
	// Tell the core we want it to actually generate some audio...
	m1sdr_InstallGenerationCallback();
	
	// Run the application event loop
	InstallApplicationEventHandler(NewEventHandlerUPP(mac_events_handler), SUPPORTED_EVENTS, events_supported, 0, &dispatcher_ref);
    // TODO: Since RunApplicationEventLoop is deprecated, we should probably just spin up a socket or some sort of web API here so that
    // any frontend can communicate with the emulator
	//RunApplicationEventLoop();

	// Stop the sound driver
	m1sdr_PlayStop();
	m1sdr_RemoveGenerationCallback();
	
	// Shut down the window
	//window_close();
	
	// Close units
	mac_roms_close();
	
	// Stop wave out
	waveLogStop();
	
	// Stop sound driver
	m1sdr_Exit();		
	
	// Done
	return 0;
}
