/*-------------------------------------------------------------------------------
 * slice.c
 * M17 Project - Soft Dibit Slicer, Viterbi for LSF frames (symbol based viterbi)
 *
 * LWVMOBILE
 * 2025-01 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//same as symbol levels in m17.h (m17-fme version)
const int8_t symbol_list[4]={-3, -1, +1, +3};

//may convert to ptr later
void slice_symbols(uint16_t out[2*SYM_PER_PLD], const float inp[SYM_PER_PLD])
{

    for(uint8_t i=0; i<SYM_PER_PLD; i++)
    {
        //bit 0
        if(inp[i]>=symbol_list[3])
        {
            out[i*2+1]=0xFFFF;
        }
        else if(inp[i]>=symbol_list[2])
        {
            out[i*2+1]=-(float)0xFFFF/(symbol_list[3]-symbol_list[2])*symbol_list[2]+inp[i]*(float)0xFFFF/(symbol_list[3]-symbol_list[2]);
        }
        else if(inp[i]>=symbol_list[1])
        {
            out[i*2+1]=0x0000;
        }
        else if(inp[i]>=symbol_list[0])
        {
            out[i*2+1]=(float)0xFFFF/(symbol_list[1]-symbol_list[0])*symbol_list[1]-inp[i]*(float)0xFFFF/(symbol_list[1]-symbol_list[0]);
        }
        else
        {
            out[i*2+1]=0xFFFF;
        }

        //bit 1
        if(inp[i]>=symbol_list[2])
        {
            out[i*2]=0x0000;
        }
        else if(inp[i]>=symbol_list[1])
        {
            out[i*2]=0x7FFF-inp[i]*(float)0xFFFF/(symbol_list[2]-symbol_list[1]);
        }
        else
        {
            out[i*2]=0xFFFF;
        }
    }
}

/**
 * @brief Utility function returning the absolute value of a difference between
 * two fixed-point values.
 *
 * @param v1 First value.
 * @param v2 Second value.
 * @return abs(v1-v2) value.
 */
uint16_t q_abs_diff(const uint16_t v1, const uint16_t v2)
{
    if(v2 > v1) return v2 - v1;
    return v1 - v2;
}

// M17 C library - lib/decode/viterbi.c
#define M17_CONVOL_K				      5									        //constraint length K=5
#define M17_CONVOL_STATES	        (1 << (M17_CONVOL_K - 1))	//number of states of the convolutional encoder

static uint32_t prevMetrics[M17_CONVOL_STATES];
static uint32_t currMetrics[M17_CONVOL_STATES];
static uint32_t prevMetricsData[M17_CONVOL_STATES];
static uint32_t currMetricsData[M17_CONVOL_STATES];
static uint16_t viterbi_history[244];

/**
 * @brief Reset the decoder state. No args.
 *
 */
void viterbi_reset(void)
{
  memset((uint8_t*)viterbi_history, 0, 2*244);
  memset((uint8_t*)currMetrics, 0, 4*M17_CONVOL_STATES);
  memset((uint8_t*)prevMetrics, 0, 4*M17_CONVOL_STATES);
  memset((uint8_t*)currMetricsData, 0, 4*M17_CONVOL_STATES);
  memset((uint8_t*)prevMetricsData, 0, 4*M17_CONVOL_STATES);
}

/**
 * @brief Decode one bit and update trellis.
 *
 * @param s0 Cost of the first symbol.
 * @param s1 Cost of the second symbol.
 * @param pos Bit position in history.
 */
void viterbi_decode_bit(uint16_t s0, uint16_t s1, const size_t pos)
{
    static const uint16_t COST_TABLE_0[] = {0, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    static const uint16_t COST_TABLE_1[] = {0, 0xFFFF, 0xFFFF, 0, 0, 0xFFFF, 0xFFFF, 0};

    for(uint8_t i = 0; i < M17_CONVOL_STATES/2; i++)
    {
        uint32_t metric = q_abs_diff(COST_TABLE_0[i], s0)
                        + q_abs_diff(COST_TABLE_1[i], s1);


        uint32_t m0 = prevMetrics[i] + metric;
        uint32_t m1 = prevMetrics[i + M17_CONVOL_STATES/2] + (0x1FFFE - metric);

        uint32_t m2 = prevMetrics[i] + (0x1FFFE - metric);
        uint32_t m3 = prevMetrics[i + M17_CONVOL_STATES/2] + metric;

        uint8_t i0 = 2 * i;
        uint8_t i1 = i0 + 1;

        if(m0 >= m1)
        {
            viterbi_history[pos]|=(1<<i0);
            currMetrics[i0] = m1;
        }
        else
        {
            viterbi_history[pos]&=~(1<<i0);
            currMetrics[i0] = m0;
        }

        if(m2 >= m3)
        {
            viterbi_history[pos]|=(1<<i1);
            currMetrics[i1] = m3;
        }
        else
        {
            viterbi_history[pos]&=~(1<<i1);
            currMetrics[i1] = m2;
        }
    }

    //swap
    uint32_t tmp[M17_CONVOL_STATES];
    for(uint8_t i=0; i<M17_CONVOL_STATES; i++)
    {
    	tmp[i]=currMetrics[i];
	}
	for(uint8_t i=0; i<M17_CONVOL_STATES; i++)
    {
    	currMetrics[i]=prevMetrics[i];
    	prevMetrics[i]=tmp[i];
	}
}

/**
 * @brief History chainback to obtain final byte array.
 *
 * @param out Destination byte array for decoded data.
 * @param pos Starting position for the chainback.
 * @param len Length of the output in bits.
 * @return Minimum Viterbi cost at the end of the decode sequence.
 */
uint32_t viterbi_chainback(uint8_t* out, size_t pos, uint16_t len)
{
    uint8_t state = 0;
    size_t bitPos = len+4;

    memset(out, 0, (len-1)/8+1);

    while(pos > 0)
    {
        bitPos--;
        pos--;
        uint16_t bit = viterbi_history[pos]&((1<<(state>>4)));
        state >>= 1;
        if(bit)
        {
        	state |= 0x80;
        	out[bitPos/8]|=1<<(7-(bitPos%8));
		    }
    }

    uint32_t cost = prevMetrics[0];

    for(size_t i = 0; i < M17_CONVOL_STATES; i++)
    {
        uint32_t m = prevMetrics[i];
        if(m < cost) cost = m;
    }

    return cost;
}

/**
 * @brief Decode unpunctured convolutionally encoded data.
 *
 * @param out Destination array where decoded data is written.
 * @param in Input data.
 * @param len Input length in bits.
 * @return Number of bit errors corrected.
 */
uint32_t viterbi_decode(uint8_t* out, const uint16_t* in, const uint16_t len)
{
    if(len > 244*2)
		fprintf(stderr, "Input size exceeds max history\n");

    viterbi_reset();

    size_t pos = 0;
    for(size_t i = 0; i < len; i += 2)
    {
        uint16_t s0 = in[i];
        uint16_t s1 = in[i + 1];

        viterbi_decode_bit(s0, s1, pos);
        pos++;
    }

    return viterbi_chainback(out, pos, len/2);
}

/**
 * @brief Decode punctured convolutionally encoded data.
 *
 * @param out Destination array where decoded data is written.
 * @param in Input data.
 * @param punct Puncturing matrix.
 * @param in_len Input data length.
 * @param p_len Puncturing matrix length (entries).
 * @return Number of bit errors corrected.
 */
uint32_t viterbi_decode_punctured(uint8_t* out, const uint16_t* in, const uint8_t* punct, const uint16_t in_len, const uint16_t p_len)
{
  if(in_len > 244*2)
  fprintf(stderr, "Input size exceeds max history\n");

  uint16_t umsg[244*2];       //unpunctured message
  uint8_t p=0;		            //puncturer matrix entry
  uint16_t u=0;		            //bits count - unpunctured message
  uint16_t i=0;               //bits read from the input message

	while(i<in_len)
	{
		if(punct[p])
		{
			umsg[u]=in[i];
			i++;
		}
		else
		{
			umsg[u]=0x7FFF;
		}

		u++;
		p++;
		p%=p_len;
	}

  return viterbi_decode(out, umsg, u) - (u-in_len)*0x7FFF;
}

/**
 * @brief Soft logic NOT.
 * 
 * @param a Input A.
 * @return uint16_t Output = not A.
 */
uint16_t soft_bit_NOT(const uint16_t a)
{
	return 0xFFFFU-a;
}

//randomizing pattern
const uint8_t rand_seq[46]=
{
    0xD6, 0xB5, 0xE2, 0x30, 0x82, 0xFF, 0x84, 0x62, 0xBA, 0x4E, 0x96, 0x90, 0xD8, 0x98, 0xDD, 0x5D, 0x0C, 0xC8, 0x52, 0x43, 0x91, 0x1D, 0xF8,
    0x6E, 0x68, 0x2F, 0x35, 0xDA, 0x14, 0xEA, 0xCD, 0x76, 0x19, 0x8D, 0xD5, 0x80, 0xD1, 0x33, 0x87, 0x13, 0x57, 0x18, 0x2D, 0x29, 0x78, 0xC3
};

/**
 * @brief Randomize type-4 soft bits.
 * 
 * @param inp Input 368 soft type-4 bits.
 */
void randomize_soft_bits(uint16_t inp[SYM_PER_PLD*2])
{
    for(uint16_t i=0; i<SYM_PER_PLD*2; i++)
    {
        if((rand_seq[i/8]>>(7-(i%8)))&1) //flip bit if '1'
        {
            inp[i]=soft_bit_NOT(inp[i]);
        }
    }
}



//interleaver pattern
const uint16_t intrl_seq[SYM_PER_PLD*2]=
{
	0, 137, 90, 227, 180, 317, 270, 39, 360, 129, 82, 219, 172, 309, 262, 31,
	352, 121, 74, 211, 164, 301, 254, 23, 344, 113, 66, 203, 156, 293, 246, 15,
	336, 105, 58, 195, 148, 285, 238, 7, 328, 97, 50, 187, 140, 277, 230, 367,
	320, 89, 42, 179, 132, 269, 222, 359, 312, 81, 34, 171, 124, 261, 214, 351,
	304, 73, 26, 163, 116, 253, 206, 343, 296, 65, 18, 155, 108, 245, 198, 335,
	288, 57, 10, 147, 100, 237, 190, 327, 280, 49, 2, 139, 92, 229, 182, 319,
	272, 41, 362, 131, 84, 221, 174, 311, 264, 33, 354, 123, 76, 213, 166, 303,
	256, 25, 346, 115, 68, 205, 158, 295, 248, 17, 338, 107, 60, 197, 150, 287,
	240, 9, 330, 99, 52, 189, 142, 279, 232, 1, 322, 91, 44, 181, 134, 271,
	224, 361, 314, 83, 36, 173, 126, 263, 216, 353, 306, 75, 28, 165, 118, 255,
	208, 345, 298, 67, 20, 157, 110, 247, 200, 337, 290, 59, 12, 149, 102, 239,
	192, 329, 282, 51, 4, 141, 94, 231, 184, 321, 274, 43, 364, 133, 86, 223,
	176, 313, 266, 35, 356, 125, 78, 215, 168, 305, 258, 27, 348, 117, 70, 207,
	160, 297, 250, 19, 340, 109, 62, 199, 152, 289, 242, 11, 332, 101, 54, 191,
	144, 281, 234, 3, 324, 93, 46, 183, 136, 273, 226, 363, 316, 85, 38, 175,
	128, 265, 218, 355, 308, 77, 30, 167, 120, 257, 210, 347, 300, 69, 22, 159,
	112, 249, 202, 339, 292, 61, 14, 151, 104, 241, 194, 331, 284, 53, 6, 143,
	96, 233, 186, 323, 276, 45, 366, 135, 88, 225, 178, 315, 268, 37, 358, 127,
	80, 217, 170, 307, 260, 29, 350, 119, 72, 209, 162, 299, 252, 21, 342, 111,
	64, 201, 154, 291, 244, 13, 334, 103, 56, 193, 146, 283, 236, 5, 326, 95,
	48, 185, 138, 275, 228, 365, 318, 87, 40, 177, 130, 267, 220, 357, 310, 79,
	32, 169, 122, 259, 212, 349, 302, 71, 24, 161, 114, 251, 204, 341, 294, 63,
	16, 153, 106, 243, 196, 333, 286, 55, 8, 145, 98, 235, 188, 325, 278, 47
};

/**
 * @brief Reorder (interleave) 368 soft bits.
 * 
 * @param outp Reordered soft bits.
 * @param inp Input soft bits.
 */
void reorder_soft_bits(uint16_t outp[SYM_PER_PLD*2], const uint16_t inp[SYM_PER_PLD*2])
{
	for(uint16_t i=0; i<SYM_PER_PLD*2; i++)
        outp[i]=inp[intrl_seq[i]];
}

//TODO: Cleanup and properly credit Woj 
//--------------------------------------------------------------------
// M17 C library - decode/viterbi.c
//
// This file contains:
// - the Viterbi decoder
//
// Wojciech Kaczmarski, SP5WWP
// M17 Project, 29 December 2023
//--------------------------------------------------------------------
