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

    //'4' key, simulate no_carrier_sync (reset states)
    case 52:
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'C' key, Reset Call History (Capital C)
    case 67:
      for (int i = 0; i < 10; i++)
        sprintf (super->m17d.callhistory[i], "%s", "");
      break;

    //'E' key, Toggle AES Encryption (only when not TX, and a key is loaded)
    case 69:
      if (super->m17e.str_encoder_tx == 0 && super->enc.aes_key_is_loaded)
      {
        if (super->enc.enc_type == 0) super->enc.enc_type = 2;
        else super->enc.enc_type = 0;
      }
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

    //'e' key, Toggle Scrambler Encryption (only when not TX, and a key is loaded)
    case 101:
      if (super->m17e.str_encoder_tx == 0 && super->enc.scrambler_key != 0)
      {
        if (super->enc.enc_type == 0) super->enc.enc_type = 1;
        else super->enc.enc_type = 0;
      }
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

    //'r' key, toggle RRC Input / Output Filtering
    case 114:
      if (super->opts.disable_rrc_filter == 0) super->opts.disable_rrc_filter = 1;
      else super->opts.disable_rrc_filter = 0;
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'x' key, toggle inversion
    case 120:
      if (super->opts.inverted_signal == 0) super->opts.inverted_signal = 1;
      else super->opts.inverted_signal = 0;
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'8' key, toggle high pass filter on CODEC2 Output
    case 56:
      if (super->opts.use_hpfilter_dig == 0) super->opts.use_hpfilter_dig = 1;
      else super->opts.use_hpfilter_dig = 0;
      break;

    //'v' key, toggle vox
    case 118:
      if (super->m17e.str_encoder_vox == 0) super->m17e.str_encoder_vox = 1;
      else super->m17e.str_encoder_vox = 0;
      break;


    //Manual Gain Controls
    //'/' key, decrement voice input gain by 1%
    case 47:
      if (super->opts.input_gain_vx > 0.01f)
        super->opts.input_gain_vx -= 0.01f;
      break;

    //'*' key, increment voice input gain by 1%
    case 42:
      if (super->opts.input_gain_vx < 1.99f)
        super->opts.input_gain_vx += 0.01f;
      break;

    //'-' key, decrement voice output gain by 1%
    case 45:
      if (super->opts.output_gain_vx > 0.01f)
        super->opts.output_gain_vx -= 0.01f;
      break;

    //'+' key, increment voice output gain by 1%
    case 43:
      if (super->opts.output_gain_vx < 1.99f)
        super->opts.output_gain_vx += 0.01f;
      break;

    //'[' key, decrement rf input gain by 1%
    case 91:
      if (super->opts.input_gain_rf > 0.01f)
        super->opts.input_gain_rf -= 0.01f;
      break;

    //']' key, increment rf input gain by 1%
    case 93:
      if (super->opts.input_gain_rf < 1.99f)
        super->opts.input_gain_rf += 0.01f;
      break;

    //'{' key, decrement rf output gain by 1%
    case 123:
      if (super->opts.output_gain_rf > 0.01f)
        super->opts.output_gain_rf -= 0.01f;
      break;

    //'}' key, increment rf output gain by 1%
    case 125:
      if (super->opts.output_gain_rf < 1.99f)
        super->opts.output_gain_rf += 0.01f;
      break;

    //'<' key, decrement input squelch by 10
    case 60:
      if (super->demod.input_sql > 10)
        super->demod.input_sql -= 10;
      break;

    //'>' key, increment input squelch by 10
    case 62:
      // if (super->demod.input_sql < 1.99f)
        super->demod.input_sql += 10;
      break;
    
  }
}

#endif