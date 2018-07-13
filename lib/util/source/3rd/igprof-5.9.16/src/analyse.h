#ifndef ANALYSE_H
#define ANALYSE_H

#include "sym-resolve.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>
#include <list>
#include <iostream>
#include <cmath>
#include <sys/stat.h>
#include <map>
#include <stdint.h>
#include <inttypes.h>
#include <cstdio>
#include <cassert>
#include <unistd.h>

/** This class is the payload for a node in the stacktrace
    and holds all the information about the counter that we looking at.
  */
struct Counter
{
public:
  Counter()
  :cnt(0), freq(0), ccnt(0), cfreq(0)
  {}

  /** Adds the counts and freqs of @a other to this Counter.

      @a other source counter.

      @a isMax whether or not it needs to sum the counts
       or take the maximum between the two.
    */
  void add(const Counter &other, bool isMax)
  {
    this->freq += other.freq;

    if (isMax)
    {
      if (this->cnt < other.cnt)
        this->cnt = other.cnt;
    }
    else
      this->cnt += other.cnt;
  }

  /** Adds the cumulative counts and freqs of @a other
      to this Counter.

      @a other source counter.

      @a isMax whether or not it needs to sum the counts
       or take the maximum between the two.
    */
  void accumulate(const Counter &other, bool isMax)
  {
    this->cfreq += other.cfreq;

    if (isMax)
    {
      if (this->ccnt < other.ccnt)
        this->ccnt = other.ccnt;
    }
    else
      this->ccnt += other.ccnt;
  }

  /** The total counts of the counter. */
  int64_t cnt;
  /** The number of times the counter got triggered (e.g. number of allocations) */
  int64_t freq;
  /** The accumulated counts. */
  int64_t ccnt;
  /** The accumulated number of times the counter got triggered (e.g. the
      number of allocations of a node and all its children.)
    */
  int64_t cfreq;
};

class NameChecker
{
public:
  NameChecker(const std::string& arg) : m_arg(arg) {};
  bool operator()(const char *fullname) { return m_arg == fullname; }
  bool operator()(const char *fullname, const char *abbr)
    {
      return (m_arg == fullname) || (m_arg == abbr);
    }
private:
  const std::string m_arg;
};

class ArgsLeftCounter
{
public:
  typedef std::list<std::string> ArgsList;
  ArgsLeftCounter(const ArgsList::const_iterator& end) : m_end(end) {};
  int operator()(ArgsList::const_iterator arg)
    {
      int size = 0;
      while (arg++ != m_end) { size++; }
      return size;
    }
private:
  const ArgsList::const_iterator m_end;
};

/** Format a message on stderr and exit with exitcode 1. */
void
die(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}

/** @return FILE which reads the profile dump called @a filename.

    @a filename the filename to be opened.
    @a isPipe set to @c true if the input is from a pipe.
 */
FILE *
openDump(const char *filename, bool &isPipe)
{
  // If filename is not a real file, simply exit.
  if (access(filename, R_OK))
  {
    if (errno == ENOENT)
      die("%s does not exists", filename);
    else if (errno == EACCES)
      die("%s is not readable", filename);
    else
      die("Unable to open file %s", filename);
  }

  // Determine the kind of file (compressed or not) from the header.
  FILE *f = fopen(filename, "r");
  if (!f || ferror(f))
    die("Cannor open %s.", filename);

  unsigned char header[128];
  fread(header, 4, 1, f);
  fclose(f);
  std::string command;

  // If a command to be run on the file if specified,
  // just run it.
  // If the file is compressed, uncompress it.
  // Otherwise just read the file.
  if (header[0] == 0x1f && header[1] == 0x8b)
    command = std::string("gzip -dc ") + filename;
  else if (header[0] == 'B'
           && header[1] == 'Z'
           && header[2] == 'h')
    command = std::string("bzip2 -dc ") + filename;

  FILE *in;
  if (command.empty())
    in = fopen(filename, "r");
  else
    in = popen(command.c_str(), "r");

  if (!in)
    die("Cannot open %s.", filename);

  isPipe = ! command.empty();
  setvbuf(in, 0, _IOFBF, 128*1024);
  return in;
}

std::string
thousands(int64_t value, int leftPadding=0)
{
  // Converts an integer value to a string
  // adding `'` to separate thousands and possibly
  assert(leftPadding >= 0);
  int64_t n = 1; int digitCount = 0;
  std::string result = "";
  bool sign = value >= 0;

  if (value < 0)
    value = -value;
  if (!value)
    result = "0";

  char d[2];
  d[1] = 0;

  while ((value / n))
  {
    int digit = (value / n) % 10;
    assert(digit < 10);
    if ((! digitCount) && (n != 1))
    { result = "'" + result; }
    d[0] = static_cast<char>('0'+ static_cast<char>(digit));
    result = std::string(d) + result;
    n *= 10;
    digitCount = (digitCount + 1) % 3;
  }

  if (leftPadding)
  {
    assert(leftPadding-digitCount > 0);
    result = std::string("", leftPadding-digitCount) + result;
  }
  return(sign ? "" : "-")  + result;
}

std::string
thousands(double value, int leftPadding, int decimalPositions)
{
  value = round(value * 100.) / 100.;
  decimalPositions += value < 0 ? 1 : 0;
  int padding = leftPadding-decimalPositions;
  std::string result = thousands(int64_t(value), padding > 0 ? padding : 0);
  assert(decimalPositions < 63);
  char buffer[64];
  double decimal = fabs(value-int64_t(value));
  sprintf(buffer+1, "%.2f", decimal);
  buffer[decimalPositions+3] = 0;
  return result + &buffer[2];
}


std::string
toString(int64_t value)
{
  char buffer[1024];
  sprintf(buffer,"%" PRIi64, value);
  return buffer;
}

class AlignedPrinter
{
public:
  AlignedPrinter(int size)
    :m_size(size)
    {}

  void operator()(const std::string &n)
    {
      printf("%*s", m_size, n.c_str());
      printf("  ");
    }
private:
  int m_size;
};

class FractionPrinter
{
public:
  FractionPrinter(int size)
    :m_size(size)
    {}

  void operator()(const std::string &n, const std::string &d)
    {
      printf("%*s", m_size, n.c_str());
      char denBuffer[256];
      sprintf(denBuffer, " / %%-%ds", m_size);
      printf(denBuffer, d.c_str());
    }

private:
  int m_size;
};

class PrintIf
{
public:
  PrintIf(int size)
    :m_size(size)
    {}

  void operator()(bool condition, const std::string &value)
    {
      if (condition)
        printf("%*s  ", m_size, value.c_str());
    }
private:
  int m_size;
};

class SymbolFilter
{
public:
  SymbolFilter &operator=(const char *symbolName)
    {
      return this->addFilteredSymbol(symbolName);
    }

  SymbolFilter &operator,(const char *symbolName)
    {
      return this->addFilteredSymbol(symbolName);
    }

  SymbolFilter &addFilteredSymbol(const char *symbolName)
    {
      m_symbols.push_back(symbolName); return *this;
    }

  bool contains(const std::string &name)
    {
      bool result = std::find(m_symbols.begin(), m_symbols.end(), name) != m_symbols.end();
      return result;
    }

private:
  typedef std::list<std::string> SymbolNames;
  SymbolNames m_symbols;
};

#endif // ANALYSE_H
