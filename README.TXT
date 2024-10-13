
# Compilation Guide for Linux and Windows

## Linux

### Prerequisites:
1. **g++ Compiler**: Make sure you have `g++` installed. You can install it using the following command:
   ```bash
   sudo apt update
   sudo apt install g++
   ```

2. **ncurses library**: Install the `libncurses-dev` library using the following command:
   ```bash
   sudo apt install libncurses-dev
   ```

3. **X11 and XInput2**: You will need `libx11-dev` and `libxi-dev` for X11 and XInput2 support:
   ```bash
   sudo apt install libx11-dev libxi-dev
   ```

### Compilation Command:
To compile the code, use the following command:
```bash
g++ CScreenkey.cpp -o screen_key -lncurses -lpthread -lX11 -lXi
```
Explanation:
- `-lncurses`: Links the ncurses library for terminal-based UI.
- `-lpthread`: Links the pthread library for threading.
- `-lX11`: Links the X11 library for Linux GUI functionality.
- `-lXi`: Links the XInput2 extension library.

## Windows

### Prerequisites:
1. **MinGW (Minimalist GNU for Windows)**: You need to install MinGW, which provides the `g++` compiler for Windows.

   Download MinGW from [MinGW official site](https://sourceforge.net/projects/mingw/), and make sure to install the C++ compiler.

2. **ncurses for Windows**: Since ncurses is primarily a Linux library, you can use the PDCurses library for Windows. Download it from the [PDCurses GitHub repository](https://github.com/wmcbrine/PDCurses).

3. **Pthreads for Windows**: Download pthreads for Windows from [sourceware.org](https://sourceware.org/pthreads-win32/).

4. **X11 Libraries**: X11 is not available on Windows, so you can either remove the X11-specific code or use a cross-platform library like `SDL2` for Windows.

### Compilation Command:
You can compile using MinGW with the following command:
```bash
g++ CScreenkey.cpp -o screen_key.exe -lpdcurses -lpthread
```
Explanation:
- `-lpdcurses`: Links PDCurses for terminal UI in Windows.
- `-lpthread`: Links the pthreads library for multithreading on Windows.

### Additional Notes:
- Ensure that you adjust the `#ifdef` sections in your code for cross-platform compatibility, particularly handling X11 functionality on Linux and using PDCurses and Windows APIs on Windows.
- Ensure all the necessary libraries are either in your path or linked correctly for successful compilation.
