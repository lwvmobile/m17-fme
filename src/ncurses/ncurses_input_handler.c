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
  int xpos = 5;
  int ypos = 5;
  
  entry = newwin(rows, colums, ypos, xpos);
  box (entry, 0, 0);
  mvwprintw(entry, 2, 2, "%s", label); //" Enter Text Message:", etc
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
  char inp_str[825];
  int16_t can_bkp = 0;
  uint8_t lockout = 0;
  int lo_index = 0;

  switch (c)
  {

    //escape character (from arrow keys, read buffered input and ignore)
    case '\033':
      getch(); //[
      getch(); //A,B,C,D, etc
      break;

    //'0' key, Quick Stream Burst
    case 48:
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        if (super->m17e.str_encoder_tx == 0)
        {
          super->m17e.str_encoder_tx = 1;
          super->m17e.str_encoder_bst = 1;
        }
      }
      break;

    //'1' key, Generate Random Scrambler Key (24-bit)
    case 49:
      if ( (super->opts.use_m17_str_encoder == 1 || super->opts.use_m17_duplex_mode == 1) && !super->m17e.str_encoder_tx && super->opts.use_m17_reflector_mode == 0)
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
      if ( (super->opts.use_m17_str_encoder == 1 || super->opts.use_m17_duplex_mode == 1) && !super->m17e.str_encoder_tx && super->opts.use_m17_reflector_mode == 0)
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
      if (super->opts.use_m17_str_encoder == 1 && !super->m17e.str_encoder_tx && super->opts.use_m17_reflector_mode == 0)
        ecdsa_generate_random_keys (super);
      break;

    // //'4' key, simulate no_carrier_sync (reset states)
    // case 52:
    //   super->m17d.dt = 4; //fake for carrier reset
    //   no_carrier_sync (super); //reset demod
    //   break;

    //'4' key, Scroll Call History Up
    case 52:
      if (super->m17d.scroll_index > 0)
        super->m17d.scroll_index--;
      break;

    //'5' key, Disable Signature
    case 53:
      super->m17e.ecdsa.keys_loaded = 0;
      super->m17d.ecdsa.keys_loaded = 0;
      break;

    //'6' key, Scroll Call History Down
    case 54:
      if (super->m17d.scroll_index < 245)
        super->m17d.scroll_index++;  
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

    //'B' key, Toggle Packet Burst
    case 66:
      if (super->opts.use_m17_reflector_mode == 0) //disabled for now on reflector
      {
        if (super->opts.use_m17_packet_burst == 0) super->opts.use_m17_packet_burst = 1;
        else super->opts.use_m17_packet_burst = 0;
      }
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
        else
        {
          super->enc.enc_type = 0;
          super->enc.enc_subtype = 0; //may consider using a meta subtype value instead
        }

        if (super->enc.aes_key_is_loaded == 1)
        {
          super->enc.aes_key_is_loaded = 0;
          memset (super->enc.aes_key, 0, 32*sizeof(uint8_t));
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

    // //'L' key, Print All Call History to Console
    // case 76:
    //   print_call_history(super);
    //   break;

    //'L' key, Lockout Current SRC CSD String
    case 76:
      for (lo_index = 0; lo_index < super->m17d.lockout_index; lo_index++)
      {
        if (strncmp(super->m17d.src_csd_lockout[lo_index], "         ", 9) != 0)
        {
          if (strncmp(super->m17d.src_csd_str, super->m17d.src_csd_lockout[lo_index], 9) == 0)
          {
            lockout = 1;
            break;
          }
        }
      }
      //if the SRC hasn't been locked out already, THEY JUST MADE THE LIST!
      if (lockout == 0)
      {
        sprintf(super->m17d.src_csd_lockout[super->m17d.lockout_index++], "%s", super->m17d.src_csd_str);

        //debug
        // fprintf (stderr, "\n SRC: %s; Has been Locked Out by User; Index: %d; ", super->m17d.src_csd_lockout[super->m17d.lockout_index-1], super->m17d.lockout_index-1);
      }
        
      //if they SRC has already been locked out, remove them from the lockout list
      else if (lockout == 1)
      {
        //debug
        // fprintf (stderr, "\n SRC: %s; Has been Unlocked by User; Index: %d; ", super->m17d.src_csd_lockout[lo_index], lo_index);

        sprintf(super->m17d.src_csd_lockout[lo_index], "%s", "         ");
      }
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
      if (super->opts.use_m17_duplex_mode == 0)
      {
        if (super->opts.use_otask == 0) super->opts.use_otask = 1;
        else super->opts.use_otask = 0;
      }
      break;

    //'S' key, Toggle Scope Display
    // case 83:
    //   if (super->opts.ncurses_show_scope == 0) super->opts.ncurses_show_scope = 1;
    //   else super->opts.ncurses_show_scope = 0;
    //   break;

    //'V' key, Mute All Voice Audio
    case 86:
      if (super->opts.playback_voice_mute == 0) super->opts.playback_voice_mute = 1;
      else super->opts.playback_voice_mute = 0;
      break;

    //'Z' key, Cycle Demodulator Verbosity
    case 90:
      if (super->opts.demod_verbosity <= 4) super->opts.demod_verbosity++;
      else super->opts.demod_verbosity = 0;
      break;

    //'\' key, Toggle TX
    case 92:
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        if (super->m17e.str_encoder_tx == 0) super->m17e.str_encoder_tx = 1;
        else super->m17e.str_encoder_tx = 0;

        if (super->m17e.str_encoder_tx == 0)
          super->m17e.str_encoder_eot = 1;
      }
      break;

    //'a' key, toggle auto or manual gain
    case 97:
      if (super->opts.auto_gain_voice == 0)
      {
        super->opts.auto_gain_voice = 1;
        super->opts.output_gain_vx = 1.0f;
      }
      else
      {
        super->opts.auto_gain_voice = 0;
        super->opts.output_gain_vx = 1.0f;
      }
      reset_auto_gain_vx(super);
      break;

    //'b' key, set new CAN value, if using TX and RX or stream encoder
    case 98:
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        can_bkp = super->m17e.can;

        //Scan in CAN Value
        char canstr[50]; memset(canstr, 0, 50*sizeof(char));

        sprintf (label, " Enter Channel Access Number (0-15):"); //set label to be displayed in the entry box window
        entry_string_ncurses_terminal(label, canstr);

        //convert canstr to numerical value
        sscanf (canstr, "%hd", &super->m17e.can);

        if (super->m17e.can > 15) super->m17e.can = can_bkp;

        //debug dump new can to stderr
        // fprintf (stderr, "\n New CAN: %02i", super->m17e.can);
      }
      break;

    //'c' key, Reset Call History (lower c)
    case 99:
      for (int i = 0; i < 255; i++)
        sprintf (super->m17d.callhistory[i], "%s", "");

      sprintf (super->m17d.sms, "%s", "Call History Cleared;");
      event_log_writer (super, super->m17d.sms, 0xFC);
      break;

    //'d' key, Enter Destination Address Value, if using TX and RX or stream encoder
    case 100:
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && (super->opts.use_m17_str_encoder == 1 || super->opts.use_m17_duplex_mode == 1) )
        {
          //backup current string found here
          char tempsrc[50]; memset(tempsrc, 0, 50*sizeof(char));
          sprintf (tempsrc, "%s", super->m17e.dsts);
          sprintf (super->m17e.dsts, "%s", "");

          sprintf (label, " Enter Destination Callsign:"); //set label to be displayed in the entry box window
          entry_string_ncurses_terminal(label, super->m17e.dsts);

          if (super->m17e.dsts[0] != '#')
          {

            //check and capatalize any letters in the CSD
            for (int i = 0; super->m17e.dsts[i]!='\0'; i++)
            {
              if(super->m17e.dsts[i] >= 'a' && super->m17e.dsts[i] <= 'z')
                super->m17e.dsts[i] -= 32;
            }

            //terminate string
            super->m17e.dsts[9] = '\0';

          }

          //if no entry provided, revert to backup
          if (super->m17e.dsts[0] == 0)
          {
            sprintf (super->m17e.dsts, "%s", tempsrc);
          }
        }
      }
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
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && (super->opts.use_m17_str_encoder == 1 || super->opts.use_m17_duplex_mode == 1))
        {
          //zero out all meta text round robin slots
          for (int i = 2; i <= 16; i++)
            memset(super->m17e.lsf3.meta_rr[i], 0, sizeof(super->m17e.lsf3.meta_rr[i]));

          sprintf (inp_str, "%s", "");
          sprintf (label, " Enter Meta Text:"); //set label to be displayed in the entry box window
          entry_string_ncurses_terminal(label, inp_str);
          parse_meta_txt_string(super, inp_str);

          if (super->m17e.lsf3.meta_rr[2][0] == 0) //Empty String loaded
          {
            //zero out all meta text round robin slots
            for (int i = 2; i <= 16; i++)
              memset(super->m17e.lsf3.meta_rr[i], 0, sizeof(super->m17e.lsf3.meta_rr[i]));

            memset  (super->m17e.meta_data, 0, sizeof(super->m17e.meta_data));
            sprintf (super->m17d.dat, "%s", "");

          }
        }
      }
      break;

    //'o' key, send one time OTAKD Packet
    case 111: //NOTE: Sending LSF for SID, not really random, but eh, M17 Protocol doesn't officially support IP Packets either.
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->enc.enc_type != 0)
      {
        super->demod.in_sync = 1;
        encode_ota_key_delivery_pkt(super, super->opts.m17_use_ip, super->m17d.lsf, super->enc.enc_type, super->enc.enc_subtype);
        super->demod.in_sync = 0;
      }
      break;

    //'p' key, send one time OTASK Packet
    case 112:
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->m17d.ecdsa.keys_loaded == 1 && super->opts.use_m17_duplex_mode == 0)
      {
        super->demod.in_sync = 1;
        encode_ota_key_delivery_pkt(super, super->opts.m17_use_ip, super->m17d.lsf, 3, 0);
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

    //'s' key, Enter Source Address Value, if using TX and RX or stream encoder
    case 115:
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && (super->opts.use_m17_str_encoder == 1 || super->opts.use_m17_duplex_mode == 1) )
        {
          //backup current string found here
          char tempsrc[50]; memset(tempsrc, 0, 50*sizeof(char));
          sprintf (tempsrc, "%s", super->m17e.srcs);
          sprintf (super->m17e.srcs, "%s", "");

          sprintf (label, " Enter Source Callsign:"); //set label to be displayed in the entry box window
          entry_string_ncurses_terminal(label, super->m17e.srcs);

          //If using is not passing a #XXXXXXXXXX reserved value
          if (super->m17e.srcs[0] != '#')
          {

            //check and capatalize any letters in the CSD
            for (int i = 0; super->m17e.srcs[i]!='\0'; i++)
            {
              if(super->m17e.srcs[i] >= 'a' && super->m17e.srcs[i] <= 'z')
                super->m17e.srcs[i] -= 32;
            }

            //terminate string
            super->m17e.srcs[9] = '\0';

          }

          //if no entry provided, revert to backup
          if (super->m17e.srcs[0] == 0)
          {
            sprintf (super->m17e.srcs, "%s", tempsrc);
          }
        }
      }
      break;

    //'t' key, Enter Text Message (will zero out any raw packet data)
    case 116:
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && (super->opts.use_m17_str_encoder == 1 || super->opts.use_m17_duplex_mode == 1) ) //&& super->opts.use_m17_str_encoder == 1
        {
          sprintf (super->m17e.sms, "%s", "");
          sprintf (super->m17d.sms, "%s", "");
          memset  (super->m17e.raw, 0, sizeof(super->m17e.raw)); //zero out any raw packet data (two aren't mutually exclusive)
          sprintf (label, " Enter Text Message:"); //set label to be displayed in the entry box window
          entry_string_ncurses_terminal(label, super->m17e.sms);
          if (super->m17e.sms[0]) //only send if there is an SMS text message loaded, else do nothing
          {
            sprintf (super->m17d.sms, "%s", super->m17e.sms);
            super->demod.in_sync = 1;
            encode_pkt(super, 0);
            super->demod.in_sync = 0;
          }
        }
      }
      break;

    //'u' key, Enter RAW Packet (will zero out loaded SMS Message)
    case 117:
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && (super->opts.use_m17_str_encoder == 1 || super->opts.use_m17_duplex_mode == 1))
        {
          sprintf (super->m17e.sms, "%s", ""); //zero out any loaded SMS message
          memset  (super->m17e.raw, 0, sizeof(super->m17e.raw)); //zero out any raw packet data
          sprintf (label, " Enter Raw Packet:"); //set label to be displayed in the entry box window
          sprintf (inp_str, "%s", "");    
          entry_string_ncurses_terminal(label, inp_str);
          uint16_t len = parse_raw_user_string(super, inp_str); UNUSED(len);
          if (super->m17e.raw[0]) //only send if there is a packet loaded, else do nothing
          {
            //below is disabled, as it now causes stale Meta to present in call history
            //and there isn't really a good reason to do this now
            // decode_pkt_contents(super, super->m17e.raw+1, len); //decode content locally for display

            super->demod.in_sync = 1;
            encode_pkt(super, 0);
            super->demod.in_sync = 0;
          }
        }
      }
      break;

    //'v' key, Toggle Vox Mode
    case 118:
      if (super->opts.use_m17_duplex_mode == 0)
      {
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
      }
      break;

    //'w' key, Enter Arb Text Message
    case 119:
      if (super->opts.send_conn_or_lstn != 4 || super->opts.use_m17_reflector_mode == 0)
      {
        if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && (super->opts.use_m17_str_encoder == 1 || super->opts.use_m17_duplex_mode == 1))
        {
          //unload anything in the .dat field (Meta Text)
          // sprintf (super->m17e.dat, "%s", "");
          // sprintf (super->m17d.dat, "%s", "");
          // memset  (super->m17e.meta_data, 0, sizeof(super->m17e.meta_data)); //this doesn't nuke encryption stuff

          //unload anything in the .arb field (Arb Text)
          memset(super->m17e.arb, 0, sizeof(super->m17e.arb));
          memset(super->m17d.arb, 0, sizeof(super->m17d.arb));
          sprintf (super->m17e.arb, "%s", "");
          sprintf (super->m17d.arb, "%s", "");
          sprintf (label, " Enter Arbitrary Data Text:"); //set label to be displayed in the entry box window
          entry_string_ncurses_terminal(label, super->m17e.arb);
          if (super->m17e.arb[0]) //only send if there is an arb text message loaded, else signal 3200 voice
          {
            super->opts.m17_str_encoder_dt = 3;
            sprintf (super->m17d.arb, "%s", super->m17e.arb);
          }
          else super->opts.m17_str_encoder_dt = 2;
        }
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
      if (super->opts.output_gain_vx < 2.99f)
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