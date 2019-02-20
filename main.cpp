#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <ncurses.h>
#include <random>
#include <thread>
#include <vector>

static constexpr int KEY_ESCAPE{27};
static WINDOW *playfield;
static std::mutex lock;

/* Convinience implementation for random integer generation */
struct Random
{
  private:
    std::mt19937 engine;
    std::uniform_int_distribution<int> dist;

  public:
    inline Random(const int smallest, const int biggest)
        : engine(std::chrono::system_clock::now().time_since_epoch().count()), dist(smallest, biggest)
    {
    }
    inline int operator()() { return dist(engine); }
};

enum PieceType
{
    L = 0,
    J,
    Z,
    S,
    O,
    I,
    T
};
static constexpr std::array<uint16_t, 7> pieces{
    0b0100010001100000,
    0b0010001001100000,
    0b0000011000110000,
    0b0000001101100000,
    0b0000011001100000,
    0b0100010001000100,
    0b0000111001000000};
static constexpr uint16_t transpose(const uint16_t mask)
{
    uint16_t retval = 0;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            if (mask & (0x8000 >> (y * 4 + x)))
            {
                retval |= (0x8000 >> (x * 4 + y));
            }
        }
    }
    return retval;
}
static constexpr uint16_t reverse(const uint16_t mask)
{
    uint16_t retval = 0;
    for (int y = 0; y < 4; y++)
    {
        retval |= (mask & (0x8000 >> (4 * y))) ? 0x1000 >> (4 * y) : 0;
        retval |= (mask & (0x4000 >> (4 * y))) ? 0x2000 >> (4 * y) : 0;
        retval |= (mask & (0x2000 >> (4 * y))) ? 0x4000 >> (4 * y) : 0;
        retval |= (mask & (0x1000 >> (4 * y))) ? 0x8000 >> (4 * y) : 0;
    }
    return retval;
}
static constexpr uint16_t rotate_mask(const uint16_t mask)
{
    uint16_t retval = transpose(mask);
    return reverse(retval);
}

struct Piece
{
  private:
    uint16_t type;
    int x, y, w, h;

  public:
    Piece(const PieceType t, const int x = 1, const int y = 1)
        : type{pieces[t]}, x{x}, y{y}
    {
        calcWH();
    }
    void calcWH()
    {
        // Set width and height
        w = 0;
        h = 0;
        uint16_t pmaskw = type;
        uint16_t pmaskh = type;
        for (int i = 0; i < 4; i++)
        {
            if (0x8888 & pmaskw)
            {
                w++;
            }
            if (0xF000 & pmaskh)
            {
                h++;
            }
            pmaskw <<= 1;
            pmaskh <<= 4;
        }
    }
    void draw() const
    {
        uint16_t pmask = type;
        for (int k = y; k < y + 4; k++)
        {
            for (int j = x; j < x + 4; j++)
            {
                if (pmask & 0x8000)
                {
                    mvwaddch(playfield, k, j, ACS_BLOCK);
                }
                pmask <<= 1;
            }
        }
    }
    bool collidesWith(const Piece &p) const
    {
        return collidesWith(p.getType(), p.getX(), p.getY(), p.getW(), p.getH());
    }
    bool collidesWith(const uint16_t m, const int x, const int y, const int w, const int h) const
    {
        std::lock_guard<std::mutex> g(lock);
        // Check if in bounds
        if (y <= this->y + this->h && y + h >= this->y && x <= this->x + this->w && x + w >= this->x)
        {
            uint16_t pmask = m;
            uint16_t tmask = this->getType();

            // Move the mask of the most right piece to the right
            if (x > this->x)
            {
                for (int i = 0; i < x - this->x; i++)
                {
                    pmask = ((0xEEEE & pmask) >> 1);
                }
            }
            else
            {
                for (int i = 0; i < this->x - x; i++)
                {
                    tmask = ((0xEEEE & tmask) >> 1);
                }
            }

            // Move the mask of the most lower piece down
            if (this->y > y)
            {
                tmask >>= ((this->y - y) * 4);
            }
            else
            {
                pmask >>= ((y - this->y) * 4);
            }

            if ((pmask & tmask) != 0)
            {
                return true;
            }
        }
        return false;
    }

    inline void rotate()
    {
        type = rotate_mask(type);
        calcWH();
    }

    inline void setX(const int x) noexcept { this->x = x; }
    inline void setY(const int y) noexcept { this->y = y; }
    inline uint16_t getType() const noexcept { return type; }
    inline int getX() const noexcept { return x; }
    inline int getY() const noexcept { return y; }
    inline int getW() const noexcept { return w; }
    inline int getH() const noexcept { return h; }
};

static void update(const Piece &cur_piece, const std::vector<Piece> &placed_pieces)
{
    std::lock_guard<std::mutex> g(lock);
    /* Render */
    refresh();
    wclear(playfield);
    for (const Piece &p : placed_pieces)
    {
        p.draw();
    }
    cur_piece.draw();
    box(playfield, 0, 0);
    wrefresh(playfield);
}

int main()
{
    bool is_running{true};
    Random rand{0, pieces.size() - 1};
    Piece cur_piece{(PieceType)rand()};
    std::vector<Piece> placed_pieces;
    std::thread inputThread([&cur_piece, &is_running, &placed_pieces]() {
        while (is_running)
        {
            bool is_colliding = false;
            /* Input */
            switch (getch())
            {
            case KEY_ESCAPE:
                is_running = false;
                break;
            case KEY_LEFT:
            {
                // Check if next position would collide with other pieces
                Piece check_piece = cur_piece;
                check_piece.setX(cur_piece.getX() - 1);
                if (!check_piece.collidesWith(0x8888, 0, check_piece.getY(), 1, 4))
                {
                    for (const Piece &p : placed_pieces)
                    {
                        if (p.collidesWith(check_piece))
                        {
                            is_colliding = true;
                            break;
                        }
                    }
                    if (!is_colliding)
                    {
                        cur_piece.setX(cur_piece.getX() - 1);
                    }
                }
            }
            break;
            case KEY_RIGHT:
            {
                // Check if next position would collide with other pieces
                Piece check_piece = cur_piece;
                check_piece.setX(cur_piece.getX() + 1);
                if (!check_piece.collidesWith(0xFFFF, 8, check_piece.getY(), 4, 4))
                {
                    for (const Piece &p : placed_pieces)
                    {
                        if (p.collidesWith(check_piece))
                        {
                            is_colliding = true;
                            break;
                        }
                    }
                    if (!is_colliding)
                    {
                        cur_piece.setX(cur_piece.getX() + 1);
                    }
                }
            }
            break;
            case KEY_UP:
                // Check if rotation would collide with other pieces
                Piece check_piece = cur_piece;
                check_piece.rotate();
                for (const Piece &p : placed_pieces)
                {
                    if (p.collidesWith(check_piece))
                    {
                        is_colliding = true;
                        break;
                    }
                }
                // Check if rotation would collide with the playfield
                is_colliding = is_colliding || check_piece.collidesWith(0xFFFF, 8, check_piece.getY(), 4, 4) || check_piece.collidesWith(0x8888, 0, check_piece.getY(), 1, 4) || check_piece.collidesWith(0xFFFF, check_piece.getX(), 16, 4, 4);
                if (!is_colliding)
                {
                    cur_piece.rotate();
                }
            }
            update(cur_piece, placed_pieces);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    /* Init curses */
    initscr();
    noecho();
    keypad(stdscr, true);
    curs_set(0);

    /* Init playfield */
    playfield = newwin(17, 9, 0, 0);

    /* Game loop */
    while (is_running)
    {
        update(cur_piece, placed_pieces);

        /* Logic */
        bool is_colliding = false;
        Piece check_piece = cur_piece;
        check_piece.setY(check_piece.getY() + 1);
        // Check for collision with other pieces
        for (const Piece &p : placed_pieces)
        {
            if (p.collidesWith(check_piece))
            {
                is_colliding = true;
                break;
            }
        }
        // Check for collision with bottom of the playfield
        if (check_piece.collidesWith(0xFFFF, check_piece.getX(), 16, 4, 4))
        {
            is_colliding = true;
        }

        if (is_colliding)
        {
            Piece copy = cur_piece;
            placed_pieces.push_back(copy);
            cur_piece = Piece((PieceType)rand());
        }
        else
        {
            cur_piece.setY(cur_piece.getY() + 1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    inputThread.join();

    delwin(playfield);
    endwin();
}
