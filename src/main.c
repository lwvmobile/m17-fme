/*-------------------------------------------------------------------------------
 * main.c
 * Project M17 - Florida Man Edition
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#define _MAIN

#include "main.h"
#include "git_ver.h"
#include <signal.h>

//signal handling
volatile uint8_t exitflag;

void handler(int sgnl)
{
  UNUSED(sgnl);
  exitflag = 1;
}

//Banner
char * FM_banner[9] = {
  "                            ",
  "                            ",
  " ███╗   ███╗   ███╗  ███████╗   ███████╗███╗   ███╗███████╗",
  " ████╗ ████║  ████║  ╚════██║   ██╔════╝████╗ ████║██╔════╝",
  " ██╔████╔██║ ██╔██║      ██╔╝   █████╗  ██╔████╔██║█████╗  ",
  " ██║╚██╔╝██║ ╚═╝██║     ██╔╝    ██╔══╝  ██║╚██╔╝██║██╔══╝  ",
  " ██║ ╚═╝ ██║ ███████╗  ██╔╝     ██║     ██║ ╚═╝ ██║███████╗",
  " ╚═╝     ╚═╝ ╚══════╝  ╚═╝      ╚═╝     ╚═╝     ╚═╝╚══════╝",
  "Project M17 - Florida Man Edition                          "
};

//example of initializing struct items
void init_opts (config_opts * opts)
{
  opts->a = 0;
  sprintf (opts->b, "%s", "initialize");
  opts->c[0] = 0;
  memset (opts->d, 0, sizeof(opts->d));
}

void usage ()
{
  printf ("\n");
  printf ("Usage: m17-fme [options]            Start the Program\n");
  printf ("  or:  m17-fme -h                   Show Help\n");
  printf ("\n");
}

void cleanupAndExit (config_opts * opts)
{
  // Signal that everything should shutdown.
  exitflag = 1;

  //do things before exiting, like closing open files, etc
  opts->a = 0;
  sprintf (opts->b, "%s", "shutdown");

  fprintf (stderr, "\n");
  fprintf (stderr,"Exiting.\n");

  exit(0);
}

void function_a (config_opts * opts)
{
  sprintf (opts->b, "%s", "running");
  while (!exitflag)
  {
    //function loop
    
  }
}

int main (int argc, char **argv)
{
  int i, c;
  extern char *optarg;
  extern int optind, opterr, optopt;

  //declare structure and initialize its elements
  config_opts opts;
  init_opts (&opts);

  //set the exitflag to 0
  exitflag = 0;

  //print banner
  for (i = 1; i < 9; i++)
    fprintf (stderr,"%s\n", FM_banner[i]);

  //print git tag and version number
  fprintf (stderr, "Build Version: %s \n", GIT_TAG);

  //process user CLI optargs (options that require arguments have a colon after them)
  while ((c = getopt (argc, argv, "ha:b:")) != -1)
  {
    opterr = 0;
    switch (c)
    {
      case 'h':
        usage ();
        exit (0);
        break;
        
      case 'a':
        opts.a = 1;
        break;

      case 'b':
        strncpy(opts.b, optarg, 1023);
        opts.b[1023] = '\0';
        fprintf (stderr,"B: %s\n", opts.b);
        break;
    }
  }

  //call signal handler so things like ctrl+c will allow us to gracefully close
  signal (SIGINT, handler);
  signal (SIGTERM, handler);

  //call a function to run if contextual
  if (opts.a == 1)
    framesync (&opts);

  //exit gracefully
  cleanupAndExit (&opts);

  return (0);
}
