/*-------------------------------------------------------------------------------
 * wav.c
 * Project M17 - Wav File Handling
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void open_wav_out_rf (wav_state * wav)
{
  SF_INFO info;
  info.samplerate = 48000;
  info.channels = 1;
  info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
  wav->wav_out_rf = sf_open (wav->wav_out_file_rf, SFM_WRITE, &info);

  if (wav->wav_out_rf == NULL)
  {
    fprintf (stderr,"Error - could not open RF wav output file %s\n", wav->wav_out_file_rf);
    return;
  }
}

void open_wav_out_vx (wav_state * wav)
{
  SF_INFO info;
  info.samplerate = 48000;
  info.channels = 1;
  info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
  wav->wav_out_vx = sf_open (wav->wav_out_file_vx, SFM_WRITE, &info);

  if (wav->wav_out_vx == NULL)
  {
    fprintf (stderr,"Error - could not open VX wav output file %s\n", wav->wav_out_file_vx);
    return;
  }
}

void close_wav_out_rf (wav_state * wav)
{
  sf_close(wav->wav_out_rf);
}

void close_wav_out_vx (wav_state * wav)
{
  sf_close(wav->wav_out_vx);
}

void write_wav_out_rf (wav_state * wav, short * out, size_t nsam)
{
  sf_write_short(wav->wav_out_rf, out, nsam);
}

void write_wav_out_vx (wav_state * wav, short * out, size_t nsam)
{
  sf_write_short(wav->wav_out_vx, out, nsam);
}