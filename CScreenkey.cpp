#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <set>
#include <ncurses.h>  // ncurses for lightweight terminal-based UI
#include <cstdlib>    // for system()

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <X11/Xlib.h>
    #include <X11/XKBlib.h>
    #include <X11/keysym.h>
    #include <X11/extensions/XInput2.h>
#endif

std::mutex output_mutex;
std::set<std::string> activeKeys;
std::map<int, std::string> specialKeyMap;
bool quit = false;
bool updated = false;

void initNcurses() {
    initscr();  // Initialize the ncurses screen
    cbreak();   // Disable line buffering
    noecho();   // Disable echoing of typed characters
    curs_set(0);  // Hide the cursor
    timeout(0);  // Non-blocking input
    start_color();  // Enable colors if the terminal supports it
    init_pair(1, COLOR_WHITE, COLOR_BLACK);  // Define text color

#ifdef __linux__
    // Command to make the terminal always on top using wmctrl (Linux only)
    system("wmctrl -r :ACTIVE: -b add,above");
#elif _WIN32
    // Make console window always on top in Windows
    SetWindowPos(GetConsoleWindow(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#endif
}

void renderText(const std::string& inputText) {
    std::lock_guard<std::mutex> lock(output_mutex);

    clear();  // Clear the screen
    attron(COLOR_PAIR(1));  // Set the color

    int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);  // Get the terminal size

    // Calculate the centered position for the text
    int text_length = inputText.length();
    int start_x = (term_width - text_length) / 2;
    int start_y = term_height / 2;

    // Print the text at the center of the screen
    mvprintw(start_y, start_x, inputText.c_str());

    refresh();  // Refresh the screen to show changes
}

void initializeKeyMappings() {
    specialKeyMap[XK_Page_Up] = "PAGE UP";        // PageUp key
    specialKeyMap[XK_Page_Down] = "PAGE DOWN";    // PageDown key
    specialKeyMap[XK_Home] = "HOME";              // Home key
    specialKeyMap[XK_End] = "END";                // End key
    specialKeyMap[1] = "MOUSE LEFT CLICK";
    specialKeyMap[2] = "MOUSE MIDDLE CLICK";  // Fixed middle-click support
    specialKeyMap[3] = "MOUSE RIGHT CLICK";
    specialKeyMap[4] = "MOUSE SCROLL UP";
    specialKeyMap[5] = "MOUSE SCROLL DOWN";
}

void showPressedKey(const std::string &combination) {
    std::string uppercase_combination;
    for (char c : combination) {
        uppercase_combination += std::toupper(c);
    }
    renderText(uppercase_combination);  // Display the keypress or mouse event
}

void updateKeyCombination() {
    if (!activeKeys.empty()) {
        std::string combination;
        for (const auto& key : activeKeys) {
            if (!combination.empty()) {
                combination += " + ";
            }
            combination += key;
        }
        showPressedKey(combination);
    }
    updated = true;
}

void addSpecialKey(const std::string &keyStr) {
    activeKeys.insert(keyStr);
    updateKeyCombination();
}

void removeSpecialKey(const std::string &keyStr) {
    activeKeys.erase(keyStr);
    updateKeyCombination();
}

void clearActiveKeys() {
    activeKeys.clear();
}

#ifdef _WIN32
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *kbdStruct = (KBDLLHOOKSTRUCT *)lParam;
        char key[16];
        GetKeyNameTextA((LONG)(kbdStruct->scanCode << 16), key, sizeof(key));
        std::string keyStr(key);

        if (wParam == WM_KEYDOWN) {
            activeKeys.insert(keyStr);
            updateKeyCombination();
        } else if (wParam == WM_KEYUP) {
            activeKeys.erase(keyStr);
            updateKeyCombination();
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT *mouseStruct = (MSLLHOOKSTRUCT *)lParam;
        std::string buttonStr;

        if (wParam == WM_LBUTTONDOWN) {
            buttonStr = "MOUSE LEFT CLICK";
        } else if (wParam == WM_MBUTTONDOWN) {  // Added middle mouse button support
            buttonStr = "MOUSE MIDDLE CLICK";
        } else if (wParam == WM_RBUTTONDOWN) {
            buttonStr = "MOUSE RIGHT CLICK";
        } else if (wParam == WM_MOUSEWHEEL) {
            buttonStr = (GET_WHEEL_DELTA_WPARAM(mouseStruct->mouseData) > 0) ? "MOUSE SCROLL UP" : "MOUSE SCROLL DOWN";
        }

        if (!buttonStr.empty()) {
            activeKeys.insert(buttonStr);
            updateKeyCombination();
        }

        if (wParam == WM_LBUTTONUP || wParam == WM_MBUTTONUP || wParam == WM_RBUTTONUP) {
            activeKeys.erase(buttonStr);
            updateKeyCombination();
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void startWindowsScreenKey() {
    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, nullptr, 0);

    MSG msg;
    while (!quit && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
}
#endif

#ifdef __linux__
Display *display;

void handleLinuxKeyPress(XIDeviceEvent *xide) {
    KeySym keysym = XkbKeycodeToKeysym(display, xide->detail, 0, 0);
    std::string keyStr;

    // Check for special key mapping
    if (specialKeyMap.count(keysym)) {
        keyStr = specialKeyMap[keysym];
    } else {
        keyStr = XKeysymToString(keysym);
    }

    if (!keyStr.empty()) {
        activeKeys.insert(keyStr);
        updateKeyCombination();
    }
}

void handleLinuxKeyRelease(XIDeviceEvent *xide) {
    KeySym keysym = XkbKeycodeToKeysym(display, xide->detail, 0, 0);
    std::string keyStr;

    if (specialKeyMap.count(keysym)) {
        keyStr = specialKeyMap[keysym];
    } else {
        keyStr = XKeysymToString(keysym);
    }

    if (!keyStr.empty()) {
        activeKeys.erase(keyStr);
        updateKeyCombination();
    }
}

void handleLinuxButtonPress(XIDeviceEvent *xide) {
    std::string buttonStr;

    if (specialKeyMap.count(xide->detail)) {
        buttonStr = specialKeyMap[xide->detail];
    } else {
        buttonStr = "Unknown Mouse Button";
    }

    if (!buttonStr.empty()) {
        activeKeys.insert(buttonStr);
        updateKeyCombination();
    }
}

void handleLinuxButtonRelease(XIDeviceEvent *xide) {
    std::string buttonStr;

    if (specialKeyMap.count(xide->detail)) {
        buttonStr = specialKeyMap[xide->detail];
    } else {
        buttonStr = "Unknown Mouse Button";
    }

    if (!buttonStr.empty()) {
        activeKeys.erase(buttonStr);
        updateKeyCombination();
    }
}

void startLinuxScreenKey() {
    display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Cannot open X display" << std::endl;
        return;
    }

    initializeKeyMappings();

    int opcode, event, error;
    if (!XQueryExtension(display, "XInputExtension", &opcode, &event, &error)) {
        std::cerr << "X Input extension not available" << std::endl;
        XCloseDisplay(display);
        return;
    }

    Window root = DefaultRootWindow(display);

    XIEventMask evmask;
    unsigned char mask[(XI_LASTEVENT + 7) / 8] = {0};
    evmask.deviceid = XIAllDevices;
    evmask.mask_len = sizeof(mask);
    evmask.mask = mask;

    XISetMask(mask, XI_KeyPress);
    XISetMask(mask, XI_KeyRelease);
    XISetMask(mask, XI_ButtonPress);
    XISetMask(mask, XI_ButtonRelease);

    XISelectEvents(display, root, &evmask, 1);

    while (!quit) {
        XEvent event;
        XNextEvent(display, &event);

        if (event.xcookie.type == GenericEvent && event.xcookie.extension == opcode) {
            XGetEventData(display, &event.xcookie);
            XIDeviceEvent *xide = (XIDeviceEvent *)event.xcookie.data;

            if (event.xcookie.evtype == XI_KeyPress) {
                handleLinuxKeyPress(xide);
            } else if (event.xcookie.evtype == XI_KeyRelease) {
                handleLinuxKeyRelease(xide);
            } else if (event.xcookie.evtype == XI_ButtonPress) {
                handleLinuxButtonPress(xide);
            } else if (event.xcookie.evtype == XI_ButtonRelease) {
                handleLinuxButtonRelease(xide);
            }
            XFreeEventData(display, &event.xcookie);
        }
    }

    XCloseDisplay(display);
}
#endif

int main() {
    initNcurses();  // Initialize ncurses

#ifdef _WIN32
    std::thread screenKeyThread(startWindowsScreenKey);
#elif __linux__
    std::thread screenKeyThread(startLinuxScreenKey);
#endif

    while (!quit) {
        int ch = getch();  // Get user input
        if (ch == 'q') {
            quit = true;  // Press 'q' to quit the program
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (screenKeyThread.joinable()) {
        screenKeyThread.join();
    }

    endwin();  // End ncurses mode
    return 0;
}
