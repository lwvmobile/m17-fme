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

    //'2' key, toggle RRC Input / Output Filtering
    case 50:
      if (super->opts.disable_rrc_filter == 0) super->opts.disable_rrc_filter = 1;
      else super->opts.disable_rrc_filter = 0;
      no_carrier_sync (super); //reset demod
      break;

    //'3' key, toggle inversion
    case 51:
      if (super->opts.inverted_signal == 0) super->opts.inverted_signal = 1;
      else super->opts.inverted_signal = 0;
      no_carrier_sync (super); //reset demod
      break;

    //'4' key, simulate no_carrier_sync (reset states)
    case 52:
      no_carrier_sync (super); //reset demod
      break;

    //'\' key, toggle TX
    case 92:
      if (super->m17e.str_encoder_tx == 0) super->m17e.str_encoder_tx = 1;
      else super->m17e.str_encoder_tx = 0;

      if (super->m17e.str_encoder_tx == 0)
        super->m17e.str_encoder_eot = 1;
      break;

    //'c' key, no banner / compact mode
    case 99:
      if (super->opts.ncurses_no_banner == 0) super->opts.ncurses_no_banner = 1;
      else super->opts.ncurses_no_banner = 0;
      break;

    //'h' key, no history
    case 104:
      if (super->opts.ncurses_no_history == 0) super->opts.ncurses_no_history = 1;
      else super->opts.ncurses_no_history = 0;
      break;

    //q key, quit
    case 113:
      exitflag = 1;
      break;
  }
}

#endif