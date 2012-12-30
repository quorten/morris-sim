/* Morris Sim common code declarations.

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
 * Morris Sim common functions.
 */

#ifndef MORRIS_H
#define MORRIS_H

#define NUM_PLAYERS 2
enum Player_tag { EMPTY, PLAYER1, PLAYER2 };
typedef guchar Player;

#define BOARD_SIZE 24
#define TOTAL_MILLS 20

#ifdef USE_PACKED

/**
 * A grouping of four board positions into one byte.  When more data
 * is packed into a smaller space, it is possible to take advantage of
 * SSE instructions to parallel process the packed data.
 */
struct BoardQuad_tag
{
  guchar field0 : 2;
  guchar field1 : 2;
  guchar field2 : 2;
  guchar field3 : 2;
};

/**
 * BoardQuad type.
 *
 * Usually, this is defined to be the structure ::BoardQuad_tag.  When
 * tight array packing is disabled, this is no longer defined to be a
 * grouping of four board positions, but rather only a single board
 * position.
 */
typedef struct BoardQuad_tag BoardQuad;

#define MASK_SIZE 8 /**< The size of the mill masks in bytes */

#else /* not USE_PACKED */
#define MASK_SIZE 32
typedef guchar BoardQuad;
#endif

struct GameState_tag
{
  Player cur_player;
  /** Number of piece-placing rounds left.  */
  guchar setup_rounds_left;
  bool remove_state; /**< Should the next move be a remove?  */
  /** Number of pieces each player has.  */
  guchar player_pieces[NUM_PLAYERS];
  BoardQuad board[MASK_SIZE];
};
typedef struct GameState_tag GameState;

inline Player board_ref (BoardQuad *board, guchar index);
inline void set_board_pos (BoardQuad *board, guchar index, guchar value);
inline Player get_opponent (GameState *state);
inline bool are_adjacent (guchar pos1, guchar pos2);
inline bool is_valid_place (GameState *state, guchar pos);
inline bool is_valid_move (GameState *state, guchar src, guchar dest);
inline bool is_valid_remove (GameState *state, guchar pos);
bool is_mill_formed (GameState *state, guchar pos);
void next_player (GameState *state);
bool place_piece (GameState *state, guchar pos);
bool move_piece (GameState *state, guchar src, guchar dest);
bool remove_piece (GameState *state, guchar pos);
guchar get_winner (GameState *state);
void init_game_state (GameState *state);

#endif /* not MORRIS_H */
