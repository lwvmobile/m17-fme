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

void cleanup_and_exit (Super * super)
{
  // Signal that everything should shutdown.
  exitflag = 1;

  //do things before exiting, like closing open devices, etc
  // super->opts.a = 0;
  // sprintf (super->opts.b, "%s", "shutdown");

  #ifdef USE_PULSEAUDIO
  if (super->pa.pa_input_is_open)
    close_pulse_audio_input(super);

  if (super->pa.pa_output_rf_is_open)
    close_pulse_audio_output_rf(super);

  if (super->pa.pa_output_vx_is_open)
    close_pulse_audio_output_vx(super);
  #else
  // UNUSED(pa);
  #endif

  if (super->wav.wav_out_rf)
    close_wav_out_rf(super);

  if (super->wav.wav_out_vx)
    close_wav_out_vx(super);

  #ifdef USE_CODEC2
  codec2_destroy(super->m17d.codec2_1600);
  codec2_destroy(super->m17d.codec2_3200);
  codec2_destroy(super->m17e.codec2_1600);
  codec2_destroy(super->m17e.codec2_3200);
  #endif

  if (super->opts.m17_udp_sock)
    close (super->opts.m17_udp_sock);

  if (super->opts.float_symbol_out)
    fclose (super->opts.float_symbol_out);

  fprintf (stderr, "\n");
  fprintf (stderr,"Exiting.\n");

  exit(0);
}

int main (int argc, char **argv)
{
  int i, c;
  extern char *optarg;
  extern int optind, opterr, optopt;

  //The Super Struct with nested structs has replaced passing them around
  //and referencing them directly, much easier when I don't have to backtrack
  //and contantly pass more things around
  Super super;
  init_super(&super);

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
  //NOTE: Try to observe conventions that lower case is decoder, UPPER is ENCODER, numerical 0-9 are for debug related testing
  while ((c = getopt (argc, argv, "12dhimns:v:A:D:F:IM:PS:U:VX")) != -1)
  {
    opterr = 0;
    switch (c)
    {
      case 'h':
        usage ();
        exit (0);
        break;

      //disable high pass filter on digital
      case '1':
        super.opts.use_hpfilter_dig = 0;
        fprintf (stderr, " Disable HPFilter on Digital Voice Decoding. \n");
        break;
      
      //disable RRC Filter
      case '2':
        super.opts.disable_rrc_filter = 1;
        fprintf (stderr, " Disable RRC Filter on RF Audio Encoding / Decoding. \n");
        break;
        
      // case 'a':
      //   super.opts.a = 1;
      //   break;

      // case 'b':
      //   strncpy(super.opts.b, optarg, 1023);
      //   super.opts.b[1023] = '\0';
      //   fprintf (stderr,"B: %s\n", super.opts.b);
      //   break;

      //Enable IP Frame Input (testing using default values)
      case 'i':
        super.opts.use_m17_ipf_decoder = 1;
        fprintf (stderr, "Project M17 Encoder IP Frame Receiver Enabled. \n");
        break;

      case 'd':
        super.opts.use_m17_pkt_decoder = 1;
        super.opts.use_m17_str_decoder = 1;
        fprintf (stderr, "Project M17 RF Audio Stream and Packet Decoder Mode. \n");
        break;

      case 'm':
        super.opts.monitor_encode_internally = 1;
        fprintf (stderr, "Internal Encoder Loopback to Decoder. \n");
        break;

      case 'n':
        super.opts.use_ncurses_terminal = 1;
        fprintf (stderr, "Ncurses Terminal Mode. \n");
        break;

      case 's':
        super.demod.input_sql = atoi(optarg);
        fprintf (stderr, "Input Squelch: %ld; \n", super.demod.input_sql);
        break;

      case 'v':
        super.opts.payload_verbosity = atoi(optarg);
        fprintf (stderr, "Payload Verbosity: %d; \n", super.opts.payload_verbosity);
        break;

      //Specify M17 STR Encoder Arbitrary Data For 1600
      case 'A':
        strncpy(super.m17e.arb, optarg, 772);
        super.m17e.arb[772] = '\0';
        super.opts.m17_str_encoder_dt = 3;
        break;

      //Specify M17 PKT Encoder Raw Encoded Data Packet (truncates at 772)
      case 'D':
        strncpy(super.m17e.dat, optarg, 772);
        super.m17e.dat[772] = '\0';
        break;

      //Specify M17 RF Float Symbol Output (For M17_Implementations PKT Decoder)
      case 'F':
        strncpy(super.opts.float_symbol_output_file, optarg, 1023);
        super.opts.float_symbol_output_file[1023] = '\0';
        super.opts.use_float_symbol_output = 1;
        fprintf (stderr, "Float Symbol Output File: %s \n", super.opts.float_symbol_output_file);
        break;

      //Enable IP Frame Output
      case 'I':
        super.opts.m17_use_ip = 1;
        fprintf (stderr, "Project M17 Encoder IP Frame Enabled. \n");
        break;

      //Specify Encoder CAN, SRC, and DST Callsign Data
      case 'M':
        strncpy(super.m17e.user, optarg, 49);
        super.m17e.user[49] = '\0';
        break;

      //Enable the PKT Encoder
      case 'P':
        super.opts.use_m17_pkt_encoder = 1;
        fprintf (stderr, "Project M17 Packet Encoder. \n");
        break;

      //Specify M17 PKT Encoder SMS Message (truncates at 772)
      case 'S':
        strncpy(super.m17e.sms, optarg, 772);
        super.m17e.sms[772] = '\0';
        break;

      //Specify M17 UDP Frame String Format, i.e., 'localhost:17000' or 'mycustomhost.xyz:17001'
      case 'U':
        strncpy(super.opts.m17_udp_input, optarg, 1024);
        super.opts.m17_udp_input[1024] = '\0';
        break;

      //Enable the Stream Voice Encoder
      case 'V':
        super.opts.use_m17_str_encoder = 1;
        fprintf (stderr, "Project M17 Stream Voice Encoder. \n");
        break;

      case 'X':
        super.m17e.str_encoder_vox = 1;
        fprintf (stderr, "Project M17 Stream Voice Encoder TX on Vox. \n");
        break;

    }
  }

  //call signal handler so things like ctrl+c will allow us to gracefully close
  signal (SIGINT, handler);
  signal (SIGTERM, handler);

  #ifdef USE_PULSEAUDIO
  open_pulse_audio_input (&super);
  open_pulse_audio_output_rf (&super);
  open_pulse_audio_output_vx (&super);
  #endif

  //open float symbol output file, if needed.
  if (super.opts.use_float_symbol_output)
    super.opts.float_symbol_out = fopen (super.opts.float_symbol_output_file, "w");

  open_wav_out_rf(&super);
  open_wav_out_vx(&super);
  // open_stdout_pipe(&super); //works

  //Parse any User Input Strings that need to be broken into smaller components UDP and USER CSD
  char * curr;
  if (super.opts.m17_udp_input[0] != 0 && super.opts.m17_use_ip == 1)
  {
    curr = strtok(super.opts.m17_udp_input, ":");
    if (curr != NULL)
      strncpy (super.opts.m17_hostname, curr, 1023);
    curr = strtok(NULL, ":"); //host port
      if (curr != NULL) super.opts.m17_portno = atoi (curr);

    fprintf (stderr, "UDP Host: %s; ", super.opts.m17_hostname);
    fprintf (stderr, "Port: %d \n", super.opts.m17_portno);
  }

  if (super.m17e.user[0] != 0)
  {
    //check and capatalize any letters in the CSD
    for (int i = 0; super.m17e.user[i]!='\0'; i++)
    {
      if(super.m17e.user[i] >= 'a' && super.m17e.user[i] <= 'z')
        super.m17e.user[i] = super.m17e.user[i] -32;
    }

    curr = strtok(super.m17e.user, ":"); //CAN
    if (curr != NULL)
      super.m17e.can = atoi(curr);

    curr = strtok(NULL, ":"); //m17 src callsign
    if (curr != NULL)
    {
      strncpy (super.m17e.srcs, curr, 9); //only read first 9
      super.m17e.srcs[9] = '\0';
    }

    curr = strtok(NULL, ":"); //m17 dst callsign
    if (curr != NULL)
    {
      strncpy (super.m17e.dsts, curr, 9); //only read first 9
      super.m17e.dsts[9] = '\0';
    }

    fprintf (stderr, "M17 User Data: ");
    fprintf (stderr, "CAN: %d; ", super.m17e.can);
    fprintf (stderr, "SRC: %s; ", super.m17e.srcs);
    fprintf (stderr, "DST: %s; ", super.m17e.dsts);
  }

  //call a function to run if contextual
  if (super.opts.use_m17_str_decoder == 1)
    framesync (&super);

  if (super.opts.use_m17_pkt_encoder == 1)
    encodeM17PKT(&super);

  if (super.opts.use_m17_str_encoder == 1)
    encodeM17STR(&super);

  if (super.opts.use_m17_ipf_decoder == 1)
    decode_ipf(&super);

  //exit gracefully
  cleanup_and_exit (&super);

  return (0);
}
