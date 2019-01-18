#include <ncurses.h>
#include <array>
#include <cstdint>
#include <random>
#include <chrono>

static constexpr int KEY_ESCAPE{27};

/* Convinience implementation for random integer generation */
struct Random {
private:
  std::mt19937 engine;
  std::uniform_int_distribution<int> dist;
public:
  inline Random(const int smallest, const int biggest)
	 : engine(std::chrono::system_clock::now().time_since_epoch().count())
	 , dist(smallest, biggest) {}
  inline int operator()() { return dist(engine); }
};

enum PieceType { L = 0, J, Z, S, O, I, T };
static constexpr std::array<uint16_t, 7> pieces {
  0b0100010001100000,
  0b0010001001100000,
  0b0110001100000000,
  0b0110110000000000,
  0b0110011000000000,
  0b0100010001000100,
  0b1110010000000000
};

struct Piece {
private:
  PieceType type;
  int x, y;
public:
  inline Piece(const PieceType t, const int x = 0, const int y = 0)
	 : type{t}, x{x}, y{y} {}
};

int main() {
  int input_delay_deciseconds{10};
  bool is_running{true};
  Random rand{0, pieces.size() - 1};
  Piece cur_piece{(PieceType)rand()};
  
  /* Init curses */
  initscr();
  noecho();
  keypad(stdscr, true);
  halfdelay(input_delay_deciseconds);
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
	 case KEY_ESCAPE:
		is_running = false;
	 }
  }
  
  delwin(playfield);
  endwin();
}
