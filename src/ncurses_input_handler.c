/*-------------------------------------------------------------------------------
 * ncurses_input_handler.c
 * M17 Project - Ncurses Keystroke Input Handling
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/


//have at least one thing in a file available if the entire thing is wrapped in an ifdef
//or -pendantic warning: ISO C forbids an empty translation unit if USE_CURSES not defined

#include "main.h"

#ifdef USE_CURSES

void input_ncurses_terminal (Super * super, int c)
{

  switch (c)
  {

    //'1' key, Generate 1 time use Scrambler Key (24-bit)
    case 49:
      if (super->opts.use_m17_str_encoder == 1 && !super->m17e.str_encoder_tx)
      {
        super->enc.scrambler_key = rand() & 0xFFFFFF;
        super->enc.enc_type = 1;
        pn_sequence_generator(super); //generate pN Sequence
        if (super->opts.internal_loopback_decoder)
        {
          super->m17d.enc_et = 1;
          super->m17d.enc_st = 2;
        }
      }
      break;

    //'2' key, key, Generate 1 time use AES Key (256-bit)
    case 50:
      if (super->opts.use_m17_str_encoder == 1 && !super->m17e.str_encoder_tx)
      {
        super->enc.A1 = ((uint64_t)rand() << 32ULL) + rand();
        super->enc.A2 = ((uint64_t)rand() << 32ULL) + rand();
        super->enc.A3 = ((uint64_t)rand() << 32ULL) + rand();
        super->enc.A4 = ((uint64_t)rand() << 32ULL) + rand();
        super->enc.enc_type = 2;
        aes_key_loader(super);
        if (super->opts.internal_loopback_decoder)
        {
          super->m17d.enc_et = 2;
          super->m17d.enc_st = 0;
        }
      }
      break;

    //'4' key, simulate no_carrier_sync (reset states)
    case 52:
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'A' key, Toggle Audio Level Display
    case 65:
      if (super->opts.ncurses_show_audio == 0) super->opts.ncurses_show_audio = 1;
      else super->opts.ncurses_show_audio = 0;
      break;

    //'C' key, show banner or compact mode (Capital C)
    case 67:
      if (super->opts.ncurses_show_banner == 0) super->opts.ncurses_show_banner = 1;
      else super->opts.ncurses_show_banner = 0;
      break;

    //'D' key, Toggle Encode / Decode Display
    case 68:
      if (super->opts.ncurses_show_decode == 0) super->opts.ncurses_show_decode = 1;
      else super->opts.ncurses_show_decode = 0;
      break;

    //'E' key, Toggle AES Encryption (only when not TX, and a key is loaded)
    case 69:
      if (super->m17e.str_encoder_tx == 0 && super->enc.aes_key_is_loaded)
      {
        if (super->enc.enc_type == 0) super->enc.enc_type = 2;
        else super->enc.enc_type = 0;

        if (super->enc.aes_key_is_loaded == 1)
        {
          super->enc.aes_key_is_loaded = 0;
          memset (super->enc.aes_key, 0, 64*sizeof(uint8_t));
        }

        sprintf (super->m17d.sms, "%s", "Encryption Key Cleared;");
        
        if (super->opts.internal_loopback_decoder)
        {
          super->m17d.enc_et = 0;
          super->m17d.enc_st = 0;
        }
      }
      break;

    //'H' key, show history
    case 72:
      if (super->opts.ncurses_show_history == 0) super->opts.ncurses_show_history = 1;
      else super->opts.ncurses_show_history = 0;
      break;

    //'I' key, Toggle IO Display
    case 73:
      if (super->opts.ncurses_show_io == 0) super->opts.ncurses_show_io = 1;
      else super->opts.ncurses_show_io = 0;
      break;

    //'S' key, Toggle Scope Display
    case 83:
      if (super->opts.ncurses_show_scope == 0) super->opts.ncurses_show_scope = 1;
      else super->opts.ncurses_show_scope = 0;
      break;

    //'Z' key, cycle demod verbosity
    case 90:
      if (super->opts.demod_verbosity <= 4) super->opts.demod_verbosity++;
      else super->opts.demod_verbosity = 0;
      break;

    //'\' key, toggle TX
    case 92:
      if (super->m17e.str_encoder_tx == 0) super->m17e.str_encoder_tx = 1;
      else super->m17e.str_encoder_tx = 0;

      if (super->m17e.str_encoder_tx == 0)
        super->m17e.str_encoder_eot = 1;
      break;

    //'c' key, Reset Call History (lower c)
    case 99:
      for (int i = 0; i < 10; i++)
        sprintf (super->m17d.callhistory[i], "%s", "");
      break;

    //'e' key, Toggle Scrambler Encryption (only when not TX, and a key is loaded)
    case 101:
      if (super->m17e.str_encoder_tx == 0 && super->enc.scrambler_key != 0)
      {
        if (super->enc.enc_type == 0) super->enc.enc_type = 1;
        else super->enc.enc_type = 0;

        if (super->enc.scrambler_key != 0)
          super->enc.scrambler_key = 0;

        sprintf (super->m17d.sms, "%s", "Encryption Key Cleared;");

        if (super->opts.internal_loopback_decoder)
        {
          super->m17d.enc_et = 0;
          super->m17d.enc_st = 0;
        }
      }
      break;

    //'h' key, toggle high pass filter on CODEC2 Output
    case 104:
      if (super->opts.use_hpfilter_dig == 0) super->opts.use_hpfilter_dig = 1;
      else super->opts.use_hpfilter_dig = 0;
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

    //'v' key, toggle vox
    case 118:
      if (super->m17e.str_encoder_vox == 0) super->m17e.str_encoder_vox = 1;
      else
      {
        super->m17e.str_encoder_vox = 0;
        if (super->m17e.str_encoder_tx == 1)
        {
          super->m17e.str_encoder_tx  = 0;
          super->m17e.str_encoder_eot = 1;
        }
      }
      break;

    //'x' key, toggle inversion
    case 120:
      if (super->opts.inverted_signal == 0) super->opts.inverted_signal = 1;
      else super->opts.inverted_signal = 0;
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'z' key, toggle payload verbosity
    case 122:
      if (super->opts.payload_verbosity == 0) super->opts.payload_verbosity = 1;
      else super->opts.payload_verbosity = 0;
      break;

    //Manual Gain Controls (keep seperate)
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