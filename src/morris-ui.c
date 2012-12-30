/* User interface startup for N Mens Morris.

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
 * N Mens Morris main startup file.
 *
 * This file contains both the main startup code and the GTK+
 * graphical user interface code.  Note that there is also a
 * line-based terminal interface available too.  Its implementation is
 * contained in the file morris-term.c.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "core.h"
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "support.h"
#include "morris.h"

gchar *package_prefix = PACKAGE_PREFIX;
gchar *package_data_dir = PACKAGE_DATA_DIR;
gchar *package_locale_dir = PACKAGE_LOCALE_DIR;

GtkWidget *main_window;
GtkWidget *vbox;
GtkWidget *drawing_area;
GtkWidget *scroll_window;
GtkTextBuffer *text_buffer;
GtkWidget *text_view;

#define DELTA 32 /**< Board drawing unit size */
#define MARK_SIZE 24 /**< Size of the selection mark */
#define PIECE_SIZE 16 /**< Diameter of the board pieces */
/** Mapping of graphical positions to board positions */
guint board_points[24][2] = { {0, 0}, {3, 0}, {6, 0},
			      {1, 1}, {3, 1}, {5, 1},
			      {2, 2}, {3, 2}, {4, 2},
			      {0, 3}, {1, 3}, {2, 3},
			      {4, 3}, {5, 3}, {6, 3},
			      {2, 4}, {3, 4}, {4, 4},
			      {1, 5}, {3, 5}, {5, 5},
			      {0, 6}, {3, 6}, {6, 6} };
static guint board_src_mark = 0;
static guint board_sel = 0;
static guint setup_player = 0;
/** move_state: 0 when not in the main game state, 1 when setting the
    source position, 2 when setting the destination position.  */
static guint move_state = 0;
static GameState state;

int term_main (int argc, char *argv[]);

/**
 * Update the text window with a new message.
 *
 * @param update_style 0 to append @a message to the text buffer, 1 to
 * clear and reset the text buffer with a new header, 2 to display an
 * end game winning message
 * @param message the message to display or add to the text window
 */
void
update_text_view (guint update_style, const gchar *message)
{
  GtkTextIter iter;
  GtkTextMark *mark;

  if (update_style == 0)
    gtk_text_buffer_get_end_iter (text_buffer, &iter);
  if (update_style == 1)
    {
      char *buffer;
      char *fmt_string;
      gtk_text_buffer_set_text (text_buffer, "", -1);
      gtk_text_buffer_get_start_iter (text_buffer, &iter);
      fmt_string = _("Player %u's turn.\n");
      buffer = g_malloc (strlen (fmt_string) + 4 + 1);
      sprintf (buffer, fmt_string, (guint) state.cur_player);
      gtk_text_buffer_insert_with_tags_by_name
	(text_buffer, &iter, buffer, -1,
	 "word_wrap", "not_editable", "heading", NULL);
      g_free (buffer);
    }
  if (update_style == 2)
    {
      Player player;
      char *buffer;
      char *fmt_string;
      player = get_winner (&state);
      gtk_text_buffer_set_text (text_buffer, "", -1);
      gtk_text_buffer_get_start_iter (text_buffer, &iter);
      fmt_string = _("Player %u won.\n");
      buffer = g_malloc (strlen (fmt_string) + 4 + 1);
      sprintf (buffer, fmt_string, (guint) player);
      gtk_text_buffer_insert_with_tags_by_name
	(text_buffer, &iter, buffer, -1,
	 "word_wrap", "not_editable", "heading", NULL);
      g_free (buffer);
    }

  gtk_text_buffer_insert_with_tags_by_name
    (text_buffer, &iter, message, -1,
     "word_wrap", "not_editable", NULL);
  if (message[0] != '\0')
    gtk_text_buffer_insert_with_tags_by_name
      (text_buffer, &iter, "\n", -1,
       "word_wrap", "not_editable", NULL);
  mark = gtk_text_buffer_get_mark (text_buffer, "end");
  gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (text_view), mark);
}

/**
 * Signal handler for the "expose_event" event sent to the drawing
 * area.
 *
 * This signal handler renders the board within the N Mens Morris
 * window.
 */
gboolean
expose_event (GtkWidget * widget, GdkEventExpose * event,
	      gpointer user_data)
{
  static GdkGC *gc = NULL;
  GdkColor ltgreen, tan, black, red, blue, dkgreen;
  gint lines[8][4] = { { 0, 0, 2, 2 }, { 3, 0, 3, 2 },
		       { 6, 0, 4, 2 }, { 6, 3, 4, 3 },
		       { 6, 6, 4, 4 }, { 3, 6, 3, 4 },
		       { 0, 6, 2, 4 }, { 0, 3, 2, 3 } };
  guint i;

  ltgreen.red = 0x8000;
  ltgreen.green = 0xffff;
  ltgreen.blue = 0x8000;
  black.red = 0x0000;
  black.green = 0x0000;
  black.blue = 0x0000;
  tan.red = 0xffff;
  tan.green = 0xf000;
  tan.blue = 0xc000;
  red.red = 0xffff;
  red.green = 0x0000;
  red.blue = 0x0000;
  blue.red = 0x0000;
  blue.green = 0x0000;
  blue.blue = 0xffff;
  dkgreen.red = 0x0000;
  dkgreen.green = 0xa000;
  dkgreen.blue = 0x0000;

  /* At the start of an expose handler, a clip region of event->area
     is set on the window, and event->area has been cleared to the
     widget's background color.  */

  if (gc == NULL)
    gc = gdk_gc_new (widget->window);

  /* Draw the board background.  */
  /* gdk_gc_set_rgb_fg_color (gc, &tan);
  gdk_draw_rectangle
    (widget->window, gc, TRUE, DELTA*1, DELTA*1, DELTA*6, DELTA*6); */

  /* Draw the curent selection.  */
  gdk_gc_set_rgb_fg_color (gc, &ltgreen);
  gdk_draw_rectangle (widget->window, gc, TRUE,
	      DELTA * (board_points[board_sel][0] + 1) - MARK_SIZE / 2,
	      DELTA * (board_points[board_sel][1] + 1) - MARK_SIZE / 2,
	      MARK_SIZE, MARK_SIZE);

  /* Draw the marker for the source position of a move.  */
  if (move_state == 2)
    {
      gdk_gc_set_rgb_fg_color (gc, &dkgreen);
      gdk_draw_rectangle (widget->window, gc, TRUE,
	  DELTA * (board_points[board_src_mark][0] + 1) - MARK_SIZE / 2,
	  DELTA * (board_points[board_src_mark][1] + 1) - MARK_SIZE / 2,
	  MARK_SIZE, MARK_SIZE);
    }

  /* Draw the 11 Mens Morris board.  */
  gdk_gc_set_rgb_fg_color (gc, &black);
  gdk_draw_rectangle
    (widget->window, gc, FALSE, DELTA*1, DELTA*1, DELTA*6, DELTA*6);
  gdk_draw_rectangle
    (widget->window, gc, FALSE, DELTA*2, DELTA*2, DELTA*4, DELTA*4);
  gdk_draw_rectangle
    (widget->window, gc, FALSE, DELTA*3, DELTA*3, DELTA*2, DELTA*2);
  for (i = 0; i < 8; i++)
    gdk_draw_line (widget->window, gc, DELTA * (lines[i][0] + 1),
		   DELTA * (lines[i][1] + 1), DELTA * (lines[i][2] + 1),
		   DELTA * (lines[i][3] + 1));

  /* Draw the board pieces.  */
  for (i = 0; i < BOARD_SIZE; i++)
    {
      Player player;
      player = board_ref (state.board, i);
      if (player == PLAYER1)
	  gdk_gc_set_rgb_fg_color (gc, &red);
      if (player == PLAYER2)
	  gdk_gc_set_rgb_fg_color (gc, &blue);
      if (player != EMPTY)
	  gdk_draw_arc (widget->window, gc, TRUE,
			DELTA * (board_points[i][0] + 1) - PIECE_SIZE / 2,
			DELTA * (board_points[i][1] + 1) - PIECE_SIZE / 2,
			PIECE_SIZE, PIECE_SIZE, 0, 360 * 64);
      else
	{
	  gdk_gc_set_rgb_fg_color (gc, &black);
	  gdk_draw_arc (widget->window, gc, TRUE,
			DELTA * (board_points[i][0] + 1) - 2,
			DELTA * (board_points[i][1] + 1) - 2,
			4, 4, 0, 360 * 64);
	}
    }

  return TRUE;
}

/**
 * Set the selected board position based off of a mouse position.
 *
 * @param x the mouse x-coordinate
 * @param y the mouse y-coordinate
 * @return TRUE if a board position was set, FALSE otherwise
 */
gboolean
mouse_set_board_sel (gdouble x, gdouble y)
{
  guint i;
  for (i = 0; i < 24; i++)
    {
      gdouble minx, miny, maxx, maxy;
      minx = DELTA * (board_points[i][0] + 1) - MARK_SIZE / 2;
      miny = DELTA * (board_points[i][1] + 1) - MARK_SIZE / 2;
      maxx = DELTA * (board_points[i][0] + 1) + MARK_SIZE / 2;
      maxy = DELTA * (board_points[i][1] + 1) + MARK_SIZE / 2;
      if (minx <= x && x < maxx &&
	  miny <= y && y < maxy)
	{
	  board_sel = i;
	  gtk_widget_queue_draw (drawing_area);
	  return TRUE;
	}
    }
  return FALSE;
}

/**
 * Signal handler for the "motion_notify" event sent to the drawing
 * area.
 *
 * This signal handler updates the current board selection.
 */
gboolean
motion_notify_event (GtkWidget * widget, GdkEventMotion * event,
		     gpointer data)
{
  mouse_set_board_sel (event->x, event->y);
  return TRUE;
}

/**
 * Display a diagnostic message when an attempted gameplay remove
 * action failed.
 */
void
display_remove_diagnostic ()
{
  Player player;
  player = board_ref (state.board, board_sel);
  if (player == EMPTY)
    update_text_view (0, _("Just because there's air there " \
			       "doesn't mean you can remove it."));
  else if (player == state.cur_player)
    update_text_view (0, _("Are you crazy?  You don't need to " \
			       "attack your own people."));
  else
    update_text_view (0, _("Sorry, can't do that.  Have you "
		       "noticed there's a piece outside of a mill?"));
}

/**
 * Signal handler for the "button_release" event sent to the drawing
 * area.
 *
 * This is also where all of the gameplay logic for the graphical user
 * interface for N Mens Morris is, as moves can only be initiated
 * after the user has selected the right positions.
 */
gboolean
button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  gboolean next_move = TRUE; /* Was the current move, place, or remove
				completed successfully?  */
  if (event->button == 3 && move_state == 2)
  if (event->button != 1)
    return FALSE;

  if (!mouse_set_board_sel (event->x, event->y))
    return FALSE;

  if (state.setup_rounds_left > 0)
    {
      if (state.remove_state)
	{
	  if (!remove_piece (&state, board_sel))
	    {
	      display_remove_diagnostic ();
	      next_move = FALSE;
	    }
	}
      else
	{
	  if (!place_piece (&state, board_sel))
	    {
	      update_text_view (0, _("Don't get me wrong.  " \
				 "That space is already occupied."));
	      next_move = FALSE;
	    }
	}
      if (next_move && !state.remove_state)
	{
	  setup_player++;
	  if (setup_player >= NUM_PLAYERS)
	    {
	      setup_player = 0;
	      state.setup_rounds_left--;
	      if (state.setup_rounds_left == 0)
		{
		  move_state = 1;
		  next_move = FALSE;
		  update_text_view (1, _("Which piece will you move?"));
		}
	    }
	}
      gtk_widget_queue_draw (drawing_area);
      if (next_move)
	{
	  if (state.remove_state)
	    update_text_view (1, _("Which piece will you remove?"));
	  else
	    update_text_view (1, _("Where will you place your piece?"));
	}
    }
  else if (get_winner (&state) == EMPTY)
    {
      if (state.remove_state)
	{
	  if (!remove_piece (&state, board_sel))
	    {
	      display_remove_diagnostic ();
	      next_move = FALSE;
	    }
	}
      else
	{
	  if (move_state == 1)
	    {
	      if (board_ref (state.board, board_sel) != state.cur_player)
		{
		  update_text_view (0, _("Wrong place silly!  "	\
			   "You can only move your own pieces."));
		  next_move = FALSE;
		}
	      else
		{
		  board_src_mark = board_sel;
		  move_state = 2;
		}
	    }
	  else
	    {
	      if (board_sel == board_src_mark)
		move_state = 1;
	      else if (!move_piece (&state, board_src_mark, board_sel))
		{
		  update_text_view (0,
		    _("You can't move into an already occupied space.  " \
		      "Rules are rules."));
		  next_move = FALSE;
		}
	      else
		move_state = 1;
	    }
	}
      gtk_widget_queue_draw (drawing_area);
      if (next_move)
	{
	  Player player;
	  player = get_winner (&state);
	  if (state.remove_state)
		update_text_view (1, _("Which piece will you remove?"));
	  else if (player == EMPTY)
	    {
	      if (move_state == 1)
		update_text_view (1, _("Which piece will you move?"));
	      else if (move_state == 2)
		update_text_view (1, _("Where will you move it to?"));
	    }
	  else /* Display winner */
	    update_text_view (2, "");
	}
    }
  return FALSE;
}

/**
 * Signal handler for the "key_press_event" sent to the drawing area.
 *
 * This signal handler implements the keyboard interface the graphical
 * user interface.
 */
gboolean
key_press_event (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  switch (event->keyval)
    {
    case GDK_Up:
      break;
    case GDK_Down:
      break;
    case GDK_Left:
      break;
    case GDK_Right:
      break;
    case GDK_Return:
      break;
    case GDK_Escape:
      break;
    default:
      return FALSE;
    }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  gboolean gui_init;
#ifdef G_OS_WIN32
  package_prefix = g_win32_get_package_installation_directory (NULL, NULL);
  package_data_dir = g_build_filename (package_prefix, "share", NULL);
  package_locale_dir =
    g_build_filename (package_prefix, "share", "locale", NULL);
#endif

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, package_locale_dir);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale ();
  gui_init = gtk_init_check (&argc, &argv);
  if (argc >= 2 && (!strcmp ("-h", argv[1]) ||
		    !strcmp ("--help", argv[1])))
    {
      puts (_( \
"Ussage: morris-ui <--nogui>\n" \
"If `--nogui' is specified, then a terminal interface will be run\n"
"instead of a GTK+ graphical interface."));
      return 0;
    }
  if (!gui_init || argc >= 2 && !strcmp ("--nogui", argv[1]))
    return term_main (argc, argv);

#ifdef G_OS_WIN32
  FreeConsole();
#endif

  {
    gchar *pixmap_dir;
    pixmap_dir = g_build_filename (package_data_dir, PACKAGE,
				   "pixmaps", NULL);
    add_pixmap_directory (pixmap_dir);
    g_free (pixmap_dir);
  }

  /* {
    GList *icon_list;
    icon_list = NULL;
    icon_list = g_list_prepend (icon_list, create_pixbuf ("icon48.png"));
    icon_list = g_list_prepend (icon_list, create_pixbuf ("icon32.png"));
    icon_list = g_list_prepend (icon_list, create_pixbuf ("icon16.png"));
    icon_list = g_list_prepend (icon_list, create_pixbuf ("icon256.png"));
    icon_list = g_list_reverse (icon_list);
    gtk_window_set_default_icon_list (icon_list);
    \/* A copy of this list has been made, so it can be freed.  *\/
    g_list_free (icon_list);
  } */

  init_game_state (&state);

  {
    GdkColor color;
    GtkTextIter iter;

    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width (GTK_CONTAINER (main_window), 4);
    gtk_window_set_title (GTK_WINDOW (main_window), _("11 Mens Morris"));
    gtk_window_set_default_size (GTK_WINDOW (main_window), 256, 400);

    vbox = gtk_vbox_new (FALSE, 4);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (main_window), vbox);

    drawing_area = gtk_drawing_area_new ();
    gtk_widget_set_size_request (drawing_area, DELTA * 8, DELTA * 8);
    gtk_widget_show (drawing_area);
    gtk_box_pack_start (GTK_BOX (vbox), drawing_area, FALSE, FALSE, 0);
    color.red = 0xffff;
    color.green = 0xffff;
    color.blue = 0xffff;
    gtk_widget_modify_bg (drawing_area, GTK_STATE_NORMAL, &color);
    gtk_widget_add_events (drawing_area,
			   GDK_POINTER_MOTION_MASK |
			   GDK_BUTTON_PRESS_MASK |
			   GDK_BUTTON_RELEASE_MASK);
    g_signal_connect ((gpointer) drawing_area, "expose_event",
		      G_CALLBACK (expose_event), NULL);
    g_signal_connect ((gpointer) drawing_area, "motion_notify_event",
		      G_CALLBACK (motion_notify_event), NULL);
    g_signal_connect ((gpointer) drawing_area, "button_release_event",
		      G_CALLBACK (button_release_event), NULL);
    g_signal_connect ((gpointer) drawing_area, "key_press_event",
		      G_CALLBACK (key_press_event), NULL);

    scroll_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_window),
			    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show (scroll_window);

    text_view = gtk_text_view_new ();
    gtk_widget_show (text_view);
    gtk_box_pack_start (GTK_BOX (vbox), scroll_window, TRUE, TRUE, 0);
    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
    gtk_text_buffer_create_tag (text_buffer, "word_wrap",
				"wrap_mode", GTK_WRAP_WORD, NULL);
    gtk_text_buffer_create_tag (text_buffer, "heading",
				"weight", PANGO_WEIGHT_BOLD,
				"size", 15 * PANGO_SCALE,
				NULL);
    gtk_text_buffer_create_tag (text_buffer, "not_editable",
				"editable", FALSE, NULL);
    gtk_text_buffer_get_end_iter (text_buffer, &iter);
    gtk_text_buffer_create_mark (text_buffer, "end", &iter, FALSE);
    gtk_container_add (GTK_CONTAINER (scroll_window), text_view);

    update_text_view (1, _("Where will you place your piece?"));

    gtk_widget_show (main_window);
    g_signal_connect ((gpointer) main_window, "destroy",
		      G_CALLBACK (gtk_main_quit), NULL);

    gtk_main ();
  }

#ifdef G_OS_WIN32
  g_free (package_prefix);
  g_free (package_data_dir);
  g_free (package_locale_dir);
#endif

  return 0;
}

#ifdef _MSC_VER

int WINAPI
WinMain (HINSTANCE hInstance,
	 HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  return main (__argc, __argv);
}
#endif
