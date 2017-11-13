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

// mac-snd.h

#include "oss.h"

extern void						(*m1sdr_Callback)(unsigned long dwUser, short *samples);
extern int						nDSoundSegLen;

int m1sdr_GetPlayTime(void);
void m1sdr_GetPlayTimeStr(char *buf);
INT16 *m1sdr_GetPlayBufferPtr(void);
void m1sdr_InstallGenerationCallback(void);
void m1sdr_RemoveGenerationCallback(void);
Boolean m1sdr_CurrentlyPlaying(void);