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

  //assign ptrs to NULL
  char * timestr  = NULL;
  char * datestr  = NULL;
  char * timestrC = NULL;
  char * datestrH = NULL;
  char * timestrN = NULL;
  char * datestrN = NULL;

  //assign them with function w/ allocated memory
  timestr  = getTime();
  datestr  = getDate();
  timestrC = getTimeC();
  datestrH = getDateH();
  timestrN = getTimeN(time(NULL));
  datestrN = getDateN((time(NULL)));
  
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
  print_ncurses_callinfo(super);

  //Print Call History
  print_ncurses_callhistory(super);

  //test various time / date strings
  printw ("TIME: %s; DATE: %s; ", timestr, datestr);
  printw ("\n");
  printw ("TIME: %s; DATE: %s; ", timestrC, datestrH);
  printw ("\n");
  printw ("TIME: %s; DATE: %s; ", timestrN, datestrN);

  //Handle Input Keystrokes
  input_ncurses_terminal(super, input_keystroke);

  //refresh the terminal
  refresh();

  //free allocated memory and set ptr to NULL
  if (timestr != NULL)
  {
    free (timestr);
    timestr = NULL;
  }
  if (datestr != NULL)
  {
    free (datestr);
    datestr = NULL;
  }
  if (timestrC != NULL)
  {
    free (timestrC);
    timestrC = NULL;
  }
  if (datestrH != NULL)
  {
    free (datestrH);
    datestrH = NULL;
  }
  if (timestrN != NULL)
  {
    free (timestrN);
    timestrN = NULL;
  }
  if (datestrN != NULL)
  {
    free (datestrN);
    datestrN = NULL;
  }
    
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

void print_ncurses_callinfo (Super * super)
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

  printw ("CAN: %02d ", super->m17d.can);

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

void print_ncurses_callhistory (Super * super)
{
  UNUSED(super);
  printw ("--Call History----------------------------------------------------------------\n");
  printw ("------------------------------------------------------------------------------\n");
}

#endif