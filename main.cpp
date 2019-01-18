#include <iostream>
#include <ncurses.h>

int main()
{
    int input_delay_decisecond = 500;
    int user_input;
    bool is_running = true;

    /* Init curses */
    initscr();
    noecho();
    keypad(stdscr, true);
    halfdelay(input_delay_decisecond);

    /* Game loop */
    while(is_running) {
        user_input = getch();
        addch(user_input);
        refresh();
    }

    endwin();
}
