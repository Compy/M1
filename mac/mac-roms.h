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

// mac-roms.h

typedef struct
{
	char		zipname[16];
	Boolean		present;
	char		path[PATH_MAX];
} rompath;

extern rompath		*path_roms, *path_pars;

void mac_roms_build_paths(void);
void mac_roms_close(void);
int mac_roms_count_found(void);