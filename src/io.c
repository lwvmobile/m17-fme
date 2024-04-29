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
  {
    close_ncurses_terminal();
    super->opts.ncurses_is_open = 0;
  }
  #endif

  exit(0);
}