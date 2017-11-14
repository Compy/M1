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

// mac-roms.cpp

#include "m1ui.h"
#include "m1snd.h"

#include "mac-m1.h"
#include "mac-roms.h"

// Exported globals
rompath		*path_roms = nil, *path_pars = nil;


// 
// mac_roms_directory_iterator
// Actually look for our ROMs
// 

static pascal Boolean mac_roms_directory_iterator(Boolean containerChanged, ItemCount currentLevel, const FSCatalogInfo *info, const FSRef *in_ref, const FSSpec *spec, const HFSUniStr255 *name, void *yourDataPtr)
{
	char		buf_c[2048];
	Boolean		b, is_folder;
	OSErr		err;
	int			i;
	CFStringRef	cfs;
	FSRef		ref;
    CFDataRef   dataRef;
	
	// Make local copy
	memcpy(&ref, in_ref, sizeof(FSRef));
    
	// If this is an alias, then resolve it
	is_folder = false;
	err = FSResolveAliasFile(&ref, true, &is_folder, &b);
		
	// If we're a folder, then iterate into that
	if ((err == noErr) && is_folder)
	{
		//FSIterateContainer(&ref, 0, NULL, false, true, mac_roms_directory_iterator, NULL);
		return false;
	}

	// Otherwise, look for our name. We have the name of it already
	cfs = CFStringCreateWithCharacters(kCFAllocatorDefault, name->unicode, name->length);
	assert(cfs);	
	b = CFStringGetCString(cfs, (char *)buf_c, 1024, kCFStringEncodingMacRoman);
	assert(b);
	CFRelease(cfs);

	// Reject names too short
	if (strlen(buf_c) < 5) return false;
	
	// If zip is present...
	if (!strcasecmp(&buf_c[strlen(buf_c)-4], ".zip"))
	{
		for (i = 0; i < gTotalGames; i++)
		{
			if (!strcasecmp(buf_c, path_roms[i].zipname))
			{
				path_roms[i].present = true;
				FSRefMakePath(&ref, (UInt8*)path_roms[i].path, PATH_MAX);
			}
			
			if (!strcasecmp(buf_c, path_pars[i].zipname))
			{
				path_pars[i].present = true;
				FSRefMakePath(&ref, (UInt8*)path_roms[i].path, PATH_MAX);
			}
		}
	}
	
	// Continue iteration until we run out
	return false;
}


// 
// mac_roms_build_paths
// Function to locate the compare the actual contents of the ROMs directory against
// what we're expecting to find there.
// 

void mac_roms_build_paths(void)
{
	Boolean					b, is_folder;
	OSErr					err;
	int						i;
	FSRef					ref;
	CFURLRef				app, parent, romsdir;
		
	// Allocate memory for ROM paths
	path_roms = (rompath *)malloc((gTotalGames + 1) * sizeof(rompath));
	path_pars = (rompath *)malloc((gTotalGames + 1) * sizeof(rompath));
	if ((!path_roms) || (!path_pars))
	{
		mac_msg_callback(NULL, M1_MSG_ERROR, (char *)"Unable to allocate memory for ROM path index.", 0);
		ExitToShell();
	}

	// Build data structure
	memset(path_roms, 0, (gTotalGames + 1) * sizeof(rompath));
	memset(path_pars, 0, (gTotalGames + 1) * sizeof(rompath));
	for (i = 0; i < gTotalGames; i++)
	{
		strcpy(path_roms[i].zipname, m1snd_get_info_str(M1_SINF_ROMNAME, i));
		strcat(path_roms[i].zipname, ".zip");
		strcpy(path_pars[i].zipname, m1snd_get_info_str(M1_SINF_PARENTNAME, i));
		if (path_pars[i].zipname[0] != 0)
		{
			strcat(path_pars[i].zipname, ".zip");
		}
	}
	
	// Need to make spec hold a path to "ROMs"
	app = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	assert(app);
	
	// Parent directory
	parent = CFURLCreateCopyDeletingLastPathComponent(NULL, app);
	assert(parent);
	
	// Now ROMs directory
	romsdir = CFURLCreateCopyAppendingPathComponent(NULL, parent, CFSTR("ROMs"), false);
	assert(romsdir);
	
	// Finally, make a FSSpec from it
	if (!CFURLGetFSRef(romsdir, &ref))
	{
		mac_msg_callback(NULL, M1_MSG_ERROR, (char *)"Unable to locate ROMs directory.", 0);
		ExitToShell();
	}
	
	// Release
	CFRelease(app);
	CFRelease(parent);
	CFRelease(romsdir);
		
	// Parse
	is_folder = false;
	err = FSResolveAliasFile(&ref, true, &is_folder, &b);
	if ((err == noErr) && (is_folder))
	{
        /*
		DialogRef	bozo;
		
		bozo = GetNewDialog(130, nil, (WindowPtr)-1L);
		if (bozo == 0L)
		{
			mac_msg_callback(NULL, M1_MSG_ERROR, (char *)"Missing a vital dialog resource.", 0);
			ExitToShell();
		}
		SetPortDialogPort(bozo);
		DrawDialog(bozo);
		QDFlushPortBuffer(GetWindowPort(GetDialogWindow(bozo)), NULL);
         */
        printf("Show dialog resource 130\n");
		
		// Iterate
		//FSIterateContainer(&ref, 0, NULL, false, true, mac_roms_directory_iterator, NULL);

		// Get rid of the dialog
        /*
		HideWindow(GetDialogWindow(bozo));
		DisposeDialog(bozo);
         */
        printf("Dispose dialog 130\n");
	}
	else
	{
		mac_msg_callback(NULL, M1_MSG_ERROR, (char *)"Unable to resolve ROMs alias.", 0);
		ExitToShell();
	}
}


//
// mac_roms_close
// Deallocate all memory used
//

void mac_roms_close(void)
{
	if (path_roms)
	{
		free(path_roms);
		path_roms = nil;
	}
	if (path_pars)
	{
		free(path_pars);
		path_pars = nil;
	}
}


//
// mac_roms_count_found
// Determine how many games we have loaded
//

int mac_roms_count_found(void)
{
	int found, i;
	
	// Quick count to see how many games are here
	found = 0;
	for (i = 0; i < gTotalGames; i++)
		if (path_roms[i].present || path_pars[i].present)
			found++;
	
	return found;
}
