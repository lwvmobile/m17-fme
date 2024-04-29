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

  //assign them with function w/ allocated memory
  char * timestr  = getTimeN(super->demod.current_time); //skip time(NULL) here to avoid cycle usage
  char * datestr  = getDateN(super->demod.current_time); //skip time(NULL) here to avoid cycle usage
  
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
  if (super->opts.payload_verbosity >= 3)
    print_ncurses_scope(super);

  //Print Call History
  print_ncurses_call_history(super);

  //test various time / date strings
  // printw ("TIME: %s; DATE: %s; ", timestr, datestr);
  // printw ("In Level: %2.1f", super->demod.input_level);
  // printw ("\n");

  //Handle Input Keystrokes
  input_ncurses_terminal(super, input_keystroke);

  //refresh the terminal
  refresh();

  //free allocated memory and set ptr to NULL
  free (timestr); timestr = NULL;
  free (datestr); datestr = NULL;
    
}

void print_ncurses_banner (Super * super)
{

  if (super->opts.ncurses_no_banner == 1)
  {
    printw ("------------------------------------------------------------------------------\n");
    printw ("| Project M17: Florida Man Edition %s \n", GIT_TAG);
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
      if (i == 2) printw (" ESC to Menu");
      if (i == 3) printw (" 'q' to Quit ");
      #ifdef USE_CODEC2
      if (i == 6) printw (" CODEC2");
      #endif
      if (i == 7) printw (" %s", GIT_TAG);
      printw ("\n");
    }
    attroff(COLOR_PAIR(6));
  }
  UNUSED(M17FME_banner);
}

void print_ncurses_config (Super * super)
{
  UNUSED(super);
  printw ("--Input Output----------------------------------------------------------------\n");
  printw ("------------------------------------------------------------------------------\n");
}

void print_ncurses_scope (Super * super)
{
  //NOTE: This is just eye candy, it's not real time or anything
  //it also currently prints stale storage values too
  int i;
  printw ("--Symbol Scope----------------------------------------------------------------");
  printw ("\n| +3:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr%65535)-(71-i)] == +3.0f) printw("*"); else printw(" ");
  printw ("\n| +1:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr%65535)-(71-i)] == +1.0f) printw("*"); else printw(" ");
  printw ("\n| -1:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr%65535)-(71-i)] == -1.0f) printw("*"); else printw(" ");
  printw ("\n| -3:"); for (i = 0; i < 72; i++) if (super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr%65535)-(71-i)] == -3.0f) printw("*"); else printw(" ");
  printw ("\n| ");
  //this might actually be semi useful
  printw ("Min: %6.0f; Max: %5.0f; LMid: %6.0f; UMid: %5.0f; Center: %6.0f; In: %2.0f", 
      super->demod.fsk4_min, super->demod.fsk4_max, super->demod.fsk4_lmid, 
      super->demod.fsk4_umid, super->demod.fsk4_center, super->demod.input_level);

  printw ("\n");
  printw ("------------------------------------------------------------------------------\n");
}

void print_ncurses_call_info (Super * super)
{
  //color on or off
  if (super->demod.in_sync == 1)
    attron(COLOR_PAIR(3));
  else attron(COLOR_PAIR(6));

  printw ("--Call Info-------------------------------------------------------------------\n");

  printw ("| ");
  printw ("M17: ");

  //insert data type and frame information
  if (super->m17d.dt == 0) printw("Reserved");
  if (super->m17d.dt == 1) printw("Data ");
  if (super->m17d.dt == 2) printw("Voice (3200) ");
  if (super->m17d.dt == 3) printw("Voice (1600) + Data");

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


  //fill in any extra info, like Meta (IV, etc)
  if (super->m17d.enc_et == 1)
  {
    printw (" Scrambler - Type: %d", super->m17d.enc_st);
  }

  if (super->m17d.enc_et == 2)
  {
    printw (" AES-CTR - IV: ");
    //display packed meta as IV
    for (int i = 0; i < 16; i++)
      printw ("%02X", super->m17d.meta[i]);
  }

  if (super->m17d.enc_et == 3)
  {
    printw (" Reserved Enc - Type: %d", super->m17d.enc_st);
  }

  printw ("\n");

  //color off, back to white
  attron(COLOR_PAIR(6));

  printw ("------------------------------------------------------------------------------\n");
}

void print_ncurses_call_history (Super * super)
{
  UNUSED(super);
  printw ("--Call History----------------------------------------------------------------\n");
  printw ("------------------------------------------------------------------------------\n");
}

#endif