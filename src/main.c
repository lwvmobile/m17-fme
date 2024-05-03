/*-------------------------------------------------------------------------------
 * main.c
 * Project M17 - Florida Man Edition
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#define _MAIN

#include "main.h"
#include "git_ver.h"
#include "banner.h"
#include <signal.h>

//signal handling
volatile uint8_t exitflag;

void handler(int sgnl)
{
  UNUSED(sgnl);
  exitflag = 1;
}

void usage ()
{
  printf ("\n");
  printf ("Usage: m17-fme [options]            Start the Program\n");
  printf ("  or:  m17-fme -h                   Show Help\n");
  printf ("\n");
}

int main (int argc, char **argv)
{
  int i, c;
  extern char *optarg;

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
  fprintf (stderr, "%s\n", M17FME_banner[8]);
  fprintf (stderr, "%s", KNRM); //normal font for this terminal
  #else
  //print basic banner
  for (i = 1; i < 9; i++)
    fprintf (stderr,"%s\n", M17FME_banner[i]);
  #endif

  //print git tag and version number
  fprintf (stderr, "Build Version: %s \n", GIT_TAG);
  
  //print current session number
  fprintf (stderr, "Session Number: %04X \n", super.opts.random_number);

  //use i to count number of optargs parsed
  i = 0;

  //process user CLI optargs (try to keep them alphabatized for my personal sanity)
  //NOTE: Try to observe conventions that lower case is decoder, UPPER is ENCODER, numerical 0-9 are for debug related testing
  while ((c = getopt (argc, argv, "12345678c:df:hi:mno:s:t:u:v:w:xA:C:D:F:INM:PS:U:VX")) != -1)
  {

    i++;

    switch (c)
    {
      case 'h':
        usage ();
        exit (0);
        break;

      //disable high pass filter on digital
      case '1':
        super.opts.use_hpfilter_dig = 0;
        fprintf (stderr, "Disable HPFilter on Digital Voice Decoding. \n");
        break;
      
      //disable RRC Filter
      case '2':
        super.opts.disable_rrc_filter = 1;
        fprintf (stderr, "Disable RRC Filter on RF Audio Encoding / Decoding. \n");
        break;

      //connect to TCP Socket for SND Input with default options
      case '3':
        super.opts.use_tcp_input = 1;
        fprintf (stderr, "TCP Source Connect Debug (Default Options). \n");
        break;

      //connect to PA Server for Pulse Audio Input
      case '4':
        super.opts.use_pa_input = 1;
        fprintf (stderr, "Pulse Audio Input Debug (Default Options). \n");
        break;

      //connect to STDIN for SND Input with default options
      case '5':
        super.opts.use_stdin_input = 1;
        fprintf (stderr, "STDIN SND Audio Input Debug (Default Options). \n");
        break;

      //connect to PA Server for Pulse Audio Output (RF and VX)
      case '6':
        super.opts.use_pa_output_rf = 1;
        super.opts.use_pa_output_vx = 1;
        fprintf (stderr, "Pulse Audio Output RF and VX Debug (Default Options). \n");
        break;

      //connect to OSS device for input
      case '7':
        super.opts.use_oss_input = 1;
        fprintf (stderr, "OSS Input Debug (Default Options). \n");
        break;

      //connect to OSS device for output
      case '8':
        super.opts.use_oss_output = 1;
        fprintf (stderr, "OSS Output Debug (Default Options). \n");
        break;
        
      // case 'a':
      //   super.opts.a = 1;
      //   break;

      // case 'b':
      //   strncpy(super.opts.b, optarg, 1023);
      //   super.opts.b[1023] = '\0';
      //   fprintf (stderr,"B: %s\n", super.opts.b);
      //   break;


      case 'd':
        super.opts.use_m17_rfa_decoder = 1;
        fprintf (stderr, "Project M17 RF Audio Stream and Packet Decoder Mode. \n");
        break;

      //Specify DSD-FME Dibit Capture Bin Input File Format (RF Encoded only)
      case 'c':
        strncpy(super.opts.dibit_input_file, optarg, 1023);
        super.opts.dibit_input_file[1023] = '\0';
        super.opts.use_dibit_input = 1;
        fprintf (stderr, "DSD-FME Dibit Input File: %s \n", super.opts.dibit_input_file);
        break;

      //Specify M17 Float Symbol Input
      case 'f':
        strncpy(super.opts.float_symbol_input_file, optarg, 1023);
        super.opts.float_symbol_input_file[1023] = '\0';
        super.opts.use_float_symbol_input = 1;
        fprintf (stderr, "Float Symbol Input File: %s \n", super.opts.float_symbol_input_file);
        break;

      //Read Input String to be parsed
      case 'i':
        strncpy(super.opts.input_handler_string, optarg, 2047);
        super.opts.input_handler_string[2047] = '\0';
        fprintf (stderr, "Input String: %s \n", super.opts.input_handler_string);
        break;

      //Read Output String to be parsed
      case 'o':
        strncpy(super.opts.output_handler_string, optarg, 2047);
        super.opts.output_handler_string[2047] = '\0';
        fprintf (stderr, "Output String: %s \n", super.opts.output_handler_string);
        break;

      case 'm':
        super.opts.monitor_encode_internally = 1;
        fprintf (stderr, "Internal Encoder Loopback to Decoder. \n");
        break;

      case 'N':
      case 'n':
        #ifdef USE_CURSES
        super.opts.use_ncurses_terminal = 1;
        fprintf (stderr, "Ncurses Terminal Mode. \n");
        #else
        fprintf (stderr, "Ncurses Support Not Compiled. \n");
        #endif
        break;

      case 's':
        super.demod.input_sql = atoi(optarg);
        fprintf (stderr, "Input Squelch: %ld; \n", super.demod.input_sql);
        break;

      case 't':
        super.opts.demod_verbosity = atoi(optarg);
        fprintf (stderr, "Demodulator Verbosity: %d; \n", super.opts.demod_verbosity);
        break;

      //Enable UDP IP Frame Input (testing using default values)
      case 'u':
        super.opts.use_m17_ipf_decoder = 1;
        fprintf (stderr, "Project M17 Encoder UDP IP Frame Receiver Enabled. \n");
        break;

      case 'v':
        super.opts.payload_verbosity = atoi(optarg);
        fprintf (stderr, "Payload Verbosity: %d; \n", super.opts.payload_verbosity);
        break;

      //Specify SND Input File (.wav, .rrc, etc)
      case 'w':
        strncpy(super.snd_src_in.snd_in_filename, optarg, 1023);
        super.snd_src_in.snd_in_filename[1023] = '\0';
        super.opts.snd_input_is_a_file = 1;
        fprintf (stderr, "SNDFile (.wav, .rrc) Input File: %s \n", super.snd_src_in.snd_in_filename);
        break;

      //Expect Inverted Signal Input
      case 'x':
        super.opts.inverted_signal = 1;
        fprintf (stderr, "Expecting Inverted M17 Signal Input \n");
        break;

      //Specify M17 STR Encoder Arbitrary Data For 1600
      case 'A':
        strncpy(super.m17e.arb, optarg, 772);
        super.m17e.arb[772] = '\0';
        super.opts.m17_str_encoder_dt = 3;
        break;

      //Specify DSD-FME Dibit Capture Bin File Format (RF Encoded only)
      case 'C':
        strncpy(super.opts.dibit_output_file, optarg, 1023);
        super.opts.dibit_output_file[1023] = '\0';
        super.opts.use_dibit_output = 1;
        fprintf (stderr, "DSD-FME Dibit Output File: %s \n", super.opts.dibit_output_file);
        break;

      //Specify M17 PKT Encoder Raw Encoded Data Packet (truncates at 772)
      case 'D':
        strncpy(super.m17e.dat, optarg, 772);
        super.m17e.dat[772] = '\0';
        break;

      //Specify M17 Float Symbol Output
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

  //enable default starting state if no optargs parsed
  if (i == 0)
    enable_default_state(&super);

  #ifdef USE_CURSES
  if (super.opts.use_ncurses_terminal == 1)
  {
    open_ncurses_terminal();
    super.opts.ncurses_is_open = 1;
  }
  #endif

  //parse any user passed input or output strings
  if (super.opts.input_handler_string[0] != 0)
    parse_input_option_string(&super, super.opts.input_handler_string);
  if (super.opts.output_handler_string[0] != 0)
    parse_output_option_string(&super, super.opts.output_handler_string);

  open_audio_input (&super);
  open_audio_output (&super);

  //Parse any User Input Strings that need to be broken into smaller components UDP and USER CSD
  char * curr; // = malloc (1024*sizeof(char));
  if (super.opts.m17_udp_input[0] != 0 && super.opts.m17_use_ip == 1)
  {
    //debug
    // fprintf (stderr, "UDP INPUT STR: %s\n", super.opts.m17_udp_input);

    curr = strtok(super.opts.m17_udp_input, ":");
    if (curr != NULL)
      strncpy (super.opts.m17_hostname, curr, 1023);
    curr = strtok(NULL, ":"); //host port
      if (curr != NULL) super.opts.m17_portno = atoi (curr);

    curr = strtok(NULL, ":"); //reflector module
    if (curr != NULL)
    {
      //read reflector module
      super.m17e.reflector_module = *curr; //straight assignment to convert char 'A' to number

      //debug char to number
      // fprintf (stderr, "%d:%d:%d; ", super.m17e.reflector_module, *curr, 'A');

      //check for capitalization
      if(super.m17e.reflector_module >= 'a' && super.m17e.reflector_module <= 'z')
        super.m17e.reflector_module = super.m17e.reflector_module -32;
      
      //make sure its a value from A to Z
      if (super.m17e.reflector_module < 0x41 || super.m17e.reflector_module > 0x5A)
      {
        super.m17e.reflector_module = 0;
        fprintf (stderr, "M17 Reflector Module must be value from A-Z; \n");
      }
    }

    fprintf (stderr, "UDP Host: %s; ", super.opts.m17_hostname);
    fprintf (stderr, "Port: %d; ", super.opts.m17_portno);
    if (super.m17e.reflector_module != 0)
      fprintf (stderr, "Module: %c; ", super.m17e.reflector_module);

    fprintf (stderr, "\n");

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

  //demodulate, frame sync, and decode OTA RF Audio
  if (super.opts.use_m17_rfa_decoder == 1)
  {
    //initial current_time set
    super.demod.current_time = time(NULL);

    while (!exitflag)
    {
      //refresh ncurses printer, if enabled
      #ifdef USE_CURSES
      if (super.opts.use_ncurses_terminal == 1)
        print_ncurses_terminal(&super);
      #endif

      //look for framesync
      fsk4_framesync (&super);

      //extra verbosity debug info dump
      if (super.opts.payload_verbosity >= 3)
        print_debug_information(&super);

      //calculate sync time_delta for when to reset carrier sync ptrs and demod/decode values
      super.demod.current_time = time(NULL);
      time_t time_delta = super.demod.current_time - super.demod.sync_time;

      //no carrier sync if we were in sync and time_delta is equal to or more than 1 second
      if (super.demod.in_sync == 1 && time_delta > 0)
        no_carrier_sync(&super);
    }
  }

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
