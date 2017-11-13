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

// mac-m1.h

#include <Carbon/Carbon.h>

// Exported globals
extern int			gTotalGames;				// Total number of supported games

// Externally accessed functions
int mac_msg_callback(void *, int message, char *txt, int iparm);
void mac_switch_game(int game);
void mac_switch_song(int which);