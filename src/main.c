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

void usage ()
{
  printf ("\n");
  printf ("Usage: m17-fme [options]            Start the Program\n");
  printf ("  or:  m17-fme -h                   Show Help\n");
  printf ("\n");
}

void cleanupAndExit (config_opts * opts, pa_state * pa)
{
  // Signal that everything should shutdown.
  exitflag = 1;

  //do things before exiting, like closing open devices, etc
  opts->a = 0;
  sprintf (opts->b, "%s", "shutdown");

  if (pa->pa_input_is_open)
    close_pulse_audio_input(pa);

  if (pa->pa_output_rf_is_open)
    close_pulse_audio_output_rf(pa);

  if (pa->pa_output_vx_is_open)
    close_pulse_audio_output_vx(pa);

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

  //declare structures and initialize their elements
  config_opts opts;
  init_config_opts (&opts);

  pa_state pa;
  init_pa_state (&pa);

  m17_decoder_state m17d;
  init_m17d_state(&m17d);

  m17_decoder_state m17e;
  init_m17d_state(&m17e);

  demod_state demod;
  init_demod_state (&demod);

  //initialize convolutional decoder and golay
  convolution_init();
  Golay_24_12_init();

  //set the exitflag to 0
  exitflag = 0;

  //print banner
  for (i = 1; i < 9; i++)
    fprintf (stderr,"%s\n", FM_banner[i]);

  //print git tag and version number
  fprintf (stderr, "Build Version: %s \n", GIT_TAG);

  //process user CLI optargs (try to keep them alphabatized for my personal sanity)
  while ((c = getopt (argc, argv, "a:b:dhnv:")) != -1)
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

      case 'd':
        opts.use_m17_pkt_decoder = 1;
        opts.use_m17_str_decoder = 1;
        fprintf (stderr, "Project M17 RF Audio Stream and Packet Decoder Mode. \n");
        break;

      case 'n':
        opts.use_ncurses_terminal = 1;
        fprintf (stderr, "Ncurses Terminal Mode. \n");
        break;

      case 'v':
        opts.payload_verbosity = atoi(optarg);
        fprintf (stderr, "Payload Verbosity: %d \n", opts.payload_verbosity);
        break;

    }
  }

  //call signal handler so things like ctrl+c will allow us to gracefully close
  signal (SIGINT, handler);
  signal (SIGTERM, handler);

  open_pulse_audio_input (&pa);
  open_pulse_audio_output_rf (&pa);
  open_pulse_audio_output_vx (&pa);

  //call a function to run if contextual
  if (opts.use_m17_str_decoder == 1)
    framesync (&opts, &pa, &m17d, &demod);

  //exit gracefully
  cleanupAndExit (&opts, &pa);

  return (0);
}
