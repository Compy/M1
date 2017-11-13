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

// mac-menus.cpp

#include "m1ui.h"
#include "m1snd.h"

#include "mac-m1.h"
#include "mac-menus.h"
#include "mac-window.h"

// Private defines
#define kMenuApple				128
#define kMenuItemAppleAbout		1


//
// menus_open
// Install menubar
//

void menus_open(void)
{
	Handle	menus;
	
	// Menus
	menus = GetNewMBar(128);
	assert(menus != nil);
	SetMenuBar(menus);
	DisposeHandle(menus);
	DrawMenuBar();
	
	SetMenuItemCommandID(GetMenuRef(kMenuApple), kMenuItemAppleAbout, kHICommandAbout);
}


//
// menus_handle
// Deal with any menu selections
//

OSStatus menus_handle(EventRef event)
{
	HICommand	command;
	OSStatus	result;
	
	// Default result is failure
	result = eventNotHandledErr;
	
	// Figure out which command has been hit	
	GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);

	switch (command.commandID)
	{
		case kHICommandAbout:
			window_switch_to_about();
			result = noErr;
			break;
	}

	return result;
}