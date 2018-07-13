#ifndef SYM_RESOLVE_H
#define SYM_RESOLVE_H
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>
#include <list>
#include <cmath>
#include <sys/stat.h>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <cassert>
#include <errno.h>
#include <cstdarg>

#ifndef iggetc
# if defined getc_unlocked || defined __GLIBC__
#  define iggetc(x) getc_unlocked(x)
# else
#  define iggetc(x) fgetc(x)
# endif
#endif

/** Gets a token delimited by @a separator from the @a file and write it,
    0 terminated in the buffer found in @a buffer.

    Notice that if the token is larger than @a maxSize, the buffer is
    reallocated and @a maxSize is updated to the new size.

    The trailing separator after a token is not put in the token and is left
    in the buffer. If @a nextChar is not 0, the delimiter is put there.

    @a in the file to be read.

    @a buffer a pointer to the buffer where to put the tokens. The buffer will
     be redimensioned accordingly, if the token is larger of the buffer.

    @a maxSize, a pointer to the size of the buffer. Notice that in case the
     buffer is reallocated to have more space, maxSize is updated with the new
     size.

    @a firstChar

    @return the size of the token.
  */
size_t
fgettoken(FILE *in, char **buffer, size_t *maxSize, const char *separators,
          int *firstChar)
{
  // if the passed first character is EOF or a separator,
  // return an empty otherwise use it as first character
  // of the buffer.
  if (*firstChar == EOF
      || (int) separators[0] == *firstChar
      || strchr(separators + 1, *firstChar))
  {
    (*buffer)[0] = 0;
    return 0;
  }
  else
    (*buffer)[0] = *firstChar;

  size_t i = 1;
  while (true)
  {
    if (i >= *maxSize)
    {
      *maxSize += 1024;
      *buffer = (char*) realloc(*buffer, *maxSize);
      if (!*buffer)
      {
        fprintf(stderr, "Token too long. Not enough memory.");
        exit(1);
      }
    }

    int c = iggetc(in);

    if (c == EOF)
    {
      if (ferror(in))
      {
        fprintf(stderr, "Error while reading file.");
        exit(1);
      }
      else if (feof(in))
      {
        fprintf(stderr, "Premature end of file.");
        exit(1);
      }
      assert(false);
    }

    if (separators[0] == c || strchr(separators + 1, c))
    {
      (*buffer)[i] = 0;
      *firstChar = c;
      return i;
    }

    (*buffer)[i++] = c;
  }
}

/**Skip all the characters contained in @a skipped*/
void
skipchars(FILE *in, const char *skipped, int *nextChar)
{
  while (strchr(skipped, *nextChar))
    *nextChar = iggetc(in);
}

class FileInfo
{
public:
  typedef uint64_t Offset;
private:
  struct CacheItem {
    CacheItem(Offset offset, const std::string &name)
      :OFFSET(offset), NAME(name) {};
    Offset OFFSET;
    std::string NAME;
  };

  typedef std::vector<CacheItem> SymbolCache;

  struct CacheItemComparator {
    bool operator()(const CacheItem& a,
                    const Offset &b) const
      { return a.OFFSET < b; }

    bool operator()(const Offset &a,
                    const CacheItem &b) const
      { return a < b.OFFSET; }
  };
public:
  std::string NAME;
  FileInfo(void)
    : NAME("<dynamically generated>"),
      m_useGdb(false)
    {}
  FileInfo(const std::string &name, bool useGdb)
    : NAME(name),
      m_useGdb(useGdb)
    {
      if (useGdb)
        this->createOffsetMap();
    }

  /** Resolves a symbol by looking up the offset of
      its code in the symbol map for this file.
    */
  const char *symbolByOffset(Offset offset)
    {
      if (m_symbolCache.empty())
        return 0;

      SymbolCache::iterator i = lower_bound(m_symbolCache.begin(),
                                            m_symbolCache.end(),
                                            offset, CacheItemComparator());
      if (i->OFFSET == offset)
        return i->NAME.c_str();

      if (i == m_symbolCache.begin())
        return m_symbolCache.begin()->NAME.c_str();

      --i;

      return i->NAME.c_str();
    }

  Offset next(Offset offset)
    {
      SymbolCache::iterator i = upper_bound(m_symbolCache.begin(),
                                            m_symbolCache.end(),
                                            offset, CacheItemComparator());
      if (i == m_symbolCache.end())
        return 0;
      return i->OFFSET;
    }

  /** Return true if gdb can be used to better determine symbols inside
      files.
    */
  bool canUseGdb(void)
    {
      return m_useGdb;
    }

private:
  /** Creates a map of the offsets at which all the
      symbols in the file get loaded WRT the base address at which a given
      loadable object is going to be loaded.
    */
  void createOffsetMap(void)
    {
      // FIXME: On macosx we should really use otool
#ifndef __APPLE__
      char *commandLine = 0;
      asprintf(&commandLine, "objdump -p %s", NAME.c_str());
      FILE *pipe = popen(commandLine, "r");
      free(commandLine);

      if (!pipe || ferror(pipe))
      {
        fprintf(stderr, "Error while invoking objdump");
        exit(1);
      }
      setvbuf(pipe, 0, _IOFBF, 128*1024);

      Offset vmbase = 0;
      bool matched = false;
      size_t bufferSize = 1024;
      char *buffer = (char*) malloc(bufferSize);
      int nextChar = iggetc(pipe);

      while (nextChar != EOF)
      {
        // Checks the following regexp
        //
        //    LOAD\\s+off\\s+(0x[0-9A-Fa-f]+)\\s+vaddr\\s+(0x[0-9A-Fa-f]+)
        //
        // and sets vmbase to be $2 - $1 of the first matched entry.
        skipchars(pipe, "\n\t ", &nextChar);
        fgettoken(pipe, &buffer, &bufferSize, "\n\t ", &nextChar);
        if (strncmp("LOAD", buffer, 4))
          continue;
        skipchars(pipe, "\n\t ", &nextChar);

        fgettoken(pipe, &buffer, &bufferSize, "\n\t ", &nextChar);
        if (strncmp("off", buffer, 3))
          continue;
        skipchars(pipe, "\n\t ", &nextChar);

        fgettoken(pipe, &buffer, &bufferSize, "\n\t ", &nextChar);
        char *endptr = 0;
        Offset initialBase = strtol(buffer, &endptr, 16);
        if (buffer == endptr)
          continue;
        skipchars(pipe, "\n\t ", &nextChar);

        fgettoken(pipe, &buffer, &bufferSize, "\n\t ", &nextChar);
        if (strncmp("vaddr", buffer, 5))
          continue;
        skipchars(pipe, "\n\t ", &nextChar);

        fgettoken(pipe, &buffer, &bufferSize, "\n\t ", &nextChar);
        Offset finalBase = strtol(buffer, &endptr, 16);
        if (buffer == endptr)
          continue;
        skipchars(pipe, "\n\t ", &nextChar);

        vmbase = finalBase - initialBase;
        matched = true;
        break;
      }

      pclose(pipe);

      if (!matched)
      {
        fprintf(stderr, "Cannot determine VM base address for %s\n"
                        "Error while running \"objdump -p %s\"\n", NAME.c_str(), NAME.c_str());
        exit(1);
      }

      asprintf(&commandLine, "nm -t d -n %s", NAME.c_str());
      pipe = popen(commandLine, "r");
      free(commandLine);
      if (!pipe)
        return;

      if (ferror(pipe))
      {
	pclose(pipe);
	return;
      }

      setvbuf(pipe, 0, _IOFBF, 128*1024);
      nextChar = iggetc(pipe);
      while (nextChar != EOF)
      {
        skipchars(pipe, "\n\t ", &nextChar);
        // If line does not match "^(\\d+)[ ]\\S[ ](\S+)$", exit.
        fgettoken(pipe, &buffer, &bufferSize, "\n\t ", &nextChar);
        char *endptr = 0;
        Offset address = strtoll(buffer, &endptr, 10);
        if (buffer == endptr)
          continue;
        skipchars(pipe, "\t\n ", &nextChar);

        fgettoken(pipe, &buffer, &bufferSize, "\n\t ", &nextChar);
        if (buffer[1] != 0)
          continue;
        skipchars(pipe, "\t\n ", &nextChar);

        fgettoken(pipe, &buffer, &bufferSize, "\n\t ", &nextChar);
        // If line starts with '.' forget about it.
        if (buffer[0] == '.')
          continue;
        skipchars(pipe, "\t\n ", &nextChar);

        // Create a new symbol with the given fileoffset.
        // The symbol is automatically saved in the FileInfo cache by offset.
        // If a symbol with the same offset is already there, the new one
        // replaces the old one.
        Offset offset = address - vmbase;
        if (m_symbolCache.size() && (m_symbolCache.back().OFFSET == offset))
          m_symbolCache.back().NAME = buffer;
        else
          m_symbolCache.push_back(CacheItem(address-vmbase, buffer));
      }
      pclose(pipe);
#endif /* __APPLE__ */
    }
  bool        m_useGdb;
  SymbolCache m_symbolCache;
};

#endif // SYM_RESOLVE_H
