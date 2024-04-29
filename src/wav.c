/*-------------------------------------------------------------------------------
 * wav.c
 * Project M17 - Wav File Handling
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void open_wav_out_rf (Super * super)
{
  SF_INFO info;
  info.samplerate = super->opts.output_sample_rate;
  info.channels = 1;
  info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
  super->wav.wav_out_rf = sf_open (super->wav.wav_out_file_rf, SFM_WRITE, &info);

  if (super->wav.wav_out_rf == NULL)
  {
    fprintf (stderr,"Error - could not open RF wav output file %s\n", super->wav.wav_out_file_rf);
    return;
  }
}

void open_wav_out_vx (Super * super)
{
  SF_INFO info;
  info.samplerate = super->opts.output_sample_rate;
  info.channels = 1;
  info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
  super->wav.wav_out_vx = sf_open (super->wav.wav_out_file_vx, SFM_WRITE, &info);

  if (super->wav.wav_out_vx == NULL)
  {
    fprintf (stderr,"Error - could not open VX wav output file %s\n", super->wav.wav_out_file_vx);
    return;
  }
}

void close_wav_out_rf (Super * super)
{
  sf_close(super->wav.wav_out_rf);
}

void close_wav_out_vx (Super * super)
{
  sf_close(super->wav.wav_out_vx);
}

void write_wav_out_rf (Super * super, short * out, size_t nsam)
{
  sf_write_short(super->wav.wav_out_rf, out, nsam);
}

void write_wav_out_vx (Super * super, short * out, size_t nsam)
{
  sf_write_short(super->wav.wav_out_vx, out, nsam);
}

//move?
bool stdin_snd_audio_source_open (Super * super)
{
  bool err = false;
  super->snd_src_in.audio_in_file = sf_open_fd(fileno(stdin), SFM_READ, super->snd_src_in.audio_in_file_info, 0);
  if (super->snd_src_in.audio_in_file != NULL)
    err = true;
  return err;
}

bool file_snd_audio_source_open (Super * super)
{
  bool err = false;
  super->snd_src_in.audio_in_file = sf_open(super->snd_src_in.snd_in_filename, SFM_READ, super->snd_src_in.audio_in_file_info);
  if (super->snd_src_in.audio_in_file != NULL)
    err = true;
  return err;
}

short snd_input_read (Super * super)
{
  short sample = 0;
  int result = sf_read_short(super->snd_src_in.audio_in_file, &sample, 1);
  if(result == 0)
  {
    sf_close(super->snd_src_in.audio_in_file);
    super->snd_src_in.audio_in_file = NULL;
    exitflag = 1;
  }
  return sample;
}