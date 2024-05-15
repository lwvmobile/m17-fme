/*-------------------------------------------------------------------------------
 * main.c
 * M17 Project - Florida Man Edition
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
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
  printf ("Usage: m17-fme [options]    Start the Program\n");
  printf ("  or:  m17-fme -h           Show Help\n");
  printf ("\n");
  printf ("Display Options:\n");
  printf ("\n");
  printf ("  -N            Use NCurses Terminal\n");
  printf ("                 m17-fme -N 2> log.txt \n");
  printf ("  -v <num>      Payload Verbosity Level\n");
  printf ("  -d <num>      Demodulator Verbosity Level\n");
  printf ("\n");
  printf ("Input Options:\n");
  printf ("\n");
  printf ("  -i <device>   Audio input device (default is pulserf)\n");
  printf ("                pulserf for pulse audio RFA input \n");
  printf ("                pulsevx for pulse audio Voice / Mic input\n");
  printf ("                - for STDIN input (specify encoder or decoder options below)\n");
  printf ("                (Note: When using STDIN, Ncurses Keyboard Shortcuts Disabled)\n");
  #ifdef __CYGWIN__
  printf ("                (pulse audio options will require pactl running in Cygwin)\n");
  #else
  printf ("                (padsp wrapper required for OSS audio on Linux)\n");
  #endif
  printf ("                /dev/dsp for OSS audio\n");
  printf ("                udp for UDP Frame Input (default localhost:17000)\n");
  printf ("                udp:192.168.7.8:17001 for M17 UDP/IP bind input (Binding Address and Port)\n");
  printf ("                m17udp:192.168.7.8:17001 for M17 UDP/IP bind input (Binding Address and Port)\n");
  printf ("  -w <file>     48k/1 SNDFile Compatible RF Audio .wav or .rrc input file\n");
  printf ("  -c <file>     DSD-FME Compatible Dibit/Symbol Capture Bin input file (from RF Encoder)\n");
  printf ("  -f <file>     Float Symbol input file (from RF Encoder and M17_Implementations)\n");
  printf ("\n");
  printf ("Output Options:\n");
  printf ("\n");
  printf ("  -o <device>   Audio output device (default is pulsevx)\n");
  printf ("                pulserf for pulse audio RFA output\n");
  printf ("                pulsevx for pulse audio Voice / Loopback output\n");
  printf ("                - for STDOUT output (specify encoder or decoder options below)\n");
  printf ("                (Note: Don't use Ncurses Terminal w/ STDOUT enabled)\n");
  #ifdef __CYGWIN__
  printf ("                (pulse audio options will require pactl running in Cygwin)\n");
  #else
  printf ("                (padsp wrapper required for OSS audio on Linux)\n");
  #endif
  printf ("                /dev/dsp for OSS audio\n");
  printf ("                (OSS Can only do either RF output, or VX output,\n");
  printf ("                 not both at the same time, specify encoder and decoder options below)\n");
  printf ("                udp for UDP Frame Output (default localhost:17000)\n");
  printf ("                udp:192.168.7.8:17001 for M17 UDP/IP blaster output (Target Address and Port)\n");
  printf ("                m17udp:192.168.7.8:17001 for M17 UDP/IP blaster output (Target Address and Port)\n");
  printf ("  -W <file>     48k/1 SNDFile Compatible RF Audio .wav output file\n");
  printf ("  -C <file>     DSD-FME Compatible Dibit/Symbol Capture Bin output file\n");
  printf ("  -F <file>     Float Symbol output file (M17_Implementations Compatible)\n");
  printf ("\n");
  printf ("Encoder Options:\n");
  printf ("\n");
  printf ("  -V            Enable the Stream Voice Encoder\n");
  printf ("  -P            Enable the Packet Data  Encoder\n");
  printf ("  -I            Enable IP Frame Output with defaults (can be combined with Loopback or RFA output)\n");
  printf ("                (can be combined with Loopback or RFA output)\n");
  printf ("  -L            Enable Internal Encoder Loopback Decoder (must be used with pulsevx output)\n");
  printf ("  -X            Enable Voice Activated TX (Vox) on Stream Voice Encoder\n");
  printf ("\n");
  printf ("Encoder Input Strings:\n");
  printf ("\n");
  printf ("  -M <str>      Set M17 CAN:SRC:DST \n");
  printf ("                (example: -M 1:N0CALL:SP5WWP) \n");
  printf ("  -U <str>      Set UDP/IP Frame HOST:PORT:MODULE \n");
  printf ("                (example: -U 127.0.0.1:17001:B) \n");
  printf ("  -S <str>      Enter SMS Message (up to 772 UTF-8 characters) for Packet Data Encoder\n");
  printf ("                (example: -S 'Hello World! This is a text message') \n");
  printf ("  -A <str>      Enter SMS Message (Up to 48 UTF-8 characters) For Stream Voice Encoder (Arbitrary Data). Enables 1600 mode.\n");
  printf ("                (example: -A 'Hello World! This is arbitrary data on 1600') \n");
  printf ("  -R <hex>      Enter RAW Data for Packet Data Encoder as Hex Octets.\n");
  printf ("                (example: -R 010203040506070809) \n");
  printf ("  -x            Encode Inverted Polarity on RF Output\n");
  printf ("\n");
  printf ("Decoder Options:\n");
  printf ("\n");
  printf ("  -r            Enable RFA Demodulator and Decoding of Stream and Packet Data\n");
  printf ("  -x            Expect Inverted Polarity on RF Input\n");
  printf ("  -m            Enable Analog / Raw Input Signal Monitor on RF Input (when no sync)\n");
  printf ("  -l            Enable Event Log File: date_time_m17fme_eventlog.txt\n");
  printf ("  -u            Enable UDP IP Frame Decoder and Connect to default localhost:17000 \n");
  printf ("  -p            Per Call decoded voice wav file saving into current directory ./M17WAV folder\n");
  printf ("\n");
  printf ("Encryption Options:\n");
  printf ("\n");
  printf ("                (NOTE: Encoder and Decoder share same values here)\n");
  printf ("  -e <hex>      Enter Scrambler Key Value (up to 24-bit / 6 Hex Character)\n");
  printf ("                (example: -e ABCDEF)\n");
  printf ("  -E <hex str>  Enter AES Key Value (in single quote, space every 16 chars) \n");
  printf ("                (example: -E '0520C1B0220AFBCA 16FB1330764B26EC 5C34A197764C147A 15FBA7515ED8BCFC')\n");
  printf ("                (example: -E '0520C1B0220AFBCA 16FB1330764B26EC')\n");
  printf ("                (NOTE: Due to bug in m17-tools handling of AES keys, all keys are run as AES-128)\n");
  printf ("                (Limiting significant key value to first 32 characters to maintain compatibility)\n");
  printf ("\n");
  printf ("Debug Options:\n");
  printf ("\n");
  printf ("  -1            Generate Random One Time Use 24-bit Scrambler Key \n");
  printf ("  -2            Generate Random One Time Use 256-bit AES Key. \n");
  printf ("  -4            Permit Data Decoding on CRC Failure (not recommended). \n");
  printf ("  -6            Open All Pulse Input / Output and IP Frame Defaults and Send Voice Stream. (Fire Everything!). \n");
  printf ("  -7            Disable Symbol Timing Correction. \n");
  printf ("  -8            Disable High Pass Filter on CODEC2 Output. \n");
  printf ("  -9            Enable  RRC Filter on RF Audio Encoding / Decoding. \n");
  printf ("  -0            Disable RRC Filter on RF Audio Encoding / Decoding. \n");
  printf ("\n");
  printf ("Quick Examples:\n");
  printf ("\n");
  printf (" Stream Voice Encoder with Mic Input (pulsevx) RF Output (pulserf), float symbol file output (float.sym) \n");
  printf (" m17-fme -i pulsevx -o pulserf -V -F float.sym -N 2> m17encoder.txt \n");
  printf ("  (Note: When Using Ncurses Terminal with Encoding and Not Vox, use '\\' key to toggle TX)\n");
  printf ("\n");
  printf (" RF Demodulator for Stream Voice and Data Packet with Decoded Voice Output (pulsevx) \n");
  printf (" m17-fme -i pulserf -o pulsevx -r -N 2> m17decoder.txt \n");
  printf ("\n");
  printf (" Stream Voice Encoder with Mic Input (pulsevx) IP Frame Output \n");
  printf (" m17-fme -i pulsevx -o udp -V -N 2> m17encoder.txt \n");
  printf ("\n");
  printf (" IP Frame Decoder for Voice Stream and Packet Data Default Host and Port \n");
  printf (" m17-fme -i udp -u -o pulsevx -N 2> m17decoder.txt \n");
  printf ("\n");
  printf (" Packet Data Encoder with SMS Message to IP Frame Output to custom port and RF Audio Output\n");
  printf (" m17-fme -o pulserf -P -S 'This is a text message' -M 1:M17-FME:ALL -I -U 127.0.0.1:17001:A \n");
  printf ("\n");
  printf (" IP Frame Decoder for Voice Stream and Packet Data Bound to Custom Host and Port \n");
  printf (" m17-fme -i udp:127.0.0.1:17001 -N 2> m17decoder.txt \n");
  printf ("\n");
}

int main (int argc, char **argv)
{
  int i, c;
  extern char *optarg;
  char * pEnd;
  char string[1024]; memset (string, 0, 1024*sizeof(char));

  //Nested "Super" Struct to make it easy to pass around tons of smaller structs
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

  //print git tag / version number
  fprintf (stderr, "Build Version: %s \n", GIT_TAG);
  
  //print current session number
  fprintf (stderr, "Session Number: %04X \n", super.opts.random_number);

  //use i to count number of optargs parsed
  i = 0;

  //process user CLI optargs (try to keep them alphabetized for my personal sanity)
  //NOTE: Try to observe conventions that lower case is decoder, UPPER is ENCODER, numerical 0-9 are for debug related testing
  while ((c = getopt (argc, argv, "1234567890c:d:e:f:hi:lmno:prs:t:uv:w:xA:C:E:F:INLM:PR:S:TU:VW:X")) != -1)
  {

    i++;

    switch (c)
    {
      case 'h':
        usage ();
        exit (0);
        break;

      //generate one time randomized scrambler key
      case '1':
        super.enc.scrambler_key = rand() & 0xFFFFFF;
        super.enc.enc_type = 1;
        pn_sequence_generator(&super); //generate pN Sequence
        fprintf (stderr, "\n");
        break;

      //generate one time randomized AES key
      case '2':
        super.enc.A1 = ((uint64_t)rand() << 32ULL) + rand();
        super.enc.A2 = ((uint64_t)rand() << 32ULL) + rand();
        super.enc.A3 = ((uint64_t)rand() << 32ULL) + rand();
        super.enc.A4 = ((uint64_t)rand() << 32ULL) + rand();
        super.enc.enc_type = 2;
        aes_key_loader(&super);
        fprintf (stderr, "\n");
        break;

      //Allow CRC Failure to still be decoded (TS issue with OTA key on RFA failing)
      case '4':
        super.opts.allow_crc_failure = 1;
        fprintf (stderr, "Allow CRC Failure.\n");
        break;

      //Voice Open and Send Everything!
      case '6': //enable all input and output, loopback etc to debug any perceived lag when doing everything
        super.opts.use_pa_input = 1;
        super.opts.use_pa_output_rf = 1;
        super.opts.use_pa_output_vx = 1;
        super.opts.m17_use_ip = 1;
        super.opts.use_m17_str_encoder = 1;
        super.opts.internal_loopback_decoder = 1;
        break;

      //Disable Symbol Timing Correction
      case '7':
        super.opts.disable_symbol_timing = 1;
        fprintf (stderr, "Disable Symbol Timing Correction.\n");
        break;

      //Disable HPF on Digital Output
      case '8':
        super.opts.use_hpfilter_dig = 0;
        fprintf (stderr, "Disable High Pass Filter on CODEC2 Output.\n");
        break;

      //enable RRC Filter
      case '9':
        super.opts.disable_rrc_filter = 0;
        fprintf (stderr, "Enable RRC Filter on RF Audio Encoding / Decoding. \n");
        break;

      //disable RRC Filter
      case '0':
        super.opts.disable_rrc_filter = 1;
        fprintf (stderr, "Disable RRC Filter on RF Audio Encoding / Decoding. \n");
        break;

      //just leave these here until I wrap up and get back to the 'cookie cutter' project
      // case 'a':
      //   super.opts.a = 1;
      //   break;

      // case 'b':
      //   strncpy(super.opts.b, optarg, 1023);
      //   super.opts.b[1023] = '\0';
      //   fprintf (stderr,"B: %s\n", super.opts.b);
      //   break;

      //Specify DSD-FME Dibit Capture Bin Input File Format (RF Encoded only)
      case 'c':
        strncpy(super.opts.dibit_input_file, optarg, 1023);
        super.opts.dibit_input_file[1023] = '\0';
        super.opts.use_dibit_input = 1;
        fprintf (stderr, "DSD-FME Dibit Input File: %s \n", super.opts.dibit_input_file);
        break;

      //set demodulator verbosity levels
      case 'd':
        //rearrange to show scope, even if -d 0
        super.opts.ncurses_show_banner = 0;
        super.opts.ncurses_show_io = 0;
        super.opts.ncurses_show_audio = 1;
        super.opts.ncurses_show_history = 1;
        super.opts.ncurses_show_scope = 1;
        super.opts.demod_verbosity = atoi(optarg);
        fprintf (stderr, "Demodulator Verbosity: %d; \n", super.opts.demod_verbosity);
        break;

      //Set Scrambler Key (Encoding and Decoding)
      case 'e':
        sscanf (optarg, "%X", &super.enc.scrambler_key);
        super.enc.scrambler_key &= 0xFFFFFF; //trunc to 24-bit
        if (super.enc.scrambler_key != 0)
        {
          super.enc.enc_type = 1;
          pn_sequence_generator(&super); //generate pN Sequence
        }
        fprintf (stderr, "\n");
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
        break;

      //Enable Event Log 
      case 'l':
        super.opts.use_event_log = 1;
        fprintf (stderr, "Enable Event Log File: date_time_m17fme_eventlog.txt \n");
        break;

      //Enable Raw Audio Input Monitor
      case 'm':
        super.opts.use_raw_audio_monitor = 1;
        fprintf (stderr, "Enable Analog/Input Signal Monitor (when no sync) \n");
        break;

      //enable the ncurses terminal, if available
      case 'N':
      case 'n':
        #ifdef USE_CURSES
        super.opts.use_ncurses_terminal = 1;
        fprintf (stderr, "Ncurses Terminal Mode. \n");
        #else
        fprintf (stderr, "Ncurses Support Not Compiled. \n");
        #endif
        break;

      //Read Output String to be parsed
      case 'o':
        strncpy(super.opts.output_handler_string, optarg, 2047);
        super.opts.output_handler_string[2047] = '\0';
        break;

      //enable the RF Audio Demodulator
      case 'p':
        super.opts.use_wav_out_pc = 1;
        fprintf (stderr, "Per Call wav file saving to %s. \n", super.wav.wav_file_direct);
        break;

      //enable the RF Audio Demodulator
      case 'r':
        super.opts.use_m17_rfa_decoder = 1;
        fprintf (stderr, "M17 Project RF Audio Frame Demodulator. \n");
        break;

      //input quelch level (for vox input)
      case 's':
        super.demod.input_sql = atoi(optarg);
        fprintf (stderr, "Input Squelch: %ld; \n", super.demod.input_sql);
        break;

      //Enable UDP IP Frame Input
      case 'u':
        super.opts.use_m17_ipf_decoder = 1;
        fprintf (stderr, "M17 Project Encoder UDP IP Frame Receiver Enabled. \n");
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
        memcpy (super.m17d.arb, super.m17e.arb, 772);
        break;

      //Specify DSD-FME Dibit Capture Bin File Format (RF Encoded only)
      case 'C':
        strncpy(super.opts.dibit_output_file, optarg, 1023);
        super.opts.dibit_output_file[1023] = '\0';
        super.opts.use_dibit_output = 1;
        fprintf (stderr, "DSD-FME Dibit Output File: %s \n", super.opts.dibit_output_file);
        break;

      //Set AES Key (Encoding and Decoding)
      case 'E':
        strncpy(string, optarg, 1023);
        string[1023] = '\0';
        super.enc.A1 = strtoull (string, &pEnd, 16);
        super.enc.A2 = strtoull (pEnd, &pEnd, 16);
        super.enc.A3 = strtoull (pEnd, &pEnd, 16);
        super.enc.A4 = strtoull (pEnd, &pEnd, 16);
        aes_key_loader(&super);
        fprintf (stderr, "\n");
        break;

      //Specify M17 Float Symbol Output
      case 'F':
        strncpy(super.opts.float_symbol_output_file, optarg, 1023);
        super.opts.float_symbol_output_file[1023] = '\0';
        super.opts.use_float_symbol_output = 1;
        fprintf (stderr, "Float Symbol Output File: %s \n", super.opts.float_symbol_output_file);
        break;

      //Enable IP Frame Output (with default localhost:17000)
      case 'I':
        super.opts.m17_use_ip = 1;
        fprintf (stderr, "M17 Project Encoder IP Frame Enabled. \n");
        break;

      case 'L':
        super.opts.internal_loopback_decoder = 1;
        fprintf (stderr, "Internal Encoder Loopback to Decoder. \n");
        break;

      //Specify Encoder CAN, SRC, and DST Callsign Data
      case 'M':
        strncpy(super.m17e.user, optarg, 49);
        super.m17e.user[49] = '\0';
        break;

      //Enable the PKT Encoder
      case 'P':
        super.opts.use_m17_pkt_encoder = 1;
        fprintf (stderr, "M17 Project Packet Encoder. \n");
        break;

      //Specify M17 PKT Encoder Raw Encoded Data Packet
      case 'R':
        parse_raw_user_string (&super, optarg);
        break;

      //Specify M17 PKT Encoder SMS Message (truncates at 772)
      case 'S':
        strncpy(super.m17e.sms, optarg, 772);
        super.m17e.sms[772] = '\0';
        memcpy (super.m17d.sms, super.m17e.sms, 772);
        break;

      //Run the test pattern generator
      case 'T':
        super.opts.use_m17_tst_encoder = 1;
        fprintf (stderr, "M17 Project Test Pattern Generator. \n");
        break;

      //Specify M17 UDP Frame String Format, i.e., 'localhost:17000' or 'mycustomhost.xyz:17001'
      case 'U':
        strncpy(super.opts.m17_udp_input, optarg, 1024);
        super.opts.m17_udp_input[1024] = '\0';
        break;

      //Enable the Stream Voice Encoder
      case 'V':
        super.opts.use_m17_str_encoder = 1;
        fprintf (stderr, "M17 Project Stream Voice Encoder. \n");
        break;

      //Specify RF Audio Output Wav File
      case 'W':
        super.opts.use_wav_out_rf = 1;
        strncpy(super.wav.wav_out_file_rf, optarg, 1023);
        super.wav.wav_out_file_rf[1024] = '\0';
        fprintf (stderr, "RF Audio Wav File: %s \n", super.wav.wav_out_file_rf);
        break;

      case 'X':
        super.m17e.str_encoder_vox = 1;
        fprintf (stderr, "Stream Voice Encoder TX on Vox. \n");
        break;

    }
  }

  //call signal handler so things like ctrl+c will allow us to gracefully close
  signal (SIGINT, handler);
  signal (SIGTERM, handler);

  //set default starting state if no optargs parsed
  if (i == 0)
    set_default_state(&super);

  //open the ncurses terminal if its available and enabled
  #ifdef USE_CURSES
  if (super.opts.use_ncurses_terminal == 1)
    open_ncurses_terminal();
  #endif

  //parse any user passed input or output strings
  if (super.opts.input_handler_string[0] != 0)
    parse_input_option_string(&super, super.opts.input_handler_string);
  if (super.opts.output_handler_string[0] != 0)
    parse_output_option_string(&super, super.opts.output_handler_string);
  if (super.opts.m17_udp_input[0] != 0)
    parse_udp_user_string(&super, super.opts.m17_udp_input);
  if (super.m17e.user[0] != 0)
    parse_m17_user_string(&super, super.m17e.user);

  //open any input or output audio devices or files
  open_audio_input (&super);
  open_audio_output (&super);
  fprintf (stderr, "\n");

  //demodulate, frame sync, and decode OTA RF Audio, or captured/crafted files
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
      framesync (&super);

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

  //encode M17 Packet Data Frames
  if (super.opts.use_m17_pkt_encoder == 1)
    encode_pkt(&super);
  
  //encode M17 Voice Stream Frames
  if (super.opts.use_m17_str_encoder == 1)
    encode_str(&super);

  //Test Pattern Generator
  if (super.opts.use_m17_tst_encoder == 1)
    test_pattern_generator(&super);

  //decode IP Frames
  if (super.opts.use_m17_ipf_decoder == 1)
    decode_ipf(&super);

  //exit gracefully
  cleanup_and_exit (&super);

  return (0);
}
