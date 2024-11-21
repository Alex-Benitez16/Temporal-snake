#include "curses.h"
#include <random>
#include <chrono>
#include <thread>
#include <deque>
#include <array>

TerminalUI ui;


class Tail {
    private:
        std::deque<std::array<int, 2>> body;
        char character;
    public:
        Tail(char set_character, int y, int x) {
            character = set_character;
            body.push_back({y, x - 1});
            body.push_back({y, x - 2});
            body.push_back({y, x - 3});
        }

        void move(int new_y, int new_x) {
            ui.mvaddch(body.back().at(0), body.back().at(1), ' ');
            body.pop_back();
            body.push_front({new_y, new_x});
            ui.mvaddch(body.front().at(0), body.front().at(1), character);
        }

        void add(int new_y, int new_x) {
            body.push_front({new_y, new_x});
            ui.mvaddch(body.front().at(0), body.front().at(1), character);
        }

};

class Snake {
    private: 
        int x;
        int y;
        char head;
        Tail tail;
    public:
        Snake(int set_x, int set_y, char set_head):
            x{set_x}, y{set_y}, head{set_head},
            tail{'#', set_y, set_x}{};

        int get_x() { return x; }
        int get_y() { return y; }

        void move(int new_y, int new_x) {
            ui.mvaddch(y, x,' ');
            tail.move(y,x);
            x = new_x;
            y = new_y;
            ui.mvaddch(y, x, head); 
        }

        void move_up() {
            move(y - 1, x);
        }
        void move_down() {
            move(y + 1, x);
        }
        void move_right() {
            move(y, x + 2);
        }
        void move_left() {
            move(y, x - 2);
        }

        void add_to_tail(int new_y, int new_x) {
            tail.add(y, x);
            x = new_x;
            y = new_y;
            ui.mvaddch(y, x, head);
        }
        void add_up() {
            add_to_tail(y - 1, x);
        }
        void add_down() {
            add_to_tail(y + 1, x);
        }
        void add_right() {
            add_to_tail(y, x + 2);
        }
        void add_left() {
            add_to_tail(y, x - 2);
        }
};

class Food {
    private: 
        int x;
        int y;
        std::mt19937 gen;
        char character;
    public:
        void place(int min, int max) {
            std::random_device rd;
            gen.seed(rd());
            std::uniform_int_distribution<> intDist(min, max);
            x = intDist(gen);
            if(x % 2 == 1) x--;
            y = intDist(gen);
            ui.mvaddch(y, x ,character);
        }

        Food(char set_character, int min, int max): character(set_character) {
            place(min, max);
        };

        int get_x() { return x; }
        int get_y() { return y; }
};

int main() {
    WINDOW* main_window = ui.initscr();    
    ui.curs_set(0);
    ui.noecho();
    ui.nodelay(true);
    ui.cbreak();
    ui.keypad(true);

    Snake my_snake(4, 2, '@');

    int input;
    int direction = KEY_RIGHT;

    const int MOVE_DELAY = 100;
    auto last_move = std::chrono::steady_clock::now();

    int terminal_x, terminal_y;
    ui.getmaxyx(main_window, terminal_y, terminal_x);

    Food apple('O', 10, terminal_y - 10);

    while(true) {
        input = ui.getch();

        // ui.mvprintw(0, 0, "Key pressed: %d", input);
        // ui.refresh();

        //if(input == KEY_RIGHT || input == KEY_LEFT || input == KEY_UP || input == KEY_DOWN) {
            switch(input) {
            case KEY_RIGHT:
            case 'd':
                direction = KEY_RIGHT;
                break;
            case KEY_LEFT:
            case 'a':
                direction = KEY_LEFT;
                break;
            case KEY_UP:
            case 'w':
                direction = KEY_UP;
                break;
            case KEY_DOWN:
            case 's':
                direction = KEY_DOWN;
                break;
            }
        //}
            
        auto current_time = std::chrono::steady_clock::now();
        auto time_since_last_move = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_move).count();

        if(time_since_last_move >= MOVE_DELAY) {
            if(my_snake.get_x() == apple.get_x() && my_snake.get_y() == apple.get_y()) {
                switch(direction) {
                case KEY_UP:
                    my_snake.add_up();
                    apple.place(10, terminal_y - 10);
                    break;
                case KEY_DOWN:
                    my_snake.add_down();
                    apple.place(10, terminal_y - 10);
                    break;
                case KEY_RIGHT:
                    my_snake.add_right();
                    apple.place(10, terminal_y - 10);
                    break;
                case KEY_LEFT:
                    my_snake.add_left();
                    apple.place(10, terminal_y - 10);
                    break; 
                }
            }
            
            switch(direction) {
            case KEY_UP:
                my_snake.move_up();
                break;
            case KEY_DOWN:
                my_snake.move_down();
                break;
            case KEY_RIGHT:
                my_snake.move_right();
                break;
            case KEY_LEFT:
                my_snake.move_left();
                break;
            }

            last_move = current_time;
            ui.refresh();

        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ui.endwin();
}