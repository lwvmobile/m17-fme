/*-------------------------------------------------------------------------------
 * crc16.c
 * M17 Project - CRC16 Checksum
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

uint16_t crc16(const uint8_t *in, const uint16_t len)
{
  uint32_t crc = 0xFFFF;
  uint16_t poly = 0x5935;
  for(uint16_t i=0; i<len; i++)
  {
    crc^=in[i]<<8;
    for(uint8_t j=0; j<8; j++)
    {
      crc<<=1;
      if(crc&0x10000)
        crc=(crc^poly)&0xFFFF;
    }
  }

  return crc&(0xFFFF);
}