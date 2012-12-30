/* Simulate 11 Mens Morris by enumerating every single possible game
   board layout, then analyze which moves have the most winning
   possibilities.

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
 * 11 Mens Morris simulator engine
 *
 * Since 9 Mens Morris is a solved game, one would assume that 11 Mens
 * Morris can also be a solved game.  One possible way to solve the
 * game would be to simulate every single possible game play strategy.
 * However, this can only be done by answering an important question:
 * Is there a finite number of game play strategies?
 *
 * Well, lets start by thinking about how to build a brute force
 * simulator that keeps track of every single possible move.
 * Basically, you start at the beginning of the with an empty board.
 * There are two players: "player 1" and "player 2".  Both are assumed
 * to be "omni" players.  That is, they are players that will try out
 * every single possible move at their point in the game.  So, the
 * simulator starts with a beginning game state, and for each possible
 * valid move for the current player, the simulator will create a
 * duplicate game state with that move performed.  That duplicate game
 * state will be linked as a child of the current game state.  Then,
 * the simulator will need to perform this process recursively on
 * every child game state.  However, before performing this process
 * recursively, the simulator needs to check for two conditions:
 *
 * <ol>
 *
 * <li>If one player has won the game, then don't continue recursive
 * simulation at the generated tree node.</li>
 *
 * <li>If the generated game state is equivalent to an already
 * existing generated game state, then delete the newly generated game
 * state, create a link to the existing generated game state, and
 * don't continue recursive simulation, as it is unnecessary.</li>
 *
 * </ol>
 *
 * When every single simulation node is terminated (recursive
 * simulation is not continued at the node), then the simulation is
 * complete, and parameters can be tabulated.  So although there are
 * an infinite number of game play sequences, some of which never lead
 * to a winning game, there are a finite number of unique game states,
 * and for all reasons that we care about, a finite number of ways to
 * win the game in the shortest amount of time.
 *
 * Obviously, simulation of the game at each node of the tree at the
 * same vertical level can be computed independently of any other
 * simulation node.  However, checking of existing computed game
 * states is not independent of other nodes simulated on the tree.
 * It's certain that if the simulation would be run as a
 * single-threaded simple recursive implementation, the results would
 * not turn out very well because the simulation tree would be heavily
 * lob-sided.
 *
 * There are two solution paths to this problem: 1) spawn a new thread
 * for each recursively simulated node and 2) keep a job queue of
 * simulations to perform.  Because spawning one thread for each
 * problem might cause resource management problems, it would be ideal
 * to spawn one thread per processor core and use a job queue to push
 * on new jobs to perform and pull off pending jobs when one thread
 * finishes its work in progress.
 *
 * Another problem that would come up with writing the simulator is
 * checking if an existing game state has already been simulated
 * and/or has sub-simulations in progress.  To do this, each game
 * state could be treated as one big number, and a sorted heap of
 * pointers to game states could be maintained.  (GLib GSequence would
 * be used as the heap implementation.) To check if a game state has
 * already been simulated, just do a binary search of the game state
 * within the heap of game states.  Using a heap data structure keeps
 * insertion time and searching time down to a minimum.
 *
 * Yet another problem is that I have no idea how long the simulation
 * will take.  If it takes too long, then I will need to be able to
 * stop the simulation and continue it later.  This basically means
 * that I would need mechanisms to save the simulated game states and
 * job queue to a file.  The sorted heap of game states could be
 * regenerated when the simulator is continued.  The simulation state
 * would also be periodically saved to disk during a simulation run,
 * just in case anything bad happens.  The saves to disk would happen
 * in an atomic manner, by writing to a different file then renaming
 * it once the save succeeded.
 *
 * The only last problem is that I have no idea of exactly how many
 * game states that 11 Mens Morris may have.  The only way to find out
 * is to run the simulator.  I would hope that the total number of
 * game states is less than 4294967295 (2^32 - 1), but you never know.
 * As it turns out, I have not written the actual simulator code yet,
 * but rather have written how to write the simulator from a
 * conceptual level.
 *
 * Finally, I have not mentioned all possible optimizations.  GPGPU
 * computation and networked computation could also be used as ways to
 * perform more computations in less time, but I am pretty sure that
 * 11 Mens Morris is not @e that complicated of a game to demand those
 * computational mechanisms, is it?  Well, networked computation
 * actually isn't that hard: rather than spawning multiple threads,
 * multiple processes would be spawned, and each process would
 * communicate to each other using sockets.  This is actually a very
 * appropriate way to write the simulator.  A server process would
 * manage the heap of sorted game states along with the job queue, and
 * client processes would make requests to either fetch a job to
 * perform or insert a new game state into the heap.  If the insert
 * request succeeds, then a recursive simulation job is appended to
 * the job queue.  If it fails, then a link is instead made to the
 * existing game state.  However, the socket implementation involves
 * more data copying between the different process address spaces, so
 * it is not the best solution within a single computer.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "core.h"
#include <glib.h>

#include "morris.h"

typedef struct GameTreeNode_tag GameTreeNode;
struct GameTreeNode_tag
{
  GameState state;
  GameTreeNode *links[BOARD_SIZE];
};

int main ()
{
  return 0;
}
