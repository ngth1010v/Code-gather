#include "ui/terminal/Platform.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <poll.h>
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <unistd.h>
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
    }

    bool PlatformInit()
    {
#ifdef _WIN32
        auto& c = Ctx();
        if (c.initialized)
        {
            return true;
        }

        c.in = GetStdHandle(STD_INPUT_HANDLE);
        c.out = GetStdHandle(STD_OUTPUT_HANDLE);
        if (c.in == INVALID_HANDLE_VALUE || c.out == INVALID_HANDLE_VALUE)
        {
            return false;
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
        return WriteFile(c.out, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) != 0 && written == bytes.size();
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
}
