/*-------------------------------------------------------------------------------
 * m17_pyl_decoder.c
 * Project M17 - Payload Codec2 and Arbitrary Data Handling
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void decode_str_payload(Super * super, uint8_t * payload, uint8_t type)
{
 
  // UNUSED(opts);
  int i;
  unsigned char voice1[8];
  unsigned char voice2[8];

  for (i = 0; i < 8; i++)
  {
    voice1[i] = (unsigned char)ConvertBitIntoBytes(&payload[i*8+0], 8);
    voice2[i] = (unsigned char)ConvertBitIntoBytes(&payload[i*8+64], 8);
  }

  //TODO: Add some decryption methods?

  if (super->opts.payload_verbosity >= 1)
  {
    if (type == 2)
      fprintf (stderr, "\n CODEC2 (3200): ");
    else
      fprintf (stderr, "\n CODEC2 (1600): ");
    for (i = 0; i < 8; i++)
      fprintf (stderr, "%02X", voice1[i]);

    if (type == 2)
      fprintf (stderr, "\n CODEC2 (3200): ");
    else
      fprintf (stderr, "\n        A_DATA: ");
    for (i = 0; i < 8; i++)
      fprintf (stderr, "%02X", voice2[i]);
  }
  
  #ifdef USE_CODEC2
  size_t nsam;
  if (type == 3) nsam = 320;
  else nsam = 160;

  //allocated memory for codec2 audio handling
  short * samp1 = malloc (sizeof(short) * nsam);
  short * samp2 = malloc (sizeof(short) * nsam);
  short * upsamp1 = malloc (sizeof(short) * nsam * 6);
  short * upsamp2 = malloc (sizeof(short) * nsam * 6);

  if (type == 2)
  {
    codec2_decode(super->m17d.codec2_3200, samp1, voice1);
    codec2_decode(super->m17d.codec2_3200, samp2, voice2);
  }
  else
    codec2_decode(super->m17d.codec2_1600, samp1, voice1);

  //TODO LIST:

  //Run HPF on decoded voice prior to upsample
  if (super->opts.use_hpfilter_dig == 1)
    hpfilter_d(super, samp1, nsam);

  //Upsample 8k to 48k
  for (i = 0; i < (int)nsam; i++)
  {
    upsample_6x(samp1[i], upsamp1+(i*6));
    if (type == 2)
      upsample_6x(samp2[i], upsamp2+(i*6));
  }

  //Pulse Audio Playback
  if (super->pa.pa_output_vx_is_open == 1)
  {
    pulse_audio_output_vx(super, upsamp1, nsam*6);
    if (type == 2)
      pulse_audio_output_vx(super, upsamp2, nsam*6);
  }

  //Wav File Saving
  if (super->wav.wav_out_vx != NULL)
  {
    write_wav_out_vx(super, upsamp1, nsam*6);
    if (type == 2)
      write_wav_out_vx(super, upsamp2, nsam*6);
    sf_write_sync (super->wav.wav_out_vx);
  }

  //C2 File Save

  free (samp1);
  free (samp2);
  free (upsamp1);
  free (upsamp2);

  #endif

  //decode arbitrary data, if 1600
  if (type == 3)
  {
    uint8_t adata[9]; adata[0] = 99; //set so pkt decoder will rip these out as just utf-8 chars
    for (i = 0; i < 8; i++)
      adata[i+1] = (unsigned char)ConvertBitIntoBytes(&payload[i*8+64], 8);
    
    //look and see if the payload has stuff in it first, if so, then run this
    if (adata[1] != 0 || adata[2] != 0 || adata[3] != 0 || adata[4] != 0 || adata[5] != 0 || adata[6] != 0 || adata[7] != 0 || adata[8] != 0)
    {
      fprintf (stderr, "\n"); //linebreak
      decode_pkt_contents (super, adata, 9); //decode Arbitrary Data as UTF-8
    }
  }
  
}