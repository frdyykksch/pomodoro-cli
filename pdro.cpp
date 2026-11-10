#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <string>
#include <array>

// suppose only working on windows currently
#define NOMINMAX
#include <windows.h> 
#pragma comment(lib, "winmm.lib")

// reduces flickering fo clear screen https://stackoverflow.com/questions/34842526/update-console-without-flickering-c 
void cls() {
    // Get the Win32 handle representing standard output.
    // This generally only has to be done once, so we make it static.
    static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD topLeft = { 0, 0 };

    // std::cout uses a buffer to batch writes to the underlying console.
    // We need to flush that to the console because we're circumventing
    // std::cout entirely; after we clear the console, we don't want
    // stale buffered text to randomly be written out.
    std::cout.flush();

    // Figure out the current width and height of the console window
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        // TODO: Handle failure!
        abort();
    }   
    DWORD length = csbi.dwSize.X * csbi.dwSize.Y;
    
    DWORD written;

    // Flood-fill the console with spaces to clear it
    FillConsoleOutputCharacter(hOut, TEXT(' '), length, topLeft, &written);

    // Reset the attributes of every character to the default.
    // This clears all background colour formatting, if any.
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, length, topLeft, &written);

    // Move the cursor back to the top left for the next sequence of writes
    SetConsoleCursorPosition(hOut, topLeft);
}
#define WORK  "\x1b[0;37m"
#define BREAK "\x1b[0;33m"
#define DONE  "\x1b[0;35m"
#define RESET "\x1b[0m"
/*
 * rendering
 */

// https://stackoverflow.com/questions/23369503/get-size-of-terminal-window-rows-columns
std::pair<int, int> consoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width  = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
    int height = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    return {width, height};
}

using Glyph = std::array<std::string, 5>;
Glyph render(char character) {
    switch (character) {
        case '0': return {
            R"(   ____ )",
            R"(  / __ \)",
            R"( / / / /)",
            R"(/ /_/ / )",
            R"(\____/  )"
        };
        case '1': return {
            R"(   ___)",
            R"(  <  /)",
            R"(  / / )",
            R"( / /  )",
            R"(/_/   )"
        };
        case '2': return {
            R"(   ___ )",
            R"(  |__ \)",
            R"(  __/ /)",
            R"( / __/ )",
            R"(/____/ )"
        };
        case '3': return {
            R"(   _____)",
            R"(  |__  /)",
            R"(   /_ < )",
            R"( ___/ / )",
            R"(/____/  )"
        };
        case '4': return {
            R"(   __ __)",
            R"(  / // /)",
            R"( / // /_)",
            R"(/__  __/)",
            R"(  /_/   )"
        };
        case '5': return {
            R"(    ______)",
            R"(   / ____/)",
            R"(  /___ \  )",
            R"( ____/ /  )",
            R"(/_____/   )"
        };
        case '6': return {
            R"(   _____)",
            R"(  / ___/)",
            R"( / __ \ )",
            R"(/ /_/ / )",
            R"(\____/  )"
        };
        case '7': return {
            R"( _____)",
            R"(/__  /)",
            R"(  / / )",
            R"( / /  )",
            R"(/_/   )"
        };
        case '8': return {
            R"(   ____ )",
            R"(  ( __ ))",
            R"( / __  |)",
            R"(/ /_/ / )",
            R"(\____/  )"
        };
        case '9': return {
            R"(   ____ )",
            R"(  / __ \)",
            R"( / /_/ /)",
            R"( \__, / )",
            R"(/____/  )"
        };
        case ':': return {
            R"(     )",
            R"(   _ )",
            R"(  (_))",
            R"( _   )",
            R"((_)  )"
        };

        default: return {"", "", "", "", ""};
    }
}

std::string renderTime(int time) {
    std::string text = std::format("{:02}:{:02}", time / 60, time % 60);
    auto [width, height] = consoleSize();

    std::string out;
    out.append(std::max(0, (height - 5) / 2), '\n');

    for(int row = 0; row < 5; ++row) {
        std::string line;
        for(char c : text) line += render(c)[row];

        int pad = std::max(0, (width - (int)line.size()) / 2);
        out += std::string(pad, ' ') + line + '\n';
    } return out;
}

struct Pomodoro {
    public:
    int workDuration;
    int breakDuration;
    int workTime; // seconds
    int breakTime; // seconds

    Pomodoro(int x, int y) : workDuration(x*60), breakDuration(y*60), workTime(x*60), breakTime(y*60) {} // default 25 minutes work 5 minutes break

    void display() {
        while(true) {
            // system(getOsName() == "Windows" ? "cls" : "clear");
            cls();
            if(workTime > 0) {
                std::cout << WORK << renderTime(workTime) << std::flush;
                --workTime;
                if(workTime == 0) PlaySound(TEXT("alarm.wav"), NULL, SND_FILENAME | SND_ASYNC);
            } else if(breakTime > 0) {
                std::cout << BREAK << renderTime(breakTime) << std::endl;
                --breakTime;
                if(breakTime == 0) PlaySound(TEXT("alarm2.wav"), NULL, SND_FILENAME | SND_ASYNC);
            } else {
                workTime = workDuration;
                breakTime = breakDuration;
                break;
            } std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

// win needs it to print colors
void enableAnsi() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

// ctrl c break https://learn.microsoft.com/en-us/windows/console/registering-a-control-handler-function
BOOL WINAPI ctrlHandler(DWORD type) {
    if(type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT) {
        const char reset[] = "\x1b[0m\n";
        DWORD written;
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), reset, sizeof(reset) - 1, &written, nullptr);
        return FALSE;
    } return FALSE;
}

int main(int argc, char *argv[]) {
    enableAnsi();
    if(argc == 1 || argc == 2) std::cout << "Usage: pdro [workTime in minutes] [breakTime in minutes]" << std::endl;
    int WORK_TIME = atoi(argv[1]); int BREAK_TIME = atoi(argv[2]);
    SetConsoleCtrlHandler(ctrlHandler, TRUE);
    Pomodoro pomodoro(WORK_TIME, BREAK_TIME);
    pomodoro.display();
    std::cout << "\x1b[0m]" << std::endl;
    return 0;
}