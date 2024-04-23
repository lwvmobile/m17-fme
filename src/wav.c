/*-------------------------------------------------------------------------------
 * wav.c
 * Project M17 - Wav File Handling
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void open_wav_out_rf (Super * super)
{
  SF_INFO info;
  info.samplerate = 48000;
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
  info.samplerate = 48000;
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

//NOTE: Handling of STDIN is through SNDFILE, so I am going to place those functions here
//since they are essentially treated like wav files
void open_stdin_pipe(Super * super)
{
  super->wav.audio_in_file_info = calloc(1, sizeof(SF_INFO));
  super->wav.audio_in_file_info->samplerate = 48000;
  super->wav.audio_in_file_info->channels = 1;
  super->wav.audio_in_file_info->seekable = 0;
  super->wav.audio_in_file_info->format=SF_FORMAT_RAW|SF_FORMAT_PCM_16|SF_ENDIAN_LITTLE;
  super->wav.audio_in_file = sf_open_fd(fileno(stdin), SFM_READ, super->wav.audio_in_file_info, 0);

  if(super->wav.audio_in_file == NULL)
  {
    fprintf(stderr, "Error, couldn't open stdin with libsndfile: %s\n", sf_strerror(NULL));
    exit(1);
  }
}

short read_stdin_pipe(Super * super)
{
  short sample = 0;
  int result = sf_read_short(super->wav.audio_in_file, &sample, 1);
  if(result == 0)
  {
    sf_close(super->wav.audio_in_file);
    exitflag = 1;
  }
  return sample;
}
