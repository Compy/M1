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

// mac-window.h

int			window_open(void);
void		window_draw_scope(void);
void		window_close(void);
void		window_switch_to_about(void);
void		window_set_game_name(char *what);
void		window_set_song_number(char *what);
void		window_set_song_title(char *what);
void		window_set_driver_name(char *what);
void		window_set_hardware_desc(char *what);
void		window_set_rom_name(char *what);
void		window_channel_browser_update(void);