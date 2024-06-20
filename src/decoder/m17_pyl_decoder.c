/*-------------------------------------------------------------------------------
 * m17_pyl_decoder.c
 * M17 Project - Payload Codec2 and Arbitrary Data Handling
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void decode_str_payload(Super * super, uint8_t * payload, uint8_t type, uint8_t lich_cnt)
{
 
  int i;
  int mute = 1; //if ENC and not decrypted, mute
  unsigned char voice1[8];
  unsigned char voice2[8];

  //apply keystream pN sequence here if scrambler enc and key is available
  //note: bit_counter is now seperate for encoding and decoding (internal loopback fix)
  if (super->m17d.enc_et == 1 && super->enc.scrambler_key != 0)
  {
    mute = 0;
    //NOTE: Below condition does not gaurantee perfect reception if marginal or bad signal and FN value is misaligned randomly with SEED value
    //but if we constantly recalculate a seed, the higher the FN sequence, the higher the probability of LAG incurring on slower devices
    if (super->enc.scrambler_fn_d != 0 && super->enc.scrambler_seed_d == super->enc.scrambler_key) //if the seed is currently the key value, but the fn value is not zero
    {
      super->enc.scrambler_seed_d = scrambler_seed_calculation(super->m17d.enc_st, super->enc.scrambler_key, super->enc.scrambler_fn_d);
    }
    super->enc.scrambler_seed_d = scrambler_sequence_generator(super, 0);
    for (i = 0; i < 128; i++)
      payload[i] ^= super->enc.scrambler_pn[i];
  }
  //generate AES Keystream and apply it to payload if AES enc and key is available
  else if (super->m17d.enc_et == 2 && super->enc.aes_key_is_loaded)
  {
    mute = 0;
    aes_ctr_str_payload_crypt (super->m17d.meta, super->enc.aes_key, payload, 1);
  }
  else if (super->m17d.enc_et == 3)
  {
    mute = 1; //unknown type, so mute
    //space for custom or new ENC type
  }
  else if (super->m17d.enc_et == 0)
    mute = 0; //no encryption, unmute
  
  for (i = 0; i < 8; i++)
  {
    voice1[i] = (unsigned char)convert_bits_into_output(&payload[i*8+0], 8);
    voice2[i] = (unsigned char)convert_bits_into_output(&payload[i*8+64], 8);
  }

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

  //at this point, if we are ENC'd and no key, then log and skip playback
  if      (!mute)
    super->m17d.enc_mute = 0;
  else if (mute)
  {
    super->m17d.enc_mute = 1;
    fprintf (stderr, " MUTED ");
    goto END_PAYLOAD;
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

  //Apply Gain to Output
  output_gain_vx (super, samp1, nsam);
  if (type == 2)
    output_gain_vx (super, samp2, nsam);

  //Run HPF on decoded voice prior to upsample
  if (super->opts.use_hpfilter_dig == 1)
  {
    hpfilter_d(super, samp1, nsam);
    if (type == 2)
      hpfilter_d(super, samp2, nsam);
  }

  //Upsample 8k to 48k
  for (i = 0; i < (int)nsam; i++)
  {
    upsample_6x(samp1[i], upsamp1+(i*6));
    if (type == 2)
      upsample_6x(samp2[i], upsamp2+(i*6));
  }

  //TODO: Make Convenience Audio Output Handler

  //Pulse Audio Playback
  #ifdef USE_PULSEAUDIO
  if (super->pa.pa_output_vx_is_open == 1)
  {
    pulse_audio_output_vx(super, upsamp1, nsam*6);
    if (type == 2)
      pulse_audio_output_vx(super, upsamp2, nsam*6);
  }
  #else
  if (super->pa.pa_output_vx_is_open == 1) {}
  #endif

  else if (super->opts.oss_output_device)
  {
    oss_output_write(super, upsamp1, nsam*6);
    if (type == 2)
      oss_output_write(super, upsamp2, nsam*6);
  }

  else if (super->opts.stdout_pipe)
  {
    write_stdout_pipe(super, upsamp1, nsam*6);
    if (type == 2)
      write_stdout_pipe(super, upsamp2, nsam*6);
  }

  //VX Wav File Saving
  if (super->wav.wav_out_vx != NULL)
  {
    write_wav_out_vx(super, upsamp1, nsam*6);
    if (type == 2)
      write_wav_out_vx(super, upsamp2, nsam*6);
    sf_write_sync (super->wav.wav_out_vx);
  }

  //Per Call Wav File Saving
  if (super->wav.wav_out_pc != NULL)
  {
    write_wav_out_pc(super, upsamp1, nsam*6);
    if (type == 2)
      write_wav_out_pc(super, upsamp2, nsam*6);
    sf_write_sync (super->wav.wav_out_pc);
  }

  //TODO: C2 File Save

  free (samp1);
  free (samp2);
  free (upsamp1);
  free (upsamp2);

  #endif

  //assemble and decode arbitrary data, if 1600
  if (type == 3)
  {
    //append incoming arbitrary data segment to m17d.raw bit array
    memcpy (super->m17d.raw+(lich_cnt*64), payload+64, 64);

    //sanity check
    if (lich_cnt > 5)
      lich_cnt = 5;

    if (lich_cnt == 5)
    {
      //6 x 8 octets, plus one protocol octet
      uint8_t adata[49]; adata[0] = 0x89;
      pack_bit_array_into_byte_array (super->m17d.raw, adata+1, 48);
      fprintf (stderr, "\n"); //linebreak
      decode_pkt_contents (super, adata, 48); //decode Arbitrary Data as UTF-8
      memset (super->m17d.raw, 0, sizeof(super->m17d.raw));
    }
  }

  END_PAYLOAD: {}; //do nothing
  
}