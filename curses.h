#ifndef CROSS_PLATFORM_TERMINAL_UI_H
#define CROSS_PLATFORM_TERMINAL_UI_H

#include <iostream>
#include <vector>
#include <string>
#include <cstdarg>
#include <cstring>
#include <algorithm>

// Platform-specific headers
#ifdef _WIN32
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif

// Key definitions to mimic ncurses
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77
#define KEY_ENTER 13
#define KEY_BACKSPACE 8
#define KEY_ESC 27

class WINDOW {
public:
    int height;
    int width;
    int start_y;
    int start_x;
    std::vector<std::vector<char>> buffer;

    WINDOW(int h = 0, int w = 0, int y = 0, int x = 0) 
        : height(h), width(w), start_y(y), start_x(x) {
        buffer.resize(height, std::vector<char>(width, ' '));
    }
};

class TerminalUI {
private:
    // Current window
    WINDOW* current_window;
    
    // Terminal mode flags
    bool is_initialized;
    bool echo_mode;
    bool nodelay_mode;
    bool cursor_visible;

    // Platform-specific terminal settings
    #ifdef _WIN32
        HANDLE console_handle;
        DWORD old_console_mode;
    #else
        struct termios old_termios;
    #endif

    // Platform-independent terminal setup
    void setup_terminal() {
        #ifdef _WIN32
            console_handle = GetStdHandle(STD_INPUT_HANDLE);
            GetConsoleMode(console_handle, &old_console_mode);
            
            // Disable echo and line buffering
            DWORD mode = old_console_mode;
            mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
            mode |= ENABLE_MOUSE_INPUT;
            SetConsoleMode(console_handle, mode);
        #else
            struct termios new_termios;
            tcgetattr(STDIN_FILENO, &old_termios);
            new_termios = old_termios;
            
            // Disable canonical mode and echoing
            new_termios.c_lflag &= ~(ICANON | ECHO);
            new_termios.c_cc[VMIN] = 0;
            new_termios.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
        #endif
    }

    // Platform-independent terminal restore
    void restore_terminal() {
        #ifdef _WIN32
            SetConsoleMode(console_handle, old_console_mode);
        #else
            tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
        #endif
    }

    // Windows-specific console input method
    #ifdef _WIN32
    int windows_getch() {
        INPUT_RECORD input_record;
        DWORD events_read;
        
        while (true) {
            // Wait for an input event
            ReadConsoleInput(console_handle, &input_record, 1, &events_read);
            
            // Check if it's a key event
            if (input_record.EventType == KEY_EVENT && 
                input_record.Event.KeyEvent.bKeyDown) {
                
                // Get the key code
                WORD virtual_key_code = input_record.Event.KeyEvent.wVirtualKeyCode;
                char ascii_char = input_record.Event.KeyEvent.uChar.AsciiChar;
                
                // Handle special keys
                switch (virtual_key_code) {
                    case VK_UP: return KEY_UP;
                    case VK_DOWN: return KEY_DOWN;
                    case VK_LEFT: return KEY_LEFT;
                    case VK_RIGHT: return KEY_RIGHT;
                    case VK_RETURN: return KEY_ENTER;
                    case VK_BACK: return KEY_BACKSPACE;
                    case VK_ESCAPE: return KEY_ESC;
                }
                
                // Return ASCII character if it's a printable character
                if (ascii_char) return ascii_char;
            }
        }
    }
    #endif

public:
    // initscr() equivalent
    WINDOW* initscr() {
        // Reset flags
        is_initialized = true;
        echo_mode = true;
        nodelay_mode = false;
        cursor_visible = true;

        // Detect terminal size
        int height, width;
        detect_screen_size(height, width);
        
        // Create main window
        current_window = new WINDOW(height, width, 0, 0);

        // Setup terminal modes
        setup_terminal();

        return current_window;
    }

    // endwin() equivalent
    void endwin() {
        if (!is_initialized) return;

        // Restore terminal to original state
        restore_terminal();

        // Clear screen
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        // Clean up window
        if (current_window) {
            delete current_window;
            current_window = nullptr;
        }

        // Reset flags
        is_initialized = false;
    }

    // Detect screen size cross-platform
    void detect_screen_size(int& height, int& width) {
        #ifdef _WIN32
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        #else
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            width = w.ws_col;
            height = w.ws_row;
        #endif

        // Fallback to default if detection fails
        if (width <= 0) width = 80;
        if (height <= 0) height = 24;
    }

    // Equivalent to noecho()
    void noecho() {
        echo_mode = false;
    }

    // Equivalent to keypad()
    void keypad(bool enable) {
        // In a real implementation, this would enable special key input
        // For this simplified version, we'll just note the setting
        (void)enable;
    }

    // Equivalent to nodelay()
    void nodelay(bool enable) {
        nodelay_mode = enable;
    }

    // Equivalent to cbreak()
    void cbreak() {
        // Disable line buffering
        setup_terminal();
    }

    // Equivalent to curs_set()
    void curs_set(int visibility) {
        cursor_visible = (visibility > 0);
        #ifdef _WIN32
            CONSOLE_CURSOR_INFO cursor_info;
            GetConsoleCursorInfo(console_handle, &cursor_info);
            cursor_info.bVisible = cursor_visible;
            SetConsoleCursorInfo(console_handle, &cursor_info);
        #else
            // On Unix, this would typically use terminal-specific escape sequences
        #endif
    }

    // Equivalent to getmaxyx() macro
    void getmaxyx(WINDOW* win, int& height, int& width) {
        if (win) {
            height = win->height;
            width = win->width;
        } else {
            height = 0;
            width = 0;
        }
    }

    // Equivalent to mvaddch()
    void mvaddch(int y, int x, char ch) {
        if (!current_window) return;
        if (y >= 0 && y < current_window->height && x >= 0 && x < current_window->width) {
            current_window->buffer[y][x] = ch;
        }
    }

    // Read a character (similar to getch())
    int getch() {
        #ifdef _WIN32
            return windows_getch();  // Use Windows API input method
        #else
            char ch;
            // Set up non-blocking input for Unix
            struct termios old_settings, new_settings;
            tcgetattr(STDIN_FILENO, &old_settings);
            new_settings = old_settings;
            new_settings.c_lflag &= ~(ICANON | ECHO);
            new_settings.c_cc[VMIN] = 0;
            new_settings.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);

            // Read character
            read(STDIN_FILENO, &ch, 1);

            // Restore terminal settings
            tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
            return ch;
        #endif
    }

    // Move cursor and print (similar to mvprintw())
    void mvprintw(int y, int x, const char* format, ...) {
        if (!current_window) return;
        if (y < 0 || y >= current_window->height || x < 0 || x >= current_window->width) 
            return;

        // Prepare variadic arguments
        va_list args;
        va_start(args, format);
        
        // Format the string
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        // Copy to screen buffer
        std::string str(buffer);
        for (size_t i = 0; i < str.length() && x + i < current_window->width; ++i) {
            current_window->buffer[y][x + i] = str[i];
        }
    }

    // Refresh screen
    void refresh() {
        if (!current_window) return;

        // Clear console
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        // Print screen buffer
        for (const auto& row : current_window->buffer) {
            for (char ch : row) {
                std::cout << ch;
            }
            std::cout << std::endl;
        }
    }

    // Constructor
    TerminalUI() : 
        current_window(nullptr),
        is_initialized(false), 
        echo_mode(true), 
        nodelay_mode(false), 
        cursor_visible(true) {
        // Constructor can be empty or do minimal setup
    }

    // Destructor to ensure cleanup
    ~TerminalUI() {
        if (is_initialized) {
            endwin();
        }
    }
};

#endif // CROSS_PLATFORM_TERMINAL_UI_H