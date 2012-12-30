/* tables.h -- Various lookup tables for fast comparisons.

Copyright (C) 2012 Andrew Makousky

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

/**
 * @file
 * Various lookup tables for fast comparisons.

 * The 11 Mens Morris board is stored as a 24 element array as
 * follows:

@verbatim
    0-----------1-----------2
    | \         |         / |
    |  \3-------4-------5/  |
    |   |\      |     / |   |
    |   | \ 6---7---8/  |   |
    |   |   |       |   |   |
    9--10--11      12--13--14
    |   |   |       |   |   |
    |   | /15--16--17\  |   |
    |   |/      |     \ |   |
    | /18------19------20\  |
    |/          |         \ |
   21----------22----------23
@endverbatim

 * When the array is in packed form, each element of the array is a
 * two-bit quantum.  The quantums are packed 4 for a byte, with the
 * lowest numbered board position stored as the least significant
 * bits.  This packed structure is denoted in the code by the type
 * ::BoardQuad.  Each next byte is in an ascending memory location, so
 * byte swapping will not be an issue.  When the array is not in
 * packed form, one byte corresponds to one board position.
 *
 * By packing the data of the array into a smaller space, it is
 * possible to use the CPU's boolean logic instructions to perform
 * parallel computations on the data.  Coupled with modern Streaming
 * SIMD Extensions (SSE) available on modern x86 and x86-64 CPUs, this
 * can greatly accelerate the computational speed of the simulator.
 */

/* Verify that this adjacent place table is correct.  The other one is
   generated from this one.  */
guchar adjacent_places_canonical[][2] =
  {
    { 0,  1}, { 1,  2}, { 2, 14}, {14, 23},
    {23, 22}, {22, 21}, {21,  9}, { 9,  0},
    { 3,  4}, { 4,  5}, { 5, 13}, {13, 20},
    {20, 19}, {19, 18}, {18, 10}, {10,  3},
    { 6,  7}, { 7,  8}, { 8, 12}, {12, 17},
    {17, 16}, {16, 15}, {15, 11}, {11,  6},
    { 0,  3}, { 3,  6}, { 1,  4}, { 4,  7},
    { 2,  5}, { 5,  8}, {14, 13}, {13, 12},
    {23, 20}, {20, 17}, {22, 19}, {19, 16},
    {21, 18}, {18, 15}, { 9, 10}, {10, 11},
  };

/** A high performance lookup table for adjacent places.  */
guchar adjacent_places[BOARD_SIZE*4] =
  {
     1,  3,  9, 99, /* { 0, nn} */
     0,  2,  4, 99, /* { 1, nn} */
     1,  5, 14, 99, /* { 2, nn} */
     0,  4,  6, 10, /* { 3, nn} */
     1,  3,  5,  7, /* { 4, nn} */
     2,  4,  8, 13, /* { 5, nn} */
     3,  7, 11, 99, /* { 6, nn} */
     4,  6,  8, 99, /* { 7, nn} */
     5,  7, 12, 99, /* { 8, nn} */
     0, 10, 21, 99, /* { 9, nn} */
     3,  9, 11, 18, /* {10, nn} */
     6, 10, 15, 99, /* {11, nn} */
     8, 13, 17, 99, /* {12, nn} */
     5, 12, 14, 20, /* {13, nn} */
     2, 13, 23, 99, /* {14, nn} */
    11, 16, 18, 99, /* {15, nn} */
    15, 17, 19, 99, /* {16, nn} */
    12, 16, 20, 99, /* {17, nn} */
    10, 15, 19, 21, /* {18, nn} */
    16, 18, 20, 22, /* {19, nn} */
    13, 17, 19, 23, /* {20, nn} */
     9, 18, 22, 99, /* {21, nn} */
    19, 21, 23, 99, /* {22, nn} */
    14, 20, 22, 99, /* {23, nn} */
  };

#ifndef USE_PACKED
#include "tab_unpack.h"
#else

guchar null_mask[MASK_SIZE] =
  { 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00 };

guchar saturated_mask[MASK_SIZE] =
  { 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x00, 0x00 };

guchar p1_mask[MASK_SIZE] =
  { 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x00, 0x00 };

guchar p2_mask[MASK_SIZE] =
  { 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0x00, 0x00 };

/**
 * Mill masks.  Basically, these structures are logically ANDed with a
 * game board, and if the result is equal to the mask, then there is a
 * mill.  SSE optimizations can greatly speed up the comparison
 * process.  See tab_unpack.h for graphical representations of the
 * contents of the mill masks.
 */
guchar mill_masks[20][MASK_SIZE] =
  {
    { 0x3F, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x30, 0x00, 0x00,
      0x30, 0x00, 0xC0, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x00, 0xFC, 0x00, 0x00 },
    { 0x03, 0x00, 0x0C,
      0x00, 0x00, 0x0C, 0x00, 0x00 },
    { 0xC0, 0x0F, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x0C, 0x00,
      0x0C, 0x00, 0x03, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0xF0, 0x03, 0x00, 0x00 },
    { 0xC0, 0x00, 0x30,
      0x00, 0x30, 0x00, 0x00, 0x00 },
    { 0x00, 0xF0, 0x03,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x03,
      0x03, 0x0C, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0xC0, 0x0F, 0x00, 0x00, 0x00 },
    { 0x00, 0x30, 0xC0,
      0xC0, 0x00, 0x00, 0x00, 0x00 },
    { 0xC3, 0x30, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x0C, 0xC3, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x30, 0x0C, 0x03,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x3F, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x0C, 0xC3, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0xC3, 0x30, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0xC0, 0x30, 0x0C, 0x00, 0x00 },
    { 0x00, 0x00, 0xFC,
      0x00, 0x00, 0x00, 0x00, 0x00 },
  };

/** This mask is generated from `mill_masks'.  */
guchar p1_mill_masks[20][MASK_SIZE] =
  {
    { 0x15, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x10, 0x00, 0x00,
      0x10, 0x00, 0x40, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x00, 0x54, 0x00, 0x00 },
    { 0x01, 0x00, 0x04,
      0x00, 0x00, 0x04, 0x00, 0x00 },
    { 0x40, 0x05, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x04, 0x00,
      0x04, 0x00, 0x01, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x50, 0x01, 0x00, 0x00 },
    { 0x40, 0x00, 0x10,
      0x00, 0x10, 0x00, 0x00, 0x00 },
    { 0x00, 0x50, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x01,
      0x01, 0x04, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x40, 0x05, 0x00, 0x00, 0x00 },
    { 0x00, 0x10, 0x40,
      0x40, 0x00, 0x00, 0x00, 0x00 },
    { 0x41, 0x10, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x04, 0x41, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x10, 0x04, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x15, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x04, 0x41, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x41, 0x10, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x40, 0x10, 0x04, 0x00, 0x00 },
    { 0x00, 0x00, 0x54,
      0x00, 0x00, 0x00, 0x00, 0x00 },
  };

/** This mask is generated from `mill_masks'.  */
guchar p2_mill_masks[20][MASK_SIZE] =
  {
    { 0x2A, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x20, 0x00, 0x00,
      0x20, 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x00, 0xA8, 0x00, 0x00 },
    { 0x02, 0x00, 0x08,
      0x00, 0x00, 0x08, 0x00, 0x00 },
    { 0x80, 0x0A, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x08, 0x00,
      0x08, 0x00, 0x02, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0xA0, 0x02, 0x00, 0x00 },
    { 0x80, 0x00, 0x20,
      0x00, 0x20, 0x00, 0x00, 0x00 },
    { 0x00, 0xA0, 0x02,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x02,
      0x02, 0x08, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x80, 0x0A, 0x00, 0x00, 0x00 },
    { 0x00, 0x20, 0x80,
      0x80, 0x00, 0x00, 0x00, 0x00 },
    { 0x82, 0x20, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x08, 0x82, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x20, 0x08, 0x02,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x2A, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x08, 0x82, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x00, 0x82, 0x20, 0x00, 0x00 },
    { 0x00, 0x00, 0x00,
      0x80, 0x20, 0x08, 0x00, 0x00 },
    { 0x00, 0x00, 0xA8,
      0x00, 0x00, 0x00, 0x00, 0x00 },
  };

#endif /* USE_PACKED */

/* These are convenience arrays used to avoid switch ()
   statements.  */
void *plyr_mill_choices[3] = { NULL, p1_mill_masks, p2_mill_masks };
void *plyr_mask_choices[3] = { NULL, p1_mask, p2_mask };
void *opp_plyr_mill_choices[3] = { NULL, p2_mill_masks, p1_mill_masks };
void *opp_plyr_mask_choices[3] = { NULL, p2_mask, p1_mask };

/**
 * Take a board position and find all potential mills that the piece
 * could be part of.
 */
guchar mill_from_pos[BOARD_SIZE*3] =
  {
     0,  3, 12, /* Position  0 */
     0, 13, 13, /* Position  1 */
     0,  1, 14, /* Position  2 */
     4,  7, 12, /* Position  3 */
     4, 13, 13, /* Position  4 */
     4,  5, 14, /* Position  5 */
     8, 11, 12, /* Position  6 */
     8, 13, 13, /* Position  7 */
     8,  9, 14, /* Position  8 */
     3, 19, 19, /* Position  9 */
     7, 19, 19, /* Position 10 */
    11, 19, 19, /* Position 11 */
     9, 15, 15, /* Position 12 */
     5, 15, 15, /* Position 13 */
     1, 15, 15, /* Position 14 */
    10, 11, 18, /* Position 15 */
    10, 17, 17, /* Position 16 */
     9, 10, 16, /* Position 17 */
     6,  7, 18, /* Position 18 */
     6, 17, 17, /* Position 19 */
     5,  6, 16, /* Position 20 */
     2,  3, 18, /* Position 21 */
     2, 17, 17, /* Position 22 */
     1,  2, 16, /* Position 23 */
  };