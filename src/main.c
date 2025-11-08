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
  printf ("Device Options:\n");
  printf ("\n");
  printf ("  -a            List All Pulse Audio Input Sources and Output Sinks (devices).\n");
  printf ("  -g <float>    Set Autogain (0) or Set Manual Gain Value (Percentage) to Decoded Audio Output (0.01 - 25.00).\n");
  printf ("                Note: Decimal Gain Value of 0.01 is 1%%, Decimal Gain Value of 25.00 is 2500%%. Default is 1.0 for 100%%. \n");
  printf ("\n");
  printf ("Input Options:\n");
  printf ("\n");
  printf ("  -i <device>   Audio input device (default is pulserf)\n");
  printf ("                pulserf for pulse audio RFA input \n");
  printf ("                pulserf:6 or pulserf:m17_sink2.monitor for pulse audio RFA input on m17_sink2 (see -a) \n");
  printf ("                pulsevx for pulse audio Voice / Mic input\n");
  printf ("                pulsedxv for pulse audio Voice / Mic input on TX and RX Mode Operation\n");
  printf ("                pulsevx:2, pulsedxv:2, or pulsevx:alsa_input.pci-0000_0d_00.3.analog-stereo for pulse audio Voice / Mic input on device (see -a) \n");
  printf ("                - for STDIN input (specify encoder or decoder options below)\n");
  printf ("                (Note: When using STDIN, Ncurses Keyboard Shortcuts Disabled)\n");
  #ifdef __CYGWIN__
  printf ("                (pulse audio options will require pactl running in Cygwin)\n");
  #else
  printf ("                (padsp wrapper required for OSS audio on Linux)\n");
  #endif
  printf ("                /dev/dsp for OSS audio\n");
  printf ("                udp for UDP Frame Input (default localhost:17000)\n");
  printf ("                udp:192.168.7.8:17001:A for M17 UDP/IP Adhoc input (Address, Port, (A)dhock)\n");
  printf ("                udp:192.168.7.8:17001:R:C:YES for M17 UDP/IP Reflector input (Address, Port, (R)eflector, Module, Affirmation (See Below))\n");
  printf ("                tcp for Network Audio TCP Source at 48000 (SDR++)\n");
  printf ("                tcp:192.168.7.5:7355 for Network Audio TCP Source at 48000 (SDR++)\n"); 
  printf ("  -w <file>     48k/1 SNDFile Compatible RF Audio .wav or .rrc input file\n");
  printf ("  -c <file>     DSD-FME Compatible Dibit/Symbol Capture Bin input file (from RF Encoder)\n");
  printf ("  -f <file>     Float Symbol input file (from RF Encoder and M17_Implementations)\n");
  printf ("  -^ <file>     IP Frame input file (from M17-FME IP Frame Decoder)\n");
  printf ("\n");
  printf ("Output Options:\n");
  printf ("\n");
  printf ("  -o <device>   Audio output device (default is pulsevx)\n");
  printf ("                pulserf for pulse audio RFA output\n");
  printf ("                pulserf:5 or pulserf:m17_sink2 for pulse audio RFA output on m17_sink2 (see -a) \n");
  printf ("                pulsevx for pulse audio Voice / Loopback output\n");
  printf ("                pulsedxv for pulse audio Voice output on TX and RX Mode Operation\n");
  printf ("                pulsevx:1, pulsedxv:1, or pulsevx:alsa_output.pci-0000_0d_00.3.analog-stereo for pulse audio Voice / Loopback output on device (see -a) \n");
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
  printf ("  -* <file>     IP Frame output file (from M17-FME IP Frame Decoder)\n");
  printf ("\n");
  printf ("Encoder Options:\n");
  printf ("\n");
  printf ("  -V            Enable the Stream Voice Encoder\n");
  printf ("  -P            Enable the Packet Data  Encoder\n");
  printf ("  -!            Enable the Bit Error Rate Test (BERT) Encoder (RFA only)\n");
  printf ("  -I            Enable IP Frame Output with defaults (can be combined with Loopback or RFA output)\n");
  printf ("  -L            Enable Internal Encoder Loopback Decoder (must be used with pulsevx output)\n");
  printf ("  -X            Enable Voice Activated TX (Vox) on Stream Voice Encoder\n");
  printf ("  -s <dec>      Input Squelch v RMS Level (Vox) on Stream Voice Encoder\n");
  printf ("  -x            Modulate Inverted Polarity on RF Output\n");
  printf ("  -K <file>     Load secp256r1 Private Key from file. (see example key: key/sig_pri_key.txt)\n");
  printf ("\n");
  printf ("Encoder Input Strings:\n");
  printf ("\n");
  printf ("  -M <str>      Set M17 CAN:SRC:DST \n");
  printf ("                (example: -M 1:N0CALL:SP5WWP) \n");
  printf ("                Note: Use an Underscore (_) for any required spaces when using a reflector, etc. \n");
  printf ("  -U <str>      Set UDP/IP Frame HOST:PORT:MODE:MODULE:AFFIRMATION \n");
  printf ("                (example: -U 127.0.0.1:17001:A) \n");
  printf ("                (example: -U 127.0.0.1:17001:R:C:YES) \n");
  printf ("                 Note: Affirmation YES means you have a valid callsign and verify you are allowed to legally TX\n");
  printf ("  -S <str>      Enter SMS Message (up to 821 UTF-8 characters) for Packet Data Encoder\n");
  printf ("                (example: -S 'Hello World! This is a text message') \n");
  printf ("  -A <str>      Enter SMS Message (Up to 48 ASCII characters) For Stream Voice Encoder (Arbitrary Data). Enables 1600 mode.\n");
  printf ("                (example: -A 'Hello World! This is arbitrary data on 1600') \n");
  printf ("  -R <hex>      Enter RAW Data for Packet Data Encoder as Hex Octets (up to 823 octets).\n");
  printf ("                (example: -R 81F0F2B42B20ABC500C80424064000) for Packet GNSS Position @ Wally World) \n");
  printf ("\n");
  printf ("                (NOTE: Using Meta Fields is not compatible with Using Encryption!) \n");
  printf ("  -Y <str>      Enter META Data for Stream Voice Encoder as Text String (Up to 52 UTF-8 characters, 4 Segment Meta);\n");
  printf ("                (example: -Y 'Hello World!! This is a longer M17 Text Message!!!!' ) for Meta Text \n");
  printf ("  -Z <hex>      Enter META Data for Stream Voice Encoder as Hex Octets (1 Meta Type Octet + 14 Hex Octets Max);\n");
  printf ("                (example: -Z 01F0F2B42B20ABC500C80424064000) for Meta GNSS Position @ Wally World \n");
  printf ("\n");
  printf ("Decoder Options:\n");
  printf ("\n");
  printf ("  -r            Enable RFA Demodulator and Decoding of Stream and Packet Data\n");
  printf ("  -x            Demodulate Inverted Polarity on RF Input\n");
  printf ("  -m            Enable Analog / Raw Input Signal Monitor on RF Input (when no sync)\n");
  printf ("  -l <file>     Enable Event Log File with specified name\n");
  printf ("  -u            Enable UDP IP Frame Decoder and Connect to default localhost:17000 \n");
  printf ("  -p            Per Call decoded voice output file saving into current directory ./m17pc folder\n");
  printf ("  -k <file>     Load secp256r1 Public Key from file. (see example key: key/sig_pub_key.txt)\n");
  printf ("\n");
  printf ("TX and RX Options:\n");
  printf ("\n");
  printf ("  -D            Enable TX and RX Mode (Send and Receive over RF or IP Frame)\n");
  printf ("                 Current Implementation Requires Pulse Audio and Ncurses Availability for RF Mode.\n");
  printf ("                 RF Example (w/ Multiple Audio Devices or Virtual / Null Sinks):\n");
  printf ("                 m17-fme -D 2> m17e.txt\n");
  printf ("\n");
  printf ("                 IP Frame Example(s):\n");
  printf ("\n");
  printf ("                 Adhoc Mode:\n");
  printf ("                 LAN Machine 1: m17-fme -D 2> m17e.txt -I -U 192.168.7.255:17000:A\n");
  printf ("                 LAN MAchine 2: m17-fme -D 2> m17e.txt -I -U 192.168.7.255:17000:A\n");
  printf ("                 Note: Adhoc Mode does not require the use of a module selection.\n");
  printf ("\n");
  printf ("                 Reflector Client Mode:\n");
  printf ("                 LSTN MODE: m17-fme -D 2> m17e.txt -M 0:M17FME000:ALL -I -U 112.213.34.65:17000:R:C:NO\n");
  printf ("                 CONN MODE: m17-fme -D 2> m17e.txt -M 0:SP5WWP__D:ALL -I -U 112.213.34.65:17000:R:C:YES\n");
  printf ("                 Note: Using Reflector mode, you must enter all fields, including R for reflector, module\n");
  printf ("                       and YES to affirm you have a valid callsign for TX. NO is Listen Only (LSTN) Mode.\n");
  printf ("\n");
  printf ("Encryption Options:\n");
  printf ("\n");
  printf ("                (NOTE: Encoder and Decoder share same values here)\n");
  printf ("  -e <hex>      Enter Scrambler Key Value (up to 24-bit / 6 Hex Character)\n");
  printf ("                (example: -e ABCDEF)\n");
  printf ("  -E <hex str>  Enter AES Key Value (in single quote, space every 16 chars) \n");
  printf ("                (example: -E '0520C1B0220AFBCA 16FB1330764B26EC 5C34A197764C147A 15FBA7515ED8BCFC')\n");
  printf ("                (example: -E '0520C1B0220AFBCA 16FB1330764B26EC')\n");
  printf ("                (Limiting significant key value to first 32 characters to maintain compatibility)\n");
  printf ("  -J <file>     Load AES Key from file. (see example key: key/aes_key.txt)\n");
  printf ("  -O            Send OTA Key Delivery Packets for AES and Scrambler Keys\n");
  printf ("  -Q            Send OTA Key Delivery Packets for Signature Public Keys\n");
  printf ("\n");
  printf ("Debug Options:\n");
  printf ("\n");
  printf ("  -1            Generate Random One Time Use 24-bit Scrambler Key \n");
  printf ("  -2            Generate Random One Time Use 256-bit AES Key. \n");
  printf ("  -3            Generate Random Keys For secp256r1 Signatures. Enable Signing and Verification.\n");
  printf ("  -5            Generate Random Keys For secp256r1 Signatures, and exit.\n");
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
  printf (" Stream Voice Encoder with Mic Input (pulsevx) IP Frame Output Adhoc Host and Port\n");
  printf (" m17-fme -i pulsevx -o udp:192.168.7.255:17000:A -V -I -N 2> m17encoder.txt \n");
  printf ("\n");
  printf (" IP Frame Decoder for Voice Stream and Packet Data Adhoc Host and Port (Adhoc 0.0.0.0:17000)\n");
  printf (" m17-fme -i udp -u -o pulsevx -N 2> m17decoder.txt \n");
  printf ("\n");
  printf (" Packet Data Encoder with SMS Message to Adhoc IP Frame Output to custom port and RF Audio Output\n");
  printf (" m17-fme -o pulserf -P -S 'This is a text message' -M 1:M17-FME:ALL -I -U 127.0.0.1:17001:A \n");
  printf ("\n");
  printf (" IP Frame Decoder for Voice Stream and Packet Data Bound to Adhoc Custom Address and Port \n");
  printf (" m17-fme -i udp:127.0.0.1:17001:A -N 2> m17decoder.txt \n");
  printf ("\n");
  printf (" IP Frame Decoder for Voice Stream and Packet Data Connect to Reflector Address, Port, R, Module in LSTN mode\n");
  printf (" m17-fme -i udp:172.234.217.28:17000:R:C -M 0:M17FME0:ALL -o pulsevx -N 2> m17decoder.txt \n");
  printf ("\n");
}

int main (int argc, char **argv)
{
  int i, c;
  extern char *optarg;
  char * pEnd;
  char string[1024]; memset (string, 0, 1024*sizeof(char));
  char * source_str;

  float gain_value = 0.0f;

  //Nested "Super" Struct to make it easy to pass around tons of smaller structs
  Super super;
  init_super(&super);

  //initialize golay
  golay_24_12_init();

  //init BERT
  init_brt();

  //init static
  m17_udp_socket_duplex_init();

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

  //print Specification Version Number and Date on Specification PDF
  fprintf (stderr, "Specification Version: %s; \n", SPEC_VERSION);
  fprintf (stderr, "Specification Date: %s \n", SPEC_DATE);
  
  //print current session number
  fprintf (stderr, "Session Number: %04X \n", super.opts.random_number);

  //use i to count number of optargs parsed
  i = 0;

  //process user CLI optargs (try to keep them alphabetized for my personal sanity)
  //NOTE: Try to observe conventions that lower case is decoder, UPPER is ENCODER, numerical 0-9 are for debug related testing
  while ((c = getopt (argc, argv, "^:*:!1234567890ac:d:e:f:g:hi:k:l:mno:prs:t:uv:w:xA:BC:DE:F:IJ:K:LM:NOPQR:S:TU:VW:XY:Z:")) != -1)
  {

    i++;

    switch (c)
    {
      case 'h':
        usage ();
        exit (0);
        break;

      //Specify IP Frame Input File
      case '^':
        strncpy(super.ip_io.ip_frame_input_filename, optarg, 1023);
        super.ip_io.ip_frame_input_filename[1023] = '\0';
        super.ip_io.use_ip_frame_in = 1;
        super.opts.use_m17_rfa_decoder = 0;
        break;

      //Specify IP Frame Output File
      case '*':
        strncpy(super.ip_io.ip_frame_output_filename, optarg, 1023);
        super.ip_io.ip_frame_output_filename[1023] = '\0';
        super.ip_io.use_ip_frame_out = 1;
        break;

      //generate one time randomized scrambler key
      case '1':
        super.enc.scrambler_key = rand() & 0xFFFFFF;
        super.enc.enc_type = 1;
        // pn_sequence_generator(&super); //generate pN Sequence
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

      case '3':
        #ifdef USE_UECC
        ecdsa_generate_random_keys(&super);
        #else
        fprintf (stderr, " uECC Support Not Compiled;");
        #endif
        break;

      //Allow CRC Failure to still be decoded
      case '4':
        super.opts.allow_crc_failure = 1;
        fprintf (stderr, "Allow CRC Failure.\n");
        break;

      case '5':
        #ifdef USE_UECC
        ecdsa_generate_random_keys(&super);
        #else
        fprintf (stderr, " uECC Support Not Compiled;");
        #endif
        exitflag = 1;
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

      //List Pulse Audio Input and Output
      case 'a':
        #ifdef USE_PULSEAUDIO
        pulse_list();
        #endif
        exit(0);
        break;

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
          // pn_sequence_generator(&super); //generate pN Sequence
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
        if (super.opts.input_handler_string[0] != 0)
          parse_input_option_string(&super, super.opts.input_handler_string);
        memset (super.opts.input_handler_string, 0, 2048*sizeof(char));
        break;

      case 'g':
        sscanf (optarg, "%f", &gain_value);
        if (gain_value <= 0.0f) //zero, or negative value
        {
          fprintf (stderr,"Enabling Auto Gain on Voice Payload. \n");
          super.opts.output_gain_vx = 1.0f;
          super.opts.auto_gain_voice = 1;
        }
        else if (gain_value >= 0.01f && gain_value <= 25.0f)
        {
          fprintf (stderr,"Manual Gain on Voice Payload Set to %.1f%%\n", gain_value*100);
          super.opts.output_gain_vx = gain_value;
          super.opts.auto_gain_voice = 0;
        }
        else
        {
          fprintf (stderr,"Invalid Gain Setting %.1f; Choose Range 0.01 to 25. \n", gain_value);
          super.opts.output_gain_vx = 1.0f;
          super.opts.auto_gain_voice = 0;
        }
        break;

      //Specify ECDSA Public Key File (Decoder)
      case 'k':
        #ifdef USE_UECC
        strncpy(super.opts.pub_key_file, optarg, 1023);
        super.opts.pub_key_file[1023] = '\0';
        fprintf (stderr, "secp256r1 Public Key File: %s \n", super.opts.pub_key_file);
        source_str = calloc(128, sizeof(char));
        super.opts.pub_key = fopen(super.opts.pub_key_file, "r");
        if (!super.opts.pub_key)
            fprintf(stderr, "Failed to secp256r1 Public Key file %s.\n", super.opts.pub_key_file);
        else
        {
          fread(source_str, 1, 128, super.opts.pub_key);
          fclose(super.opts.pub_key);
        }
        convert_string_into_array(source_str, super.m17d.ecdsa.public_key);
        super.m17d.ecdsa.keys_loaded = 1;
        fprintf (stderr, "Pub Key:");
        for (int j = 0; j < 64; j++)
        {
          if (j == 16 || j == 32 || j == 48)
            fprintf (stderr, "\n        ");
          fprintf (stderr, " %02X", super.m17d.ecdsa.public_key[j]);
        }
        fprintf (stderr, "\n");
        free(source_str);
        source_str = NULL;
        #else
        fprintf (stderr, " uECC Support Not Compiled;");
        #endif
        break;

      //Enable Event Log 
      case 'l':
        strncpy(super.opts.event_log_file, optarg, 1023);
        super.opts.event_log_file[1023] = '\0';
        super.opts.use_event_log = 1;
        fprintf (stderr, "Enable Event Log File: %s \n", super.opts.event_log_file);
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
        if (super.opts.output_handler_string[0] != 0)
          parse_output_option_string(&super, super.opts.output_handler_string);
        memset(super.opts.output_handler_string, 0, 2048*sizeof(char));
        break;

      //per-call output file
      case 'p':
        super.opts.use_wav_out_pc = 1;
        fprintf (stderr, "Per Call ogg file saving to %s. \n", super.wav.wav_file_direct);
        break;

      //enable the RF Audio Demodulator
      case 'r':
        super.opts.use_m17_rfa_decoder = 1;
        fprintf (stderr, "M17 Project RF Audio Frame Demodulator. \n");
        break;

      //input squelch level (for vox input)
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

      //Enable Encoder / TX and RX Packet Bursts
      case 'B':
        super.opts.use_m17_packet_burst = 1;
        fprintf (stderr, "M17 Project Packet Burst Enabled. \n");
        break;

      //Specify DSD-FME Dibit Capture Bin File Format (RF Encoded only)
      case 'C':
        strncpy(super.opts.dibit_output_file, optarg, 1023);
        super.opts.dibit_output_file[1023] = '\0';
        super.opts.use_dibit_output = 1;
        fprintf (stderr, "DSD-FME Dibit Output File: %s \n", super.opts.dibit_output_file);
        break;

      //M17 Repeater Mode / TX and RX Mode
      case 'D':
        #ifdef USE_PULSEAUDIO
        {} //continue
        #else
        fprintf (stderr, "M17 Project TX and RX Mode Requires Pulse Audio. \n");
        exitflag = 1;
        #endif

        #ifdef USE_CURSES
        {} //continue
        #else
        fprintf (stderr, "M17 Project TX and RX Mode Requires Ncurses. \n");
        exitflag = 1;
        #endif

        super.opts.use_m17_duplex_mode = 1;
        fprintf (stderr, "M17 Project TX and RX Mode Enabled. \n");

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

      //Specify AES Key File
      case 'J':
        strncpy(super.opts.aes_key_file, optarg, 1023);
        super.opts.aes_key_file[1023] = '\0';
        fprintf (stderr, "AES Key File: %s \n", super.opts.aes_key_file);
        source_str = calloc(128, sizeof(char));
        super.opts.aes_key = fopen(super.opts.aes_key_file, "r");
        if (!super.opts.aes_key)
            fprintf(stderr, "Failed to load file %s.\n", super.opts.aes_key_file);
        else
        {
          fread(source_str, 1, 64, super.opts.aes_key);
          fclose(super.opts.aes_key);
        }
        uint8_t tmp_a[64]; memset(tmp_a, 0, 64*sizeof(uint8_t));
        convert_string_into_array(source_str, tmp_a);
        uint8_t tmp_b[256]; unpack_byte_array_into_bit_array(tmp_a, tmp_b, 64);
        super.enc.A1 = (unsigned long long int)convert_bits_into_output(&tmp_b[0], 64);
        super.enc.A2 = (unsigned long long int)convert_bits_into_output(&tmp_b[64], 64);
        super.enc.A3 = (unsigned long long int)convert_bits_into_output(&tmp_b[128], 64);
        super.enc.A4 = (unsigned long long int)convert_bits_into_output(&tmp_b[192], 64);
        aes_key_loader(&super);
        free(source_str);
        source_str = NULL;
        break;

      //Specify ECDSA Private Key File (Encoder)
      case 'K':
        #ifdef USE_UECC
        strncpy(super.opts.pri_key_file, optarg, 1023);
        super.opts.pri_key_file[1023] = '\0';
        fprintf (stderr, "secp256r1 Private Key File: %s \n", super.opts.pri_key_file);
        source_str = calloc(128, sizeof(char));
        super.opts.pub_key = fopen(super.opts.pri_key_file, "r");
        if (!super.opts.pub_key)
            fprintf(stderr, "Failed to secp256r1 Private Key file %s.\n", super.opts.pri_key_file);
        else
        {
          fread(source_str, 1, 64, super.opts.pub_key);
          fclose(super.opts.pub_key);
        }
        convert_string_into_array(source_str, super.m17e.ecdsa.private_key);
        super.m17e.ecdsa.keys_loaded = 1;
        fprintf (stderr, "Pri Key:");
        for (int j = 0; j < 32; j++)
        {
          if (j == 16)
            fprintf (stderr, "\n        ");
          fprintf (stderr, " %02X", super.m17e.ecdsa.private_key[j]);
        }
        fprintf (stderr, "\n");
        free(source_str);
        source_str = NULL;
        #else
        fprintf (stderr, " uECC Support Not Compiled;");
        #endif
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

      //Encode and Send OTA Key Delivery Packets and Embedded LSD
      case 'O':
        super.opts.use_otakd = 1;
        fprintf (stderr, "Send OTA Key Delivery Packets and Embedded LSD. (AES and Scrambler)\n");
        break;

      //Enable the PKT Encoder
      case 'P':
        super.opts.use_m17_pkt_encoder = 1;
        fprintf (stderr, "M17 Project Packet Encoder. \n");
        break;

      //Encode and Send OTA Key Delivery Packets for Signature Public Keys
      case 'Q':
        super.opts.use_otask = 1;
        fprintf (stderr, "Send OTA Key Delivery Packets for Signature Public Keys\n");
        break;

      //Specify M17 PKT Encoder Raw Encoded Data Packet
      case 'R':
        parse_raw_user_string (&super, optarg);
        break;

      //Specify M17 PKT Encoder SMS Message (truncates at 821)
      case 'S':
        strncpy(super.m17e.sms, optarg, 821);
        super.m17e.sms[822] = '\0';
        memcpy (super.m17d.sms, super.m17e.sms, 822);
        break;

      //Run the test pattern generator
      case 'T':
        super.opts.use_m17_tst_encoder = 1;
        fprintf (stderr, "M17 Project Test Pattern Generator. \n");
        break;

      //Specify M17 UDP Frame String Format, i.e., 'localhost:17000' or 'mycustomhost.xyz:17001'
      case 'U':
        strncpy(super.opts.m17_udp_input, optarg, 1023);
        super.opts.m17_udp_input[1023] = '\0';
        break;

      //Enable the Stream Voice Encoder
      case 'V':
        super.opts.use_m17_str_encoder = 1;
        fprintf (stderr, "M17 Project Stream Voice Encoder. \n");
        break;

      //Enable the BERT Encoder
      case '!':
        super.opts.use_m17_brt_encoder = 1;
        fprintf (stderr, "M17 Project BERT Encoder. \n");
        break;

      //Specify RF Audio Output Wav File
      case 'W':
        super.opts.use_wav_out_rf = 1;
        strncpy(super.wav.wav_out_file_rf, optarg, 1023);
        super.wav.wav_out_file_rf[1023] = '\0';
        fprintf (stderr, "RF Audio Wav File: %s \n", super.wav.wav_out_file_rf);
        break;

      case 'X':
        super.m17e.str_encoder_vox = 1;
        fprintf (stderr, "Stream Voice Encoder TX on Vox. \n");
        break;

      //Specify M17 STR Encoder UTF-8 Meta Text Message
      case 'Y':
        parse_meta_txt_string (&super, optarg);
        break;

      //Specify M17 STR Encoder Raw Encoded Meta Data
      case 'Z':
        parse_meta_raw_string (&super, optarg);
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
    open_ncurses_terminal(&super);
  #endif

  //parse any user passed input or output strings
  if (super.opts.m17_udp_input[0] != 0)
    parse_udp_user_string(&super, super.opts.m17_udp_input);
  if (super.m17e.user[0] != 0)
    parse_m17_user_string(&super, super.m17e.user);

  //open any input or output audio devices or files
  if (!super.opts.use_m17_duplex_mode) //temp measure to get this working
  {
    open_audio_input (&super);
    open_audio_output (&super);
    fprintf (stderr, "\n");
  }

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
    encode_pkt(&super, 1);
  
  //encode M17 Voice Stream Frames
  if (super.opts.use_m17_str_encoder == 1)
    encode_str(&super);

  //encode M17 BERT Frame
  if (super.opts.use_m17_brt_encoder == 1)
    encode_brt(&super);

  //Test Pattern Generator
  if (super.opts.use_m17_tst_encoder == 1)
    test_pattern_generator(&super);

  //decode IP Frames
  if (super.opts.use_m17_ipf_decoder == 1)
  {
    start_ipf(&super);
    while (!exitflag)
      decode_ipf(&super, 0);
  }

  //decode IP Frames from file
  if (super.ip_io.use_ip_frame_in == 1)
  {
    open_ip_input_file(&super);
    while (!exitflag)
      decode_ipf(&super, 0);
  }

  //M17 TX and RX Mode
  if (super.opts.use_m17_duplex_mode == 1)
    m17_rx_tx_mode(&super);

  //exit gracefully
  cleanup_and_exit (&super);

  return (0);
}

//Current Tag: 2025