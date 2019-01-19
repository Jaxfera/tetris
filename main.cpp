#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <ncurses.h>
#include <random>
#include <vector>

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
    0b0010001001100000,
    0b0110001100000000,
    0b0110110000000000,
    0b0110011000000000,
    0b0100010001000100,
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
    inline Piece(const Piece& lval)
    {
        type = lval.getType();
        x = lval.getX();
        y = lval.getY();
        w = lval.getW();
        h = lval.getH();
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
        // Check if in bounds
        if (p.getY() <= y + h && p.getY() + p.getH() >= y
            && p.getX() <= x + w && p.getX() + p.getW() >= x) {
            // Check if masks overlap
            uint16_t pmask = p.getType();
            uint16_t tmask = this->getType();
            // Bitshift left to match x position
            if (p.getX() + p.getW() < x + w) {
                for (int i = 0; i < p.getX() + p.getW() - (x + w); i++) {
                    tmask <<= tmask & 0x7777;
                }
            } else {
                for (int i = 0; i < x + w - (p.getX() + p.getW()); i++) {
                    pmask <<= pmask & 0x7777;
                }
            }
            // Bitshift down to match y position
            if (p.getY() + p.getH() < y + h) {
                pmask <<= (p.getY() + p.getH() - (y + h)) * 4;
            } else {
                tmask <<= (y + h - (p.getY() + p.getH())) * 4;
            }
            if (pmask & tmask) {
                mvprintw(6, 20, "COLISION=1");
                return true;
            }
        }
        mvprintw(6, 20, "COLISION=0");
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
        case KEY_DOWN:
            cur_piece.setY(cur_piece.getY() + 1);
            break;
        case 'd': // DEBUG
            break;
        case KEY_ESCAPE:
            is_running = false;
        }

        /* Logic */
        bool is_colliding = false;
        // Check for collision with other pieces
        for (const Piece& p : placed_pieces) {
            if (p.collidesWith(cur_piece)) {
                is_colliding = true;
                break;
            }
        }
        // Check for collision with playfield
        if (cur_piece.getY() + cur_piece.getH() > 15) {
            is_colliding = true;
        }

        if (is_colliding) {
            placed_pieces.push_back(Piece{ cur_piece });
            cur_piece = Piece((PieceType)rand());
        } else {
            cur_piece.setY(cur_piece.getY() + 1);
        }
    }

    delwin(playfield);
    endwin();
}
