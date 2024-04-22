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

//Basic Banner
char * FM_banner[9] = {
  "                                                           ",
  "                                                           ",
  " ███╗   ███╗   ███╗  ███████╗   ███████╗███╗   ███╗███████╗",
  " ████╗ ████║  ████║  ╚════██║   ██╔════╝████╗ ████║██╔════╝",
  " ██╔████╔██║ ██╔██║      ██╔╝   █████╗  ██╔████╔██║█████╗  ",
  " ██║╚██╔╝██║ ╚═╝██║     ██╔╝    ██╔══╝  ██║╚██╔╝██║██╔══╝  ",
  " ██║ ╚═╝ ██║ ███████╗  ██╔╝     ██║     ██║ ╚═╝ ██║███████╗",
  " ╚═╝     ╚═╝ ╚══════╝  ╚═╝      ╚═╝     ╚═╝     ╚═╝╚══════╝",
  "Project M17 - Florida Man Edition                          "
};

//Color Segmented Banner
char * M_banner[9] = {
  "             ",
  "             ",
  " ███╗   ███╗ ",
  " ████╗ ████║ ",
  " ██╔████╔██║ ",
  " ██║╚██╔╝██║ ",
  " ██║ ╚═╝ ██║ ",
  " ╚═╝     ╚═╝ ",
  "             "
};

char * S_banner[9] = {
  "                    ",
  "                    ",
  "    ███╗  ███████╗  ",
  "   ████║  ╚════██║  ",
  "  ██╔██║      ██╔╝  ",
  "  ╚═╝██║     ██╔╝   ",
  "  ███████╗  ██╔╝    ",
  "  ╚══════╝  ╚═╝     ",
  "                    "
};

char * FME_banner[9] = {
  "                             ",
  "                             ",
  " ███████╗███╗   ███╗███████╗ ",
  " ██╔════╝████╗ ████║██╔════╝ ",
  " █████╗  ██╔████╔██║█████╗   ",
  " ██╔══╝  ██║╚██╔╝██║██╔══╝   ",
  " ██║     ██║ ╚═╝ ██║███████╗ ",
  " ╚═╝     ╚═╝     ╚═╝╚══════╝ ",
  "                             "
};

void usage ()
{
  printf ("\n");
  printf ("Usage: m17-fme [options]            Start the Program\n");
  printf ("  or:  m17-fme -h                   Show Help\n");
  printf ("\n");
}

void cleanup_and_exit (config_opts * opts, pa_state * pa, wav_state * wav, m17_decoder_state * m17d, m17_encoder_state * m17e)
{
  // Signal that everything should shutdown.
  exitflag = 1;

  //do things before exiting, like closing open devices, etc
  opts->a = 0;
  sprintf (opts->b, "%s", "shutdown");

  #ifdef USE_PULSEAUDIO
  if (pa->pa_input_is_open)
    close_pulse_audio_input(pa);

  if (pa->pa_output_rf_is_open)
    close_pulse_audio_output_rf(pa);

  if (pa->pa_output_vx_is_open)
    close_pulse_audio_output_vx(pa);
  #else
  UNUSED(pa);
  #endif

  if (wav->wav_out_rf)
    close_wav_out_rf(wav);

  if (wav->wav_out_vx)
    close_wav_out_vx(wav);

  #ifdef USE_CODEC2
  codec2_destroy(m17d->codec2_1600);
  codec2_destroy(m17d->codec2_3200);
  codec2_destroy(m17e->codec2_1600);
  codec2_destroy(m17e->codec2_3200);
  #endif

  if (opts->m17_udp_sock)
    close (opts->m17_udp_sock);

  if (opts->float_symbol_out)
    fclose (opts->float_symbol_out);

  fprintf (stderr, "\n");
  fprintf (stderr,"Exiting.\n");

  exit(0);
}

int main (int argc, char **argv)
{
  int i, c;
  extern char *optarg;
  extern int optind, opterr, optopt;

  //declare structures and initialize their elements
  config_opts opts;
  init_config_opts(&opts);

  pa_state pa;
  init_pa_state(&pa);

  m17_decoder_state m17d;
  init_m17d_state(&m17d);

  m17_encoder_state m17e;
  init_m17e_state(&m17e);

  demod_state demod;
  init_demod_state(&demod);

  wav_state wav;
  init_wav_state(&wav);

  HPFilter hpf_d;
  HPFilter hpf_a;
  //do I have these backwards in DSD-FME, just oing to set them up as it is there for now
  // HPFilter_Init(HPFilter *filter, float cutoffFreqHz, float sampleTimeS)
  // HPFilter_Init(HPFilter *filter, float cutoffFreqHz, float sampleTimeS)
  HPFilter_Init (&hpf_a, 960, (float)1/(float)48000);
  HPFilter_Init (&hpf_d, 960, (float)1/(float)48000);

  //initialize convolutional decoder and golay
  convolution_init();
  Golay_24_12_init();

  //set the exitflag to 0
  exitflag = 0;

  #ifdef PRETTY_COLORS
  //print pretty color banner
  for (i = 1; i < 8; i++)
  {
    fprintf (stderr, "%s", BWHT); //white background
    fprintf (stderr, "%s", KBLK); //black text
    fprintf (stderr, "%s", M_banner[i]);
    
    fprintf (stderr, "%s", KRED); //red text
    fprintf (stderr, "%s", S_banner[i]);

    fprintf (stderr, "%s", KBLU); //blue text
    fprintf (stderr, "%s", FME_banner[i]);

    fprintf (stderr, "%s", BNRM); //normal background for this terminal
    fprintf (stderr, "%s", KNRM); //normal font for this terminal
    fprintf (stderr, "\n"); //line break
  }
  fprintf (stderr, "%s\n", FM_banner[8]);
  fprintf (stderr, "%s", KNRM); //normal font for this terminal
  #else
  //print basic banner
  for (i = 1; i < 9; i++)
    fprintf (stderr,"%s\n", FM_banner[i]);
  #endif

  //print git tag and version number
  fprintf (stderr, "Build Version: %s \n", GIT_TAG);

  //process user CLI optargs (try to keep them alphabatized for my personal sanity)
  //NOTE: Try to observe conventions that lower case is decoder, UPPER is ENCODER
  while ((c = getopt (argc, argv, "a:b:dhnv:A:D:F:IPM:S:U:V")) != -1)
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

      //Specify M17 STR Encoder Arbitrary Data For 1600
      case 'A':
        strncpy(m17e.arb, optarg, 772);
        m17e.arb[772] = '\0';
        opts.m17_str_encoder_dt = 3;
        break;

      //Specify M17 PKT Encoder Raw Encoded Data Packet (truncates at 772)
      case 'D':
        strncpy(m17e.dat, optarg, 772);
        m17e.dat[772] = '\0';
        break;

      //Specify M17 RF Float Symbol Output (For M17_Implementations PKT Decoder)
      case 'F':
        strncpy(opts.float_symbol_output_file, optarg, 1023);
        opts.float_symbol_output_file[1023] = '\0';
        opts.use_float_symbol_output = 1;
        fprintf (stderr, "Float Symbol Output File: %s \n", opts.float_symbol_output_file);
        break;

      //Enable IP Frame Output
      case 'I':
        opts.m17_use_ip = 1;
        fprintf (stderr, "Project M17 Encoder IP Frame Enabled. \n");
        break;

      //Enable the PKT Encoder
      case 'P':
        opts.use_m17_pkt_encoder = 1;
        fprintf (stderr, "Project M17 Packet Encoder. \n");
        break;

      //Specify Encoder CAN, SRC, and DST Callsign Data
      case 'M':
        strncpy(m17e.user, optarg, 49);
        m17e.user[49] = '\0';
        break;

      //Specify M17 PKT Encoder SMS Message (truncates at 772)
      case 'S':
        strncpy(m17e.sms, optarg, 772);
        m17e.sms[772] = '\0';
        break;

      //Specify M17 UDP Frame String Format, i.e., 'localhost:17000' or 'mycustomhost.xyz:17001'
      case 'U':
        strncpy(opts.m17_udp_input, optarg, 1024);
        opts.m17_udp_input[1024] = '\0';
        break;

      //Enable the Stream Voice Encoder
      case 'V':
        opts.use_m17_str_encoder = 1;
        fprintf (stderr, "Project M17 Stream Voice Encoder. \n");
        break;

    }
  }

  //call signal handler so things like ctrl+c will allow us to gracefully close
  signal (SIGINT, handler);
  signal (SIGTERM, handler);

  #ifdef USE_PULSEAUDIO
  open_pulse_audio_input (&pa);
  open_pulse_audio_output_rf (&pa);
  open_pulse_audio_output_vx (&pa);
  #endif

  //open float symbol output file, if needed.
  if (opts.use_float_symbol_output)
    opts.float_symbol_out = fopen (opts.float_symbol_output_file, "w");

  open_wav_out_rf(&wav);
  // open_stdout_pipe(&opts); //works

  //Parse any User Input Strings that need to be broken into smaller components UDP and USER CSD
  char * curr;
  if (opts.m17_udp_input[0] != 0 && opts.m17_use_ip == 1)
  {
    curr = strtok(opts.m17_udp_input, ":");
    if (curr != NULL)
      strncpy (opts.m17_hostname, curr, 1023);
    curr = strtok(NULL, ":"); //host port
      if (curr != NULL) opts.m17_portno = atoi (curr);

    fprintf (stderr, "UDP Host: %s; ", opts.m17_hostname);
    fprintf (stderr, "Port: %d \n", opts.m17_portno);
  }

  if (m17e.user[0] != 0)
  {
    //check and capatalize any letters in the CSD
    for (int i = 0; m17e.user[i]!='\0'; i++)
    {
      if(m17e.user[i] >= 'a' && m17e.user[i] <= 'z')
        m17e.user[i] = m17e.user[i] -32;
    }

    curr = strtok(m17e.user, ":"); //CAN
    if (curr != NULL)
      m17e.can = atoi(curr);

    curr = strtok(NULL, ":"); //m17 src callsign
    if (curr != NULL)
    {
      strncpy (m17e.srcs, curr, 9); //only read first 9
      m17e.srcs[9] = '\0';
    }

    curr = strtok(NULL, ":"); //m17 dst callsign
    if (curr != NULL)
    {
      strncpy (m17e.dsts, curr, 9); //only read first 9
      m17e.dsts[9] = '\0';
    }

    fprintf (stderr, "M17 User Data: ");
    fprintf (stderr, "CAN: %d; ", m17e.can);
    fprintf (stderr, "SRC: %s; ", m17e.srcs);
    fprintf (stderr, "DST: %s; ", m17e.dsts);
  }

  //call a function to run if contextual
  if (opts.use_m17_str_decoder == 1)
    framesync (&opts, &pa, &m17d, &demod);

  if (opts.use_m17_pkt_encoder == 1)
    encodeM17PKT(&opts, &pa, &wav, &m17e, &m17d);

  if (opts.use_m17_str_encoder == 1)
    encodeM17STR(&opts, &pa, &wav, &m17e, &m17d);

  //exit gracefully
  cleanup_and_exit (&opts, &pa, &wav, &m17d, &m17e);

  return (0);
}
