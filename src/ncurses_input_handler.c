/*-------------------------------------------------------------------------------
 * ncurses_input_handler.c
 * Project M17 - Ncurses Keystroke Input Handling
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#ifdef USE_CURSES

#include "main.h"

void input_ncurses_terminal (Super * super, int c)
{

  switch (c)
  {

    //'c' key, no banner / compact mode
    case 99:
      if (super->opts.ncurses_no_banner == 1) super->opts.ncurses_no_banner = 0;
      else super->opts.ncurses_no_banner = 0;
      break;

    //'h' key, no history
    case 104:
      if (super->opts.ncurses_no_history == 1) super->opts.ncurses_no_history = 0;
      else super->opts.ncurses_no_history = 0;
      break;

    //q key, quit
    case 113:
      exitflag = 1;
      break;
  }
}

#endif