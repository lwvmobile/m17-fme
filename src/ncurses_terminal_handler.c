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

  char * timestr  = getTime();
  char * datestr  = getDate();
  char * timestrC = getTimeC();
  char * datestrH = getDateH();
  char * timestrN = getTimeN(time(NULL));
  char * datestrN = getDateN((time(NULL)));
  
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

  //free allocated memory
  free (timestr);
  free (datestr);
  free (timestrC);
  free (datestrH);
  free (timestrN);
  free (datestrN);
  
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
  UNUSED(super);
  printw ("--Call Info-------------------------------------------------------------------\n");
  printw ("------------------------------------------------------------------------------\n");
}

void print_ncurses_callhistory (Super * super)
{
  UNUSED(super);
  printw ("--Call History----------------------------------------------------------------\n");
  printw ("------------------------------------------------------------------------------\n");
}

#endif