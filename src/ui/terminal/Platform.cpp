#include "ui/terminal/Platform.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <poll.h>
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <unistd.h>
#endif

#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
#endif

namespace cgt::ui::teminal
{
    namespace
    {
#ifdef _WIN32
        struct WinContext
        {
            HANDLE in{INVALID_HANDLE_VALUE};
            HANDLE out{INVALID_HANDLE_VALUE};
            DWORD old_in_mode{0};
            DWORD old_out_mode{0};
            UINT old_in_cp{0};
            UINT old_out_cp{0};
            bool initialized{false};
        };

        WinContext& Ctx()
        {
            static WinContext ctx{};
            return ctx;
        }
#else
        struct PosixContext
        {
            int in_fd{-1};
            int out_fd{-1};
            struct termios old_termios{};
            bool has_old_termios{false};
            int old_flags{-1};
            bool initialized{false};
        };

        PosixContext& Ctx()
        {
            static PosixContext ctx{};
            return ctx;
        }
#endif

    bool ParseAnsiColorResponse(const std::string& resp, RGB& color) {
            std::size_t idx = resp.find("rgb:");
            if (idx == std::string::npos) return false;
            
            idx += 4; // Skip past "rgb:"
            unsigned int r = 0, g = 0, b = 0;
            
            // Terminal responses can use 4, 8, 12, or 16 bits per channel (e.g., rgb:1a1a/2b2b/3c3c)
            // We read them dynamically and scale them to a standard 8-bit scale [0-255]
            char divider;
            std::string_view sv(resp.data() + idx, resp.size() - idx);
            std::stringstream ss;
            ss << sv;
            
            std::string r_str, g_str, b_str;
            if (std::getline(ss, r_str, '/') && std::getline(ss, g_str, '/') && std::getline(ss, b_str, '\x07')) {
                // Strip trailing structural bytes if necessary
                if (!b_str.empty() && (b_str.back() == '\x1b' || b_str.back() == '\\')) {
                    b_str.pop_back();
                }
                
                try {
                    unsigned long r_val = std::stoul(r_str, nullptr, 16);
                    unsigned long g_val = std::stoul(g_str, nullptr, 16);
                    unsigned long b_val = std::stoul(b_str, nullptr, 16);
                    
                    // Downscale values down to 8 bits mapping matching terminal resolution format
                    color.r = static_cast<std::uint8_t>(r_val >> (4 * (r_str.size() - 2)));
                    color.g = static_cast<std::uint8_t>(g_val >> (4 * (g_str.size() - 2)));
                    color.b = static_cast<std::uint8_t>(b_val >> (4 * (b_str.size() - 2)));
                    return true;
                } catch (...) {
                    return false;
                }
            }
            return false;
        }
    }

    bool PlatformInit()
    {
#ifdef _WIN32
        auto& c = Ctx();
        if (c.initialized)
        {
            return true;
        }

        // Fix: Explicitly open CONIN$ and CONOUT$ to bypass ConPTY proxy limitations inside VS Code
        c.in = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        c.out = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

        if (c.in == INVALID_HANDLE_VALUE || c.out == INVALID_HANDLE_VALUE)
        {
            // Fallback to GetStdHandle if direct console files are completely inaccessible
            c.in = GetStdHandle(STD_INPUT_HANDLE);
            c.out = GetStdHandle(STD_OUTPUT_HANDLE);
            if (c.in == INVALID_HANDLE_VALUE || c.out == INVALID_HANDLE_VALUE)
            {
                return false;
            }
        }

        if (!GetConsoleMode(c.in, &c.old_in_mode))
        {
            return false;
        }
        if (!GetConsoleMode(c.out, &c.old_out_mode))
        {
            return false;
        }

        c.old_in_cp = GetConsoleCP();
        c.old_out_cp = GetConsoleOutputCP();

        DWORD in_mode = c.old_in_mode;
        in_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE);
        in_mode |= ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
        if (!SetConsoleMode(c.in, in_mode))
        {
            return false;
        }

        DWORD out_mode = c.old_out_mode;
        // ENABLE_VIRTUAL_TERMINAL_PROCESSING tells VS Code's terminal to parse your ANSI sequences instead of printing raw text
        out_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
        if (!SetConsoleMode(c.out, out_mode))
        {
            SetConsoleMode(c.in, c.old_in_mode);
            return false;
        }

        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
        c.initialized = true;
        return true;
#else
        auto& c = Ctx();
        if (c.initialized)
        {
            return true;
        }

        c.in_fd = STDIN_FILENO;
        c.out_fd = STDOUT_FILENO;
        if (!isatty(c.in_fd) || !isatty(c.out_fd))
        {
            return false;
        }

        if (tcgetattr(c.in_fd, &c.old_termios) == 0)
        {
            c.has_old_termios = true;
            struct termios raw = c.old_termios;
            raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | IEXTEN | ISIG));
            raw.c_iflag &= static_cast<tcflag_t>(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
            raw.c_cflag |= static_cast<tcflag_t>(CS8);
            raw.c_oflag &= static_cast<tcflag_t>(~(OPOST));
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(c.in_fd, TCSANOW, &raw) != 0)
            {
                return false;
            }
        }

        c.old_flags = fcntl(c.in_fd, F_GETFL, 0);
        if (c.old_flags >= 0)
        {
            if (fcntl(c.in_fd, F_SETFL, c.old_flags | O_NONBLOCK) != 0)
            {
                if (c.has_old_termios)
                {
                    tcsetattr(c.in_fd, TCSANOW, &c.old_termios);
                }
                return false;
            }
        }

        c.initialized = true;
        return true;
#endif
    }

    void PlatformDestroy()
    {
#ifdef _WIN32
        auto& c = Ctx();
        if (!c.initialized)
        {
            return;
        }

        SetConsoleMode(c.in, c.old_in_mode);
        SetConsoleMode(c.out, c.old_out_mode);
        SetConsoleCP(c.old_in_cp);
        SetConsoleOutputCP(c.old_out_cp);

        // Safely close the opened file descriptors if they were created via CreateFileA
        if (c.in != INVALID_HANDLE_VALUE && c.in != GetStdHandle(STD_INPUT_HANDLE)) CloseHandle(c.in);
        if (c.out != INVALID_HANDLE_VALUE && c.out != GetStdHandle(STD_OUTPUT_HANDLE)) CloseHandle(c.out);

        c = WinContext{};
#else
        auto& c = Ctx();
        if (!c.initialized)
        {
            return;
        }

        if (c.has_old_termios)
        {
            tcsetattr(c.in_fd, TCSANOW, &c.old_termios);
        }
        if (c.old_flags >= 0)
        {
            fcntl(c.in_fd, F_SETFL, c.old_flags);
        }
        c = PosixContext{};
#endif
    }

    bool PlatformWrite(std::string_view bytes)
    {
#ifdef _WIN32
        auto& c = Ctx();
        if (!c.initialized)
        {
            return false;
        }

        DWORD written = 0;
        bool success = WriteFile(c.out, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) != 0 && written == bytes.size();
        std::fflush(stdout);
        return success;
#else
        auto& c = Ctx();
        if (!c.initialized)
        {
            return false;
        }

        const char* p = bytes.data();
        std::size_t left = bytes.size();
        while (left > 0)
        {
            const ssize_t n = ::write(c.out_fd, p, left);
            if (n < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                return false;
            }
            p += n;
            left -= static_cast<std::size_t>(n);
        }
        return true;
#endif
    }

    bool PlatformGetSize(WindowSize& size)
    {
#ifdef _WIN32
        auto& c = Ctx();
        if (!c.initialized)
        {
            return false;
        }

        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(c.out, &info))
        {
            return false;
        }

        size.w = static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
        size.h = static_cast<int>(info.srWindow.Bottom - info.srWindow.Top + 1);
        return true;
#else
        struct winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0)
        {
            if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0)
            {
                return false;
            }
        }

        size.w = static_cast<int>(ws.ws_col);
        size.h = static_cast<int>(ws.ws_row);
        return true;
#endif
    }

    bool PlatformGetCursorPos(CursorPos& pos)
    {
#ifdef _WIN32
        auto& c = Ctx();
        if (!c.initialized)
        {
            return false;
        }

        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(c.out, &info))
        {
            return false;
        }

        pos.x = static_cast<int>(info.dwCursorPosition.X);
        pos.y = static_cast<int>(info.dwCursorPosition.Y);
        return true;
#else
        auto& c = Ctx();
        if (!c.initialized)
        {
            return false;
        }

        const char seq[] = "\x1b[6n";
        if (!PlatformWrite(seq))
        {
            return false;
        }
        ::tcdrain(c.out_fd);

        std::string response;
        response.reserve(32);

        struct pollfd pfd{};
        pfd.fd = c.in_fd;
        pfd.events = POLLIN;

        for (int attempt = 0; attempt < 20; ++attempt)
        {
            const int pr = ::poll(&pfd, 1, 10);
            if (pr <= 0)
            {
                continue;
            }

            char buf[32];
            const ssize_t n = ::read(c.in_fd, buf, sizeof(buf));
            if (n <= 0)
            {
                continue;
            }
            response.append(buf, buf + n);

            const std::size_t end = response.find('R');
            if (end != std::string::npos)
            {
                const std::size_t esc = response.find("[", 0);
                if (esc == std::string::npos)
                {
                    return false;
                }

                int row = 0;
                int col = 0;
                if (std::sscanf(response.c_str() + esc + 1, "%d;%d", &row, &col) == 2)
                {
                    pos.x = col - 1;
                    pos.y = row - 1;
                    return true;
                }
                return false;
            }
        }
        return false;
#endif
    }

#ifndef _WIN32
    bool PlatformReadBytes(std::string& bytes)
    {
        auto& c = Ctx();
        if (!c.initialized)
        {
            return false;
        }

        bytes.clear();
        char buf[512];
        for (;;)
        {
            const ssize_t n = ::read(c.in_fd, buf, sizeof(buf));
            if (n > 0)
            {
                bytes.append(buf, buf + n);
                continue;
            }

            if (n < 0 && (errno == EINTR))
            {
                continue;
            }

            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                break;
            }

            break;
        }
        return true;
    }
#endif
bool PlatformGetOriginalColors(RGB& fg, RGB& bg)
    {
        // 1. Fire off the theme query sequences to stdout
        const char query_seq[] = "\x1b]10;?\x07\x1b]11;?\x07";
        if (!PlatformWrite(query_seq))
        {
            return false;
        }

        // Force downstream pipeline synchronization
#ifdef _WIN32
        auto& c = Ctx();
        std::fflush(stdout);
        
        // Temporarily change STDIN to binary/raw mode if it's acting as a stream pipe
        int old_stdin_mode = _setmode(_fileno(stdin), _O_BINARY);
#else
        ::tcdrain(Ctx().out_fd);
#endif

        std::string buffer;
        buffer.reserve(128);
        
        bool fg_found = false;
        bool bg_found = false;

        // Query polling window loop
        for (int attempt = 0; attempt < 40; ++attempt)
        {
#ifdef _WIN32
            // FIX: Try reading from pipe/stream first (Handles VS Code / ConPTY environments)
            DWORD bytes_avail = 0;
            HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
            
            // Check if there is data pending on the standard input handle pipeline
            if (PeekNamedPipe(h_stdin, nullptr, 0, nullptr, &bytes_avail, nullptr) && bytes_avail > 0) {
                std::string pipe_buf(bytes_avail, '\0');
                DWORD bytes_read = 0;
                if (ReadFile(h_stdin, &pipe_buf[0], bytes_avail, &bytes_read, nullptr) && bytes_read > 0) {
                    buffer.append(pipe_buf.c_str(), bytes_read);
                }
            } 
            else {
                // Fallback: Read from native Win32 Console Input events array (Handles raw Windows Terminal / cmd)
                DWORD available_events = 0;
                if (GetNumberOfConsoleInputEvents(c.in, &available_events) && available_events > 0) {
                    INPUT_RECORD records[64];
                    DWORD read_count = 0;
                    if (ReadConsoleInputA(c.in, records, 64, &read_count)) {
                        for (DWORD i = 0; i < read_count; ++i) {
                            if (records[i].EventType == KEY_EVENT && records[i].Event.KeyEvent.bKeyDown) {
                                char ch = records[i].Event.KeyEvent.uChar.AsciiChar;
                                if (ch != 0) buffer.push_back(ch);
                            }
                        }
                    }
                }
            }
#else
            // POSIX remains perfectly happy with your raw bytes descriptor parser
            std::string chunks;
            if (PlatformReadBytes(chunks) && !chunks.empty()) {
                buffer.append(chunks);
            }
#endif

            // Check for the visual color patterns inside incoming data blocks
            if (!fg_found && buffer.find("\x1b]10;") != std::string::npos) {
                std::size_t fg_start = buffer.find("\x1b]10;");
                if (buffer.find('\x07', fg_start) != std::string::npos) {
                    fg_found = ParseAnsiColorResponse(buffer.substr(fg_start), fg);
                }
            }
            if (!bg_found && buffer.find("\x1b]11;") != std::string::npos) {
                std::size_t bg_start = buffer.find("\x1b]11;");
                if (buffer.find('\x07', bg_start) != std::string::npos) {
                     bg_found = ParseAnsiColorResponse(buffer.substr(bg_start), bg);
                }
            }

            // Break early the moment both components are fetched cleanly
            if (fg_found && bg_found) {
                break;
            }

#ifdef _WIN32
            Sleep(5);
#else
            ::usleep(5000);
#endif
        }

#ifdef _WIN32
        // Restore standard console input operational state mode properties
        if (old_stdin_mode != -1) {
            _setmode(_fileno(stdin), old_stdin_mode);
        }
#endif

        // If VS Code completely blocks query loops under a specific version, 
        // fall back directly to standard matching dark background colors instead of pure black {0,0,0}
        if (!fg_found) fg = RGB{235, 235, 235}; 
        if (!bg_found) bg = RGB{25, 25, 25};    // Matches default VS Code Dark Modern background value
        
        return true;
    }
}