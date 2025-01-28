/*-------------------------------------------------------------------------------
 * convolution.c
 * M17 Project - Convolutional Encoder and Convolutional / Viterbi Decoder(s)
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void simple_conv_encoder (uint8_t * input, uint8_t * output, int len)
{
  int i, k = 0;
  uint8_t g1 = 0;
  uint8_t g2 = 0;
  uint8_t d  = 0;
  uint8_t d1 = 0;
  uint8_t d2 = 0;
  uint8_t d3 = 0;
  uint8_t d4 = 0;

  for (i = 0; i < len; i++)
  {
    d = input[i];

    g1 = (d + d3 + d4) & 1;
    g2 = (d + d1 + d2 + d4) & 1;

    d4 = d3;
    d3 = d2;
    d2 = d1;
    d1 = d;

    output[k++] = g1;
    output[k++] = g2;
  }
}

/*   Convolution Decoder Original Header Credits / License
 *   Copyright (C) 2009-2016,2018 by Jonathan Naylor G4KLX
 *
 *   Copyright (C) 2018 by Edouard Griffiths F4EXB:
 *   - Cosmetic changes to integrate with DSDcc
 *
 *   Copyright (C) 2018 by Louis HERVE F4HUZ:
 *   - Transform C++ lib into C lib to integrate with DSD
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

static const uint8_t CONVOLUTION_BIT_MASK_TABLE[8] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | CONVOLUTION_BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~CONVOLUTION_BIT_MASK_TABLE[(i)&7])

static const uint8_t CONVOLUTION_BRANCH_TABLE1[8] = {0U, 0U, 0U, 0U, 2U, 2U, 2U, 2U};
static const uint8_t CONVOLUTION_BRANCH_TABLE2[8] = {0U, 2U, 2U, 0U, 0U, 2U, 2U, 0U};

static const unsigned int CONVOLUTION_NUM_OF_STATES_D2 = 8U;
static const uint32_t     CONVOLUTION_M = 4U;
static const unsigned int CONVOLUTION_K = 5U;

static uint16_t   m_metrics1[16];
static uint16_t   m_metrics2[16];
static uint64_t   m_decisions[8*300];
uint16_t * previous_metrics = NULL;
uint16_t * current_metrics = NULL;
uint64_t * m_dp = NULL;

uint16_t convolution_decode(uint8_t s0, uint8_t s1)
{
  uint8_t    i = 0;
  uint8_t    j = 0;
  uint8_t    decision0 = 0;
  uint8_t    decision1 = 0;
  uint16_t   metric = 0;
  uint16_t   m0 = 0;
  uint16_t   m1 = 0;
  uint16_t * tmp = NULL;

  *m_dp = 0U;

  for (i = 0U; i < CONVOLUTION_NUM_OF_STATES_D2; i++)
  {
    j = i * 2U;

    metric = abs(CONVOLUTION_BRANCH_TABLE1[i] - s0) + abs(CONVOLUTION_BRANCH_TABLE2[i] - s1);

    m0 = previous_metrics[i] + metric;
    m1 = previous_metrics[i + CONVOLUTION_NUM_OF_STATES_D2] + (CONVOLUTION_M - metric);

    decision0 = (m0 >= m1) ? 1U : 0U;
    current_metrics[j + 0U] = decision0 != 0U ? m1 : m0;

    m0 = previous_metrics[i] + (CONVOLUTION_M - metric);
    m1 = previous_metrics[i + CONVOLUTION_NUM_OF_STATES_D2] + metric;

    decision1 = (m0 >= m1) ? 1U : 0U;
    current_metrics[j + 1U] = decision1 != 0U ? m1 : m0;

    *m_dp |= ((uint64_t)(decision1) << (j + 1U)) | ((uint64_t)(decision0) << (j + 0U));
  }

  ++m_dp;

  tmp = current_metrics;
  current_metrics = previous_metrics;
  previous_metrics = tmp;

  //what was the lowest metric
  uint32_t cost = previous_metrics[0];
  for(i = 1; i < 16; i++)
  {
    if(previous_metrics[i] < cost)
      cost = (uint32_t)previous_metrics[i];
  }

  return cost; //return the lowest metric
}

void convolution_chainback(unsigned char* out, unsigned int nBits)
{
  uint32_t state = 0U;
  uint32_t i = 0;
  uint8_t  bit = 0;

  while (nBits-- > 0)
  {
    --m_dp;

    i = state >> (9 - CONVOLUTION_K);
    bit = (uint8_t)(*m_dp >> i) & 1;
    state = (bit << 7) | (state >> 1);

    WRITE_BIT1(out, nBits, bit != 0U);
  }

  //look in chainback for last lowest metric
  uint32_t cost = previous_metrics[0];
  for(i = 1; i < 16; i++)
  {
    if(previous_metrics[i] < cost)
      cost = (uint32_t)previous_metrics[i];
  }
  // fprintf (stderr, " cost: %05d; ", cost);
}

void convolution_start()
{
  convolution_init(); //reset metrics and decisions
  previous_metrics = m_metrics1;
  current_metrics = m_metrics2;
  m_dp = m_decisions;
}

void convolution_init()
{
  memset(m_metrics1, 0x0, 16*sizeof(uint16_t));
  memset(m_metrics2, 0x0, 16*sizeof(uint16_t));
  memset(m_decisions,0x0, 8*300*sizeof(uint64_t));
}
