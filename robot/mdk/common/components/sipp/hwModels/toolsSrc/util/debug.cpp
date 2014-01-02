#include "util/debug.h"

#if defined(_DEBUG) && !defined(NDEBUG)

#include <stdlib.h>

#if defined (_WIN32) && defined (_CRTDBG_MAP_ALLOC)
#include <crtdbg.h>
#endif

#if defined (_WIN32)
#include <windows.h>
#endif

#if defined (_WIN32) && defined (_CRTDBG_MAP_ALLOC)

namespace util {
namespace debug {

bool memory_leak_detection_init_()
{
    static bool skip = false;
    if (skip)
    {
        return false;
    }
    OutputDebugStringA("Enabling memory leak detection\n");
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
    skip = true;
    return true;
}

}} // namespace util::debug

#endif

#include <string>
#include <signal.h>
#include <sstream>

namespace util {
namespace debug {

using std::string;
using std::ostringstream;
using std::endl;

bool break_dlg(const char * message, const char *file, int line)
{
    ostringstream msg;
    msg << "Debug break encountered" << endl << endl;
    if (file)
    {
        msg << "In file: " << endl << file;
        if (line > 0) 
        {
            msg << ":" << line;
        }
        msg << endl << endl;
    }
    if (message && *message)
    {
        msg << message << endl<< endl;
    }
    msg << "(Press Retry to start debugging the application)";
#ifdef WIN32
    switch (MessageBoxA(NULL, msg.str().c_str(), NULL, MB_ICONERROR | MB_ABORTRETRYIGNORE)) 
    {
    case SIGABRT:
        raise(SIGABRT);
        return false;
    case IDRETRY:
        return true;
        break;
    case IDIGNORE:
    default:
        return false;
    }
#else
    string args = msg.str();
    size_t pos = 0;
    while (pos < args.size())
    {
        if (args[pos] == '\'')
        {
            args.replace(pos, 1, "\'\\\'\'", 4);
            pos += 4;
        }
        else
        {
            ++pos;
        }
    }
    // returns 1 on any error
    string command = "xmessage -center -buttons Abort:3,Retry:4,Ignore:5 '" + args + "' 2>/dev/null";
    int result = system(command.c_str());
    // fprintf(stderr, "%s\nreturned %d\n", command.c_str(), WEXITSTATUS(result));
    switch (WEXITSTATUS(result))
    {
    case 3: // Abort
        raise(SIGABRT);
        return false;
    case 4: // Retry
        raise(SIGTRAP);
        return true;
    case 0: // Ignore
    default:
        return false;
    }
#endif
}

} // namespace debug
} // namespace util

#endif
