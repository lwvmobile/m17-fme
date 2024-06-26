/*-------------------------------------------------------------------------------
 * m17_lich_decoder.c
 * M17 Project - LICH Contents Assembly and Decoder
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

int decode_lich_contents(Super * super, uint8_t * lich_bits)
{
  int i, j, err;
  err = 0;

  uint8_t lich[4][24];
  uint8_t lich_decoded[48];
  uint8_t temp[96];
  bool g[4];

  uint8_t lich_counter = 0;
  uint8_t lich_reserve = 0; UNUSED(lich_reserve);

  uint16_t crc_cmp = 0;
  uint16_t crc_ext = 0;
  uint8_t crc_err = 0;

  memset(lich, 0, sizeof(lich));
  memset(lich_decoded, 0, sizeof(lich_decoded));
  memset(temp, 0, sizeof(temp));

  //execute golay 24,12 or 4 24-bit chunks and reorder into 4 12-bit chunks
  for (i = 0; i < 4; i++)
  {
    g[i] = true;

    for (j = 0; j < 24; j++)
      lich[i][j] = lich_bits[(i*24)+j];

    g[i] = golay_24_12_decode(lich[i]);
    if(g[i] == false)
    {
      //track errors
      super->error.golay_err++;
      err = -1;
    }

    for (j = 0; j < 12; j++)
      lich_decoded[i*12+j] = lich[i][j];

  }

  lich_counter = (uint8_t)convert_bits_into_output(&lich_decoded[40], 3); //lich_cnt
  lich_reserve = (uint8_t)convert_bits_into_output(&lich_decoded[43], 5); //lich_reserved

  //sanity check to prevent out of bounds
  if (lich_counter > 5) lich_counter = 5;

  if (err == 0)
    fprintf (stderr, "LC: %d/6 ", lich_counter+1);
  else fprintf (stderr, "LICH G24 ERR");

  //reset enc bit counter on each 0
  // if (lich_counter == 0)
  //   super->enc.bit_counter_d = 0;

  //transfer to storage
  for (i = 0; i < 40; i++)
    super->m17d.lsf[lich_counter*40+i] = lich_decoded[i];

  if (super->opts.payload_verbosity >= 1)
  {
    fprintf (stderr, "\n");
    fprintf (stderr, " LICH:");
    for (i = 0; i < 6; i++)
      fprintf (stderr, " %02X", (uint8_t)convert_bits_into_output(&lich_decoded[i*8], 8)); 
  }

  uint8_t lsf_packed[30];
  memset (lsf_packed, 0, sizeof(lsf_packed));

  if (lich_counter == 5)
  {

    //need to pack bytes for the sw5wwp variant of the crc (might as well, may be useful in the future)
    for (i = 0; i < 30; i++)
      lsf_packed[i] = (uint8_t)convert_bits_into_output(&super->m17d.lsf[i*8], 8);

    crc_cmp = crc16(lsf_packed, 28);
    crc_ext = (uint16_t)convert_bits_into_output(&super->m17d.lsf[224], 16);

    if (crc_cmp != crc_ext) crc_err = 1;

    if (crc_err == 0)
      decode_lsf_contents(super);
    else if (super->opts.allow_crc_failure == 1)
      decode_lsf_contents(super);

    if (super->opts.payload_verbosity >= 1)
    {
      fprintf (stderr, "\n LSF:");
      for (i = 0; i < 30; i++)
      {
        if (i == 15) fprintf (stderr, "\n     ");
        fprintf (stderr, " %02X", lsf_packed[i]);
      }
      fprintf (stderr, "\n      (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
    }

    memset (super->m17d.lsf, 0, sizeof(super->m17d.lsf));

    if (crc_err == 1) fprintf (stderr, "\n Embedded LSF CRC ERR");

    //track errors
    if (crc_err == 1) super->error.lsf_emb_crc_err++;

  }

  return err;
  
}