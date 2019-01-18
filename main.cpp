#include <ncurses.h>

struct Piece {
};

int main()
{
  int input_delay_decisecond = 10;
  int user_input;
  bool is_running = true;
  
  /* Init curses */
  initscr();
  noecho();
  keypad(stdscr, true);
  halfdelay(input_delay_decisecond);
  curs_set(0);
  
  /* Create playfield */
  WINDOW* playfield = newwin(17, 9, 0, 0);
  box(playfield, 0, 0);
  
  /* Game loop */
  while(is_running) {
	 refresh();
	 wrefresh(playfield);

	 switch(getch()) {
	 case KEY_LEFT:
		break;
	 case KEY_RIGHT:
		break;
	 case KEY_UP:
		break;
	 case KEY_DOWN:
		break;
	 }
  }
  
  delwin(playfield);
  endwin();
}
