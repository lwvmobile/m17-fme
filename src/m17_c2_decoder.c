/*-------------------------------------------------------------------------------
 * m17_c2_decoder.c
 * Project M17 - Codec2 Decoding and and Arbitrary Data Handling
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void decode_str_payload(config_opts * opts, m17_decoder_state * m17d, wav_state * wav, pa_state * pa, uint8_t * payload, uint8_t type)
{
 
  UNUSED(opts); UNUSED(m17d); UNUSED(wav); UNUSED(pa);
  int i, j; UNUSED(j);
  unsigned char voice1[8];
  unsigned char voice2[8];

  for (i = 0; i < 8; i++)
  {
    voice1[i] = (unsigned char)ConvertBitIntoBytes(&payload[i*8+0], 8);
    voice2[i] = (unsigned char)ConvertBitIntoBytes(&payload[i*8+64], 8);
  }

  //TODO: Add some decryption methods
  // if (state->m17_enc != 0)
  // {
  //   //process scrambler or AES-CTR decryption 
  //   //(no AES-CTR right now, Scrambler should be easy enough)
  // }

  // if (opts->payload == 1)
  {
    fprintf (stderr, "\n CODEC2: ");
    for (i = 0; i < 8; i++)
      fprintf (stderr, "%02X", voice1[i]);
    fprintf (stderr, " (1600)");

    if (type == 3)
      fprintf (stderr, "\n A_DATA: "); //arbitrary data
    else fprintf (stderr, "\n CODEC2: ");
    for (i = 0; i < 8; i++)
      fprintf (stderr, "%02X", voice2[i]);
  }
  
  #ifdef USE_CODEC2
  size_t nsam;
  if (type == 3) nsam = 320;
  else nsam = 160;

  //converted to using allocated memory pointers to prevent the overflow issues
  short * samp1 = malloc (sizeof(short) * nsam);
  short * samp2 = malloc (sizeof(short) * nsam);

  short * upsamp1 = malloc (sizeof(short) * nsam * 6);
  short * upsamp2 = malloc (sizeof(short) * nsam * 6);
  short * out = malloc (sizeof(short) * 6);

  if (type == 3)
    codec2_decode(m17d->codec2_1600, samp1, voice1);
  else
  {
    codec2_decode(m17d->codec2_3200, samp1, voice1);
    codec2_decode(m17d->codec2_3200, samp2, voice2);
  }

  //TODO LIST:

  //Run HPF on decoded voice prior to upsample
  // if (opts->use_hpf_d == 1)
  //   hpf_dL(state, samp1, nsam);

  //Upsample

  //PA Playback

  //Wav File Playback

  //C2 File Save

  //TODO: Codec2 Raw file saving
  // if(mbe_out_dir)
  // {

  // }

  free (samp1);
  free (samp2);
  free (upsamp1);
  free (upsamp2);
  free (out);

  #endif

  //handle arbitrary data, if 1600
  if (type == 3)
  {
    uint8_t adata[9]; adata[0] = 99; //set so pkt decoder will rip these out as just utf-8 chars
    for (i = 0; i < 8; i++)
      adata[i+1] = (unsigned char)ConvertBitIntoBytes(&payload[i*8+64], 8);
    
    //look and see if the payload has stuff in it first, if so, then run this
    if (adata[1] != 0 || adata[2] != 0 || adata[3] != 0 || adata[4] != 0 || adata[5] != 0 || adata[6] != 0 || adata[7] != 0 || adata[8] != 0)
    {
      fprintf (stderr, "\n"); //linebreak
      decode_pkt_contents (adata, 9); //decode Arbitrary Data as UTF-8
    }
  }
  
}