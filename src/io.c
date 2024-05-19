/*-------------------------------------------------------------------------------
 * io.c
 * M17 Project - Input and Output Open and Close Functions
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void open_audio_input (Super * super)
{

  //float symbol input file
  if (super->opts.use_float_symbol_input == 1)
    super->opts.float_symbol_in = fopen (super->opts.float_symbol_input_file, "r");

  //DSD-FME Dibit Capture Bin File
  else if (super->opts.use_dibit_input == 1)
    super->opts.dibit_in = fopen (super->opts.dibit_input_file, "r");
  
  //TCP Sourced SB16LE Network Sink
  else if (super->opts.use_tcp_input == 1)
  {
    super->opts.tcp_input_sock = 
      tcp_socket_connect(super->opts.tcp_input_hostname, super->opts.tcp_input_portno);

    if (super->opts.tcp_input_sock)
    {
      tcp_snd_audio_source_open(super);
      super->opts.tcp_input_open = 1;
      super->opts.use_snd_input = 1;
    }
    else exitflag = 1;
  }

  //STDIN Sourced SB16LE Input
  else if (super->opts.use_stdin_input == 1)
  {
    stdin_snd_audio_source_open(super);
    super->opts.tcp_input_open = 1;
    super->opts.use_snd_input = 1;
  }

  //.wav, .rrc (or other) Sourced SB16LE file based input
  else if (super->opts.snd_input_is_a_file == 1)
  {
    file_snd_audio_source_open(super);
    super->opts.use_snd_input = 1;
  }

  //OSS "dev/dsp" Input
  else if (super->opts.use_oss_input == 1)
  {
    open_oss_input(super);
  }

  //Pulse Audio Input
  #ifdef USE_PULSEAUDIO
  else if (super->opts.use_pa_input == 1)
  {
    open_pulse_audio_input (super);
  }
  #endif

}

//audio devices and output files (wav, bin, float, event log)
void open_audio_output (Super * super)
{

  //event log
  if (super->opts.use_event_log == 1)
  {
    char * timestr  = get_time();
    char * datestr  = get_date();
    sprintf (super->opts.event_log_file, "%s_%s_m17fme_eventlog.txt", datestr, timestr);
    super->opts.event_log = fopen (super->opts.event_log_file, "a");
    free (timestr); free (datestr);
  }

  //wav and misc output files
  if (super->opts.use_wav_out_rf == 1)
    open_wav_out_rf(super);
  if (super->opts.use_wav_out_vx == 1)
    open_wav_out_vx(super);

  //float symbol output file
  if (super->opts.use_float_symbol_output == 1)
    super->opts.float_symbol_out = fopen (super->opts.float_symbol_output_file, "w");

  //DSD-FME Dibit Capture Bin File
  if (super->opts.use_dibit_output == 1)
    super->opts.dibit_out = fopen (super->opts.dibit_output_file, "w");

  if (super->opts.use_oss_output == 1)
    open_oss_output(super);

  else if (super->opts.use_stdout_output == 1)
    open_stdout_pipe(super);

  #ifdef USE_PULSEAUDIO
  else
  {
    if (super->opts.use_pa_output_rf == 1)
      open_pulse_audio_output_rf (super);
    if (super->opts.use_pa_output_vx == 1)
      open_pulse_audio_output_vx (super);
  }
  #endif

}

void open_stdout_pipe(Super * super)
{
  super->opts.stdout_pipe = fileno(stdout);
}

void write_stdout_pipe(Super * super, short * out, size_t nsam)
{
  write (super->opts.stdout_pipe, out, nsam*2);
}

void cleanup_and_exit (Super * super)
{
  // Signal that everything should shutdown.
  exitflag = 1;

  //go to no_carrier_state and push call history, event writer, etc
  if (super->demod.in_sync)
    no_carrier_sync(super);

  #ifdef USE_PULSEAUDIO
  if (super->pa.pa_input_is_open)
    close_pulse_audio_input(super);

  if (super->pa.pa_output_rf_is_open)
    close_pulse_audio_output_rf(super);

  if (super->pa.pa_output_vx_is_open)
    close_pulse_audio_output_vx(super);
  #endif

  if (super->wav.wav_out_rf)
    close_wav_out_rf(super);

  if (super->wav.wav_out_vx)
    close_wav_out_vx(super);

  //close per call wav file, if opened
  if (super->wav.wav_out_pc)
    close_wav_out_pc (super);

  //close event log, if opened
  if (super->opts.event_log)
    fclose (super->opts.event_log);

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

  if (super->opts.dibit_out)
    fclose (super->opts.dibit_out);

  //SND FILE
  if (super->snd_src_in.audio_in_file)
    sf_close(super->snd_src_in.audio_in_file);

  // TCP Audio Input Socket
  if (super->opts.tcp_input_sock)
    close (super->opts.tcp_input_sock); 

  //RIGCTL Remote Socket
  if (super->opts.rig_remote_sock)
    close (super->opts.rig_remote_sock);

  if (super->opts.use_m17_rfa_decoder || super->opts.internal_loopback_decoder)
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "Total Errors:\n");
    fprintf (stderr, "Link Setup Frame ERR: %05d;\n", super->error.lsf_hdr_crc_err);
    fprintf (stderr, "Embedded Link    ERR: %05d;\n", super->error.lsf_emb_crc_err);
    fprintf (stderr, "LICH Golay 24    ERR: %05d;\n", super->error.golay_err);
    fprintf (stderr, "Packet Data      ERR: %05d;\n", super->error.pkt_crc_err);
    fprintf (stderr, "IP Frame         ERR: %05d;\n", super->error.ipf_crc_err);
    
    //TODO: Implement Metric / Error rates on Viterbi / Conv decoder and BERT when done.
    // fprintf (stderr, "BERT Frame       ERR: %05d;\n", super->error.bert_err);
    // fprintf (stderr, "Viterbi          ERR: %05d;\n", super->error.viterbi_err);
  }

  fprintf (stderr, "\n");

  fprintf (stderr,"Exiting.\n");

  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    close_ncurses_terminal();
  #endif

  exit(0);
}

//io string parsing
void parse_input_option_string (Super * super, char * input)
{

  if ( (strncmp(input, "null", 4) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "Audio Input Device: NULL;");
    super->opts.use_pa_input = 0;
    super->opts.use_oss_input = 0;
    super->opts.use_stdin_input = 0;
    super->opts.use_tcp_input = 0;
    super->opts.use_snd_input = 0;
    super->opts.snd_input_is_a_file = 0;
  }

  else if ( (strncmp(input, "tcp", 3) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "Audio Input Device: TCP;");
    super->opts.use_tcp_input = 1;
  }

  else if ( (strncmp(input, "/dev/dsp", 8) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "Audio Input Device: OSS (/dev/dsp);");
    super->opts.use_oss_input = 1;
  }

  else if ( (strncmp(input, "-", 1) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "Audio Input Device: STDIN (-);");
    super->opts.use_stdin_input = 1;
  }

  else if ( (strncmp(input, "udp", 3) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "UDP IP Frame Input;");
    super->opts.use_m17_ipf_decoder = 1;

    //NOTE: We can further yeet the string to udp handler to get the rest
    parse_udp_user_string(super, input+4);
  }

  //Mirror the DSD-FME Variation on this string
  else if ( (strncmp(input, "m17udp", 6) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "M17 UDP IP Frame Input: ");
    super->opts.use_m17_ipf_decoder = 1;

    //NOTE: We can further yeet the string to udp handler to get the rest
    parse_udp_user_string(super, input+7);
  }

  #ifdef USE_PULSEAUDIO
  else if ( (strncmp(input, "pulse", 5) == 0) )
  {
    fprintf (stderr, "\n");
    if ( (strncmp(input, "pulserf", 7) == 0) )
    {
      fprintf (stderr, "Audio Input Device: Pulse RF Input;");
      super->opts.use_m17_rfa_decoder = 1; //enable the RF decoder if puslerf is detected
    }
    else if ( (strncmp(input, "pulsevx", 7) == 0) )
      fprintf (stderr, "Audio Input Device: Pulse Mic Input;"); //default is for encoder voice
    else
    {
      fprintf (stderr, "Audio Input Device: Pulse RF Input;");
      super->opts.use_m17_rfa_decoder = 1; //enable the RF decoder if puslerf is detected
    }

    //string yeet
    parse_pulse_input_string(super, input+7);

    super->opts.use_pa_input = 1;
  }
  #else
  else if ( (strncmp(input, "pulse", 5) == 0) )
    fprintf (stderr, " Pulse Audio Support Not Found / Compiled;");
  #endif

  //anything not recognized
  else fprintf (stderr, "\nAudio Output Device: Unknown %s;", input);

}

void parse_output_option_string (Super * super, char * output)
{
  
  if ( (strncmp(output, "null", 4) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "Audio Output Device(s): NULL;");
    super->opts.use_pa_output_rf = 0;
    super->opts.use_pa_output_vx = 0;
    super->opts.use_oss_output = 0;
    super->opts.use_stdout_output = 0;
  }

  else if ( (strncmp(output, "/dev/dsp", 8) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "Audio Output Device: OSS (/dev/dsp);");
    super->opts.use_oss_output = 1;
  }

  else if ( (strncmp(output, "-", 1) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "Audio Output Device: STDOUT (-);");
    super->opts.use_stdout_output = 1;
  }

  else if ( (strncmp(output, "udp", 3) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "UDP IP Frame Output;");
    super->opts.m17_use_ip = 1;

    //NOTE: We can further yeet the string to udp handler to get the rest
    parse_udp_user_string(super, output+4);
  }

  //Mirror the DSD-FME Variation on this string
  else if ( (strncmp(output, "m17udp", 3) == 0) )
  {
    fprintf (stderr, "\n");
    fprintf (stderr, "M17 UDP IP Frame Output;");
    super->opts.m17_use_ip = 1;

    //NOTE: We can further yeet the string to udp handler to get the rest
    parse_udp_user_string(super, output+7);
  }

  #ifdef USE_PULSEAUDIO
  else if ( (strncmp(output, "pulse", 5) == 0) )
  {
    fprintf (stderr, "\n");
    if ( (strncmp(output, "pulserf", 7) == 0) )
      super->opts.use_pa_output_rf = 1;
    else if ( (strncmp(output, "pulsevx", 7) == 0) )
      super->opts.use_pa_output_vx = 1;
    else super->opts.use_pa_output_vx = 1;

    if (super->opts.use_pa_output_rf == 1)
    {
      fprintf (stderr, "Audio Output Device: Pulse RF Output;");

      //string yeet
      parse_pulse_outrf_string(super, output+7);
    }
    
    else if (super->opts.use_pa_output_vx == 1)
    {
      super->opts.internal_loopback_decoder = 1; //may disable this later
      fprintf (stderr, "Audio Output Device: Pulse Voice Output;");

      //string yeet
      parse_pulse_outvx_string(super, output+7);
    }

    //should never get here hopefully
    else fprintf (stderr, "Audio Output Device: Error Parsing String;");
  }
  #else
  else if ( (strncmp(output, "pulse", 5) == 0) )
    fprintf (stderr, " Pulse Audio Support Not Found / Compiled;");
  #endif

  //anything not recognized
  else fprintf (stderr, "\nAudio Output Device: Unknown %s;", output);

}

void parse_m17_user_string (Super * super, char * input)
{
  char * curr;

  //check and capatalize any letters in the CSD
  for (int i = 0; input[i]!='\0'; i++)
  {
    if(input[i] >= 'a' && input[i] <= 'z')
      input[i] = input[i] -32;
  }

  curr = strtok(input, ":"); //CAN
  if (curr != NULL)
    super->m17e.can = atoi(curr);

  curr = strtok(NULL, ":"); //m17 src callsign
  if (curr != NULL)
  {
    strncpy (super->m17e.srcs, curr, 9); //only read first 9
    super->m17e.srcs[9] = '\0';
  }

  curr = strtok(NULL, ":"); //m17 dst callsign
  if (curr != NULL)
  {
    strncpy (super->m17e.dsts, curr, 9); //only read first 9
    super->m17e.dsts[9] = '\0';
  }

  fprintf (stderr, "\n");
  fprintf (stderr, "M17 User Data: ");
  fprintf (stderr, "CAN: %d; ", super->m17e.can);
  fprintf (stderr, "SRC: %s; ", super->m17e.srcs);
  fprintf (stderr, "DST: %s; ", super->m17e.dsts);
}

void parse_pulse_input_string (Super * super, char * input)
{
  char * curr;
  curr = strtok(input, ":");
  if (curr != NULL)
  {
    strncpy (super->pa.pa_input_idx, curr, 49);
    super->pa.pa_input_idx[49] = 0;
    fprintf (stderr, "\n");
    fprintf (stderr, "Pulse Input Device: %s; ", super->pa.pa_input_idx);
  }
}

void parse_pulse_outrf_string (Super * super, char * input)
{
  char * curr;
  curr = strtok(input, ":");
  if (curr != NULL)
  {
    strncpy (super->pa.pa_outrf_idx, curr, 49);
    super->pa.pa_outrf_idx[49] = 0;
    fprintf (stderr, "\n");
    fprintf (stderr, "Pulse Output Device: %s; ", super->pa.pa_outrf_idx);
  }
}

void parse_pulse_outvx_string (Super * super, char * input)
{
  char * curr;
  curr = strtok(input, ":");
  if (curr != NULL)
  {
    strncpy (super->pa.pa_outvx_idx, curr, 49);
    super->pa.pa_outvx_idx[49] = 0;
    fprintf (stderr, "\n");
    fprintf (stderr, "Pulse Output Device: %s; ", super->pa.pa_outvx_idx);
  }
}

//may put this in the input parser instead, 
//but also is needed for output, so this may be better
void parse_udp_user_string (Super * super, char * input)
{

  char * curr;

  curr = strtok(input, ":");
  if (curr != NULL)
    strncpy (super->opts.m17_hostname, curr, 1023);
  curr = strtok(NULL, ":"); //host port
    if (curr != NULL) super->opts.m17_portno = atoi (curr);

  curr = strtok(NULL, ":"); //reflector module
  if (curr != NULL)
  {
    //read reflector module
    super->m17e.reflector_module = *curr; //straight assignment to convert char 'A' to number

    //check for capitalization
    if(super->m17e.reflector_module >= 'a' && super->m17e.reflector_module <= 'z')
      super->m17e.reflector_module = super->m17e.reflector_module -32;
    
    //make sure its a value from A to Z
    if (super->m17e.reflector_module < 0x41 || super->m17e.reflector_module > 0x5A)
    {
      super->m17e.reflector_module = 0;
      fprintf (stderr, "\n");
      fprintf (stderr, "M17 Reflector Module must be value from A-Z; \n");
    }
  }

  fprintf (stderr, "\n");
  fprintf (stderr, "UDP Host: %s; ", super->opts.m17_hostname);
  fprintf (stderr, "Port: %d; ", super->opts.m17_portno);
  if (super->m17e.reflector_module != 0)
    fprintf (stderr, "Module: %c; ", super->m17e.reflector_module);

  fprintf (stderr, "\n");

}

//convert a user string into a uint8_t array for raw packet encoding
void parse_raw_user_string (Super * super, char * input)
{
  //since we want this as octets, get strlen value, then divide by two
  uint16_t len = strlen((const char*)input);
  
  //if zero is returned, just do two
  if (len == 0) len = 2;

  //if odd number, then user didn't pass complete octets, but just add one to len value to make it even
  if (len&1) len++;

  //divide by two to get octet len
  len /= 2;

  //sanity check, maximum strlen should not exceed 771 for a full encode
  if (len > 771) len = 771;
  
  super->m17e.raw[0]  = 1;      //flag as 1 so the encoder will know to parse the data here and not on SMS 
  super->m17e.raw_len = len+1; //assign plus one to add terminating zero byte for CRC fix;

  char octet_char[3];
  octet_char[2] = 0;
  uint16_t k = 0;
  uint16_t i = 0;

  //debug
  fprintf (stderr, "\n Raw Len: %d; Raw Octets:", len);
  for (i = 0; i <= len; i++)
  {
    strncpy (octet_char, input+k, 2);
    octet_char[2] = 0;
    sscanf (octet_char, "%hhX", &super->m17e.raw[i+1]);

    //debug
    // fprintf (stderr, " (%s)", octet_char);
    fprintf (stderr, " %02X", super->m17e.raw[i+1]);
    k += 2;
  }
}

void push_call_history (Super * super)
{

  char dt[9]; memset (dt, 0, 9*sizeof(char));
  if      (super->m17d.dt == 0) sprintf (dt, "RESERVED");
  else if (super->m17d.dt == 1) sprintf (dt, "PKT DATA");
  else if (super->m17d.dt == 2) sprintf (dt, "VOX 3200");
  else if (super->m17d.dt == 3) sprintf (dt, "V+D 1600");
  else if (super->m17d.dt == 4) sprintf (dt, "RESET DM");
  else if (super->m17d.dt == 5) sprintf (dt, "IPF DISC");
  else if (super->m17d.dt == 6) sprintf (dt, "IPF CONN");
  else                          sprintf (dt, "UNK TYPE");

  char * timestr  = get_time_n(super->demod.current_time);
  char * datestr  = get_date_n(super->demod.current_time);
  for (uint8_t i = 0; i < 9; i++)
    memcpy (super->m17d.callhistory[i], super->m17d.callhistory[i+1], 500*sizeof(char));
  sprintf (super->m17d.callhistory[9], "%s %s CAN: %02d; SRC: %s; DST: %s; %s;", datestr, timestr, super->m17d.can, super->m17d.src_csd_str, super->m17d.dst_csd_str, dt);

  //make a version without the timestamp, but include other info
  char event_string[500]; char key[75]; sprintf(key, "%s", "");
  sprintf (event_string, " CAN: %02d; SRC: %s; DST: %s; %s;", super->m17d.can, super->m17d.src_csd_str, super->m17d.dst_csd_str, dt);

  if (super->m17d.enc_et == 0)
    strcat (event_string, " Clear");

  if (super->m17d.enc_et == 1)
  {
    strcat (event_string, " Scrambler");

    if (super->m17d.enc_st == 0)
      strcat (event_string, " 8-bit");
    if (super->m17d.enc_st == 1)
      strcat (event_string, " 16-bit");
    if (super->m17d.enc_st == 2)
      strcat (event_string, " 24-bit");

    if (super->enc.scrambler_key)
    {
      sprintf (key, " Key: %06X;", super->enc.scrambler_key);
      strcat (event_string, key);
    }
  }
    
  if (super->m17d.enc_et == 2)
  {
    strcat (event_string, " AES");
    if (super->enc.aes_key_is_loaded)
    {
      sprintf (key, " Key: %016llX %016llX %016llX %016llX;", super->enc.A1, super->enc.A2, super->enc.A3, super->enc.A4);
      strcat (event_string, key);
    }
  }

  //other misc special events
  if (super->m17d.dt == 4) sprintf (event_string, " Demodulator Reset; Polarity Change, RRC Filtering Change, or Debug Carrier Reset Event;");
  else if (super->m17d.dt > 6) sprintf (event_string, " Unknown Error Event (Synchronization Error?);");

  //send last call history to event_log_writer
  event_log_writer (super, event_string, 255);

  free (timestr); free (datestr);
}

//write events, like last call from call history, GNSS, text messages, etc to a log file, if enabled
void event_log_writer (Super * super, char * event_string, uint8_t protocol)
{
  //datestr and timestr are used if event is type 1 (does not already include a timestamp in its string)
  char * timestr  = get_time_n(super->demod.current_time);
  char * datestr  = get_date_n(super->demod.current_time);

  //only if the log file is open
  if (super->opts.event_log)
  {
    //write date and time
    fprintf (super->opts.event_log, "%s_%s ", datestr, timestr);

    //add type of event by protocol (mirrors pkt decoder, plus special numbers for call history, per call, etc)
    if (protocol == 255)
      fprintf (super->opts.event_log, "Call History: ");

    else if (protocol == 254)
      fprintf (super->opts.event_log, "Per Call Wav Closed: ");

    else if (protocol == 253)
      fprintf (super->opts.event_log, "Per Call Wav Opened: ");

    else if (protocol == 0)
      fprintf (super->opts.event_log, "RAW: ");

    else if (protocol == 1)
      fprintf (super->opts.event_log, "AX.25: ");

    else if (protocol == 2)
      fprintf (super->opts.event_log, "APRS: ");

    else if (protocol == 3)
      fprintf (super->opts.event_log, "6LoWPAN: ");

    else if (protocol == 4)
      fprintf (super->opts.event_log, "IPv4: ");

    else if (protocol == 5)
      fprintf (super->opts.event_log, "SMS: ");

    else if (protocol == 6)
      fprintf (super->opts.event_log, "Winlink: ");

    else if (protocol == 9)
      fprintf (super->opts.event_log, "OTAKD: ");

    else if (protocol == 90)
      fprintf (super->opts.event_log, "Meta Text Data: ");

    else if (protocol == 91)
      fprintf (super->opts.event_log, "Meta GNSS: ");

    else if (protocol == 92)
      fprintf (super->opts.event_log, "Meta Extended CSD: ");

    else if (protocol == 99)
      fprintf (super->opts.event_log, "1600 Arbitrary Data: ");

    else fprintf (super->opts.event_log, "Unknown Event Protocol %02X: ", protocol);

    fprintf (super->opts.event_log, "%s\n", event_string);
    fflush (super->opts.event_log); //flush event so its immediately available without having to close the file
  }

  free (timestr); free (datestr);
}