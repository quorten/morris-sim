/* Code common to both the GUI and the simulator.

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
 * Morris Sim common function definitions.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "core.h"
#include <string.h>
#include <glib.h>

#include "morris.h"
#include "tables.h"

/**
 * @mainpage Morris Sim
 *
 * Morris Sim was primarily intended to be a high performance, brute
 * force simulator for the game 11 Mens Morris, a variation of 9 Mens
 * Morris.  As such, it uses various techniques to speed up
 * computations.  However, the actual simulator engine has not been
 * written yet.  You can see the basic plan for the simulator engine
 * in morris-sim.c, though.
 *
 * All the core functions for manipulating the game state and
 * performing moves are located in morris.c.  The public functions
 * intended to be used by the simulator engine are located in
 * morris.h.  For a description of how the game board layout is
 * represented internally by the simulator engine, take a look at
 * tables.h.
 *
 * Note that Morris Sim uses various preprocessor defines to select
 * different pieces of code to use based off of the architecture of
 * the target system.  The Doxygen configuration file should be
 * properly set up now so that all Doxygen comments are extracted and
 * placed in the generated documentation.
 */

/********************************************************************/
/* SIMD wrapper functions.  */
/* NOTE: I had to add a few quirks to the calling convention of these
   functions so that I could define non-SIMD alternatives.  Note that
   the non-SIMD compatibility code has not be tested as much as the
   SIMD code.  */

#ifdef USE_PACKED

typedef guchar v8uc __attribute__ ((vector_size (8)));
typedef int v2si __attribute__ ((vector_size (8)));
#define SIMD_ARRAY(board) *(v8uc *) board
#define BOARD_QUAD(array) (BoardQuad *) &array

#define simd_and(unused, board1, board2) simd_and_real (board1, board2)
#define simd_or(unused, board1, board2)  simd_or_real (board1, board2)
#define simd_not(unsused, board)         simd_not_real (board)

static inline v8uc simd_and_real (v8uc board1, v8uc board2)
{ return board1 & board2; }
/* { return (v8uc) __builtin_ia32_pand ((v2si) board1, (v2si) board2); } */

static inline v8uc simd_or_real (v8uc board1, v8uc board2)
{ return board1 | board2; }
/* { return (v8uc) __builtin_ia32_por ((v2si) board1, (v2si) board2); } */

static inline v8uc simd_not_real (v8uc board)
{ return ~board; }

#else /* not USE_PACKED */

#include <stdlib.h> /* for alloca () */

typedef BoardQuad * v8uc;
#define SIMD_ARRAY(board) board
#define BOARD_QUAD(array) array

static inline v8uc
simd_and (v8uc dest_board, v8uc board1, v8uc board2)
{
    guchar i;
    for (i = 0; i < BOARD_SIZE; i++)
	dest_board[i] = board1[i] & board2[i];
    return dest_board;
}

static inline v8uc
simd_or (v8uc dest_board, v8uc board1, v8uc board2)
{
  guchar i;
  for (i = 0; i < BOARD_SIZE; i++)
    dest_board[i] = board1[i] | board2[i];
  return dest_board;
}

static inline v8uc
simd_not (v8uc dest_board, v8uc board)
{
  guchar i;
  for (i = 0; i < BOARD_SIZE; i++)
    dest_board[i] = ~board[i];
  return dest_board;
}

#endif

/********************************************************************/
/* Board referencing and setting code  */

#ifdef USE_PACKED

/**
 * Get the contents of a game board position.
 *
 * @param board the game board to index
 * @param index the zero-based game board index of the desired
 * contents
 * @return the contents at the given game board position
 */
Player
board_ref (BoardQuad *board, guchar index)
{
  guchar quad_ref;
  guchar mask = 0x03; /* 00000011 */
  guchar bitpos;
  guchar quantum;
  quad_ref = ((guchar *) board)[index/4];
  bitpos = (index % 4) * 2;
  mask <<= bitpos;
  quantum = quad_ref & mask;
  return quantum >> bitpos;
}

/**
 * Set the contents of a game board position.
 *
 * @param board the game board to mutate
 * @param index the zero-based game board index specifying the
 * part to change
 * @param value the new value of the board at the specified part
 */
void
set_board_pos (BoardQuad *board, guchar index, guchar value)
{
  guchar mask = 0xfc; /* 11111100 */
  guchar bitpos;
  guchar pack_value;
  bitpos = (index % 4) * 2;
  pack_value = value << bitpos;
  /* x86 rotate left assembly code */
#ifdef HAVE_X86_ASM
  asm ("rolb %1, %0" : "+rm" (mask) : "c" (bitpos));
#else
  switch (bitpos) {
    case 0: mask = 0b11111100; break;
    case 1: mask = 0b11110011; break;
    case 2: mask = 0b11001111; break;
    case 3: mask = 0b00111111; break; }
#endif
  ((guchar *) board)[index/4] &= mask;
  ((guchar *) board)[index/4] |= pack_value;
}

#else /* not USE_PACKED */

Player
board_ref (BoardQuad *board, guchar index)
{ return board[index]; }

void
set_board_pos (BoardQuad *board, guchar index, guchar value)
{ ((guchar *) board)[index] = value; }

#endif

/********************************************************************/

/**
 * Gets the opponent of the current player.
 *
 * Note that this function only works for two-player games.
 * @param state the game state to use
 * @return the opponent of the current player
 */
Player
get_opponent (GameState *state)
{
  if (state->cur_player == PLAYER1)
    return PLAYER2;
  return PLAYER1;
}

/**
 * Tests whether two positions are adjacent to each other.
 *
 * @param pos1 a zero-based game board position index
 * @param pos2 another zero-based game board position index
 * @return @a true if the two positions are adjacent, @a false
 * otherwise
 */
bool
are_adjacent (guchar pos1, guchar pos2)
{
  guchar i;
  for (i = pos1 * 4; i < pos1 * 4 + 4; i++)
    {
      if (pos2 == adjacent_places[i])
	return true;
    }
  return false;
}

/**
 * Tests whether placing a piece at a certain position is valid.
 *
 * This function does not change the game state.
 * @param state the game state to use
 * @param pos the position to test placing a piece at
 * @return @a true if a piece may be placed at the given position, @a
 * false otherwise
 */
bool
is_valid_place (GameState *state, guchar pos)
{
  if (board_ref (state->board, pos) == EMPTY)
    return true;
  return false;
}

/**
 * Tests whether moving a piece from one position to another is valid.
 *
 * This function does not change the game state.
 * @param state the game state to use
 * @param src a zero-based game board index specifying the piece to
 * move
 * @param dest a zero-based game board index specifying where to move
 * the piece
 * @return @a true if the move is valid, @a false otherwise
 */
bool
is_valid_move (GameState *state, guchar src, guchar dest)
{
  if (are_adjacent (src, dest) &&
      board_ref (state->board, src) == state->cur_player &&
      board_ref (state->board, dest) == EMPTY)
    return true;
  return false;
}

/**
 * Checks if a piece can be removed by testing it against mill masks.
 *
 * @param state the game state to use
 * @param pos a zero-based game board index specifying the piece to
 * remove
 * @return @a true if a piece can be removed from the given position,
 * @a false otherwise
 */
bool
is_valid_remove (GameState *state, guchar pos)
{
  v8uc test_board; /* For intermediate calculations */
  /* Accumulation (logical OR) of all applicable mill masks.  */
  v8uc cur_mills_mask;
  guchar (*plyr_mill_masks)[MASK_SIZE];
  guchar *plyr_mask;
  guchar i;

  {
    Player player;
    player = board_ref (state->board, pos);
    if (player == EMPTY || player == state->cur_player)
      return false;
  }

#ifndef USE_PACKED
  test_board = (v8uc) alloca (MASK_SIZE);
  cur_mills_mask = (v8uc) alloca (MASK_SIZE);
  memset (BOARD_QUAD (test_board), 0, MASK_SIZE);
#endif

  /* Get the masks for the opposite player.  */
  plyr_mill_masks = opp_plyr_mill_choices[state->cur_player];
  plyr_mask =       opp_plyr_mask_choices[state->cur_player];

  /* Tag off all opponent pieces that are in mills, accumulating all
     formed mills to a mill mask.  */
  memset (BOARD_QUAD (cur_mills_mask), 0, MASK_SIZE);
  for (i = 0; i < TOTAL_MILLS; i++)
    {
      test_board = simd_and (test_board, SIMD_ARRAY (state->board),
			     SIMD_ARRAY (plyr_mill_masks[i]));
      if (!memcmp (BOARD_QUAD (test_board), plyr_mill_masks[i], MASK_SIZE))
	{
	  cur_mills_mask = simd_or (cur_mills_mask, cur_mills_mask,
				    SIMD_ARRAY (plyr_mill_masks[i]));
	}
    }

  /* Mask off all opponent pieces in mills.  */
  test_board = simd_and (test_board,
			 simd_not (test_board, cur_mills_mask),
			 SIMD_ARRAY (state->board));
  /* Mask off the current player's pieces too.  */
  test_board = simd_and (test_board, test_board, SIMD_ARRAY (plyr_mask));
  /* print_board (BOARD_QUAD (test_board)); */
  /* If all opponent pieces are in mills, then always return true.  */
  if (!memcmp (BOARD_QUAD (test_board), null_mask, MASK_SIZE))
    return true;
  /* Otherwise, return false if the position is in a mill.  */
  else if (board_ref (BOARD_QUAD (cur_mills_mask), pos) != 0)
    return false;
  return true;
}

/**
 * Determines if a piece just placed or moved formed a mill.
 *
 * @param state the game state to use
 * @param pos the zero-based game board index of the piece just placed
 * or moved
 * @return @a true if the piece formed a mill, @a false otherwise
 */
bool
is_mill_formed (GameState *state, guchar pos)
{
  v8uc test_board; /* For intermediate calculations */
  /* Accumulation (logical OR) of all applicable mill masks.  */
  v8uc cur_mills_mask;
  guchar (*plyr_mill_masks)[MASK_SIZE];
  guchar *plyr_mask;
  guchar i;

  if (board_ref (state->board, pos) != state->cur_player)
    return false;

#ifndef USE_PACKED
  test_board = (v8uc) alloca (MASK_SIZE);
  cur_mills_mask = (v8uc) alloca (MASK_SIZE);
  memset (BOARD_QUAD (test_board), 0, MASK_SIZE);
#endif

  /* Get the masks for the current player.  */
  plyr_mill_masks = plyr_mill_choices[state->cur_player];
  plyr_mask =       plyr_mask_choices[state->cur_player];

  /* Tag off the mills that the piece is in.  */
  memset (BOARD_QUAD (cur_mills_mask), 0, MASK_SIZE);
  for (i = 0; i < 3; i++)
    {
      guchar *plyr_mill_mask;
      plyr_mill_mask = plyr_mill_masks[mill_from_pos[pos*3+i]];
      test_board = simd_and (test_board, SIMD_ARRAY (state->board),
			     SIMD_ARRAY (plyr_mill_mask));
      if (!memcmp (BOARD_QUAD (test_board), plyr_mill_mask, MASK_SIZE))
	{
	  cur_mills_mask = simd_or (cur_mills_mask, cur_mills_mask,
				    SIMD_ARRAY (plyr_mill_mask));
	}
    }

  /* Test if the placed piece is in a mill.  */
  if (board_ref (BOARD_QUAD (cur_mills_mask), pos) != 0)
    return true;
  return false;
}

/**
 * Change the current player to the next player.
 *
 * This function should be called at the end of a player's turn.
 * GameState::cur_player will be either incremented or reset to one
 * to properly switch to the next player.
 *
 * @param state the game state to use
 */
void
next_player (GameState *state)
{
  state->cur_player++;
  if (state->cur_player > NUM_PLAYERS)
    state->cur_player = 1;
}

/**
 * Places a new piece during gameplay.
 *
 * This function places a new piece on the game board, but only if it
 * is valid to place at the given position.  This function also
 * updates other game state parameters.  Essentially, this function
 * corresponds to placing a piece while playing the game rather than
 * just mutating the board.
 *
 * @param state the game state to use
 * @param pos the zero-based game board index to place the piece at
 * @return @a true if the piece was successfully placed, @a false
 * otherwise
 */
bool
place_piece (GameState *state, guchar pos)
{
  if (!is_valid_place (state, pos))
    return false;
  set_board_pos (state->board, pos, state->cur_player);
  state->player_pieces[state->cur_player-1]++;
  if (is_mill_formed (state, pos))
    state->remove_state = true;
  else
    next_player (state);
  return true;
}

/**
 * Moves a piece during gameplay.
 *
 * This function moves a piece on the game board, but only if the move
 * is valid.  This function also updates other game state parameters.
 * Essentially, this function corresponds to moving a piece while
 * playing the game rather than just mutating the board.
 *
 * @param state the game state to use
 * @param src the zero-based game board index of the piece to move
 * @param dest the zero-based game board index to move the piece to
 * @return @a true if the piece was successfully moved, @a false
 * otherwise
 */
bool
move_piece (GameState *state, guchar src, guchar dest)
{
  if (!is_valid_move (state, src, dest))
    return false;
  set_board_pos (state->board, src, EMPTY);
  set_board_pos (state->board, dest, state->cur_player);
  if (is_mill_formed (state, dest))
    state->remove_state = true;
  else
    next_player (state);
  return true;
}

/**
 * Removes a piece during gameplay.
 *
 * This function removes a piece from the game board, but only if the
 * piece may be removed by the rules of the game.  This function also
 * updates other game state parameters.  Essentially, this function
 * corresponds to removing a piece while playing the game rather than
 * just mutating the board.
 *
 * @param state the game state to use
 * @param pos the zero-based game board index of the piece to remove
 * @return @a true if the piece was successfully removed, @a false
 * otherwise
 */
bool
remove_piece (GameState *state, guchar pos)
{
  Player player;
  if (!is_valid_remove (state, pos))
    return false;
  player = board_ref (state->board, pos);
  set_board_pos (state->board, pos, EMPTY);
  state->player_pieces[player-1]--;
  state->remove_state = false;
  next_player (state);
  return true;
}

/**
 * Get the winner of the game.
 *
 * Get the winner of the game, if there is any yet.
 *
 * @param state the game state to use
 * @return the player who won, or @a EMPTY if no player won yet
 */
guchar
get_winner (GameState *state)
{
  /* If all players except one are down to only two pieces, then
     there is a winner.  */
  if (state->player_pieces[0] == 2)
    return PLAYER2;
  if (state->player_pieces[1] == 2)
    return PLAYER1;
  return EMPTY;
}

/**
 * Initialize a new game.
 *
 * Initialize a game state context structure to that of a new game.
 * To initialize a game state context to anything else, manually set
 * the fields accordingly.
 *
 * @param state the game state to use
 */
void
init_game_state (GameState *state)
{
  state->cur_player = PLAYER1;
  state->setup_rounds_left = 11;
  state->remove_state = false;
  memset (state->player_pieces, 0, NUM_PLAYERS);
  memset (state->board, 0, MASK_SIZE);
}
