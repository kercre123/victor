#ifndef ANKI_CORETECH_EXCEPTIONS_
#define ANKI_CORETECH_EXCEPTIONS_

#include <stdexcept>
#include <string>
#include <stdio.h>
#include <sstream>

#define CORETECH_ASSERT(x) assert(x);
#define CORETECH_THROW(msg) throw CoreTechException(msg, __FILE__, __LINE__);
#define CORETECH_THROW_IF(x) {if(x) {throw CoreTechException(#x, __FILE__, __LINE__);} else {}}

using namespace std;

namespace Anki {

class CoreTechException : public runtime_error
{
  string msg;
public:
  CoreTechException(const string& arg, const char* file, int line) : runtime_error(arg)
  {
    std::ostringstream o;
    o << file << ":" << line << ": " << arg;
    msg = o.str();
  };
  CoreTechException(const char* msg, const char* file, int line) : CoreTechException(string(msg),file,line) {};
  
  ~CoreTechException() throw() {}
  const char *what() const throw() {
    return msg.c_str();
  }
};

  
}

#endif // ANKI_CORETECH_EXCEPTIONS_