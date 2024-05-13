/*-------------------------------------------------------------------------------
 * golay.c
 * M17 Project - Golay 24_12 Encoder and Decoder
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016 Edouard Griffiths, F4EXB.                                  //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include "main.h"

unsigned char Golay_24_12_m_corr[4096][3]; //!< up to 3 bit error correction by syndrome index

//!< Generator matrix of bits
const unsigned char Golay_24_12_m_G[24*12] = {
1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1,
0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1,
0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,   1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0,
0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,   0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0,
0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,   0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0,
0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,   1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1,
0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,   0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1,
0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,   0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1,
0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,   1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,   1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,   1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,   1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1,
};

//!< Parity check matrix of bits
const unsigned char Golay_24_12_m_H[24*12] = {
1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1,   1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0,   0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0,   0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0,   0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1,   0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1,   0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0,   0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1,   0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
};

void Golay_24_12_init()
{
    int i1 = 0, i2 = 0, i3 = 0, ir = 0, ip = 0;
    int syndromeI = 0, syndromeIP = 0;
    int ip1 = 0, ip2 = 0, ip3 = 0;
    int syndromeIP1 = 0, syndromeIP2 = 0, syndromeIP3 = 0;

    memset (Golay_24_12_m_corr, 0xFF, 3*4096);

    for (i1 = 0; i1 < 12; i1++)
    {
        for (i2 = i1+1; i2 < 12; i2++)
        {
            for (i3 = i2+1; i3 < 12; i3++)
            {
                // 3 bit patterns
                syndromeI = 0;

                for (ir = 0; ir < 12; ir++)
                {
                    syndromeI += ((Golay_24_12_m_H[24*ir + i1] +  Golay_24_12_m_H[24*ir + i2] +  Golay_24_12_m_H[24*ir + i3]) % 2) << (11-ir);
                }

                Golay_24_12_m_corr[syndromeI][0] = i1;
                Golay_24_12_m_corr[syndromeI][1] = i2;
                Golay_24_12_m_corr[syndromeI][2] = i3;
            }

            // 2 bit patterns
            syndromeI = 0;

            for (ir = 0; ir < 12; ir++)
            {
                syndromeI += ((Golay_24_12_m_H[24*ir + i1] +  Golay_24_12_m_H[24*ir + i2]) % 2) << (11-ir);
            }

            Golay_24_12_m_corr[syndromeI][0] = i1;
            Golay_24_12_m_corr[syndromeI][1] = i2;

            // 1 possible bit flip left in the parity part
            for (ip = 0; ip < 12; ip++)
            {
                syndromeIP = syndromeI ^ (1 << (11-ip));
                Golay_24_12_m_corr[syndromeIP][0] = i1;
                Golay_24_12_m_corr[syndromeIP][1] = i2;
                Golay_24_12_m_corr[syndromeIP][2] = 12 + ip;
            }
        }

        // single bit patterns
        syndromeI = 0;

        for (ir = 0; ir < 12; ir++)
        {
            syndromeI += Golay_24_12_m_H[24*ir + i1] << (11-ir);
        }

        Golay_24_12_m_corr[syndromeI][0] = i1;

        for (ip1 = 0; ip1 < 12; ip1++) // 1 more bit flip in parity
        {
            syndromeIP1 = syndromeI ^ (1 << (11-ip1));
            Golay_24_12_m_corr[syndromeIP1][0] = i1;
            Golay_24_12_m_corr[syndromeIP1][1] = 12 + ip1;

            for (ip2 = ip1+1; ip2 < 12; ip2++) // 1 more bit flip in parity
            {
                syndromeIP2 = syndromeIP1 ^ (1 << (11-ip2));
                Golay_24_12_m_corr[syndromeIP2][0] = i1;
                Golay_24_12_m_corr[syndromeIP2][1] = 12 + ip1;
                Golay_24_12_m_corr[syndromeIP2][2] = 12 + ip2;
            }
        }
    }

    // no bit patterns (in message) -> all in parity
    for (ip1 = 0; ip1 < 12; ip1++) // 1 bit flip in parity
    {
        syndromeIP1 =  (1 << (11-ip1));
        Golay_24_12_m_corr[syndromeIP1][0] = 12 + ip1;

        for (ip2 = ip1+1; ip2 < 12; ip2++) // 1 more bit flip in parity
        {
            syndromeIP2 = syndromeIP1 ^ (1 << (11-ip2));
            Golay_24_12_m_corr[syndromeIP2][0] = 12 + ip1;
            Golay_24_12_m_corr[syndromeIP2][1] = 12 + ip2;

            for (ip3 = ip2+1; ip3 < 12; ip3++) // 1 more bit flip in parity
            {
                syndromeIP3 = syndromeIP2 ^ (1 << (11-ip3));
                Golay_24_12_m_corr[syndromeIP3][0] = 12 + ip1;
                Golay_24_12_m_corr[syndromeIP3][1] = 12 + ip2;
                Golay_24_12_m_corr[syndromeIP3][2] = 12 + ip3;
            }
        }
    }
}

// Not very efficient but encode is used for unit testing only, I think this is wrong for M17
void Golay_24_12_encode(unsigned char *origBits, unsigned char *encodedBits)
{
  int i = 0, j = 0;
  // memset(encodedBits, 0, 24);
  for (i = 0; i < 12; i++)
  {
    for (j = 0; j < 24; j++)
      encodedBits[j] += origBits[i] * Golay_24_12_m_G[24*i + j];
  }
  for (i = 0; i < 24; i++)
    encodedBits[i] %= 2;
}

bool Golay_24_12_decode(unsigned char *rxBits)
{
    unsigned int syndromeI = 0; // syndrome index
    int is = 0;
    int i = 0;

    for (is = 0; is < 12; is++)
    {
        syndromeI += (((rxBits[0]  * Golay_24_12_m_H[24*is + 0])
                     + (rxBits[1]  * Golay_24_12_m_H[24*is + 1])
                     + (rxBits[2]  * Golay_24_12_m_H[24*is + 2])
                     + (rxBits[3]  * Golay_24_12_m_H[24*is + 3])
                     + (rxBits[4]  * Golay_24_12_m_H[24*is + 4])
                     + (rxBits[5]  * Golay_24_12_m_H[24*is + 5])
                     + (rxBits[6]  * Golay_24_12_m_H[24*is + 6])
                     + (rxBits[7]  * Golay_24_12_m_H[24*is + 7])
                     + (rxBits[8]  * Golay_24_12_m_H[24*is + 8])
                     + (rxBits[9]  * Golay_24_12_m_H[24*is + 9])
                     + (rxBits[10] * Golay_24_12_m_H[24*is + 10])
                     + (rxBits[11] * Golay_24_12_m_H[24*is + 11])
                     + (rxBits[12] * Golay_24_12_m_H[24*is + 12])
                     + (rxBits[13] * Golay_24_12_m_H[24*is + 13])
                     + (rxBits[14] * Golay_24_12_m_H[24*is + 14])
                     + (rxBits[15] * Golay_24_12_m_H[24*is + 15])
                     + (rxBits[16] * Golay_24_12_m_H[24*is + 16])
                     + (rxBits[17] * Golay_24_12_m_H[24*is + 17])
                     + (rxBits[18] * Golay_24_12_m_H[24*is + 18])
                     + (rxBits[19] * Golay_24_12_m_H[24*is + 19])
                     + (rxBits[20] * Golay_24_12_m_H[24*is + 20])
                     + (rxBits[21] * Golay_24_12_m_H[24*is + 21])
                     + (rxBits[22] * Golay_24_12_m_H[24*is + 22])
                     + (rxBits[23] * Golay_24_12_m_H[24*is + 23])) % 2) << (11-is);
    }

    if (syndromeI > 0)
    {
        i = 0;

        for (; i < 3; i++)
        {
            if (Golay_24_12_m_corr[syndromeI][i] == 0xFF)
            {
                break;
            }
            else
            {
                rxBits[Golay_24_12_m_corr[syndromeI][i]] ^= 1; // flip bit
            }
        }

        if (i == 0)
        {
            return false;
        }
    }

    return true;
}