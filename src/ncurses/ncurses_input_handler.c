/*-------------------------------------------------------------------------------
 * ncurses_input_handler.c
 * M17 Project - Ncurses String and Keystroke Input Handling
 *
 * LWVMOBILE
 * 2024-06 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/


//have at least one thing in a file available if the entire thing is wrapped in an ifdef
//or -pendantic warning: ISO C forbids an empty translation unit if USE_CURSES not defined

#include "main.h"

#ifdef USE_CURSES

//open a boxed window, display label, let user input string to be set to output_string value
void entry_string_ncurses_terminal (char * label, char * output_string)
{
  WINDOW * entry;
  int rows   = 6;
  int colums = 70;
  int xpos = 10;
  int ypos = 10;
  
  entry = newwin(rows, colums, ypos, xpos);
  box (entry, 0, 0);
  mvwprintw(entry, 2, 2, label); //" Enter Text Message:", etc
  mvwprintw(entry, 3, 3, " ");
  echo();
  refresh();
  //read input, including white spaces, up to line break
  //had to google search this voodoo scan
  wscanw(entry, "%[^\n]s", output_string); //or "%[^\n]%*c"
  noecho();
}

//keyboard shortcut key handler
void input_ncurses_terminal (Super * super, int c)
{

  char label[50];
  char inp_str[800];

  switch (c)
  {

    //escape character (from arrow keys, read buffered input and ignore)
    case '\033':
      getch(); //[
      getch(); //A,B,C,D, etc
      break;

    //'1' key, Generate Random Scrambler Key (24-bit)
    case 49:
      if (super->opts.use_m17_str_encoder == 1 && !super->m17e.str_encoder_tx)
      {
        super->enc.scrambler_key = rand() & 0xFFFFFF;
        super->enc.enc_type = 1;
        scrambler_key_init(super, 1);
        super->enc.scrambler_seed_e = super->enc.scrambler_key;
        if (super->opts.internal_loopback_decoder)
        {
          super->m17d.enc_et = 1;
          super->m17d.enc_st = 2;
        }
      }
      break;

    //'2' key, Generate Random AES Key (256-bit)
    case 50:
      if (super->opts.use_m17_str_encoder == 1 && !super->m17e.str_encoder_tx)
      {
        super->enc.A1 = ((uint64_t)rand() << 32ULL) + rand();
        super->enc.A2 = ((uint64_t)rand() << 32ULL) + rand();
        super->enc.A3 = ((uint64_t)rand() << 32ULL) + rand();
        super->enc.A4 = ((uint64_t)rand() << 32ULL) + rand();
        super->enc.enc_type = 2;
        super->enc.enc_subtype = 2; //256
        super->m17e.enc_st = 2;     //256
        aes_key_loader(super);
        if (super->opts.internal_loopback_decoder)
        {
          super->m17d.enc_et = 2;
          super->m17d.enc_st = 2;
        }
      }
      break;

    //'3' key, Generate Random Signature Keys
    case 51:
      ecdsa_generate_random_keys (super); //reset demod
      break;

    //'4' key, simulate no_carrier_sync (reset states)
    case 52:
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'5' key, Disable Signature
    case 53:
      super->m17e.ecdsa.keys_loaded = 0;
      super->m17d.ecdsa.keys_loaded = 0;
      break;

    //'7' key, Toggle Symbol Timing
    case 55:
      if (super->opts.disable_symbol_timing == 0) super->opts.disable_symbol_timing = 1;
      else super->opts.disable_symbol_timing = 0;
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'A' key, Toggle Audio Level Display
    case 65:
      if (super->opts.ncurses_show_audio == 0) super->opts.ncurses_show_audio = 1;
      else super->opts.ncurses_show_audio = 0;
      break;

    //'C' key, Toggle Banner (Capital C)
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

    //'H' key, Toggle Show Call History
    case 72:
      if (super->opts.ncurses_show_history == 0) super->opts.ncurses_show_history = 1;
      else super->opts.ncurses_show_history = 0;
      break;

    //'I' key, Toggle IO Display
    case 73:
      if (super->opts.ncurses_show_io == 0) super->opts.ncurses_show_io = 1;
      else super->opts.ncurses_show_io = 0;
      break;

    //'M' key, Toggle Analog / Raw Signal Monitor (when no sync)
    case 77:
      if (super->opts.use_raw_audio_monitor == 0) super->opts.use_raw_audio_monitor = 1;
      else super->opts.use_raw_audio_monitor = 0;
      break;

    //'O' key, Toggle OTA Key Delivery (aes and scrambler)
    case 79:
      if (super->opts.use_otakd == 0) super->opts.use_otakd = 1;
      else super->opts.use_otakd = 0;
      break;

    //'P' key, Toggle OTA Key Delivery (signatures)
    case 80:
      if (super->opts.use_otask == 0) super->opts.use_otask = 1;
      else super->opts.use_otask = 0;
      break;

    //'S' key, Toggle Scope Display
    case 83:
      if (super->opts.ncurses_show_scope == 0) super->opts.ncurses_show_scope = 1;
      else super->opts.ncurses_show_scope = 0;
      break;

    //'Z' key, Cycle Demodulator Verbosity
    case 90:
      if (super->opts.demod_verbosity <= 4) super->opts.demod_verbosity++;
      else super->opts.demod_verbosity = 0;
      break;

    //'\' key, Toggle TX
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

      sprintf (super->m17d.sms, "%s", "Call History Cleared;");
      event_log_writer (super, super->m17d.sms, 0xFC);
      break;

    //'e' key, Toggle Scrambler Encryption (only when not TX, and a key is loaded)
    case 101:
      if (super->m17e.str_encoder_tx == 0 && super->enc.scrambler_key != 0)
      {
        if (super->enc.enc_type == 0) super->enc.enc_type = 1;
        else
        {
          super->enc.enc_type = 0;
          super->enc.enc_subtype = 0; //may consider using a meta subtype value instead
        }

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

    //'h' key, Toggle High Pass Filter on CODEC2 Output
    case 104:
      if (super->opts.use_hpfilter_dig == 0) super->opts.use_hpfilter_dig = 1;
      else super->opts.use_hpfilter_dig = 0;
      break;

    //'m' key, Enter Meta Text Message
    case 109:
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->opts.use_m17_str_encoder == 1)
      {
        sprintf (inp_str, "%s", "");
        sprintf (label, " Enter Meta Text:"); //set label to be displayed in the entry box window
        entry_string_ncurses_terminal(label, inp_str);
        parse_meta_txt_string(super, inp_str);
        if (super->m17e.meta_data[2] == 0x20 && super->m17e.meta_data[3] == 0x20 && super->m17e.meta_data[4] == 0x20 && super->m17e.meta_data[5] == 0x20 && 
            super->m17e.meta_data[6] == 0x20 && super->m17e.meta_data[7] == 0x20 && super->m17e.meta_data[8] == 0x20 && super->m17e.meta_data[9] == 0x20 && 
            super->m17e.meta_data[10] == 0x20 && super->m17e.meta_data[11] == 0x20 && super->m17e.meta_data[12] == 0x20 && super->m17e.meta_data[13] == 0x20 && 
            super->m17e.meta_data[14] == 0x20) //if all white spaces loaded, then zero out entire thing
        {
          super->enc.enc_type = 0;
          super->enc.enc_subtype = 0;
          super->m17e.enc_et = 0;
          super->m17e.met_st = 0;
          memset  (super->m17e.meta_data, 0, sizeof(super->m17e.meta_data));
        }
        else if (super->m17e.meta_data[0]) //meta has actual text in it
        {
          uint8_t meta_data[16]; memset (meta_data, 0, sizeof(meta_data));
          meta_data[0] = 0x80; //Meta Text
          memcpy (meta_data+1, super->m17e.meta_data+1, 14);
          fprintf (stderr, "\n ");
          decode_pkt_contents (super, meta_data, 15); //decode META
        }
      }
      break;

    //'o' key, send one time OTAKD Packet RF, if not VOX or TX enabled
    case 111: //NOTE: Sending LSF for SID is not an issue, since this can't be sent over IP Frames from here
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->enc.enc_type != 0)
      {
        super->demod.in_sync = 1;
        encode_ota_key_delivery_pkt(super, 0, super->m17d.lsf, super->enc.enc_type, super->enc.enc_subtype);
        super->demod.in_sync = 0;
      }
        
      break;

    //'p' key, send one time OTASK Packet RF, if not VOX or TX enabled
    case 112:
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->m17d.ecdsa.keys_loaded == 1)
      {
        super->demod.in_sync = 1;
        encode_ota_key_delivery_pkt(super, 0, super->m17d.lsf, 3, 0);
        super->demod.in_sync = 0;
      }
        
      break;

    //'q' key, Quit
    case 113:
      exitflag = 1;
      break;

    //'r' key, Toggle RRC Input / Output Filtering
    case 114:
      if (super->opts.disable_rrc_filter == 0) super->opts.disable_rrc_filter = 1;
      else super->opts.disable_rrc_filter = 0;
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'t' key, Enter Text Message (will zero out any raw packet data)
    case 116:
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->opts.use_m17_str_encoder == 1)
      {
        sprintf (super->m17e.sms, "%s", "");
        sprintf (super->m17d.sms, "%s", "");
        memset  (super->m17e.raw, 0, sizeof(super->m17e.raw)); //zero out any raw packet data (two aren't mutually exclusive)
        sprintf (label, " Enter Text Message:"); //set label to be displayed in the entry box window
        entry_string_ncurses_terminal(label, super->m17e.sms);
        if (super->m17e.sms[0]) //only send if there is an SMS text message loaded, else do nothing
        {
          sprintf (super->m17d.sms, super->m17e.sms);
          super->demod.in_sync = 1;
          encode_pkt(super, 0);
          super->demod.in_sync = 0;
        }
      }
      break;

    //'u' key, Enter RAW Packet (will zero out loaded SMS Message)
    case 117:
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->opts.use_m17_str_encoder == 1)
      {
        sprintf (super->m17e.sms, "%s", ""); //zero out any loaded SMS message
        memset  (super->m17e.raw, 0, sizeof(super->m17e.raw)); //zero out any raw packet data
        sprintf (label, " Enter Raw Packet:"); //set label to be displayed in the entry box window
        sprintf (inp_str, "%s", "");    
        entry_string_ncurses_terminal(label, inp_str);
        uint16_t len = parse_raw_user_string(super, inp_str);
        if (super->m17e.raw[0]) //only send if there is a packet loaded, else do nothing
        {
          decode_pkt_contents(super, super->m17e.raw+1, len); //decode content locally for display
          super->demod.in_sync = 1;
          encode_pkt(super, 0);
          super->demod.in_sync = 0;
        }
      }
      break;

    //'v' key, Toggle Vox Mode
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

    //'w' key, Enter Arb Text Message
    case 119:
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->opts.use_m17_str_encoder == 1)
      {
        sprintf (super->m17e.arb, "%s", "");
        sprintf (super->m17d.arb, "%s", "");
        sprintf (label, " Enter Arbitrary Data Text:"); //set label to be displayed in the entry box window
        entry_string_ncurses_terminal(label, super->m17e.arb);
        if (super->m17e.arb[0]) //only send if there is an arb text message loaded, else signal 3200 voice
        {
          super->opts.m17_str_encoder_dt = 3;
          sprintf (super->m17d.arb, super->m17e.arb);
        }
        else super->opts.m17_str_encoder_dt = 2;
      }
      break;

    //'x' key, Toggle Inversion
    case 120:
      if (super->opts.inverted_signal == 0) super->opts.inverted_signal = 1;
      else super->opts.inverted_signal = 0;
      super->m17d.dt = 4; //fake for carrier reset
      no_carrier_sync (super); //reset demod
      break;

    //'z' key, Toggle Payload Verbosity
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