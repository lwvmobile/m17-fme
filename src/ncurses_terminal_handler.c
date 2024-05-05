/*-------------------------------------------------------------------------------
 * ncurses_terminal_handler.c
 * Project M17 - Ncurses Terminal Open, Close, and Print
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#ifdef USE_CURSES

#include "main.h"
#include "git_ver.h"
#include "banner.h"

void close_ncurses_terminal ()
{
  endwin();
}

void open_ncurses_terminal ()
{

  setlocale(LC_ALL, "");
  initscr(); //Initialize NCURSES screen window
  start_color();
  
  #ifdef PRETTY_COLORS
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);      //Yellow/Amber for frame sync/control channel, NV style
  init_pair(2, COLOR_RED, COLOR_BLACK);        //Red for Terminated Calls
  init_pair(3, COLOR_GREEN, COLOR_BLACK);     //Green for Active Calls
  init_pair(4, COLOR_CYAN, COLOR_BLACK);     //Cyan for Site Extra and Patches
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK); //Magenta for no frame sync/signal
  init_pair(6, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(7, COLOR_BLACK, COLOR_WHITE);  //INVERT
  init_pair(8, COLOR_RED, COLOR_WHITE);  //INVERT
  init_pair(9, COLOR_BLUE, COLOR_WHITE);  //INVERT
  #else
  init_pair(1, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(2, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(3, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(4, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(5, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(6, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(7, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(8, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  init_pair(9, COLOR_WHITE, COLOR_BLACK);  //White Card Color Scheme
  #endif

  noecho();
  cbreak();

}

void print_ncurses_terminal(Super * super)
{

 
  int input_keystroke = 0;

  //can't run getch/menu when using STDIN -
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
  print_ncurses_config(super);

  //Print Call Info
  print_ncurses_call_info(super);

  //Print Symbol Scope
  if (super->opts.demod_verbosity != 0)
    print_ncurses_scope(super);

  //Print Call History
  if (!super->opts.use_m17_str_encoder && !super->opts.use_m17_ipf_encoder && !super->opts.ncurses_no_history)
    print_ncurses_call_history(super);

  //Handle Input Keystrokes
  input_ncurses_terminal(super, input_keystroke);

  //refresh the terminal
  refresh();
    
}

void print_ncurses_banner (Super * super)
{

  if (super->opts.ncurses_no_banner == 1)
  {
    printw ("------------------------------------------------------------------------------\n");
    printw ("| Project M17: Florida Man Edition - Build: %s - Session: %04X\n", GIT_TAG, super->opts.random_number);
    printw ("------------------------------------------------------------------------------\n");
  }
  else
  {
    for (int i = 1; i < 8; i++)
    {
      attron(COLOR_PAIR(7));
      printw ("%s", M_banner[i]);
      attron(COLOR_PAIR(8));
      printw ("%s", S_banner[i]);
      attron(COLOR_PAIR(9));
      printw ("%s", FME_banner[i]);
      attron(COLOR_PAIR(6));
      if (i == 2) printw ("   CTRL+C or ");
      if (i == 3) printw (" 'q' to Quit ");
      #ifdef USE_CODEC2
      if (i == 5) printw (" CODEC2");
      #endif
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

  printw ("--Input Output----------------------------------------------------------------\n");
  printw ("| ");

  //Input Methods
  if (super->opts.use_pa_input && super->opts.use_m17_rfa_decoder)
    printw ("Pulse RFA    Input:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);

  else if (super->opts.use_pa_input && !super->opts.use_m17_rfa_decoder)
    printw ("Pulse Voice  Input:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);

  else if (super->opts.use_oss_input)
  {
    printw ("OSS  Input:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    if      (super->opts.monitor_encode_internally) printw ("Voice Input;      ");
    else if (super->opts.use_m17_pkt_encoder)       printw ("Voice Input;      ");
    else if (super->opts.use_m17_str_encoder)       printw ("Voice Input;      ");
    else if (super->opts.use_m17_brt_encoder)       printw ("Voice Input;      ");
    else                                            printw ("RF Input;         ");
  }

  else if (super->opts.use_tcp_input)
    printw ("TCP SND Input: %s:%d; %d kHz; %d Ch; ", super->opts.tcp_input_hostname, super->opts.tcp_input_portno, super->opts.input_sample_rate/1000, 1);

  else if (super->opts.m17_udp_sock && super->opts.use_m17_ipf_decoder)
    printw ("UDP IP Frame Input: %s:%d; ", super->opts.m17_hostname, super->opts.m17_portno);

  else if (super->opts.snd_input_is_a_file)
    printw ("File SND Input: %s; %d kHz; ", super->snd_src_in.snd_in_filename, super->opts.input_sample_rate);

  else if (super->opts.use_stdin_input)
    printw ("STDIN SND Input: %s; %d kHz; KB Shortcuts Disabled; ", "-", super->opts.input_sample_rate);

  else if (super->opts.use_float_symbol_input == 1)
    printw ("File: M17 Float Symbol Input: %s;", super->opts.float_symbol_input_file);

  else if (super->opts.use_dibit_input == 1)
    printw ("File: DSD-FME Dibit Capture Bin Input: %s; ", super->opts.dibit_input_file);

  // if (super->opts.rig_remote_sock) //TODO: Add this functionality?
  //   printw ("RIG: %s:%d; ", opts->tcp_hostname, opts->rigctlportno);

  //debug '2' option, RRC enabled or disabled
  if (!super->opts.use_m17_ipf_decoder) //still shows up on some encoders, but perhaps we want it to...?
  {
    if (super->opts.disable_rrc_filter == 0)
      printw (" RRC(r);");
    else printw ("!RRC(r);");

    if (super->opts.inverted_signal == 0)
      printw (" ++++(x);");
    else printw (" ----(x);");
  }

  if (super->m17e.str_encoder_vox)
    printw (" Mic Vox SQL: %04ld; RMS: %04ld;", super->demod.input_sql, super->demod.input_rms);

  printw ("\n");
  printw ("| ");

  //Output Methods (Hardware)
  if (super->pa.pa_output_rf_is_open)
    printw ("Pulse RFA   Output:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);

  if (super->pa.pa_output_vx_is_open)
    printw ("Pulse Voice Output:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);

  if (super->opts.use_oss_output)
  {
    printw ("OSS Output:  %d kHz; %i Ch; ", super->opts.input_sample_rate/1000, 1);
    if      (super->opts.monitor_encode_internally) printw ("Loopback Decoder; ");
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

  if (super->opts.use_float_symbol_output == 1)
    printw ("\n| File: M17 Float Symbol Output: %s;", super->opts.float_symbol_output_file);

  if (super->opts.use_dibit_output == 1)
    printw ("\n| File: DSD-FME Dibit Capture Bin Output: %s;", super->opts.dibit_output_file);

  //Output UDP IP Frame
  if (super->opts.m17_udp_sock && !super->opts.use_m17_ipf_decoder)
    printw ("\n| UDP IP Frame Output: %s:%d; Reflector Module: %c", super->opts.m17_hostname, super->opts.m17_portno, super->m17e.reflector_module);

  //in level and symbol levels and center value
  if (!super->opts.use_m17_str_encoder && !super->opts.use_m17_ipf_decoder)
  {
    printw ("\n| ");
    printw ("In: %2.0f%%; +3.0: %5.0f; +1.0: %5.0f; -1.0: %6.0f; -3: %6.0f; Center: %4.0f; ", 
        super->demod.input_level, super->demod.fsk4_max, super->demod.fsk4_umid,
        super->demod.fsk4_lmid, super->demod.fsk4_min, super->demod.fsk4_center);
    // printw ("In: %2.0f%%; -3.0: %6.0f; -1.0: %6.0f; +1.0: %6.0f; +3: %5.0f; Center: %4.0f; ", 
    //     super->demod.input_level, super->demod.fsk4_min, super->demod.fsk4_lmid,
    //     super->demod.fsk4_umid, super->demod.fsk4_max, super->demod.fsk4_center);
  }
  else
  {
    printw ("\n| "); //add an item here?
  }
  

  printw ("\n");
  printw ("------------------------------------------------------------------------------\n");

  //color off, back to white
  attron(COLOR_PAIR(6));
}

//NOTE: This is just eye candy, it's not real time or anything.
void print_ncurses_scope (Super * super)
{
  //color on, yellow
  if (super->demod.in_sync)
    attron(COLOR_PAIR(1));

  int i;
  printw ("--Symbol Scope----------------------------------------------------------------");
  printw ("\n| +3:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr)-(71-i)] == +3.0f) printw("*"); else printw(" ");
  printw ("\n| +1:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr)-(71-i)] == +1.0f) printw("*"); else printw(" ");
  printw ("\n| -1:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr)-(71-i)] == -1.0f) printw("*"); else printw(" ");
  printw ("\n| -3:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr)-(71-i)] == -3.0f) printw("*"); else printw(" ");
  // printw ("\n| ");
  printw ("\n");
  printw ("------------------------------------------------------------------------------\n");

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
    printw ("--Encode Info-----------------------------------------------------------------\n");
  else
    printw ("--Decode Info-------------------------------------------------------------------\n");

  printw ("| ");
  printw ("M17: ");

  if (super->opts.use_m17_str_encoder)
    printw ("Stream Encoder");
  else if (super->opts.use_m17_pkt_encoder == 1)
    printw ("Packet Encoder"); //this doesn't use ncurses terminal, but it may later
  else if (super->opts.use_m17_brt_encoder == 1)
    printw ("Bit Error Rate Test Encoder"); //this doesn't use ncurses terminal, but it may later
  else if (super->opts.use_m17_ipf_decoder == 1)
    printw ("UDP/IP Frame Decoder");
  else printw ("RF Stream and Packet Decoder"); //not sure what to put here, if anything

  if (!super->opts.use_m17_rfa_decoder && !super->opts.use_m17_ipf_decoder)
  {
    printw ("\n");
    printw ("| ");
    printw ("M17: ");
    printw ("Press (\\) to Toggle TX");
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
    printw("RESERVED (%012llx) ", super->m17d.dst);
  else
    printw("%s", super->m17d.dst_csd_str);

  printw ("\n");
  printw ("| ");

  printw ("SRC: ");
  if (super->m17d.src != 0 && super->m17d.src >= 0xEE6B28000000)
    printw("RESERVED (%012llx)", super->m17d.src);
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
    printw ("Scrambler Type: %d; ", super->m17d.enc_st);
    if (super->enc.scrambler_key != 0)
      printw( ("Key: %X"), super->enc.scrambler_key);
  }
  else if (super->m17d.enc_et == 2)
  {
    printw ("AES-CTR IV: ");
    //display packed meta as IV
    for (int i = 0; i < 16; i++)
      printw ("%02X", super->m17d.meta[i]);

    if (super->enc.aes_key_is_loaded)
    {
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

      for (int i = 0; i < 32; i++)
      {
        if (i == 8 || i == 16 || i == 24) printw (" ");
        printw ("%02X", super->enc.aes_key[i]);
      }

      if (super->demod.in_sync == 1)
        attron(COLOR_PAIR(3));
      else attron(COLOR_PAIR(6));
    }
  }
  else if (super->m17d.enc_et == 3)
    printw (" Reserved Encryption Type: %d", super->m17d.enc_st);
  else printw ("Clear");

  if (super->m17d.enc_et != 0)
  {
    if (super->demod.in_sync == 1)
      attron(COLOR_PAIR(3));
    else attron(COLOR_PAIR(6));
  }

  //Display any Decoded Messages

  //take a truncated string, only display first 71 chars on Ncurses Terminal (see log for full messages)
  char shortstr[76]; memset (shortstr, 0, 76*sizeof(char));
  memcpy (shortstr, super->m17d.sms, 71);
  shortstr[72] = 0; //terminate string

  printw ("\n");
  printw ("| ");
  printw ("SMS: ");
  printw ("%s", shortstr);

  memcpy (shortstr, super->m17d.dat, 71);
  printw ("\n");
  printw ("| ");
  printw ("POS: ");
  printw ("%s", shortstr); //TODO: need hex print on these, store a len value

  memcpy (shortstr, super->m17d.arb, 71);
  printw ("\n");
  printw ("| ");
  printw ("ARB: ");
  printw ("%s", shortstr);

  printw ("\n");
  printw ("------------------------------------------------------------------------------\n");

  //color off, back to white
  attron(COLOR_PAIR(6));
}

void print_ncurses_call_history (Super * super)
{
  int i;

  //color on, cyan
  attron(COLOR_PAIR(4));

  printw ("--Call History----------------------------------------------------------------");
  for (i = 0; i < 10; i++)
  {
    if (super->m17d.callhistory[9-i][0] != 0)
      printw ("\n| #%02d. %s", i+1, super->m17d.callhistory[9-i]);
    else printw ("\n| ");
  }
  printw ("\n| Reset Call History with (C) key.\n");
  printw ("------------------------------------------------------------------------------\n");

  //color off, back to white
  attron(COLOR_PAIR(6));
}

#endif