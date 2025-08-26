/*-------------------------------------------------------------------------------
 * ncurses_terminal_handler.c
 * M17 Project - Ncurses Terminal Open, Close, and Print
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

//have at least one thing in a file available if the entire thing is wrapped in an ifdef
//or -pendantic warning: ISO C forbids an empty translation unit if USE_CURSES not defined

#include "main.h"

#ifdef USE_CURSES

#include "git_ver.h"
#include "banner.h"

void close_ncurses_terminal ()
{
  endwin();
}

void open_ncurses_terminal (Super * super)
{

  setlocale(LC_ALL, "");
  initscr(); //Initialize NCURSES screen window
  start_color();
  
  #ifdef PRETTY_COLORS
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);  //Yellow/Amber
  init_pair(2, COLOR_RED, COLOR_BLACK);     //Red
  init_pair(3, COLOR_GREEN, COLOR_BLACK);   //Green
  init_pair(4, COLOR_CYAN, COLOR_BLACK);    //Cyan
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK); //Magenta
  init_pair(6, COLOR_WHITE, COLOR_BLACK);   //White
  init_pair(7, COLOR_BLACK, COLOR_WHITE);   //INVERT
  init_pair(8, COLOR_RED, COLOR_WHITE);     //INVERT
  init_pair(9, COLOR_BLUE, COLOR_WHITE);    //INVERT
  #else
  init_pair(1, COLOR_WHITE, COLOR_BLACK);  //White
  init_pair(2, COLOR_WHITE, COLOR_BLACK);  //White
  init_pair(3, COLOR_WHITE, COLOR_BLACK);  //White
  init_pair(4, COLOR_WHITE, COLOR_BLACK);  //White
  init_pair(5, COLOR_WHITE, COLOR_BLACK);  //White
  init_pair(6, COLOR_WHITE, COLOR_BLACK);  //White
  init_pair(7, COLOR_WHITE, COLOR_BLACK);  //White
  init_pair(8, COLOR_WHITE, COLOR_BLACK);  //White
  init_pair(9, COLOR_WHITE, COLOR_BLACK);  //White
  #endif

  noecho();
  cbreak();

  super->opts.ncurses_is_open = 1;

}

void print_ncurses_terminal(Super * super)
{

  int input_keystroke = 0;

  //can't run getch when using STDIN -
  if (super->opts.use_stdin_input != 1)
  {
    timeout(0);
    input_keystroke = getch();
  }

  //Erase the terminal
  erase();

  //Print Banner
  print_ncurses_banner(super);

  //Print Config
  if (super->opts.ncurses_show_io)
    print_ncurses_config(super);
  else printw ("--Input-Output-(I)-------------------------------------------------------------\n");

  //Print Audio Levels
  if (super->opts.ncurses_show_audio)
    print_ncurses_levels(super);
  else printw ("--Audio-Level--(A)-------------------------------------------------------------\n");

  //Print Symbol Scope
  if (super->opts.ncurses_show_scope)
    print_ncurses_scope(super);
  else printw ("--Symbol-Scope-(S)-------------------------------------------------------------\n");

  //Print Decoded Call Info
  if (super->opts.ncurses_show_decode)
    print_ncurses_call_info(super);
  else if (!super->opts.use_m17_rfa_decoder && !super->opts.use_m17_ipf_decoder)
    printw ("--Encode-Info--(D)-------------------------------------------------------------\n");
  else
    printw ("--Decode-Info--(D)-------------------------------------------------------------\n");

  //Print Call History
  if (super->opts.use_m17_str_encoder || super->opts.use_m17_ipf_encoder || super->opts.use_m17_pkt_encoder) {}
  else if (super->opts.ncurses_show_history)
    print_ncurses_call_history(super);
  else printw ("--Call-History-(H)-------------------------------------------------------------\n");

  //Handle Input Keystrokes
  input_ncurses_terminal(super, input_keystroke);

  //refresh the terminal
  refresh();
    
}

void print_ncurses_banner (Super * super)
{

  int i;
  if (super->opts.ncurses_show_banner == 0)
  {
    printw ("-------------------------------------------------------------------------------\n");
    printw ("| M17 Project: Florida Man Edition - Build: %s - Session: %04X\n", GIT_TAG, super->opts.random_number);
    printw ("---------------(C)-------------------------------------------------------------\n");
  }
  else
  {
    for (i = 1; i < 8; i++)
    {
      attron(COLOR_PAIR(7));
      printw ("%s", M_banner[i]);
      attron(COLOR_PAIR(8));
      printw ("%s", S_banner[i]);
      attron(COLOR_PAIR(9));
      printw ("%s", FME_banner[i]);
      attron(COLOR_PAIR(6));
      #ifdef USE_CODEC2
      if (i == 1) printw (" CODEC2");
      #endif
      if (i == 2) printw (" 'q' to Quit ");

      if (i == 4) printw (" SV: %s", SPEC_VERSION);
      if (i == 5) printw (" SD: %s", SPEC_DATE);
      if (i == 6) printw (" Session: %04X", super->opts.random_number);
      if (i == 7) printw (" %s", GIT_TAG);
      printw ("\n");
    }
    attroff(COLOR_PAIR(6));
  }
  UNUSED(M17FME_banner);
}

void print_ncurses_config (Super * super)
{
  //color on, cyan
  attron(COLOR_PAIR(4));

  printw ("--Input-Output-(I)-------------------------------------------------------------\n");
  printw ("| ");

  //Input Methods (Hardware)
  if (super->opts.use_m17_duplex_mode)
  {
    if (super->opts.m17_use_ip == 0)
      printw ("Pulse RFA    Input:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    printw ("Pulse Voice  Input:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
  }

  else if (super->opts.use_pa_input && super->opts.use_m17_rfa_decoder)
  {
    printw ("Pulse RFA    Input:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    if (super->pa.pa_input_idx[0] != 0)
      printw ("D: %s;", super->pa.pa_input_idx);
  }
    
  else if (super->opts.use_pa_input && !super->opts.use_m17_rfa_decoder)
  {
    printw ("Pulse Voice  Input:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    if (super->pa.pa_input_idx[0] != 0)
      printw ("D: %s;", super->pa.pa_input_idx);
  }

  else if (super->opts.use_oss_input)
  {
    printw ("OSS  Input:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    if      (super->opts.internal_loopback_decoder) printw ("Voice Input;      ");
    else if (super->opts.use_m17_pkt_encoder)       printw ("Voice Input;      ");
    else if (super->opts.use_m17_str_encoder)       printw ("Voice Input;      ");
    else if (super->opts.use_m17_brt_encoder)       printw ("Voice Input;      ");
    else                                            printw ("RF Input;         ");
  }

  //Input Methods (Network)
  else if (super->opts.use_tcp_input)
    printw ("TCP SND Input: %s:%d; %d kHz; %d Ch; ", super->opts.tcp_input_hostname, super->opts.tcp_input_portno, super->opts.input_sample_rate/1000, 1);

  else if (super->opts.m17_udp_sock && super->opts.use_m17_ipf_decoder)
    printw ("UDP IP Frame Input: %s:%d; ", super->opts.m17_hostname, super->opts.m17_portno);

  //Input Methods (Files)
  else if (super->opts.snd_input_is_a_file)
    printw ("File SND Input: %s; %d kHz; ", super->snd_src_in.snd_in_filename, super->opts.input_sample_rate);

  else if (super->opts.use_stdin_input)
    printw ("STDIN SND Input: %s; %d kHz; KB Shortcuts Disabled; ", "-", super->opts.input_sample_rate);

  else if (super->opts.use_float_symbol_input == 1)
    printw ("File: M17 Float Symbol Input: %s;", super->opts.float_symbol_input_file);

  else if (super->opts.use_dibit_input == 1)
    printw ("File: DSD-FME Dibit Capture Bin Input: %s; ", super->opts.dibit_input_file);

  printw ("\n");
  printw ("| ");

  //Output Methods (Hardware)
  if (super->pa.pa_output_rf_is_open)
  {
    printw ("Pulse RFA   Output:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    if (super->pa.pa_outrf_idx[0] != 0)
      printw ("D: %s;", super->pa.pa_outrf_idx);
  }

  if (super->pa.pa_output_vx_is_open)
  {
    printw ("Pulse Voice Output:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    if (super->pa.pa_outvx_idx[0] != 0)
      printw ("D: %s;", super->pa.pa_outvx_idx);
  }
    
  if (super->opts.use_oss_output)
  {
    printw ("OSS Output:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    if      (super->opts.internal_loopback_decoder) printw ("Loopback Decoder; ");
    else if (super->opts.use_m17_pkt_encoder)       printw ("RF Output;        ");
    else if (super->opts.use_m17_str_encoder)       printw ("RF Output;        ");
    else if (super->opts.use_m17_brt_encoder)       printw ("RF Output;        ");
    else                                            printw ("Voice Decoder;    ");
  }

  //Output Methods (Files)
  if (super->opts.use_wav_out_rf == 1)
    printw ("\n| File: RF Modulated Audio Output: %s;", super->wav.wav_out_file_rf);

  if (super->opts.use_wav_out_vx == 1)
    printw ("\n| File: Decoded Voice Audio Output: %s;", super->wav.wav_out_file_vx);

  if (super->opts.use_wav_out_pc == 1)
    printw ("\n| Per Call: %s", super->wav.wav_out_file_pc);

  if (super->opts.use_float_symbol_output == 1)
    printw ("\n| File: M17 Float Symbol Output: %s;", super->opts.float_symbol_output_file);

  if (super->opts.use_dibit_output == 1)
    printw ("\n| File: DSD-FME Dibit Capture Bin Output: %s;", super->opts.dibit_output_file);

  //Output UDP IP Frame
  if (super->opts.m17_udp_sock && !super->opts.use_m17_ipf_decoder && !super->opts.use_m17_duplex_mode)
    printw ("\n| UDP IP Frame Output: %s:%d; Reflector Module: %c", super->opts.m17_hostname, super->opts.m17_portno, super->m17e.reflector_module);

  if (super->opts.m17_udp_sock && super->opts.use_m17_duplex_mode)
  {
    printw ("\n| UDP IP Frame  Input: %s:%d; ", "localhost", 17000);
    printw ("\n| UDP IP Frame Output: %s:%d; Reflector Module: %c", super->opts.m17_hostname, super->opts.m17_portno, super->m17e.reflector_module);
  }

  if (super->opts.event_log)
    printw ("\n| Event Log: %s", super->opts.event_log_file);

  printw ("\n");
  printw ("-------------------------------------------------------------------------------\n");

  //color off, back to white
  attron(COLOR_PAIR(6));
}

//NOTE: This is just eye candy, it's not real time or anything.
void print_ncurses_scope (Super * super)
{
  //color on, yellow
  if (super->demod.in_sync)
    attron(COLOR_PAIR(1));

  uint8_t i; uint8_t end = 71; //uint8_t so it isn't a negative number when cycling backwards, but rollover
  printw ("--Symbol-Scope-(S)-------------------------------------------------------------");
  printw ("\n| +3:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr)-(end-i)] == +3.0f) printw("*"); else printw(" ");
  printw ("\n| +1:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr)-(end-i)] == +1.0f) printw("*"); else printw(" ");
  printw ("\n| -1:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr)-(end-i)] == -1.0f) printw("*"); else printw(" ");
  printw ("\n| -3:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr)-(end-i)] == -3.0f) printw("*"); else printw(" ");

  //in level and symbol levels and center value
  if (!super->opts.use_m17_str_encoder && !super->opts.use_m17_ipf_decoder)
  {
    printw ("\n| ");
    printw ("In: %2.0f%%; +3.0: %5.0f; +1.0: %5.0f; -1.0: %6.0f; -3: %6.0f; Center: %4.0f; ",
        super->demod.input_level, super->demod.fsk4_max, super->demod.fsk4_umid,
        super->demod.fsk4_lmid, super->demod.fsk4_min, super->demod.fsk4_center);

    //symbol edges
    printw ("\n| %s", super->demod.fsk4_timing_string);

    //corrective value
    if (!super->opts.disable_symbol_timing)
    {
      if (super->demod.fsk4_timing_correction < 0)
        printw ( " Off: %02d;", super->demod.fsk4_timing_correction);
      else
        printw ( " Off: +%d;", super->demod.fsk4_timing_correction);
    }
    else if (super->opts.disable_symbol_timing)
      printw ( " Off: DIS;"); //disabled

    printw (" Sync Symbol Distance: %2.0f", super->demod.sync_distance);
    
  }

  printw ("\n");
  printw ("-------------------------------------------------------------------------------\n");

  //color off, back to white
  attron(COLOR_PAIR(6));
}

void print_ncurses_levels (Super * super)
{
  //color on, yellow
  if (super->demod.in_sync)
    attron(COLOR_PAIR(1));

  printw ("--Audio-Level--(A)-------------------------------------------------------------\n");

  if (super->opts.use_m17_duplex_mode)
  {
    printw ("| Voice  Input: %3.0f%% (/|*) ", super->opts.input_gain_vx  * 100);
    printw ("\n");
    printw ("| Voice Output: %3.0f%% (-|+) ", super->opts.output_gain_vx * 100);
    if (super->opts.use_hpfilter_dig == 1)
      printw (" HPF(h); ");
    else printw ("!HPF(h); ");
    if (super->opts.use_raw_audio_monitor)
      printw ( " Analog Monitor(M);");
    else printw ( "!Analog Monitor(M);");
    printw ("\n");

    if (!super->opts.m17_use_ip)
    {
      printw ("|   RFA  Input: %3.0f%% ([|]) ", super->opts.input_gain_rf  * 100);
      if (super->opts.disable_rrc_filter == 0)
        printw (" RRC(r);");
      else printw ("!RRC(r);");
      if (super->opts.inverted_signal == 0)
        printw (" +Polarity(x);");
      else printw (" -Polarity(x);");
      printw ("\n");
      printw ("|   RFA Output: %3.0f%% ({|}) ", super->opts.output_gain_rf * 100);
      printw ("\n");
    }
  }

  if (super->opts.use_m17_rfa_decoder == 1)
  {
    printw ("|   RFA  Input: %3.0f%% ([|]) ", super->opts.input_gain_rf  * 100);
    if (super->opts.disable_rrc_filter == 0)
      printw (" RRC(r);");
    else printw ("!RRC(r);");
    if (super->opts.inverted_signal == 0)
      printw (" +Polarity(x);");
    else printw (" -Polarity(x);");
    printw ("\n");
  }

  else if (super->opts.use_pa_output_rf || super->opts.use_oss_output || super->opts.use_stdout_output || super->wav.wav_out_rf)
  {
    printw ("|   RFA Output: %3.0f%% ({|}) ", super->opts.output_gain_rf * 100);
    if (super->opts.disable_rrc_filter == 0)
      printw (" RRC(r);");
    else printw ("!RRC(r);");
    if (super->opts.inverted_signal == 0)
      printw (" +Polarity(x);");
    else printw (" -Polarity(x);");
    printw ("\n");
  }

  if (super->opts.use_m17_str_encoder)
  {
    printw ("| Voice  Input: %3.0f%% (/|*) ", super->opts.input_gain_vx  * 100);
    if (super->m17e.str_encoder_vox)
    {
      printw (" Mic Vox(v); SQL: %04ld (<|>); RMS: %04ld;", super->demod.input_sql, super->demod.input_rms);
      if (super->m17e.str_encoder_tx)
        printw (" !!!!!");
    }
    else printw ("!Mic Vox(v); ");
    printw ("\n");
  }

  if (super->opts.use_m17_rfa_decoder || super->opts.internal_loopback_decoder)
  {
    printw ("| Voice Output: %3.0f%% (-|+) ", super->opts.output_gain_vx * 100);
    if (super->opts.use_hpfilter_dig == 1)
      printw (" HPF(h); ");
    else printw ("!HPF(h); ");
    if (super->opts.use_raw_audio_monitor)
      printw ( " Analog Monitor(M);");
    else printw ( "!Analog Monitor(M);");
    printw ("\n");
  }
  printw ("-------------------------------------------------------------------------------\n");

  //color off, back to white
  attron(COLOR_PAIR(6));
}

void print_ncurses_call_info (Super * super)
{
  //color on or off
  if (super->demod.in_sync == 1)
    attron(COLOR_PAIR(3));
  else attron(COLOR_PAIR(6));

  if (!super->opts.use_m17_rfa_decoder && !super->opts.use_m17_ipf_decoder)
    printw ("--Encode-Info--(D)-------------------------------------------------------------\n");
  else
    printw ("--Decode-Info--(D)-------------------------------------------------------------\n");

  printw ("| ");
  printw ("M17: ");

  if (super->opts.use_m17_str_encoder)
  {
    printw ("Stream Encoder");
    if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0)
      printw ( " DST(d); SRC(s); CAN(b);");
  }
  else if (super->opts.use_m17_pkt_encoder == 1)
    printw ("Packet Encoder"); //this doesn't use ncurses terminal, but it may later
  else if (super->opts.use_m17_brt_encoder == 1)
    printw ("Bit Error Rate Test Encoder"); //this doesn't use ncurses terminal, but it may later
  else if (super->opts.use_m17_ipf_decoder == 1)
    printw ("UDP/IP Frame Decoder");
  else if (super->opts.use_m17_duplex_mode == 1)
  {
    printw ("Duplex Encoder and Decoder");
    if (super->opts.m17_use_ip) printw(" (IP)");
    else printw(" (RF)");
    if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0)
      printw ( " DST(d); SRC(s); CAN(b);");
  }
  else
  {
    printw ("Stream and Packet Decoder");
    if (super->opts.m17_use_ip) printw(" (IP)");
    else printw(" (RF)");
  }


  if (super->opts.payload_verbosity)
    printw (" Payload Verb: %d;", super->opts.payload_verbosity);

  if (super->opts.demod_verbosity)
    printw (" Demod Verb: %d;", super->opts.demod_verbosity);

  if (!super->opts.use_m17_rfa_decoder && !super->opts.use_m17_ipf_decoder)
  {
    printw ("\n");
    printw ("| ");
    printw ("M17: ");
    if (super->m17e.str_encoder_vox)
    {
      printw ("Voice Activated TX (VOX)");
      if (super->m17e.str_encoder_tx)
        printw (" !!!!!");
      else
        printw ("      ");
    }
    else
    {
      printw ("Press (\\) to Toggle TX");
      if (super->m17e.str_encoder_tx == 0)
        printw (" (OFF);");
      else printw (" ( ON);");
    }

  }

  printw ("\n");
  printw ("| ");
  printw ("M17: ");

  //insert data type and frame information
  if (super->m17d.dt == 0) printw("Reserved");
  if (super->m17d.dt == 1) printw("Packet Data ");
  if (super->m17d.dt == 2) printw("Voice (3200) ");
  if (super->m17d.dt == 3) printw("Voice (1600) + Arbitrary Data");

  printw ("\n");
  printw ("| ");

  printw ("DST: ");
  if (super->m17d.dst == 0xFFFFFFFFFFFF)
    printw("BROADCAST ");
  else if (super->m17d.dst != 0 && super->m17d.dst >= 0xEE6B28000000)
    printw("RESERVED (%012llX) ", super->m17d.dst);
  else
    printw("%s", super->m17d.dst_csd_str);

  printw ("\n");
  printw ("| ");

  printw ("SRC: ");
  if (super->m17d.src != 0 && super->m17d.src >= 0xEE6B28000000)
    printw("RESERVED (%012llX)", super->m17d.src);
  else
    printw("%s", super->m17d.src_csd_str);


  printw ("\n");
  printw ("| ");

  if (super->m17d.can != -1)
    printw ("CAN: %02d ", super->m17d.can);
  else printw ("CAN:   ");

  printw ("\n");
  printw ("| ");
  printw ("ENC: ");

  //Display and Encryption Methods, if used
  if (super->m17d.enc_et != 0)
  {
    if (super->demod.in_sync == 1)
      attron(COLOR_PAIR(1));
    else attron(COLOR_PAIR(6));
  }

  if (super->m17d.enc_et == 1)
  {
    printw ("Scrambler: ");
    if (super->m17d.enc_st == 0)
      printw("8-bit; ");
    if (super->m17d.enc_st == 1)
      printw("16-bit; ");
    if (super->m17d.enc_st == 2)
      printw("24-bit; ");

    //may disable seed display if it gets too annoying later on
    // if ( (super->opts.use_m17_str_encoder || super->opts.use_m17_duplex_mode) && super->m17e.str_encoder_tx && super->enc.scrambler_key)
    //   printw("Seed: %06X; ", super->enc.scrambler_seed_e);

    // else if (!super->opts.use_m17_str_encoder && super->enc.scrambler_key)
    //   printw("Seed: %06X; ", super->enc.scrambler_seed_d);

    if (super->demod.in_sync == 1)
      attron(COLOR_PAIR(2));
    else attron(COLOR_PAIR(6));

    if (super->m17d.enc_mute)
      printw ("MUTED");

    if (super->demod.in_sync == 1)
      attron(COLOR_PAIR(1));
    else attron(COLOR_PAIR(6));
    
  }
  else if (super->m17d.enc_et == 2)
  {
    printw ("AES-");
    if (super->m17d.enc_st == 0)
      printw("128 ");
    if (super->m17d.enc_st == 1)
      printw("192 ");
    if (super->m17d.enc_st == 2)
      printw("256 ");
    
    printw ("IV: ");
    
    //display packed meta as IV
    for (int i = 0; i < 16; i++)
      printw ("%02X", super->m17d.meta[i]);

    if (super->demod.in_sync == 1)
      attron(COLOR_PAIR(2));
    else attron(COLOR_PAIR(6));

    if (super->m17d.enc_mute)
      printw (" MUTED");

    if (super->demod.in_sync == 1)
      attron(COLOR_PAIR(1));
    else attron(COLOR_PAIR(6));

  }
  else if (super->m17d.enc_et == 3)
    printw (" Reserved Encryption Type: %d", super->m17d.enc_st);
  // else 
  //   printw ("Clear; ");

  //Too many color switches, but gotta pick the nits
  if (super->demod.in_sync == 1)
    attron(COLOR_PAIR(3));
  else attron(COLOR_PAIR(6));

  printw ("\n");
  printw ("| ");
  printw ("KEY: ");

  if (super->demod.in_sync == 1)
    attron(COLOR_PAIR(1));
  else attron(COLOR_PAIR(6));

  int keylen = 32;
  
  if (super->m17d.enc_st == 0)
    keylen = 16;

  if (super->m17d.enc_st == 1)
    keylen = 24;
  
  if (super->m17d.enc_st == 2)
    keylen = 32;

  if (super->m17d.enc_et == 2 && super->enc.aes_key_is_loaded)
  {
    for (int i = 0; i < keylen; i++)
      printw ("%02X", super->enc.aes_key[i]);
  }
  else if (super->m17d.enc_et == 1 && super->enc.scrambler_key != 0)
    printw ("%X", super->enc.scrambler_key);

  if (super->demod.in_sync == 1)
    attron(COLOR_PAIR(3));
  else attron(COLOR_PAIR(6));

  if (super->m17d.enc_et != 0)
  {
    if (super->demod.in_sync == 1)
      attron(COLOR_PAIR(3));
    else attron(COLOR_PAIR(6));
  }

  //Display any Decoded Messages

  //take a truncated string, only display first 71 chars on Ncurses Terminal (see log for full messages)
  char shortstr[76]; sprintf (shortstr, "%s", "");
  memcpy (shortstr, super->m17d.sms, 71);
  shortstr[72] = '\0'; //terminate string

  printw ("\n");
  printw ("| ");
  printw ("SMS: ");
  printw ("%s", shortstr);

  sprintf (shortstr, "%s", "");
  memcpy (shortstr, super->m17d.dat, 71);
  printw ("\n");
  printw ("| ");
  printw ("MET: ");
  printw ("%s", shortstr);

  sprintf (shortstr, "%s", "");
  memcpy (shortstr, super->m17d.arb, 71);
  printw ("\n");
  printw ("| ");
  printw ("ARB: ");
  printw ("%s", shortstr);

  //OTA Options for sending OTAKD and other things (for encoder)
  if (!super->opts.use_m17_rfa_decoder && !super->opts.use_m17_ipf_decoder)
  {

    printw ("\n");
    printw ("| ");
    printw ("OTA:");
    if (super->enc.enc_type != 0)
    {
      if ( (super->opts.use_m17_str_encoder || super->opts.use_m17_duplex_mode) && super->opts.use_otakd)
        printw (" Disable OTAKD(O);");

      if ( (super->opts.use_m17_str_encoder || super->opts.use_m17_duplex_mode) && !super->opts.use_otakd)
        printw (" Enable OTAKD(O);");
      
      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->enc.enc_type != 0)
        printw (" Send OTAKD(o);");
    }

    if (super->m17d.ecdsa.keys_loaded != 0)
    {
      if (super->opts.use_m17_str_encoder && super->opts.use_otask)
        printw (" Disable OTASK(P);");

      if (super->opts.use_m17_str_encoder && !super->opts.use_otask)
        printw (" Enable OTASK(P);");

      if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0 && super->opts.use_m17_duplex_mode == 0)
        printw (" Send OTASK(p);");
    }

    printw ("\n");
    printw ("| ");
    printw ("PKT:");
    if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0)
    {
      printw (" Send SMS Text(t);");
      printw (" Send Raw Data(u);");
      printw (" Load Arb Text(w);");
      printw (" Load Meta Text(m);");
    }

    printw ("\n");
    printw ("| ");
    printw ("DBG:");

    if (super->m17e.str_encoder_vox == 0 && super->m17e.str_encoder_tx == 0)
    {
      if (super->enc.enc_type == 0 && super->enc.aes_key_is_loaded == 0 && super->enc.scrambler_key == 0)
      {
        printw (" Random Scrambler(1);");
        printw (" Random AES(2);");
      }
      else if (super->enc.scrambler_key)
        printw (" Disable Scrambler(e);");
      else if (super->enc.aes_key_is_loaded)
        printw (" Disable AES(E);");

      #ifdef USE_UECC
      if (super->opts.use_m17_duplex_mode == 0)
      {
        if (super->m17e.ecdsa.keys_loaded == 0)
          printw (" Random Signature(3);");
        else printw (" Disable Signature(5);");
      }
      #endif

      if (super->opts.use_m17_duplex_mode == 1)
      {
        if (super->opts.use_m17_packet_burst == 0)
          printw (" Enable PKT Burst(B);");
        else printw (" Disable PKT Burst(B);");
      }
    }

  }

  //error tracking (for decoder)
  if (super->opts.use_m17_rfa_decoder || super->opts.use_m17_ipf_decoder || super->opts.use_m17_duplex_mode)
  {
    
    printw ("\n");
    printw ("| ");
    printw ("ERR: ");
    printw ("LSF: %05d; EMB: %05d; GLY: %05d; PKT: %05d; IPF: %05d; ", 
      super->error.lsf_hdr_crc_err, super->error.lsf_emb_crc_err, 
      super->error.golay_err, super->error.pkt_crc_err, super->error.ipf_crc_err);
  }

  printw ("\n");
  printw ("-------------------------------------------------------------------------------\n");

  //color off, back to white
  attron(COLOR_PAIR(6));
}

void print_ncurses_call_history (Super * super)
{
  //color on, cyan
  attron(COLOR_PAIR(4));

  printw ("--Call-History-(H)--Reset-(c)--Print-(L)---------------------------------------");
  for (int i = 0; i < 100; i++)
  {
    if (super->m17d.callhistory[99-i][0] != 0)
      printw ("\n| #%02d. %s", i+1, super->m17d.callhistory[99-i]);
  }
  printw ("\n-------------------------------------------------------------------------------\n");

  //color off, back to white
  attron(COLOR_PAIR(6));
}

#endif