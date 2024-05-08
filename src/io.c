/*-------------------------------------------------------------------------------
 * io.c
 * Project M17 - Input and Output Open and Close Functions
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
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
  
  //Might make an all-in-one SND file input open by passing a srtring to the input name for that?
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

  else if (super->opts.use_stdin_input == 1)
  {
    stdin_snd_audio_source_open(super);
    super->opts.tcp_input_open = 1;
    super->opts.use_snd_input = 1;
  }

  //NOTE: This is working well yet, needs work in demod
  else if (super->opts.snd_input_is_a_file == 1)
  {
    file_snd_audio_source_open(super);
    super->opts.use_snd_input = 1;
  }

  //TODO: Test Setup OSS Input
  else if (super->opts.use_oss_input == 1)
  {
    open_oss_input(super);
  }

  #ifdef USE_PULSEAUDIO
  else if (super->opts.use_pa_input == 1)
  {
    open_pulse_audio_input (super);

  }
  #endif

}

//Including WAV files
void open_audio_output (Super * super)
{

  //wav and misc output files
  if (super->opts.use_wav_out_rf == 1)
    open_wav_out_rf(super);
  if (super->opts.use_wav_out_vx == 1)
    open_wav_out_vx(super);

  if (super->opts.use_float_symbol_output == 1)
    super->opts.float_symbol_out = fopen (super->opts.float_symbol_output_file, "w");

  //DSD-FME Dibit Capture Bin File
  if (super->opts.use_dibit_output == 1)
    super->opts.dibit_out = fopen (super->opts.dibit_output_file, "w");

  //TODO: TEST if-elseif-elseif statements for these
  //TODO: Test Setup OSS Output
  if (super->opts.use_oss_output == 1)
    open_oss_output(super);

  else if (super->opts.use_stdout_output == 1)
    open_stdout_pipe(super);

  #ifdef USE_PULSEAUDIO
  else //honestly, this else may not really be needed
  {
    if (super->opts.use_pa_output_rf == 1)
      open_pulse_audio_output_rf (super);
    if (super->opts.use_pa_output_vx == 1)
      open_pulse_audio_output_vx (super);
  }
  #endif

}

void cleanup_and_exit (Super * super)
{
  // Signal that everything should shutdown.
  exitflag = 1;

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
    //TODO: Full String Parse, if supplied (copy and paste from somewhere else)
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
      fprintf (stderr, "Audio  Input Device: Pulse RF Input;");
      super->opts.use_m17_rfa_decoder = 1; //enable the RF decoder if puslerf is detected
    }
    else if ( (strncmp(input, "pulsevx", 7) == 0) )
      fprintf (stderr, "Audio  Input Device: Pulse Mic Input;"); //default is for encoder voice
    else
    {
      fprintf (stderr, "Audio  Input Device: Pulse RF Input;");
      super->opts.use_m17_rfa_decoder = 1; //enable the RF decoder if puslerf is detected
    }
    super->opts.use_pa_input = 1;
  }
  #else
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
      fprintf (stderr, "Audio Output Device: Pulse RF Output;");

    else if (super->opts.use_pa_output_vx == 1)
    {
      super->opts.monitor_encode_internally = 1; //may disable this later
      fprintf (stderr, "Audio Output Device: Pulse Voice Output;");
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


//may put this in the input parser instead, 
//but also is needed for output, so this may be better
void parse_udp_user_string (Super * super, char * input)
{

  char * curr;

  //debug
  // fprintf (stderr, "UDP INPUT STR: %s\n", input);

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

    //debug char to number
    // fprintf (stderr, "%d:%d:%d; ", super->m17e.reflector_module, *curr, 'A');

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

  //since it was enabled by the user, go ahead and turn it on here
  // super->opts.m17_use_ip = 1; //no, could be for input or output

}