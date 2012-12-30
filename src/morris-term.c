/* Terminal interface for N Mens Morris.

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
 * Line-based terminal user interface.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "core.h"
#include <stdio.h>
#include <glib.h>

#include "support.h"
#include "morris.h"

char *player_symbols[NUM_PLAYERS+1] = { "  ", "P1", "P2" };

/**
 * Print an ASCII-art formatted board to standard output.
 *
 * @param board the board to print
 */
void
print_board (BoardQuad *board)
{
  char *string_board[BOARD_SIZE];
  guchar i;

  for (i = 0; i < BOARD_SIZE; i++)
    {
      guchar player_enum;
      player_enum = board_ref (board, i);
      string_board[i] = player_symbols[player_enum];
    }
  printf(
	 " %s----------%s----------%s\n"
	 " 0| \\        1|         /2|\n"
	 "  |  %s------%s------%s/  |\n"
	 "  |  3|\\     4|     /5|   |\n"
	 "  |   | \\%s--%s--%s/  |   |\n"
	 "  |   |  6|   7  8|   |   |\n"
	 " %s--%s--%s      %s--%s--%s\n"
	 " 9| 10| 11|     12| 13| 14|\n"
	 "  |   | /%s--%s--%s\\  |   |\n"
	 "  |   |/15  16|  17 \\ |   |\n"
	 "  | /%s------%s------%s\\  |\n"
	 "  |/18      19|      20 \\ |\n"
	 " %s----------%s----------%s\n"
	 " 21          22          23\n",
	 string_board[ 0], string_board[ 1], string_board[ 2],
	 string_board[ 3], string_board[ 4], string_board[ 5],
	 string_board[ 6], string_board[ 7], string_board[ 8],
	 string_board[ 9], string_board[10], string_board[11],
	 string_board[12], string_board[13], string_board[14],
	 string_board[15], string_board[16], string_board[17],
	 string_board[18], string_board[19], string_board[20],
	 string_board[21], string_board[22], string_board[23]);
}

/**
 * Prompt for a zero-based game board index from standard
 * input.
 *
 * @param prompt the prompt message to send to standard output.  No
 * extra newline is printed after the prompt message.
 * @return the zero-based game board index received
 */
guint
get_pos_input (const char *prompt)
{
  int fields_read;
  guint position;
  while (1)
    {
      printf ("%s ", prompt);
      fflush (stdout);
      fields_read = scanf ("%u", &position);
      if (fields_read < 1 || position > 23)
	{
	  char buffer[128];
	  puts (_("Invalid input."));
	  fields_read = scanf ("%127s", buffer); /* Ignore the input.  */
	  continue;
	}
      break;
    }
  return position;
}

/**
 * Loop until a valid remove command is received from the user.
 *
 * @param state the game state to use
 */
void
remove_loop (GameState *state)
{
  while (1)
    {
      guchar pos;
      pos = (guchar)
	get_pos_input (_("Which piece will you remove?"));
      if (!remove_piece (state, pos))
	{
	  Player player;
	  player = board_ref (state->board, pos);
	  if (player == EMPTY)
	    puts (_("Just because there's air there " \
		    "doesn't mean you can remove it."));
	  else if (player == state->cur_player)
	    puts (_("Are you crazy?  You don't need to " \
		    "attack your own people."));
	  else
	    puts (_("Sorry, can't do that.  Have you "
		    "noticed there's a piece outside of a mill?"));
	}
      else
	break;
    }
}

/**
 * The main game loop for the line-based terminal user interface.
 *
 * @param state the game state to use
 */
void
game_loop (GameState *state)
{
  /* First do the setup phase.  */
  while (state->setup_rounds_left > 0)
    {
      guint i;
      for (i = 0; i < NUM_PLAYERS; i++)
	{
	  print_board (state->board);
	  printf (_("Player %u's turn.\n"), (guint) state->cur_player);
	  while (1)
	    {
	      guchar pos;
	      pos = (guchar)
		get_pos_input (_("Where will you place your piece?"));
	      if (!place_piece (state, pos))
		puts (_("Don't get me wrong.  " \
			"That space is already occupied."));
	      else
		break;
	    }
	  if (state->remove_state)
	    {
	      print_board (state->board);
	      printf (_("Player %u's turn.\n"), (guint) state->cur_player);
	      remove_loop (state);
	    }
	}
      state->setup_rounds_left--;
    }

  /* Next do the main game phase.  */
  while (get_winner (state) == EMPTY)
    {
      guchar src, dest;
      print_board (state->board);
      printf (_("Player %u's turn.\n"), (guint) state->cur_player);
      while (1)
	{
	  src = (guchar) get_pos_input (_("Which piece will you move?"));
	  if (board_ref (state->board, src) != state->cur_player)
	    {
	      puts (_("Wrong place silly!  " \
		      "You can only move your own pieces."));
	      continue;
	    }
	  dest = (guchar) get_pos_input (_("Where will you move it to? "));
	  if (!move_piece (state, src, dest))
	    puts (_("You can't move into an already occupied space.  " \
		    "Rules are rules."));
	  else
	    break;
	}
      if (state->remove_state)
	{
	  print_board (state->board);
	  printf (_("Player %u's turn.\n"), (guint) state->cur_player);
	  remove_loop (state);
	}
    }
  printf (_("Player %u won.\n"), (guint) get_winner (state));
}

/**
 * main() function hand-off for the line-based terminal user
 * interface.
 *
 * In an application compiled with no GTK+ support, this would be the
 * main() function of the program.
 */
int
term_main (int argc, char *argv[])
{
  GameState state;
  guint choice = 1;
  init_game_state (&state);
  puts (_("Welcome to the 11 Mens Morris simulator."));
  do
    {
      int fields_read;
      puts (_("What would you like to do?\n" \
	      "0) Quit\n" \
	      "1) Reinitialize the game state.\n" \
	      "2) Display the board\n" \
	      "3) Add a piece\n" \
	      "4) Move a piece\n" \
	      "5) Remove a piece\n" \
	      "6) Play the game\n" \
	      "7) Game state info"));
      fputs (_("Choice? "), stdout);
      fflush (stdout);
      fields_read = scanf ("%u", &choice);
      if (fields_read < 1 || choice > 7)
	{
	  char buffer[128];
	  puts (_("Invalid input."));
	  fields_read = scanf ("%127s", buffer); /* Ignore the input.  */
	  continue;
	}

      switch (choice)
	{
	case 1:
	  init_game_state (&state);
	  puts (_("Re-initialized game state."));
	  break;
	case 2: print_board (state.board); break;
	case 3:
	  if (!place_piece (&state, get_pos_input (_("Where? "))))
	    puts(_("Invalid place."));
	  break;
	case 4:
	  {
	    guchar src, dest;
	    src = get_pos_input (_("From where? "));
	    dest = get_pos_input (_("To where? "));
	    if (!move_piece (&state, src, dest))
	      puts (_("Invalid move."));
	    break;
	  }
	case 5:
	  if (!remove_piece (&state, get_pos_input (_("Where? "))))
	    puts (_("Invalid remove."));
	  break;
	case 6:
	  game_loop (&state);
	  break;
	case 7:
	  printf(_("Current player: %u\n" \
		   "Setup rounds left: %u\n" \
		   "Remove state: %u\n" \
		   "Player 1 pieces: %u\n" \
		   "Player 2 pieces: %u\n"),
		 state.cur_player, state.setup_rounds_left,
		 state.remove_state, state.player_pieces[0],
		 state.player_pieces[1]);
	  break;
	}
    } while (choice != 0);
  return 0;
}
