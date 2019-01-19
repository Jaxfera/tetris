#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <ncurses.h>
#include <random>
#include <vector>

static bool debug = false;

static constexpr int KEY_ESCAPE{ 27 };
static WINDOW* playfield;

/* Convinience implementation for random integer generation */
struct Random {
private:
    std::mt19937 engine;
    std::uniform_int_distribution<int> dist;

public:
    inline Random(const int smallest, const int biggest)
        : engine(std::chrono::system_clock::now().time_since_epoch().count())
        , dist(smallest, biggest)
    {
    }
    inline int operator()() { return dist(engine); }
};

enum PieceType { L = 0,
    J,
    Z,
    S,
    O,
    I,
    T };
static constexpr std::array<uint16_t, 7> pieces{
    0b1000100011000000,
    0b0100010011000000,
    0b1100011000000000,
    0b0110110000000000,
    0b1100110000000000,
    0b1000100010001000,
    0b1110010000000000
};

struct Piece {
private:
    uint16_t type;
    int x, y, w, h;

public:
    inline Piece(const PieceType t, const int x = 1, const int y = 1)
        : type{ pieces[t] }
        , x{ x }
        , y{ y }
        , w{ 0 }
        , h{ 0 }
    {
        // Set width and height
        uint16_t pmaskw = type;
        uint16_t pmaskh = type;
        for (int i = 0; i < 4; i++) {
            if (0x8888 & pmaskw) {
                w++;
            }
            if (0xF000 & pmaskh) {
                h++;
            }
            pmaskw <<= 1;
            pmaskh <<= 4;
        }
    }
    inline void draw() const
    {
        uint16_t pmask = type;
        for (int k = y; k < y + 4; k++) {
            for (int j = x; j < x + 4; j++) {
                if (pmask & 0x8000) {
                    mvwaddch(playfield, k, j, ACS_BLOCK);
                }
                pmask <<= 1;
            }
        }
    }
    inline bool collidesWith(const Piece& p) const
    {
        if(debug)
            debug = false;
        // Check if in bounds
        if (p.getY() <= y + h && p.getY() + p.getH() >= y
            && p.getX() <= x + w && p.getX() + p.getW() >= x) {
            uint16_t pmask = p.getType();
            uint16_t tmask = this->getType();

            // Move the mask of the most right piece to the right
            if (p.getX() > x) {
                for (int i = 0; i < p.getX() - x; i++) {
                    pmask = ((0xEEEE & pmask) >> 1);
                }
            } else {
                for (int i = 0; i < x - p.getX(); i++) {
                    tmask = ((0xEEEE & tmask) >> 1);
                }
            }

            // Move the mask of the most lower piece down
            if (y > p.getY()) {
                tmask >>= ((y - p.getY()) * 4);
            } else {
                pmask >>= ((p.getY() - y) * 4);
            }

            if ((pmask & tmask) != 0) {
                return true;
            }
        }
        return false;
    }
    inline void setX(const int x) { this->x = x; }
    inline void setY(const int y) { this->y = y; }
    inline uint16_t getType() const { return type; }
    inline int getX() const { return x; }
    inline int getY() const { return y; }
    inline int getW() const { return w; }
    inline int getH() const { return h; }
};

int main()
{
    int input_delay_deciseconds{ 10 };
    bool is_running{ true };
    Random rand{ 0, pieces.size() - 1 };
    Piece cur_piece{ (PieceType)rand() };
    std::vector<Piece> placed_pieces;

    /* Init curses */
    initscr();
    noecho();
    keypad(stdscr, true);
    halfdelay(input_delay_deciseconds);
    curs_set(0);

    /* Init playfield */
    playfield = newwin(17, 9, 0, 0);

    /* Game loop */
    while (is_running) {
        /* Render */
        // Window
        mvprintw(2, 20, "X:%02d", cur_piece.getX());
        mvprintw(3, 20, "Y:%02d", cur_piece.getY());
        mvprintw(4, 20, "W:%02d", cur_piece.getW());
        mvprintw(5, 20, "H:%02d", cur_piece.getH());
        refresh();

        // Playfield
        /* for (int y = 1; y < 17; y++) {
            for (int x = 1; x < 9; x++) {
                mvwaddch(playfield, y, x, ' ');
            }
        } */
        wclear(playfield);
        for (const Piece& p : placed_pieces) {
            p.draw();
        }
        cur_piece.draw();
        box(playfield, 0, 0);
        wrefresh(playfield);

        /* Input */
        switch (getch()) {
        case KEY_LEFT:
            cur_piece.setX(cur_piece.getX() - 1);
            break;
        case KEY_RIGHT:
            cur_piece.setX(cur_piece.getX() + 1);
            break;
        case 'd':
            debug = true;
            break;
        case KEY_ESCAPE:
            is_running = false;
        }

        /* Logic */
        bool is_colliding = false;
        Piece check_piece = cur_piece;
        check_piece.setY(check_piece.getY() + 1);
        // Check for collision with other pieces
        for (const Piece& p : placed_pieces) {
            if (p.collidesWith(check_piece)) {
                is_colliding = true;
                break;
            }
        }
        // Check for collision with playfield
        if (cur_piece.getY() + cur_piece.getH() > 15) {
            is_colliding = true;
        }

        if (is_colliding) {
            Piece copy = cur_piece;
            placed_pieces.push_back(copy);
            cur_piece = Piece((PieceType)rand());
        } else {
            cur_piece.setY(cur_piece.getY() + 1);
        }
    }

    delwin(playfield);
    endwin();
}
