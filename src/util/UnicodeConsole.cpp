#include "util/UnicodeConsole.h"
#include <stdio.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

namespace cgt::util
{
    void PrepareUnicodeConsole()
    {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);
        _setmode(_fileno(stdin), _O_U16TEXT);
#endif
    }
}
