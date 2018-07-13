#include "config.h"
#include "analyse.h"
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <cstdio>
#include <cfloat>
#include <iomanip>
#include <unistd.h>
#include <sstream>
#include <cassert>
#ifdef PCRE_FOUND
#  include <pcre.h>
#endif

#ifndef iggetc
# if defined getc_unlocked || defined __GLIBC__
#  define iggetc(x) getc_unlocked(x)
# else
#  define iggetc(x) fgetc(x)
# endif
#endif

#define IGPROF_MAX_DEPTH 1000

void dummy(void) {}


/** Helper class which is responsible for parsing an IgProf dump.

    This is done with a separate class to avoid polluting the internal state
    of the IgProfAnalyzerApplication with the state of the parser.
  */
class IgTokenizer
{
public:
  IgTokenizer(FILE *in, const char *filename);
  void            getToken(const char *delim);
  int64_t         getTokenN(const char *delim, size_t base = 10);
  int64_t         getTokenN(char delim, size_t base = 10);
  void            getTokenS(std::string &result, char delim);
  void            getTokenS(std::string &result, const char *delim);
  double          getTokenD(const char *delim);
  double          getTokenD(char delim);
  int             nextChar(void)
    {
      return m_next;
    }
  size_t          lineNum(void)
    {
      return m_lineCount;
    }

  /** Checks that the next char is @a skipped */
  void            skipChar(int skipped)
    {
      if (m_next != skipped)
        syntaxError();
      m_next = iggetc(m_in);
      return;
    }

  /** Checks that @a str is the next string in the file. */
  void            skipString(const char *str, size_t size = 0)
    {
      if (!size)
        size = strlen(str);
      for (size_t i = 0; i != size; ++i)
      {
        char skipped = str[i];
        skipChar(skipped);
      }
      return;
    }

  /** Increments the number of lines and skips the \n.
    */
  void skipEol(void)
    {
      skipChar('\n');
      m_lineCount++;
    }
  /** @return a pointer to the buffer holding the current token (including
      the separators).
    */
  const char     *buffer(void)
    {
      return m_buffer;
    }

  void            syntaxError();
private:
  FILE                *m_in;
  // Internal state of the tokenizer.
  /** The size of the buffer where read token are put.
      Notice that it will grow as larger token not fitting the initial size are
      found.
    */
  size_t              m_bufferSize;
  /** The where read token are put.
      Notice that it will grow as larger token not fitting the initial size
      are found.
    */
  char                *m_buffer;
  /// The next char after the current token.
  int                 m_next;
  size_t              m_lineCount;
  std::string         m_filename;
};

/** Initialises the parser extracting needed stuff from the passed ProfileInfo.

    @a in the file to be read and tokenized.

    @a filename string to be printed as filename on error.

  */
IgTokenizer::IgTokenizer(FILE *in, const char *filename)
  : m_in(in),
    m_bufferSize(1024),
    m_lineCount(0),
    m_filename(filename)
{
  m_buffer = (char *) malloc(m_bufferSize);
  m_next = iggetc(in);
}

/** Fills m_buffer with the next token delimited by a seguence of @a delim

    It also fills the @a result string with the contents of the token buffer.
  */
void
IgTokenizer::getTokenS(std::string &result, const char *delim)
{
  fgettoken(m_in, &m_buffer, &m_bufferSize, delim, &m_next);
  result.assign(m_buffer);
}

/** Fills m_buffer with the next token delimited by a seguence of @a delim

    It also fills the @a result string with the contents of the token buffer.

    Notice that given the fact that there is no ambiguity on the delimiter,
    we simply skip it.
  */
void
IgTokenizer::getTokenS(std::string &result, char delim)
{
  char buf[2] = {delim, 0};
  getTokenS(result, buf);
  m_next = iggetc(m_in);
}


/** Fills m_buffer with the next token delimited by a sequence of
    @a delim (including the delimiters).

  */
void
IgTokenizer::getToken(const char *delim)
{
  fgettoken(m_in, &m_buffer, &m_bufferSize, delim, &m_next);
}

/** Fills m_buffer with the next token delimited by a sequence of
    @a delim (including the delimiters). Then returns the long long found
    at @a offset in the buffer.

    @a base the base to be used for the translation.
  */
int64_t
IgTokenizer::getTokenN(const char *delim, size_t base)
{
  char *endptr = 0;
  size_t size = fgettoken(m_in, &m_buffer, &m_bufferSize, delim, &m_next);
  int64_t result = strtoll(m_buffer, &endptr, base);
  if (endptr != m_buffer + size)
    syntaxError();
  return result;
}

/** Fills m_buffer with the next token delimited by a sequence of
    @a delim (including the delimiters). Then returns the long long found
    at @a offset in the buffer.

    @a base the base to be used for the translation.

    Notice that since there is no ambiguity on the delimiter, we simply
    skip it.
  */
int64_t
IgTokenizer::getTokenN(char delim, size_t base)
{
  char buf[2] = {delim, 0};
  int64_t result = getTokenN(buf, base);
  m_next = iggetc(m_in);
  return result;
}

/**
    Fills m_buffer with the next token delimited by a sequence of
    @a delim (including the delimiters). Then returns the long long found
    at @a offset in the buffer.

    @a base the base to be used for the translation.
  */
double
IgTokenizer::getTokenD(const char *delim)
{
  char *endptr = 0;
  size_t size = fgettoken(m_in, &m_buffer, &m_bufferSize, delim, &m_next);
  double result = strtod(m_buffer, &endptr);
  if (endptr != m_buffer + size)
    syntaxError();
  return result;
}

double
IgTokenizer::getTokenD(char delim)
{
  char buf[2] = {delim, 0};
  double result = getTokenD(buf);
  skipChar(delim);
  return result;
}

/**
    Pretty prints a "syntax error" message, including line number and last
    token read.
  */
void
IgTokenizer::syntaxError()
{
  die("\n%s:%d: syntax error, last token read was '%s'\n",
      m_filename.c_str(), m_lineCount, m_buffer);
}

// The following options
//
// * --list-filter / -lf
// * -f FILTER[, FILTER]
// * -F/--filter-module [FILE]
// * --callgrind
//
// which where present in the old perl version
// are either obsolete or not supported at the moment
// by igprof-analyse. We do not show them in the usage, but
// we do show a different message if someone uses them.
const char USAGE[] = "igprof-analyse\n"
    "  {[-r/--report KEY[,KEY]...], --show-pages, --show-page-ranges, --show-locality-metrics}"
    "  [-o/--order ORDER]\n"
    "  [-p/--paths] [-c/--calls] [--value peak|normal]\n"
    "  [-mr/--merge-regexp REGEXP]\n"
    "  [-ml/--merge-libraries REGEXP]\n"
    "  [-nf/--no-filter]\n"
    "  { [-t/--text], [-s/--sqlite], [--top <n>], [--tree] }\n"
    "  [--libs] [--demangle] [--gdb] [-v/--verbose]\n"
    "  [-b/--baseline FILE [--diff-mode]]\n"
    "  [-Mc/--max-count-value <value>] [-mc/--min-count-value <value>]\n"
    "  [-Mf/--max-calls-value <value>] [-mc/--min-calls-value <value>]\n"
    "  [-Ma/--max-average-value <value>] [-ma/--min-average-value <value>]\n"
    "  [--] [FILE]...\n";
float percent(int64_t a, int64_t b)
{
  double value = static_cast<double>(a) / static_cast<double>(b);
  if (value < -1.0)
  {
    std::cerr << "Something is wrong. Invalid percentage value: " << value * 100. << "%" << std::endl;
    exit(1);
  }
  return value * 100.;
}

class SymbolInfo
{
public:
  std::string NAME;
  FileInfo    *FILE;
  int64_t     FILEOFF;
  SymbolInfo(const char *name, FileInfo *file, int fileoff)
    : NAME(name), FILE(file), FILEOFF(fileoff), RANK(-1)
    {}

  int rank(void) { return RANK; }
  void setRank(int rank) { RANK = rank; }
private:
  int RANK;
};

/** Structure which holds information about a given
    range in memory (possibly in terms of pages).
    For the moment we only care about the start and end address, not about
    the amount of memory found in it.
 */
struct RangeInfo
{
  uint64_t startAddr;       // The address of the first page in the range
  uint64_t endAddr;         // The address of the last page in the range

  RangeInfo(uint64_t iStartAddr, uint64_t iEndAddr)
    : startAddr(iStartAddr), endAddr(iEndAddr)
  {
    assert(iStartAddr < iEndAddr);
  }

  // One page range is less than the other if its final address is less than the
  // start address of the other.
  bool operator<(const RangeInfo &other) const
  {
    return this->endAddr < other.startAddr;
  }

  size_t size(void) const
  {
    if (endAddr - startAddr + 1 <= 0)
      std::cerr << endAddr << " " << startAddr << std::endl;

    assert(endAddr - startAddr + 1 > 0);
    return endAddr - startAddr + 1;
  }
};


typedef std::vector<RangeInfo>  Ranges;

void debugRanges(const char * name, const Ranges &ranges)
{
  std::cerr << name << " ";
  for (size_t i = 0, e = ranges.size(); i != e; ++i)
  {
    std::cerr << "(" << ranges[i].startAddr << ", " << ranges[i].endAddr << ")";
  }
  std::cerr << std::endl;
}


void
mergeSortedRanges(Ranges &source)
{
  if (source.empty())
    return;

  Ranges temp;
  temp.reserve(source.size());
  temp.push_back(source.front());
  for (size_t ri = 1, re = source.size(); ri != re; ++ri)
  {
    RangeInfo &current = temp.back();
    const RangeInfo &next = source[ri];
    if (next.startAddr <= current.endAddr)
    {
      if (next.endAddr >= current.endAddr)
        current.endAddr = next.endAddr;
    }
    else
      temp.push_back(next);
  }
  temp.reserve(temp.size());
  source.swap(temp);
  return;
}

/** Helper function to count number of pages in a set of
    @a ranges*/
size_t
countPages(Ranges &ranges)
{
  size_t numOfPages = 0;
  for (size_t ri = 0, re = ranges.size(); ri != re; ++ri)
  {
    assert(ranges[ri].size() > 0);
    numOfPages += ranges[ri].size();
  }

  return numOfPages;
}

// Merges to set of ranges together by sorting their union
// and the collapsing consecutive ranges.
void mergeRanges(Ranges &dest, Ranges &source)
{
  //  std::cerr << "Merging: " << std::endl;
  //  debugRanges(dest);
  //  debugRanges(source);
  // Trivial cases, when one of the lists or both is empty.
  Ranges temp;

  if (source.empty())
  {
//    mergeSortedRanges(dest);
    return;
  }

  if (dest.empty())
  {
//    mergeSortedRanges(source);
    dest = source;
    return;
  }

  size_t di = 0, si = 0;

  temp.reserve(dest.size() + source.size());

  // Select the one of the two queues which has the lowest start address.
  if (dest.front().startAddr < source.front().startAddr)
  {
    temp.push_back(dest.front());
    ++di;
  }
  else
  {
    temp.push_back(source.front());
    ++si;
  }

  while (di < dest.size() && si < source.size())
  {
    assert(temp.size());
    RangeInfo &current = temp.back();
    const RangeInfo &nextDest = dest[di];
    const RangeInfo &nextSource = source[si];
    // Select which of the two queues has the lowest start address.
    if (nextDest.startAddr < nextSource.startAddr)
    {
      // Extend previous, if overlapping.
      if (nextDest.startAddr <= current.endAddr)
      {
        if (nextDest.endAddr >= current.endAddr)
          current.endAddr = nextDest.endAddr;
      }
      else
        temp.push_back(nextDest);
      ++di;
    }
    else
    {
      // Extend previous, if overlapping.
      if (nextSource.startAddr <= current.endAddr)
      {
        if (nextSource.endAddr >= current.endAddr)
          current.endAddr = nextSource.endAddr;
      }
      else
        temp.push_back(nextSource);
      ++si;
    }

    if (temp.size() && temp.back().endAddr < temp.back().startAddr)
    {
      std::cerr << temp.back().endAddr << " " << temp.back().startAddr << std::endl;
      assert(false);
    }
  }

  // One of the two will finish earlier. Merge the remaining of the other.
  if (di < dest.size())
  {
    for (size_t ri = di, re = dest.size(); ri != re; ++ri)
    {
      RangeInfo &current = temp.back();
      const RangeInfo &next = dest[ri];
      if (next.startAddr <= current.endAddr)
      {
        if (next.endAddr >= current.endAddr)
          current.endAddr = next.endAddr;
      }
      else
        temp.push_back(next);
    }
  }
  else
  {
    for (size_t ri = si, re = source.size(); ri != re; ++ri)
    {
      RangeInfo &current = temp.back();
      const RangeInfo &next = source[ri];
      if (next.startAddr <= current.endAddr)
      {
        if (next.endAddr >= current.endAddr)
          current.endAddr = next.endAddr;
      }
      else
        temp.push_back(next);
    }
  }

//  if (countPages(temp) < countPages(source))
//  {
//    debugRanges("old parent", dest);
//    debugRanges("child", source);
//    debugRanges("new parent", temp);
//  }

//  assert(countPages(temp) >= countPages(source));

  temp.reserve(temp.size());
  dest.swap(temp);
}

/** Structure to hold information about nodes in the
    call tree. */
class NodeInfo
{
public:
  typedef std::vector<NodeInfo *> Nodes;
  typedef Nodes::iterator Iterator;

  Nodes   CHILDREN;
  Counter COUNTER;
  Ranges  RANGES;
  Ranges  CUM_RANGES;

  NodeInfo()
    : SYMBOL(0), m_reportSymbol(0) {};

  NodeInfo *getChildrenBySymbol(SymbolInfo *symbol)
    {
      for (size_t ni = 0, ne = CHILDREN.size(); ni != ne; ++ni)
      {
        NodeInfo *node = CHILDREN[ni];
        if (node->SYMBOL == symbol)
          return node;

        if (symbol && (node->SYMBOL->NAME == symbol->NAME))
          return node;
      }
      return 0;
    }

  void printDebugInfo(int level=0)
    {
      std::string indent(level*4, ' ');
      std::cerr << indent << "Node: " << this
                << " Symbol name: " << this->symbol()->NAME
                << " File name: " << this->symbol()->FILE->NAME
                << std::endl;
      for (size_t ni = 0, ne = CHILDREN.size(); ni != ne; ++ni)
        CHILDREN[ni]->printDebugInfo(level+1);
    }

  void removeChild(NodeInfo *node) {

    assert(node);
    Nodes::iterator new_end = std::remove_if(CHILDREN.begin(),
                                             CHILDREN.end(),
                                             std::bind2nd(std::equal_to<NodeInfo *>(), node));
    if (new_end != CHILDREN.end())
      CHILDREN.erase(new_end, CHILDREN.end());
  }

  SymbolInfo *symbol(void) const
    {
      return m_reportSymbol ? m_reportSymbol : SYMBOL;
    }

  void reportSymbol(SymbolInfo *reportSymbol)
    {
      m_reportSymbol = reportSymbol;
    }

  SymbolInfo *reportSymbol(void) const
    {
      return m_reportSymbol;
    }
  SymbolInfo *originalSymbol(void) const
    {
      return SYMBOL;
    }
  void setSymbol(SymbolInfo *symbol)
    {
      SYMBOL = symbol;
      m_reportSymbol = 0;
    }
private:
  SymbolInfo *SYMBOL;
  SymbolInfo *m_reportSymbol;
};


class ProfileInfo
{
private:
  struct FilesComparator
  {
    bool operator()(FileInfo *f1, FileInfo *f2) const
      {
        return strcmp(f1->NAME.c_str(), f2->NAME.c_str()) < 0;
      }
  };

public:
  typedef std::set<FileInfo *, FilesComparator> Files;
  typedef std::vector<SymbolInfo *> Syms;
  typedef std::vector<NodeInfo *> Nodes;
  typedef std::vector<int> Counts;
  typedef std::map<std::string, Counts> CountsMap;
  typedef std::map<std::string, std::string> Freqs;
  typedef std::map<std::string, std::string> LeaksMap;
  typedef std::map<std::string, SymbolInfo*> SymCacheByFile;
  typedef std::map<std::string, SymbolInfo*> SymCacheByName;
  typedef std::set<SymbolInfo*> SymCache;

  ProfileInfo(void)
    {
      FileInfo *unknownFile = new FileInfo("<unknown>", false);
      SymbolInfo *spontaneousSym = new SymbolInfo("<spontaneous>", unknownFile, 0);
      m_spontaneous = new NodeInfo();
      m_spontaneous->setSymbol(spontaneousSym);
      m_files.insert(unknownFile);
      m_syms.push_back(spontaneousSym);
      m_nodes.push_back(m_spontaneous);
    };

  Files & files(void) { return m_files; }
  Syms & syms(void) { return m_syms; }
  Nodes & nodes(void) { return m_nodes; }
  NodeInfo *spontaneous(void) { return m_spontaneous; }
  SymCacheByFile &symcacheByFile(void) { return m_symcacheFile; }
  SymCache &symcache(void) { return m_symcache; }
private:
  Files m_files;
  Syms m_syms;
  Nodes m_nodes;
  LeaksMap m_leaks;
  SymCacheByFile m_symcacheFile;
  SymCache m_symcache;
  NodeInfo  *m_spontaneous;
};

class IgProfFilter;

/** Structure that hold information
    about the regular expressions to apply.
  */
struct RegexpSpec
{
  // The actual regular expression to match.
  std::string  re;
  // The replacement string.
  std::string  with;
};

struct AncestorsSpec
{
  // List of ancestos that need to match
  std::vector<std::string> ancestors;
  // New symbol name for nodes that has ancestors matching to ancestors list
  std::string with;
};

class Configuration
{
public:
  enum OutputType {
    TEXT=0,
    XML=1,
    HTML=2,
    SQLITE=3,
    JSON=4
  };
  enum Ordering {
    DESCENDING=-1,
    ASCENDING=1
  };

  Configuration(void);

  void setShowLib(bool value) { m_showLib = value;}
  bool showLib(void) { return m_showLib; }

  void setCallgrind(bool value)  { m_callgrind = value ? 1 : 0; }
  bool callgrind(void) { return m_callgrind == 1; }
  bool isCallgrindDefined(void) { return m_callgrind != -1; }

  void setShowCalls(bool value) { m_showCalls = value ? 1 : 0; }
  bool isShowCallsDefined(void) { return m_showCalls != -1; }
  bool showCalls(void) { return m_showCalls == 1; }

  void setDoDemangle(bool value) { m_doDemangle = value;}
  bool doDemangle(void) { return m_doDemangle; }

  void setShowPaths(bool value)  { m_showPaths = value;}
  bool showPaths(void) { return m_showPaths; }

  void setVerbose(bool value) { m_verbose = value;}
  bool verbose(void) { return m_verbose; }

  void setOutputType(enum OutputType value) { m_outputType = value; }
  OutputType outputType(void) { return m_outputType; }

  void setOrdering(enum Ordering value) { m_ordering = value; }
  Ordering ordering(void) { return m_ordering; }

  void setNormalValue(bool value) { m_normalValue = value; }
  bool normalValue(void) { return m_normalValue; }

  void setMergeLibraries(bool value) { m_mergeLibraries = value; }
  bool mergeLibraries(void) { return m_mergeLibraries; }

  void setBaseline(const std::string &baseline)
    {
      m_baseline = baseline;
    }

  const std::string &baseline(void)
    {
      return m_baseline;
    }

  void setDiffMode(bool value)
    {
      m_diffMode = value;
    }

  bool diffMode(void)
    {
      return m_diffMode;
    }

  bool hasHitFilter(void)
    {
      return minCountValue > 0
        || maxCountValue > 0
        || minCallsValue > 0
        || maxCallsValue > 0
        || minAverageValue > 0
        || maxAverageValue > 0;
    }

private:
  std::string m_key;
  OutputType  m_outputType;
  Ordering  m_ordering;
  bool m_showLib;
  int m_callgrind;
  bool m_doDemangle;
  bool m_showPaths;
  int m_showCalls;
  bool m_verbose;
  bool m_normalValue;
  bool m_mergeLibraries;
  std::string m_baseline;
  bool m_diffMode;
public:
  int64_t  minCountValue;
  int64_t  maxCountValue;
  int64_t  minCallsValue;
  int64_t  maxCallsValue;
  int64_t  minAverageValue;
  int64_t  maxAverageValue;
  bool     tree;
  bool     useGdb;
  bool     dumpAllocations;
  std::vector<RegexpSpec>   regexps;
  AncestorsSpec  ancestors;
};

Configuration::Configuration()
  :m_outputType(Configuration::TEXT),
   m_ordering(Configuration::ASCENDING),
   m_showLib(false),
   m_callgrind(-1),
   m_doDemangle(false),
   m_showPaths(false),
   m_showCalls(-1),
   m_verbose(false),
   m_normalValue(true),
   m_mergeLibraries(false),
   m_diffMode(false),
   minCountValue(-1),
   maxCountValue(-1),
   minCallsValue(-1),
   maxCallsValue(-1),
   minAverageValue(-1),
   maxAverageValue(-1),
   tree(false),
   useGdb(false),
   dumpAllocations(false)
{}

static Configuration *s_config = 0;

class StackTraceFilter
{
public:
  virtual ~StackTraceFilter();
  virtual void filter(SymbolInfo *symbol,
                      int64_t &counter,
                      int64_t &freq) = 0;
};

StackTraceFilter::~StackTraceFilter() {}

class ZeroFilter : public StackTraceFilter
{
public:
  virtual void filter(SymbolInfo *, int64_t &counter, int64_t &freq)
    {
      counter=0; freq = 0;
    }
};

class BaseLineFilter : public StackTraceFilter
{
public:
  virtual void filter(SymbolInfo *, int64_t &counter, int64_t &freq)
    {
      counter=-counter; freq = -freq;
    }
};

// Filters the incoming stacktraces by the size
// of their counters(either absolute total, average, or
// by the number of calls to a given stacktrace).
// If the counter value is not within limits,
// the the stacktrace is not accumulated in the
// tree.
class HitFilter : public StackTraceFilter
{
public:
  HitFilter(Configuration &config)
    :m_minValue(config.minCountValue), m_maxValue(config.maxCountValue),
     m_minFreq(config.minCallsValue), m_maxFreq(config.maxCallsValue),
     m_minAvg(config.minAverageValue), m_maxAvg(config.maxAverageValue)
    {}

  virtual void filter(SymbolInfo *, int64_t &counter, int64_t &freq)
    {
      if (m_minValue > 0 && counter < m_minValue)
      {
        counter = 0; freq = 0;
        return;
      }

      if (m_maxValue > 0 && counter > m_maxValue)
      {
        counter = 0; freq = 0;
        return;
      }

      if (m_minFreq > 0 && freq < m_minFreq)
      {
        counter = 0; freq = 0;
        return;
      }

      if (m_maxFreq > 0 && freq > m_maxFreq)
      {
        counter = 0; freq = 0;
        return;
      }

      if (m_minAvg > 0 && freq && (counter/freq < m_minAvg))
      {
        counter = 0; freq = 0;
        return;
      }

      if (m_maxAvg > 0 && freq && (counter/freq > m_maxAvg))
      {
        counter = 0; freq = 0;
        return;
      }
    }

private:
  int64_t m_minValue;
  int64_t m_maxValue;
  int64_t m_minFreq;
  int64_t m_maxFreq;
  int64_t m_minAvg;
  int64_t m_maxAvg;
};

class TreeMapBuilderFilter;
class FlatInfo;

class IgProfFilter
{
public:
  enum FilterType
  {
    PRE = 1,
    POST = 2,
    BOTH = 3
  };

  virtual ~IgProfFilter(void) {}
  virtual std::string name(void) const = 0;
  virtual enum FilterType type(void) const = 0;
  virtual void pre(NodeInfo *, NodeInfo *) {};
  virtual void post(NodeInfo *, NodeInfo *) {};
};

/** Merges @a node and all its children to @a parent.

    We iterate on all the children of node.
    If parent does not have any children with the same name, we simply add it
    to the list of children.
    If parent already has a children with the same symbol name
    we merge the counters and call mergeToNode for every one of the children.
    Finally if node is among the children of parent, remove it from them.

    @a node is the node to be removed.

    @a parent is the parent of that node, which will get all the
     children of node.

    @a isMax whether or not peak value has to be taken for the counts,
     rather than accumulating them.
*/
void mergeToNode(NodeInfo *parent, NodeInfo *node, bool isMax)
{
  // We iterate on all the children of node. [1]
  // * If parent does not have any children with the same name, we simply add it
  //   to the list of children. [2]
  // * If parent already has a children with the same symbol name
  //   we merge the counters and call mergeToNode for every one of the
  //   children. [3]
  // Finally if node is among the children of parent we copy all
  // its allocations to it and remove the node from the children list. [4]

  // [1]
  for (size_t i = 0, e = node->CHILDREN.size(); i != e; i++)
  {
    NodeInfo *nodeChild = node->CHILDREN[i];
    assert(nodeChild);
    if (!nodeChild->symbol())
      continue;
    NodeInfo *parentChild = parent->getChildrenBySymbol(nodeChild->symbol());
    if (!parentChild)
    {
      parent->CHILDREN.push_back(nodeChild);
      continue;
    }

    assert(parentChild != nodeChild);
    parentChild->COUNTER.add(nodeChild->COUNTER, isMax);
    mergeToNode(parentChild, nodeChild, isMax);
  }

  if (node == parent->getChildrenBySymbol(node->symbol()))
  {
    // Merge the ranges between the two nodes.
    // Not only we need to extend the range, but in case
    // we close an hole we also need to join the ranges
    // together into one single range.
    // are consecutive.
    mergeRanges(parent->RANGES, node->RANGES);

    NodeInfo::Nodes::iterator new_end = std::remove_if(parent->CHILDREN.begin(),
                                                       parent->CHILDREN.end(),
                                                       std::bind2nd(std::equal_to<NodeInfo *>(), node));
    if (new_end != parent->CHILDREN.end())
      parent->CHILDREN.erase(new_end, parent->CHILDREN.end());
  }
}

class RemoveIgProfFilter : public IgProfFilter
{
public:
  RemoveIgProfFilter(bool isMax)
  : m_isMax(isMax)
  {}

  /** Check if the symbol comes from the injected
      igprof / ighook libraries and and merge it in that case.
    */
  virtual void post(NodeInfo *parent,
                    NodeInfo *node)
    {
      if (!parent)
        return;

      assert(node);
      assert(node->originalSymbol());
      assert(node->originalSymbol()->FILE);
      const std::string &filename = node->originalSymbol()->FILE->NAME;
      // In case the filename is longer than 20 characters, we only look for
      // the last 20 character since the igprof library name is going to be
      // smaller than that.  We do not do a reverse search, because that seems
      // to be much slower that a forward search.
      size_t size = node->originalSymbol()->FILE->NAME.size();
      size_t chkOffset = size > 20 ? size - 20 : 0;
      // Handle the different library name(s) when compiled inside CMSSW.
      // We cannot ignore the old library names yet, because there
      // are plenty of reports that still use them.
      if (strstr(filename.c_str() + chkOffset, "libigprof."))
        mergeToNode(parent, node, m_isMax);
      if (strstr(filename.c_str() + chkOffset, "IgProf.")
          || strstr(filename.c_str() + chkOffset, "IgHook."))
        mergeToNode(parent, node, m_isMax);
    }

  virtual std::string name(void) const { return "igprof remover"; }
  virtual enum FilterType type(void) const { return POST; }
private:
  bool m_isMax;
};

/** Removes any malloc related symbol from the calltree
    adding their contribution to the nearest parent.
 */
class MallocFilter : public IgProfFilter
{
public:
  MallocFilter(bool isMax)
  :m_isMax(isMax)
  {
    m_filter = "malloc", "calloc", "realloc", "memalign", "posix_memalign",
               "valloc", "zmalloc", "zcalloc", "zrealloc", "_Znwj", "_Znwm",
               "_Znaj", "_Znam";
  }

  virtual void post(NodeInfo *parent, NodeInfo *node)
    {
      assert(node);
      assert(node->symbol());
      assert(m_filter.contains(std::string("_Znaj")));
      if (!parent)
        return;

      if (! m_filter.contains(node->symbol()->NAME))
        return;

      parent->COUNTER.add(node->COUNTER, m_isMax);
      mergeRanges(parent->RANGES, node->RANGES);
      parent->removeChild(node);
    }

  virtual std::string name(void) const { return "malloc"; }
  virtual enum FilterType type(void) const { return POST; }

private:
  SymbolFilter m_filter;
  bool m_isMax;
};

class IgProfGccPoolAllocFilter : public IgProfFilter
{
public:
  IgProfGccPoolAllocFilter(void)
    {
      m_filter = "_ZNSt24__default_alloc_templateILb1ELi0EE14_S_chunk_allocEjRi",
                 "_ZNSt24__default_alloc_templateILb1ELi0EE9_S_refillEj",
                 "_ZNSt24__default_alloc_templateILb1ELi0EE8allocateEj";
    }

  virtual void post(NodeInfo *parent, NodeInfo *node)
    {
      if (! m_filter.contains(node->symbol()->NAME))
        return;
      parent->removeChild(node);
    }
  virtual std::string name(void) const { return "gcc_pool_alloc"; }
  virtual enum FilterType type(void) const { return POST; }
private:
  SymbolFilter m_filter;
};

class IgProfAnalyzerApplication
{
  typedef std::vector<FlatInfo *> FlatVector;
public:
  typedef std::list <std::string> ArgsList;

  IgProfAnalyzerApplication(Configuration *config, int argc, const char **argv);

  void run(void);
  void parseArgs(const ArgsList &args);
  void analyse(ProfileInfo &prof, TreeMapBuilderFilter *baselineBuilder);
  void generateFlatReport(ProfileInfo &prof,
                          TreeMapBuilderFilter *callTreeBuilder,
                          TreeMapBuilderFilter *baselineBuilder,
                          FlatVector &sorted);
  void callgrind(ProfileInfo &prof);
  void topN(ProfileInfo &prof);
  void tree(ProfileInfo &prof);
  void readDump(ProfileInfo *prof, const std::string &filename, StackTraceFilter *filter);
  void dumpAllocations(ProfileInfo &prof);
  void prepdata(ProfileInfo &prof);
  void summarizePageInfo(FlatVector &sorted);

  /** Sets the mnemonic name of the counter is to be used as a key.

      Update the m_keyMax and m_isPerfTick to match the kind of
      counter specified.

      It also sets up the default filters (unless --no-filter / -nf is
      specified).
    */
  void setKey(const std::string &value)
  {
    m_key = value;

    if (strstr(m_key.c_str(), "_MAX") == m_key.c_str() + m_key.size() - 4)
      m_keyMax = true;
    if (m_key == "PERF_TICKS" || m_key.find("NRG_") == 0)
      m_isPerfTicks = true;

    if (m_disableFilters)
      return;

    m_filters.push_back(new RemoveIgProfFilter(m_keyMax));

    if (!m_filters.empty() && !strncmp(m_key.c_str(), "MEM_", 4))
      m_filters.push_back(new MallocFilter(m_keyMax));
    if (!m_filters.empty() && m_key == "MEM_LIVE")
      m_filters.push_back(new IgProfGccPoolAllocFilter());
  }

private:
  Configuration *               m_config;
  int                           m_argc;
  const char                    **m_argv;
  std::vector<std::string>      m_inputFiles;
  std::vector<RegexpSpec>       m_regexps;
  AncestorsSpec                 m_ancestors;
  std::vector<IgProfFilter *>   m_filters;
  std::deque<NodeInfo>          m_nodesStorage;
  std::string                   m_key;
  bool                          m_isPerfTicks;
  bool                          m_keyMax;
  bool                          m_disableFilters;
  bool                          m_showPageRanges;
  bool                          m_showPages;
  bool                          m_showLocalityMetrics;
  size_t                        m_topN;
  float                         m_tickPeriod;
};


IgProfAnalyzerApplication::IgProfAnalyzerApplication(Configuration *config, int argc, const char **argv)
  :m_config(config),
   m_argc(argc),
   m_argv(argv),
   m_isPerfTicks(false),
   m_keyMax(false),
   m_disableFilters(false),
   m_showPageRanges(false),
   m_showPages(false),
   m_showLocalityMetrics(false),
   m_topN(0),
   m_tickPeriod(0.01)
{}

static void
verboseMessage(const char *msg = 0, const char *arg = 0, const char *end = 0)
{
  if (s_config->verbose())
  {
    if (msg)
      std::cerr << msg;

    if (arg)
      std::cerr << " " << arg;

    if (msg && ! end)
      std::cerr << ": ";

    if (end)
      std::cerr << end;

    std::cerr.flush();
  }
}

static int s_counter = 0;

static void
printProgress(void)
{
  s_counter = (s_counter + 1) % 100000;
  if (! s_counter)
    verboseMessage(0, 0, ".");
}

/**
  This helper function beautifies a symbol name in the following fashion:

  * If the symbol name is in the form '@?.*' we rewrite it using the
    file name it belongs to (or <dynamically-generate_>) and the offset
    in the file.
  * Moreover if the useGdb option is specified and the file is a regular one
    it uses a combination of gdb, nm and objdump (as documented in
    FileInfo::symbolByOffset).
*/
void
symlookup(FileInfo *file, int fileoff, std::string& symname, bool useGdb)
{
  assert(file);
  std::string result = symname;
  if ((symname.size() > 1 && symname[0] == '@' && symname[1] == '?')
      && file->NAME.size() && (fileoff > 0))
  {
    char buffer[1024];
    if (file->NAME == "<dynamically generated>")
    {
      sprintf(buffer, "@?0x%x{<dynamically-generated>}", fileoff);
      result = buffer;
    }
    else
    {
      sprintf(buffer, "+%d}",fileoff);
      // Finds the filename by looking up for the last / in the path
      // and picking up only the remaining.
      // Notice that this works also in the case / is not found
      // because in that case npos is returned and npos + 1 == 0.
      // The standard infact defines npos to be the largest possible unsigned integer.
      result = "@{" + file->NAME.substr(file->NAME.find_last_of('/') + 1) + buffer;
    }
  }

  struct stat st;
  if (useGdb && ::stat(file->NAME.c_str(), &st) == 0 && S_ISREG(st.st_mode))
  {
    const char *name = file->symbolByOffset(fileoff);
    if (name)
      result = name;
  }
  symname.swap(result);
}

void
printSyntaxError(const std::string &text,
                 const std::string &filename,
                 int line, int position)
{
  std::cerr << filename << ":" << "line " << line
            << ", character " << position <<" unexpected input:\n"
            << text << "\n"
            << std::string(position, ' ') << "^\n"
            << std::endl;
}

// A simple filter which reports the number of allocations
// required to allocate the amount of memory that fits on
// one page. This is a rough indication of the actual
// fragmentation and has the advantage that the greater the number is
// the more fragmented the memory is likely to be.
//
// FIXME: a more accurate indication of fragmentation is
//        to find out how many pages were actually touched
//        by all the allocations done in a given node.
//        This is much more complicated because we would
//        have to do the binning of all the allocations
//        to see how many of them share the same page
//        and how many don't.
class AllocationsPerPage : public IgProfFilter
{
public:
  virtual void post(NodeInfo *,
                    NodeInfo *node)
    {
      assert(node);
      Counter &counter = node->COUNTER;
      if (counter.freq != 0)
        counter.cnt = 4096 * counter.freq / counter.cnt;
    }
  virtual std::string name(void) const { return "average allocation size"; }
  virtual enum FilterType type(void) const { return POST; }
};

/** This filter accumulates all the counters so that
    parents cumulative counts / freq contain all
    include those of all their children.

    On the way down (pre) the filter sets all the cumulative
    values to the counts for the node itself,
    while on the way up (post) the counts of a child are
    added properly to the parent.

    In case any of --show-pages, --show-page-ranges,
    --show-locality-metrics options is found,
    we propagate to the parent also the list of RANGES
    and we merge it to the it ranges.
  */
class AddCumulativeInfoFilter : public IgProfFilter
{
public:
  AddCumulativeInfoFilter(bool isMax)
    : m_isMax(isMax)
    {}

  /** Initialise the cumulative counters with
      the self counters.
    */
  virtual void pre(NodeInfo *, NodeInfo *node)
    {
      assert(node);
      Counter &counter = node->COUNTER;
      counter.cfreq = counter.freq;
      counter.ccnt = counter.cnt;
      node->CUM_RANGES = node->RANGES;
    }

  virtual void post(NodeInfo *parent, NodeInfo *node)
    {
      assert(node);
      if (!parent)
        return;
      parent->COUNTER.accumulate(node->COUNTER, m_isMax);

      // Merge the allocation ranges of the children into
      // the cumulative ranges of the parent.
      Ranges oldparent = parent->CUM_RANGES;
      mergeRanges(parent->CUM_RANGES, node->CUM_RANGES);

      if (countPages(parent->CUM_RANGES) < countPages(node->CUM_RANGES))
      {
        debugRanges("old parent", oldparent);
        debugRanges("child", node->CUM_RANGES);
        debugRanges("new parent", parent->CUM_RANGES);
      }
      assert(countPages(parent->CUM_RANGES) >= countPages(node->CUM_RANGES));
    }

  virtual std::string name(void) const { return "cumulative info"; }
  virtual enum FilterType type(void) const { return BOTH; }

private:
  bool m_isMax;
};

/** Simple tree consistency class that makes sure that there
    is only one node (the root) without parent.
*/
class CheckTreeConsistencyFilter : public IgProfFilter
{
public:
  CheckTreeConsistencyFilter()
    :m_noParentCount(0)
    {}

  virtual void post(NodeInfo *parent,
                    NodeInfo *)
    {
      if (!parent)
      {
        m_noParentCount++;
        assert(m_noParentCount == 1);
      }
    }
  virtual std::string name(void) const { return "Check consitency of tree"; }
  virtual enum FilterType type(void) const { return POST; }
private:
  int m_noParentCount;
};

/** Prints the whole calltree as a tree (i.e. indenting children according to their
    level) */
class PrintTreeFilter : public IgProfFilter
{
public:
  PrintTreeFilter(void)
    {}

  virtual void pre(NodeInfo *parent, NodeInfo *node)
    {
      m_parentStack.erase(std::find(m_parentStack.begin(), m_parentStack.end(), parent), m_parentStack.end());
      m_parentStack.push_back(parent);

      std::cerr << std::string(2*(m_parentStack.size()-1), ' ') << node->symbol()->NAME;
      Counter &counter = node->COUNTER;
      std::cerr << " C" << "[" << counter.cnt << ", "
                << counter.freq << ", "
                << counter.ccnt << ", "
                << counter.cfreq << "]";
      std::cerr << std::endl;
    }
  virtual std::string name(void) const { return "Printing tree structure"; }
  virtual enum FilterType type(void) const {return PRE; }
private:
  std::vector<NodeInfo *> m_parentStack;
};

/** Filter which dumps per node allocation information
    in a file.
 */
class DumpAllocationsFilter : public IgProfFilter
{
public:
  DumpAllocationsFilter(std::ostream &out)
  :m_out(out)
  {
  }

  /** While traversing down the tree, print out all the
      allocations and keep track of the symbols that we
      find.
    */
  virtual void pre(NodeInfo *, NodeInfo *node)
  {
    for (size_t i = 0, e = node->RANGES.size(); i != e; ++i)
    {
      RangeInfo &r = node->RANGES[i];
      m_out << node << "," << node->symbol() << ","
            << std::hex << r.startAddr << "," << r.endAddr - r.startAddr << "\n";
    }
  }

  virtual std::string name() const { return "Dump Allocations"; }
  virtual FilterType type() const { return PRE; }
private:
  typedef std::set<SymbolInfo *> Symbols;
  std::ostream            &m_out;
  Symbols                 m_symbols;
};

class CollapsingFilter : public IgProfFilter
{
public:
  CollapsingFilter(bool isMax)
  : m_isMax(isMax)
  {}

  // On the way down add extra nodes for libraries.
  virtual void pre(NodeInfo *parent, NodeInfo *node)
    {
      if (!parent)
        return;
      assert(parent);
      assert(node);
      assert(node->symbol());
      assert(node->symbol()->FILE);

      std::deque<NodeInfo *> todos;
      todos.insert(todos.begin(), node->CHILDREN.begin(), node->CHILDREN.end());
      node->CHILDREN.clear();
      convertSymbol(node);

      while (!todos.empty())
      {
        NodeInfo *todo = todos.front();
        todos.pop_front();

        // Obtain a SymbolInfo with the filename, rather than the actual symbol name.
        convertSymbol(todo);

        // * If the parent has the same SymbolInfo, we merge the node to the parent
        // and add its children to the todo list.
        // * If there is already a child of the parent with the same symbol info,
        //   we merge with it.
        // * Otherwise we simply re-add the node.
        if (todo->symbol() == node->symbol())
        {
          todos.insert(todos.end(), todo->CHILDREN.begin(), todo->CHILDREN.end());
          node->COUNTER.add(todo->COUNTER, m_isMax);
          mergeRanges(node->RANGES, todo->RANGES);
        }
        else if (NodeInfo *same = node->getChildrenBySymbol(todo->symbol()))
        {
          same->CHILDREN.insert(same->CHILDREN.end(), todo->CHILDREN.begin(), todo->CHILDREN.end());
          same->COUNTER.add(todo->COUNTER, m_isMax);
          mergeRanges(same->RANGES, todo->RANGES);
        }
        else
          node->CHILDREN.push_back(todo);
      }
    }
  virtual enum FilterType type(void) const {return PRE; }
protected:
  virtual void convertSymbol(NodeInfo *node) = 0;
private:
  bool m_isMax;
};

class UseFileNamesFilter : public CollapsingFilter
{
  typedef std::map<std::string, SymbolInfo *> FilenameSymbols;
public:
  UseFileNamesFilter(bool isMax)
  :CollapsingFilter(isMax)
  {}
  virtual std::string name(void) const { return "unify nodes by library."; }
  virtual enum FilterType type(void) const {return PRE; }
protected:
  void convertSymbol(NodeInfo *node)
    {
      FilenameSymbols::iterator i = m_filesAsSymbols.find(node->symbol()->FILE->NAME);
      SymbolInfo *fileInfo;
      if (i == m_filesAsSymbols.end())
      {
        fileInfo = new SymbolInfo(node->symbol()->FILE->NAME.c_str(),
                                  node->symbol()->FILE, 0);
        m_filesAsSymbols.insert(FilenameSymbols::value_type(node->symbol()->FILE->NAME,
                                                            fileInfo));
      }
      else
        fileInfo = i->second;
      node->setSymbol(fileInfo);
    }
private:
  FilenameSymbols m_filesAsSymbols;
};

class RegexpFilter : public CollapsingFilter
{
  struct Regexp
  {
#ifdef PCRE_FOUND
    pcre          *re;
#endif
    std::string   with;
  };

  typedef std::map<std::string, SymbolInfo *> CollapsedSymbols;
public:
  RegexpFilter(const std::vector<RegexpSpec> &specs, bool isMax)
  :CollapsingFilter(isMax)
  {
    for (size_t i = 0, e = specs.size(); i != e; i++)
    {
      m_regexps.resize(m_regexps.size() + 1);
      Regexp &regexp = m_regexps.back();
      const char *errptr = 0;
#ifdef PCRE_FOUND
      int erroff = 0;
      regexp.re = pcre_compile(specs[i].re.c_str(), 0, &errptr, &erroff, 0);
#endif
      if (errptr)
      {
        std::cerr << "Error while compiling regular expression" << std::endl;
        exit(1);
      }
      regexp.with = specs[i].with;
    }
  }

  virtual std::string name(void) const { return "collapsing nodes using regular expression."; }
  virtual enum FilterType type(void) const { return PRE; }

protected:
  /** Applies the text substitution to the symbol
      belonging to @a node or the filename where the @a node
      is found.

      FIXME: recode to use PCRE directly!
   */
#ifdef PCRE_FOUND
  void convertSymbol(NodeInfo *node)
    {
      if (m_symbols.find(node->symbol()->NAME) != m_symbols.end())
        return;

      std::string mutantString;
      std::string translatedName;

      for (size_t i = 0, e = m_regexps.size(); i != e; i++)
      {
        Regexp &regexp = m_regexps[i];

        //First try to find matches to regular expression from symbol name. If
        //no matches, try to find matches from FILE name. If nothig changed
        //continue to next
        mutantString = node->symbol()->NAME;
        replace(regexp.re, regexp.with, mutantString, translatedName);
        if (translatedName.compare(mutantString) == 0)
        {
          mutantString = node->symbol()->FILE->NAME;
          replace(regexp.re, regexp.with, mutantString,
                                   translatedName);
          if (translatedName.compare(mutantString) == 0)
            continue;
        }

        CollapsedSymbols::iterator csi = m_symbols.find(translatedName);
        if (csi != m_symbols.end())
          node->setSymbol(csi->second);

        SymbolInfo *newInfo = new SymbolInfo(translatedName.c_str(),
                                             node->symbol()->FILE, 0);
        m_symbols.insert(CollapsedSymbols::value_type(translatedName,
                                                        newInfo));
        node->setSymbol(newInfo);

        break;
      }
#else
  void convertSymbol(NodeInfo */*node*/)
    {
#endif
    }

  CollapsedSymbols m_symbols;
private:
  std::vector<Regexp> m_regexps;
#ifdef PCRE_FOUND
  //Find regexp match from a string and replace the match with string "with"
  void replace(const pcre *re, const std::string &with, const std::string &subject,
               std::string &result)
  {
    result.clear();
    result.reserve(subject.size());
    int ovector[30];
    int options = 0;
    int subjectLength = subject.length();
    int rc = pcre_exec(re, NULL, subject.c_str(), subjectLength, 0 ,options,
                       ovector, 30);

    //no match or matching error, do nothing
    if (rc < 0)
    {
      result = subject;
      return;
    }
    // construct replacement string
    std::string replacement;
    replacement.reserve(with.size());
    std::string::const_iterator it;
    for ( it = with.begin(); it != with.end(); ++it)
    {
      //If the string with contains \* include the correct substring into
      //replacement string.
      //  \0 == ovector[0] to ovector[1]
      //  \1 == ovector[2] to ovector[3]
      //  \2 == ovector[4] to ovector[5]
      //  ...
      //  \9 == ovector[2*9] to ovector[2*9+1]
      if (*it == '\\')
      {
        ++it;
        //Check if there is enough substrings
        if (rc < (*it - '/'))
          die("Regexp error: Not enough substrings\n");
        replacement.append(subject.substr(ovector[2*(*it - '0')],
                           ovector[2*(*it - '0')+1]-ovector[2*(*it - '0')]));
      }
      else
        replacement.push_back(*it);
    }
    //construct first part of the result string
    result.append(subject.substr(0, ovector[0]));
    result.append(replacement);
    int endOfMatch = ovector[1];

    //Find posible later matches and contruct the result
    for(;;)
    {
      if (ovector[0] == ovector[1])
      {
        if (ovector[0] == subjectLength)
        {
          result.append(subject.substr(endOfMatch));
          break;
        }
        options = PCRE_NOTEMPTY | PCRE_ANCHORED;
      }
      // Find new matches.
      rc = pcre_exec(re, NULL, subject.c_str(), subjectLength, endOfMatch,
                     options, ovector, 30);

      //No new matches. Add the the tail of the subject into result.
      if (rc < 0)
      {
        result.append(subject.substr(endOfMatch));
        break;
      }
      //Add next part of the subject to result
      result.append(subject.substr(endOfMatch, ovector[0] - endOfMatch));

      //Construct new replacement string and append it to the result
      replacement.clear();
      replacement.reserve(with.size());
      for ( it = with.begin(); it != with.end(); ++it)
      {
        if (*it == '\\')
        {
          ++it;
          if (rc < (*it - '/'))
            die("Regexp error: Not enough substrings\n");
          replacement.append(subject.substr(ovector[2*(*it - '0')],
                ovector[2*(*it - '0')+1]-ovector[2*(*it - '0')]));
        }
        else
          replacement.push_back(*it);
      }
      result.append(replacement);

      //Mark the endOf this match
      endOfMatch = ovector[1];
    }
  }
#endif
};

/** Filter to merge use by C++ std namespace entities to parents.
 */
class RemoveStdFilter : public IgProfFilter
{
public:
  RemoveStdFilter(bool isMax)
  :m_isMax(isMax)
  {}

  virtual void post(NodeInfo *parent,
                    NodeInfo *node)
    {
      if (!parent)
        return;
      assert(node);
      assert(node->originalSymbol());
      // Check if the symbol refers to a definition in the c++ "std" namespace.
      const char *symbolName = node->originalSymbol()->NAME.c_str();

      if (*symbolName++ != '_' || *symbolName++ != 'Z')
        return;
      if (strncmp(symbolName, "NSt", 3) || strncmp(symbolName, "St", 2))
        return;

      // Yes it was.  Squash resource usage to the caller and hide this
      // function from the call tree.(Note that the std entry may end
      // up calling something in the user space again, so we don't want
      // to lose that information.)
      std::cerr << "Symbol " << node->originalSymbol()->NAME << " is "
                << " in " << node->originalSymbol()->FILE->NAME
                << ". Merging." << std::endl;
      mergeToNode(parent, node, m_isMax);
    }
  virtual std::string name(void) const { return "remove std"; }
  virtual enum FilterType type(void) const { return POST; }
private:
  bool m_isMax;
};

/** Structure to keep track of the cost of a given edge. */
class CallInfo
{
public:
  int64_t     VALUES[3];
  SymbolInfo  *SYMBOL;
  Ranges      RANGES;

  CallInfo(SymbolInfo *symbol)
    : SYMBOL(symbol)
    {
      memset(VALUES, 0, sizeof(VALUES));
    }
};

template<class T>
struct CompareBySymbol
{
  bool operator()(T *a, T *b) const
    {
      return a->SYMBOL < b->SYMBOL;
    }
};

class FlatInfo;
typedef std::map<SymbolInfo *, FlatInfo *> FlatInfoMap;

class FlatInfo
{
public:
  typedef std::set<CallInfo*, CompareBySymbol<CallInfo> > Calls;
  typedef std::set<SymbolInfo *> Callers;

  CallInfo *getCallee(SymbolInfo *symbol, bool create=false)
    {
      static CallInfo dummy(0);
      assert(symbol);
      dummy.SYMBOL = symbol;
      Calls::const_iterator i = CALLS.find(&dummy);
      if (i != CALLS.end())
        return *i;
      if (!create)
        return 0;

      CallInfo *callInfo = new CallInfo(symbol);
      this->CALLS.insert(callInfo);
      return callInfo;
    }

  static FlatInfo *getInMap(FlatInfoMap *flatMap, SymbolInfo *sym, bool create=true)
    {
      FlatInfoMap::iterator i = flatMap->find(sym);
      if (i != flatMap->end())
        return i->second;

      if (!create)
        return 0;

      FlatInfo *result = new FlatInfo(sym);
      flatMap->insert(FlatInfoMap::value_type(sym, result));
      return result;
    }

  std::string filename(void)
    {
      assert(SYMBOL);
      assert(SYMBOL->FILE);
      return SYMBOL->FILE->NAME;
    }

  const char *name(void)
    {
      assert(SYMBOL);
      return SYMBOL->NAME.c_str();
    }

  Callers CALLERS;
  Calls CALLS;
  SymbolInfo *SYMBOL;
  int DEPTH;
  int rank(void) {
    assert(SYMBOL);
    return SYMBOL->rank();
  }
  void setRank(int rank) {
    assert(SYMBOL);
    SYMBOL->setRank(rank);
  }

  int64_t  SELF_KEY[3];
  int64_t  CUM_KEY[3];
  Ranges   SELF_RANGES;
  Ranges   CUM_RANGES;
protected:
  FlatInfo(SymbolInfo *symbol)
    : SYMBOL(symbol), DEPTH(-1)
  {
    memset(SELF_KEY, 0, sizeof(SELF_KEY));
    memset(CUM_KEY, 0, sizeof(CUM_KEY));
  }
};

class SymbolInfoFactory
{
public:
  typedef std::map<std::string, SymbolInfo *> SymbolsByName;

  /**
      Initialises the SymbolInfoFactory. In particular,
      it reads the $PATH variable and saves the splitted
      filenames in one single place.
    */
  SymbolInfoFactory(ProfileInfo *prof, bool useGdb)
    : m_prof(prof), m_useGdb(useGdb)
    {
      char *paths = 0;
      if (const char *p = getenv("PATH"))
        paths = strdup(p);
      if (! paths)
        return;

      size_t npaths = 1;
      for (char *p = strchr(paths, ':'); p; p = strchr(p+1, ':'))
        ++npaths;

      char *bkpt, *path;
      std::string tmpPath;
      m_paths.reserve(npaths);
      for (path = strtok_r(paths, ":", &bkpt);
           path;
           path = strtok_r(0, ":", &bkpt))
      {
        if (! path[0])
          m_paths.push_back("./");
        else
        {
          tmpPath = path;
          if (tmpPath[tmpPath.size() - 1] != '/')
            tmpPath += "/";

          m_paths.push_back(tmpPath);
        }
      }
      free(paths);
    }

  SymbolInfo *getSymbol(unsigned int id)
    {
      assert(id <= m_symbols.size());
      return m_symbols[id];
    }

  FileInfo *getFile(unsigned int id)
    {
      assert(id <= m_files.size());
      return m_files[id];
    }

  FileInfo *createFileInfo(const std::string &origname, unsigned int fileid)
    {
      if ((m_files.size() >= fileid + 1) && m_files[fileid] == 0)
        die("Error in igprof input file.");

      struct stat st;
      bool found = false;
      bool useGdb = false;
      std::string abspath;

      if (origname.empty())
      {
        abspath = "<dynamically generated>";
        found = true;
      }

      // Make origname full, absolute, symlink-resolved path name.  If it is
      // a bare name with no path components, search in $PATH, otherwise just
      // resolve it to full path.  Ignore non-files while searching.
      if (! found && origname.find('/') == std::string::npos)
        for (size_t i = 0, e = m_paths.size(); i != e && ! found; ++i)
        {
          abspath = m_paths[i];
          abspath += origname;
          found = (access(abspath.c_str(), R_OK) == 0
                   && stat(abspath.c_str(), &st) == 0
                   && S_ISREG(st.st_mode));
        }

      if (! found)
        abspath = origname;

      if (char *p = realpath(abspath.c_str(), 0))
      {
	if (access(p, R_OK) == 0 && stat(p, &st) == 0 && S_ISREG(st.st_mode))
          useGdb = m_useGdb;

        abspath = p;
        free(p);
      }

      FilesByName::iterator fileIter = m_namedFiles.find(abspath);
      if (fileIter != m_namedFiles.end())
        return fileIter->second;
      else
        return insertFileInfo(fileid, abspath, useGdb);
    }

  /**
      Creates a SymbolInfo object using the information read from @a parser.
    */
  SymbolInfo *createSymbolInfo(std::string &symname, size_t fileoff, FileInfo *file, unsigned int symid)
    {
      // Regular expressions matching the file and symbolname information.
      symlookup(file, fileoff, symname, m_useGdb);

      SymbolInfoFactory::SymbolsByName::iterator symiter = namedSymbols().find(symname);

      if (symiter != namedSymbols().end())
      {
        assert(symiter->second);
        if (m_symbols.size() < symid+1)
          m_symbols.resize(symid+1);

        m_symbols[symid] = symiter->second;
        assert(getSymbol(symid) == symiter->second);
        return symiter->second;
      }

      SymbolInfo *sym = new SymbolInfo(symname.c_str(), file, fileoff);
      namedSymbols().insert(SymbolInfoFactory::SymbolsByName::value_type(symname, sym));
      assert(symid >= m_symbols.size());
      m_symbols.resize(symid + 1);
      m_symbols[symid] = sym;
      return sym;
    }


  static SymbolsByName &namedSymbols(void)
    {
      static SymbolsByName s_namedSymbols;
      return s_namedSymbols;
    }

private:

  /** Helper to insert a given fileinfo entry into the map.
    */
  FileInfo  *insertFileInfo(size_t fileid, const std::string &name, bool useGdb)
  {
    FileInfo *file = new FileInfo(name, useGdb);
    m_namedFiles.insert(FilesByName::value_type(name, file));
    int oldsize = m_files.size();
    int missingSize = fileid + 1 - oldsize;
    if (missingSize > 0)
    {
      m_files.resize(fileid + 1);
      for (int i = oldsize; i < oldsize + missingSize; i++)
        assert(m_files[i] == 0);
    }
    m_files[fileid] = file;
    return file;
  }

  typedef std::vector<FileInfo *> Files;
  typedef std::map<std::string, FileInfo *> FilesByName;
  typedef std::vector<SymbolInfo *> Symbols;
  Files m_files;
  Symbols m_symbols;
  FilesByName m_namedFiles;
  ProfileInfo *m_prof;
  bool m_useGdb;
  std::vector<std::string>      m_paths;
};

struct SuffixOps
{
  static void splitSuffix(const std::string &fullSymbol,
                          std::string &oldSymbol,
                          std::string &suffix)
    {
      size_t tickPos = fullSymbol.rfind("'");
      if (tickPos == std::string::npos)
      {
        oldSymbol = fullSymbol;
        suffix = "";
        return;
      }
      assert(tickPos < fullSymbol.size());
      oldSymbol.assign(fullSymbol.c_str(), tickPos - 1);
      suffix.assign(fullSymbol.c_str() + tickPos);
    }

  static std::string removeSuffix(const std::string &fullSymbol)
    {
      size_t tickPos = fullSymbol.rfind("'");
      if (tickPos == std::string::npos)
        return fullSymbol;
      assert(tickPos < fullSymbol.size());
      return std::string(fullSymbol.c_str(), tickPos - 1);
    }
};

class MassifTreeBuilder : public IgProfFilter
{
public:
  MassifTreeBuilder(Configuration *)
  :m_indent(0),
   m_totals(0),
   m_sorter(false),
   m_reverseSorter(true)
  {
  }

  virtual void pre(NodeInfo *parent, NodeInfo *node)
  {
    std::sort(node->CHILDREN.begin(), node->CHILDREN.end(), m_reverseSorter);

    Counter &nodeCounter = node->COUNTER;

    float pct = 0;
    if (!parent)
      m_totals = nodeCounter.ccnt;

    pct = percent(nodeCounter.ccnt, m_totals);

    // Determine which children are above threshold.
    // Sum up the contribution of those below threshold.
    // FIXME: Add a new node called "others" to the list
    //        with the aggregated sum.
    int lastPrinted = -1;
    float others = 0.;
    for (size_t i = 0, e = node->CHILDREN.size(); i != e; i++)
    {
      Counter &childCounter = node->CHILDREN[i]->COUNTER;
      float childPct = percent(childCounter.ccnt, m_totals);
      if ((childPct < 1. && pct < 1.) || childPct < 0.1)
        others += childPct;
      else
        lastPrinted++;
    }

    if ((size_t)(lastPrinted + 1) != node->CHILDREN.size())
      node->CHILDREN.resize(lastPrinted + 1);

    std::sort(node->CHILDREN.begin(), node->CHILDREN.end(), m_sorter);

    if (m_kids.size() <= m_indent)
      m_kids.resize(m_indent+1);

    m_kids[m_indent] = node->CHILDREN.size();

    for (int i = 0, e = m_kids.size() - 1; i < e; i++)
    {
      std::cout << " ";
      if (m_kids[i] == 0)
        std::cout << " ";
      else
        std::cout << "|";
    }

    if (nodeCounter.cnt)
      std::cout << "->(" << std::fixed << std::setprecision(2) << pct << ") ";

    if (node->symbol() && !node->symbol()->NAME.empty())
      std::cout << node->symbol()->NAME;
    else
      std::cout << "<no symbol>";

    // After the node gets printed, it's parent has one kid less.
    if (m_indent)
      m_kids[m_indent-1]--;

    std::cout << std::endl;
    m_indent++;
  }

  virtual void post(NodeInfo *, NodeInfo *)
  {
    m_indent--;
    m_kids.resize(m_indent);
  }

  virtual std::string name() const { return "massif tree"; }
  virtual enum FilterType type() const { return BOTH; }
private:

  struct SortByCumulativeCount
  {
    SortByCumulativeCount(bool reverse)
    :m_reverse(reverse)
    {
    }

    bool operator()(NodeInfo * const&a, NodeInfo * const&b)
    {
      if (!a)
        return !m_reverse;
      if (!b)
        return m_reverse;
      Counter &ca = a->COUNTER;
      Counter &cb = b->COUNTER;

      return (ca.ccnt < cb.ccnt) ^ m_reverse;
    }
    bool                  m_reverse;
  };

  size_t                m_indent;
  int64_t               m_totals;
  std::vector<size_t>   m_kids;
  SortByCumulativeCount m_sorter;
  SortByCumulativeCount m_reverseSorter;
};

/** This filter navigates the tree, finds out which
    are the top N contributions for a given counter
    and reports them together with their stacktrace.

    While parsing the tree it maintains a stacktrace
    and if a leaf has a self counter which enters
    in the "top N", it saves the stacktrace.
*/
class TopNBuilderFilter : public IgProfFilter
{
public:
  typedef std::vector<NodeInfo *> StackTrace;

  TopNBuilderFilter(size_t rankingSize)
  :m_rankingSize(rankingSize)
  {
    m_topNValues = new int64_t[rankingSize];
    memset(m_topNValues, 0, sizeof(int64_t) * rankingSize);
    m_topNStackTrace = new StackTrace[rankingSize];
  }

  ~TopNBuilderFilter()
  {
    delete[] m_topNValues;
    delete[] m_topNStackTrace;
  }

  virtual void pre(NodeInfo *, NodeInfo *node)
  {
    // If a node has children, simply record it on the stacktrace
    // and navigate further.
    m_currentStackTrace.push_back(node);

    // If the node does not have children, compare its "self" values
    // against the topten, find the minimum of the top ten and
    // if it is smaller than the counter, replace the
    Counter &nodeCounter = node->COUNTER;

    int min = -1;
    for (size_t i = 0; i != m_rankingSize; ++i)
      if (m_topNValues[i] < nodeCounter.cnt
          && (min == -1 || m_topNValues[i] < m_topNValues[min]))
        min = i;

    if (min == -1)
      return;

    m_topNValues[min] = nodeCounter.cnt;
    m_topNStackTrace[min].resize(m_currentStackTrace.size());
    std::copy(m_currentStackTrace.begin(),
              m_currentStackTrace.end(),
              m_topNStackTrace[min].begin());
  }

  virtual void post(NodeInfo * /*parent*/, NodeInfo * /*node*/)
  {
    if (!m_currentStackTrace.empty())
      m_currentStackTrace.pop_back();
  }

  StackTrace &stackTrace(size_t pos, int64_t &value)
  {
    std::vector<std::pair<int64_t, size_t> > sorted;
    for (size_t i = 0; i != m_rankingSize; ++i)
      sorted.push_back(std::make_pair(m_topNValues[i], i));

    std::sort(sorted.begin(), sorted.end(), Cmp());
    value = m_topNValues[sorted[pos].second];
    return m_topNStackTrace[sorted[pos].second];
  }

  virtual std::string name() const { return "top N builder"; }
  virtual enum FilterType type() const { return BOTH; }

private:
  struct Cmp
  {
    bool operator()(const std::pair<int64_t, size_t> &a, const std::pair<int64_t, size_t> &b)
    {
      return b.first < a.first;
    }
  };

  size_t                    m_rankingSize;
  int64_t                   *m_topNValues;
  StackTrace                *m_topNStackTrace;
  std::vector<NodeInfo *>   m_currentStackTrace;
};

class TreeMapBuilderFilter : public IgProfFilter
{
public:
  TreeMapBuilderFilter(bool isMax, ProfileInfo *prof)
    :m_prof(prof), m_flatMap(new FlatInfoMap), m_firstInfo(0), m_isMax(isMax)
    {
    }

  /** Creates the GPROF like output.

  * Gets invoked with a node and it's parent.
  * Finds a unique name for the node, keeping into account recursion.
  * Gets the unique FlatInfo associated to the symbols. Recursive calls result in same symbol.
  * Calculates the depths in the tree of the node.
  * Gets the relevant counter associated to the node.
  * For the case in which there is a parent.
  * Get the counter for the parent.
  * Find the parent unique symbol.
  * Find the parent unique FlatInfo.
  * Find the FlatInfo associated to the node, when it was called by the parent.
  * If the above does not exist, create it and insert it among those which can be reached from the parent.
  * Accumulate counts.
  */
  virtual void pre(NodeInfo *parent, NodeInfo *node)
    {
      assert(node);
      SymbolInfo *sym = symfor(node);
      assert(sym);
      FlatInfo *symnode = FlatInfo::getInMap(m_flatMap, sym);
      assert(symnode);

      if (!m_firstInfo)
        m_firstInfo = symnode;

      if (symnode->DEPTH < 0 || int(m_seen.size()) < symnode->DEPTH)
        symnode->DEPTH = int(m_seen.size());

      Counter &nodeCounter = node->COUNTER;

      if (parent)
      {
        SymbolInfo *parsym = parent->symbol();
        FlatInfo *parentInfo = FlatInfo::getInMap(m_flatMap, parsym, false);
        assert(parentInfo);

        symnode->CALLERS.insert(parsym);

        CallInfo *callInfo = parentInfo->getCallee(sym, true);

        accumulateCounts(callInfo->VALUES, nodeCounter.ccnt, nodeCounter.cfreq);
        mergeRanges(callInfo->RANGES, node->CUM_RANGES);
      }

      // Do SELF_KEY
      accumulateCounts(symnode->SELF_KEY, nodeCounter.cnt, nodeCounter.freq);
      mergeRanges(symnode->SELF_RANGES, node->RANGES);

      // Do CUM_KEY
      accumulateCounts(symnode->CUM_KEY, nodeCounter.ccnt, nodeCounter.cfreq);
      mergeRanges(symnode->CUM_RANGES, node->CUM_RANGES);
    }

  virtual void post(NodeInfo *,
                    NodeInfo *node)
    {
      assert(node);
      assert(node->symbol());
      if (m_seen.count(node->symbol()->NAME) <= 0)
        std::cerr << "Error: " << node->symbol()->NAME << std::endl;

      m_seen.erase(node->symbol()->NAME);
    }

  virtual std::string name() const { return "tree map builder"; }
  virtual enum FilterType type() const { return BOTH; }

  void getTotals(int64_t &totals, int64_t &totfreqs)
    {
      assert(m_firstInfo);
      totals = m_firstInfo->CUM_KEY[0];
      totfreqs = m_firstInfo->CUM_KEY[1];
    }

  FlatInfoMap *flatMap()
    {
      return m_flatMap;
    }
private:
  typedef std::map<std::string, SymbolInfo *>  SeenSymbols;

  void accumulateCounts(int64_t *buffer, int64_t c, int64_t f)
  {
    // Do CUM_KEY
    if (m_isMax)
    {
      if (buffer[0] < c)
        buffer[0] = c;
    }
    else
      buffer[0] += c;

    buffer[1] += f;
    buffer[2]++;
  }

  SymbolInfo *symfor(NodeInfo *node)
    {
      assert(node);
      SymbolInfo *reportSymbol = node->reportSymbol();
      if (reportSymbol)
      {
        m_seen.insert(SeenSymbols::value_type(reportSymbol->NAME,
                                              reportSymbol));
        return reportSymbol;
      }

      std::string suffix = "";

      assert(node->originalSymbol());
      std::string symbolName = node->originalSymbol()->NAME;

      SeenSymbols::iterator i = m_seen.find(symbolName);
      if (i != m_seen.end())
      {
        std::string newName = getUniqueName(symbolName);
        SymbolInfoFactory::SymbolsByName &namedSymbols = SymbolInfoFactory::namedSymbols();
        SymbolInfoFactory::SymbolsByName::iterator s = namedSymbols.find(newName);
        if (s == namedSymbols.end())
        {
          SymbolInfo *originalSymbol = node->originalSymbol();
          reportSymbol = new SymbolInfo(newName.c_str(),
                                        originalSymbol->FILE,
                                        originalSymbol->FILEOFF);
          namedSymbols.insert(SymbolInfoFactory::SymbolsByName::value_type(newName,
                                                                           reportSymbol));
        }
        else
          reportSymbol = s->second;
      }
      assert(node);
      node->reportSymbol(reportSymbol);
      assert(node->symbol());
      m_seen.insert(SeenSymbols::value_type(node->symbol()->NAME,
                                            node->symbol()));
      return node->symbol();
    }

  std::string getUniqueName(const std::string &symbolName)
    {
      int index = 2;
      std::string origname = SuffixOps::removeSuffix(symbolName);
      std::string candidate = origname;

      do
      {
        candidate = origname + "'" + toString(index++);
      } while (m_seen.find(candidate) != m_seen.end());
      return candidate;
    }

  ProfileInfo *m_prof;
  FlatInfoMap *m_flatMap;
  FlatInfo    *m_firstInfo;
  SeenSymbols m_seen;
  bool        m_isMax;
};

static void
opentemp(char *pattern, FILE *&fp)
{
  int fd;

  if ((fd = mkstemp(pattern)) < 0)
    die("%s: cannot create temporary file: %s (error %d)\n",
        pattern, strerror(errno), errno);

  if (! (fp = fdopen(fd, "w")))
    die("%s: cannot open descriptor %d for write: %s (error %d)\n",
        pattern, fd, strerror(errno), errno);

  setvbuf(fp, 0, _IOFBF, 128*1024);
}

static void
openpipe(char *&cmd, FILE *&fp, const char *pattern, const char *arg)
{
  assert(! cmd);
  asprintf(&cmd, pattern, arg);

  if (! cmd)
    die("%s (error %d)\n", strerror(errno), errno);

  if (! (fp = popen(cmd, "r")))
    die("Error while running '%s'\n", cmd);

  setvbuf(fp, 0, _IOFBF, 128*1024);
}

void
symremap(ProfileInfo &prof, std::vector<FlatInfo *> infos, bool usegdb, bool demangle)
{
  if (usegdb)
  {
    std::multimap<FileInfo *, SymbolInfo *> fileAndSymbols;
    for (size_t ii = 0, ei = infos.size(); ii != ei; ++ii)
    {
      SymbolInfo *sym = infos[ii]->SYMBOL;

      if (!sym || !sym->FILE)
        continue;

      // Only symbols that are marked to be lookup-able by gdb will be looked
      // up. This excludes, for example dynamically generated symbols or
      // symbols from files that are not in path anymore.
      if (!sym->FILE->canUseGdb())
        continue;

      if (!sym->FILEOFF || sym->FILE->NAME.empty())
        continue;

      // We lookup with gdb only those symbols that cannot be
      // resolved correctly via nm.
      if (sym->FILE->symbolByOffset(sym->FILEOFF))
        continue;

      fileAndSymbols.insert(std::pair<FileInfo *, SymbolInfo *>(sym->FILE, sym));
    }

    FILE *fp = 0;
    char *cmd = 0;
    char fname[] = "/tmp/igprof-analyse.gdb.XXXXXXXX";
    FileInfo *prevfile = 0;
    opentemp(fname, fp);
    fputs("set width 10000\n", fp);

    for (std::multimap<FileInfo *, SymbolInfo *>::iterator i = fileAndSymbols.begin();
         i != fileAndSymbols.end();
         i++)
    {
      SymbolInfo *sym = i->second;
      FileInfo *fileInfo = i->first;

      assert(sym);
      assert(fileInfo);
      assert(!fileInfo->NAME.empty());

      prof.symcache().insert(sym);

      if (strncmp(fileInfo->NAME.c_str(), "<dynamically", 12))
      {
        if (fileInfo != prevfile)
        {
          fprintf(fp, "file %s\n", fileInfo->NAME.c_str());
          prevfile = fileInfo;
        }

        fprintf(fp, "echo IGPROF_SYMCHECK <%" PRId64 ">\\n\ninfo line *%" PRId64 "\n",
                (int64_t) sym, sym->FILEOFF);
      }
    }

    fclose(fp);

    openpipe(cmd, fp, "gdb --batch --command=%s", fname);
    IgTokenizer t(fp, cmd);
    std::string oldname;
    std::string suffix;
    std::string result;
    SymbolInfo *sym = 0;

    while (!feof(fp))
    {
      result.clear();
      t.getTokenS(result, "<\n");
      // In case < is not found in the line, we skip it.
      if (t.nextChar() == '\n')
      {
        t.skipEol();
        continue;
      }
      else if (!strncmp(result.c_str(), "IGPROF_SYMCHECK", 15))
      {
        t.skipChar('<');
        int64_t symid = t.getTokenN('>');
        t.skipEol();
        ProfileInfo::SymCache::iterator symitr = prof.symcache().find((SymbolInfo *)(symid));
        assert(symitr !=prof.symcache().end());
        sym = *symitr;
        SuffixOps::splitSuffix(sym->NAME, oldname, suffix);
      }
      else if (!strncmp(result.c_str(), "Line", 4)
               || !strncmp(result.c_str(), "No line number", 14))
      {
        t.skipChar('<');
        assert(sym);
	t.getTokenS(sym->NAME, "+@>");
        t.getToken(">");
        t.skipChar('>');
        t.getToken("\n");
        t.skipEol();
        sym->NAME += suffix;
        sym = 0; suffix = "";
      }
      else
        t.syntaxError();
    }

    unlink(fname);
    pclose(fp);
    free(cmd);
  }

  if (demangle)
  {
    FILE *fp = 0;
    char *cmd = 0;
    char fname[] = "/tmp/igprof-analyse.c++filt.XXXXXXXX";

    opentemp(fname, fp);
    for (size_t ii = 0, ei = infos.size(); ii != ei; ++ii)
    {
      SymbolInfo *symbol = infos[ii]->SYMBOL;
      assert(symbol);
      fprintf(fp, "%" PRIdPTR ": %s\n", (intptr_t) symbol, symbol->NAME.c_str());
    }
    fclose(fp);

    openpipe(cmd, fp, "c++filt < %s", fname);
    IgTokenizer t(fp, cmd);
    while (!feof(fp))
    {
      SymbolInfo *symbolPtr = (SymbolInfo *)(t.getTokenN(':'));
      t.skipChar(' ');
      t.getTokenS(symbolPtr->NAME, "\n");
      t.skipEol();
    }

    unlink(fname);
    pclose(fp);
    free(cmd);
  }
}

/**
    Reads a dump and fills in ProfileInfo with the needed information.
  */
void
IgProfAnalyzerApplication::readDump(ProfileInfo *prof,
                                    const std::string &filename,
                                    StackTraceFilter *filter)
{
  std::vector<NodeInfo *> nodestack;
  nodestack.reserve(IGPROF_MAX_DEPTH);

  ProfileInfo::Nodes      &nodes = prof->nodes();

  int base = 10;
  bool isPipe = false;
  FILE *inFile = openDump(filename.c_str(), isPipe);
  IgTokenizer t(inFile, filename.c_str());
  verboseMessage("Parsing igprof output file", filename.c_str());

  // Parse the header line, which has form:
  // ^P=\(ID=[0-9]* N=\(.*\) T=[0-9]+.[0-9]*\)
  t.skipString("P=(");
  if (t.nextChar() == 'H')
  {
    base = 16;
    t.skipString("HEX ");
  }
  t.skipString("ID=");
  t.getToken(" ");
  t.skipString(" N=(");
  t.getToken(")");
  t.skipString(") T=");
  m_tickPeriod = t.getTokenD(')');
  t.skipEol();

  SymbolInfoFactory symbolsFactory(prof, m_config->useGdb);

  // A vector whose i-th element specifies whether or
  // not the counter file id "i" is a key.
  std::vector<bool> keys;

  // A vector keeping track of all the pages touched by the
  // allocations. We store here the address of the page,
  // in order to improve lookup whether or not a page is
  // already there. This way the total number of pages
  // touched is given by "count".
  std::vector<RangeInfo> ranges;
  ranges.reserve(20000);

  // String to hold the name of the function.
  std::string fn;
  std::string ctrname;

  // One node per line.
  while (! feof(inFile))
  {
    printProgress();

    // Determine node stack level matching "^C\d+ ".
    t.skipChar('C');

    int64_t newPosition = t.getTokenN(' ', base) - 1;

    if (newPosition < 0)
      t.syntaxError();

    int64_t stackSize = nodestack.size();
    if (newPosition > stackSize)
      die("Internal error on line %d", t.lineNum());

    int64_t difference = newPosition - stackSize;
    if (difference > 0)
      nodestack.resize(newPosition);
    else
      nodestack.erase(nodestack.begin() + newPosition, nodestack.end());

    // Find out the information about the current stack line.
    SymbolInfo *sym = 0;

    // Match either a function reference "FN[0-9]" followed by = or +.
    t.skipString("FN", 2);
    int64_t symid = t.getTokenN("+=", base);

    // If this is previously unseen symbol, parse full definition.
    // Otherwise look up the previously recorded symbol object.
    if (t.nextChar() == '=')
    {
      // Match file reference "=\(F(\d+)\+(-?\d+) N=\((.*?)\)\)\+\d+\s*\)"
      // or definition "=\(F(\d+)=\((.*?)\)\+(-?\d+) N=\((.*?)\)\)\+\d+\s*\)"
      // and create a symbol info accordingly.
      t.skipString("=(F");
      FileInfo *fileinfo = 0;
      std::string symname;
      size_t fileId = t.getTokenN("+=", base);

      // If we are looking at a new file definition, get file name.
      // Otherwise retrieve the previously recorded file object.
      if (t.nextChar() == '=')
      {
        t.skipString("=(", 2);
        t.getTokenS(fn, ')');
        fileinfo = symbolsFactory.createFileInfo(fn, fileId);
      }
      else
        fileinfo = symbolsFactory.getFile(fileId);

      // Get the file offset.
      t.skipChar('+');
      int64_t fileoff = t.getTokenN(' ', base);

      // Read the symbol name and offset.
      t.skipString("N=(", 3);
      t.getTokenS(symname, ')');
      if (symname == "@?(nil")
      {
        symname = "@?(nil)";
        t.skipChar(')');
      }

      sym = symbolsFactory.createSymbolInfo(symname, fileoff, fileinfo, symid);
      t.skipChar(')');
    }
    else if (! (sym = symbolsFactory.getSymbol(symid)))
      die("symbol %" PRId64 " referenced before definition\n", symid);

    // Skip unused symbol offset.
    t.skipChar('+');
    t.getTokenN(" \n", base);

    // Process this stack node.
    NodeInfo *parent = nodestack.empty() ? prof->spontaneous() : nodestack.back();
    NodeInfo *child = parent ? parent->getChildrenBySymbol(sym) : 0;

    if (!child)
    {
      // Nodes are allocated in a deque, to maximize locality
      // and reduce the actual number of allocations.
      m_nodesStorage.resize(m_nodesStorage.size() + 1);
      child = &(m_nodesStorage.back());
      child->setSymbol(sym);
      nodes.push_back(child);
      if (parent)
        parent->CHILDREN.push_back(child);
    }

    nodestack.push_back(child);

    // Parse counters, possibly with leaks attached.
    while (t.nextChar() != '\n')
    {
      t.skipString(" V", 2);
      size_t ctrId = t.getTokenN(":=", base);

      // Check if we are defining a new counter and possibly register it,
      // if it is of the kind we are interested in.
      if (t.nextChar() == '=')
      {
        // Get the counter name.
        t.skipString("=(", 2);
        t.getTokenS(ctrname, ')');

        // The first counter we meet, we make it the key, unless
        // the key was already set on command line.
        if (m_key.empty())
          setKey(ctrname);

        // Store information about ctrId being a key or not.
        if (keys.size() <= ctrId)
          keys.resize(ctrId + 1, false);
        keys[ctrId] = (ctrname == m_key);
      }

      // Get the counter counts.
      t.skipString(":(", 2);
      int64_t ctrfreq = t.getTokenN(',', base);
      int64_t ctrvalNormal = t.getTokenN(',', base);
      int64_t ctrvalPeak = t.getTokenN(')', base);

      // Record if we are interested in something related to this counter.
      if (keys[ctrId])
      {
        int64_t ctrval = m_config->normalValue() ? ctrvalNormal : ctrvalPeak;

        if (filter)
          filter->filter(sym, ctrval, ctrfreq);

        child->COUNTER.cnt += ctrval;
        child->COUNTER.freq += ctrfreq;
      }

      // Parse leaks of the form ;LK=\(0x[\da-f]+,\d+)* if any.
      // Theoretically the format allows leaks per counter, but
      // we only support leaks for one counter at a time.
      ranges.clear();
      while (t.nextChar() == ';')
      {
        t.skipString(";LK=(0x", 7);

        // Get the leak address and size.
        int64_t leakAddress = t.getTokenN(',', 16);
        int64_t leakSize = t.getTokenN(')', base);

        // In the case we specify one of the --show-pages --show-page-ranges
        // or --show-locality-metrics options, we keep track
        // of the page ranges that are referenced by all the allocations
        // (LK counters in the report).
        //
        // We first fill a vector with all the ranges, then we sort it later on
        // collapsing all the adjacent ranges.
        if (m_showPages  || m_config->dumpAllocations || m_showPageRanges)
        {
          assert(leakSize > 0);
          ranges.push_back(RangeInfo(leakAddress, (leakAddress + leakSize + 1)));
          RangeInfo &range = ranges.back();
          if (m_showPages || m_showPageRanges)
          {
            range.startAddr = range.startAddr >> 12;
            range.endAddr = range.endAddr >> 12;
          }
          assert(range.size() > 0);
        }
      }

      // Sort the leak ranges and collapse them.
      if (! ranges.empty() && keys[ctrId])
      {
        std::sort(ranges.begin(), ranges.end());
        mergeSortedRanges(ranges);
        mergeRanges(child->RANGES, ranges);
      }
    }

    // We should be looking at end of line now.
    t.skipEol();
  }

  if (isPipe)
    pclose(inFile);
  else
    fclose(inFile);

  verboseMessage(0, 0, " done\n");
  if (keys.empty())
    die("No counter values in profile data.");
}

struct StackItem
{
  NodeInfo *parent;
  NodeInfo *pre;
  NodeInfo *post;
};

void walk(NodeInfo *first, size_t total, IgProfFilter *filter=0)
{
  // TODO: Apply more than one filter at the time.
  //     This method applies one filter at the time. Is it worth to do
  //     the walk only once for all the filters? Should increase locality
  //     as well...
  assert(filter);
  assert(first);
  std::vector<StackItem> stack;
  stack.resize(1);
  StackItem &firstItem = stack.back();
  firstItem.parent = 0;
  firstItem.pre = first;
  firstItem.post = 0;
  stack.reserve(10000);

  size_t count = 0;
  size_t newProgress = 0;
  size_t oldProgress = 0;

  while (!stack.empty())
  {
    StackItem &item = stack.back();
    NodeInfo *parent = item.parent, *pre = item.pre, *post = item.post;
    stack.pop_back();
    if (pre)
    {
      if (filter->type() & IgProfFilter::PRE)
      {
        filter->pre(parent, pre);
        ++count;
      }
      if (filter->type() & IgProfFilter::POST)
      {
        StackItem newItem;
        newItem.parent = parent;
        newItem.pre = 0;
        newItem.post = pre;
        stack.push_back(newItem);
      }
      // Add all the children of pre as items in the stack.
      for (size_t ci = 0, ce = pre->CHILDREN.size(); ci != ce; ++ci)
      {
        assert(pre);
        NodeInfo *child = pre->CHILDREN[ci];
        StackItem newItem;
        newItem.parent = pre;
        newItem.pre = child;
        newItem.post = 0;
        stack.push_back(newItem);
      }
    }
    else
    {
      filter->post(parent, post);
      ++count;
    }
    newProgress = (100 * count) / total;
    if (newProgress > oldProgress)
    {
      verboseMessage(0, 0, ".");
      oldProgress = newProgress;
    }
  }
}

#ifdef PCRE_FOUND
// Modified version of walk function to go trough call tree and implement
// merge-ancestors filter
void walk_ancestors(NodeInfo *first, AncestorsSpec specs)
{
  typedef std::map<std::string, SymbolInfo *> CollapsedSymbols;
  CollapsedSymbols m_symbols;

  // Compile regular expressions listed in AncestorsSpec.
  std::vector<pcre *> ancestor_list;
  std::string replace = specs.with;
  ancestor_list.resize(specs.ancestors.size());
  for (size_t i = 0, end = specs.ancestors.size(); i != end; i++)
  {
    const char *errptr = 0;
    int erroff = 0;
    ancestor_list[i] = pcre_compile(specs.ancestors[i].c_str(), 0, &errptr, &erroff, 0);
    if (errptr)
    {
      std::cerr << "Error while compiling regular expression" << std::endl;
      exit(1);
    }
  }

  // Add the first node (spontaneous) into call stack
  std::vector<StackItem> stack;
  stack.resize(1);
  StackItem &firstItem = stack.back();
  firstItem.parent = 0;
  firstItem.pre = first;
  firstItem.post = 0;
  stack.reserve(10000);

  // variables for pcre_exec
  std::string symbolname;
  int ovector[9];
  int rc = 0;

  // Keeps track matches
  int control = 0;
  int limit = ancestor_list.size() - 1;

  // Go trough stack
  while (!stack.empty())
  {
    StackItem &item = stack.back();
    NodeInfo *parent = item.parent, *pre = item.pre;
    stack.pop_back();

    // First time all nodes go trough pre.
    if (pre)
    {

      // Check if the the node's symbol name matches to ancestor_list
      symbolname = pre->symbol()->NAME;
      rc = -1;
      if (limit >= control)
      {
        rc = pcre_exec(ancestor_list[control], NULL, symbolname.c_str(),
                       strlen(symbolname.c_str()), 0, 0, ovector, 9);
      }
      else  // We are in the branch that has match below the match
      {
        std::string replace = specs.with.append(":");
        replace.append(symbolname);
        CollapsedSymbols::iterator csi = m_symbols.find(replace);
        if (csi != m_symbols.end())
          pre->setSymbol(csi->second);

        SymbolInfo *newInfo = new SymbolInfo(replace.c_str(),
                                             pre->symbol()->FILE, 0);
        m_symbols.insert(CollapsedSymbols::value_type(replace.c_str(),
                                                      newInfo));
        pre->setSymbol(newInfo);
      }

      // Match found
      if (rc > 0)
      {
        ++control;

        // The node is child of all ancestors
        if (control == (limit + 1))
        {
          CollapsedSymbols::iterator csi = m_symbols.find(specs.with);
          if (csi != m_symbols.end())
            pre->setSymbol(csi->second);

          SymbolInfo *newInfo = new SymbolInfo(specs.with.c_str(),
                                               pre->symbol()->FILE, 0);
          m_symbols.insert(CollapsedSymbols::value_type(specs.with.c_str(),
                                                        newInfo));
          pre->setSymbol(newInfo);
        }

        // re add the node to stack with pre = 0
        StackItem newItem;
        newItem.parent = parent;
        newItem.pre = 0;
        newItem.post = pre;
        stack.push_back(newItem);
      }

      // Add all the children of pre as items in the stack.
      for (size_t ci = 0, ce = pre->CHILDREN.size(); ci != ce; ++ci)
      {
        assert(pre);
        NodeInfo *child = pre->CHILDREN[ci];
        StackItem newItem;
        newItem.parent = pre;
        newItem.pre = child;
        newItem.post = 0;
        stack.push_back(newItem);
      }
    }
    // When coming back up the call tree this will be executed in nodes that
    // were matching some ancestor.
    else
      --control;
  }
#else
void walk_ancestors(NodeInfo * /*first*/, AncestorsSpec * /*specs*/)
{
#endif
}

void
IgProfAnalyzerApplication::prepdata(ProfileInfo& prof)
{
  for (size_t fi = 0, fe = m_filters.size(); fi != fe; ++fi)
  {
    IgProfFilter *filter = m_filters[fi];
    verboseMessage("Applying filter", filter->name().c_str());
    walk(prof.spontaneous(), m_nodesStorage.size(), filter);
    verboseMessage(0, 0, " done\n");
  }

  if (m_config->mergeLibraries())
  {
    //    walk<NodeInfo>(prof.spontaneous(), new PrintTreeFilter);

    verboseMessage("Merge nodes belonging to the same library");
    walk(prof.spontaneous(), m_nodesStorage.size(), new UseFileNamesFilter(m_keyMax));
    //    walk<NodeInfo>(prof.spontaneous(), new PrintTreeFilter);
    verboseMessage(0, 0, " done\n");
  }

  if (!m_regexps.empty())
  {
    verboseMessage("Merge nodes using user-provided regular expression");
    walk(prof.spontaneous(), m_nodesStorage.size(), new RegexpFilter(m_regexps, m_keyMax));
    verboseMessage(0, 0, " done\n");
  }

#ifdef PCRE_FOUND
  if (m_ancestors.ancestors.size() != 0)
  {
    verboseMessage("Merge nodes that has has ancestors matching to given list");
    walk_ancestors(prof.spontaneous(), m_ancestors);
    verboseMessage(0, 0, " done\n");
  }
#endif // PCRE_FOUND

  verboseMessage("Summing counters");
  walk(prof.spontaneous(), m_nodesStorage.size(), new AddCumulativeInfoFilter(m_keyMax));
  walk(prof.spontaneous(), m_nodesStorage.size(), new CheckTreeConsistencyFilter());
  verboseMessage(0, 0, " done\n");
}

class FlatInfoComparator
{
public:
  virtual ~FlatInfoComparator() {}
  FlatInfoComparator(int ordering)
    :m_ordering(ordering)
    {}
  bool operator()(FlatInfo *a, FlatInfo *b)
    {
      int64_t cmp;
      cmp = cmpnodekey(a, b);
      if (cmp > 0)
        return true;
      else if (cmp < 0)
        return false;
      cmp = cmpcallers(a, b);
      if (cmp > 0)
        return true;
      else if (cmp < 0)
        return false;
      return strcmp(a->name(), b->name()) < 0;
    }
protected:
  virtual int64_t cmpnodekey(FlatInfo *a, FlatInfo *b)
    {
      int64_t aVal = a->CUM_KEY[0];
      int64_t bVal = b->CUM_KEY[0];
      return  -1 * m_ordering * (llabs(bVal) - llabs(aVal));
    }

  int cmpcallers(FlatInfo *a, FlatInfo *b)
    {
      return b->DEPTH - a->DEPTH;
    }

  int m_ordering;
};

class FlatInfoComparatorWithBaseline : public FlatInfoComparator
{
public:
  FlatInfoComparatorWithBaseline(int ordering, FlatInfoMap *map)
    :FlatInfoComparator(ordering),
     m_baselineMap(map)
    {}

protected:
  virtual int64_t cmpnodekey(FlatInfo *a, FlatInfo *b)
    {
      int64_t aVal = a->CUM_KEY[0];
      int64_t bVal = b->CUM_KEY[0];

      FlatInfoMap::iterator origA = m_baselineMap->find(a->SYMBOL);
      FlatInfoMap::iterator origB = m_baselineMap->find(b->SYMBOL);

      int64_t aOrigVal = aVal;
      int64_t bOrigVal = bVal;

      if (origA != m_baselineMap->end())
        aOrigVal = origA->second->CUM_KEY[0];
      if (origB != m_baselineMap->end())
        bOrigVal = origB->second->CUM_KEY[0];

      return  -1 * m_ordering * ((bVal-bOrigVal)/bOrigVal - (aVal - aOrigVal)/aOrigVal);
    }
private:
  FlatInfoMap *m_baselineMap;
};

class GProfRow
{
public:
  int64_t FILEOFF;
  float   PCT;
  float   SELF_PCT;

  void initFromInfo(FlatInfo *info)
    {
      assert(info);
      m_info = info;
    }

  const char *name()
    {
      return m_info->name();
    }

  const char *filename()
    {
      return m_info->filename().c_str();
    }

  unsigned int depth()
    {
      return m_info->DEPTH;
    }

  unsigned int rank()
    {
      return m_info->rank();
    }

  intptr_t symbolId()
    {
      return(intptr_t)(m_info->SYMBOL);
    }

  intptr_t fileId()
    {
      return(intptr_t)(m_info->SYMBOL->FILE);
    }

private:
  FlatInfo *m_info;
};

/** Helper functor to print percentage numbers
    in different way, depending on whether we are in
    diff mode or not.

    @a mode true in case we are in diff mode,
            false otherwise.
*/
class PercentagePrinter
{
public:
  PercentagePrinter(bool mode)
  :m_mode(mode)
  {}

  void operator()(float value, const char *numeric = "%7.1f  ", const char *overflow = "    new  ")
  {
    if (m_mode && value == FLT_MAX)
      printf(overflow, "");
    else
      printf(numeric, value);
  }
private:
  bool m_mode;
};


class OtherGProfRow : public GProfRow
{
public:
  int64_t SELF_COUNTS;
  int64_t CHILDREN_COUNTS;
  int64_t SELF_CALLS;
  int64_t TOTAL_CALLS;
  int64_t SELF_PATHS;
  int64_t TOTAL_PATHS;

  OtherGProfRow(void)
    :SELF_COUNTS(0), CHILDREN_COUNTS(0), SELF_CALLS(0),
     TOTAL_CALLS(0), SELF_PATHS(0), TOTAL_PATHS(0)
    {}
};

std::ostream & operator<<(std::ostream &stream, OtherGProfRow& row)
{
  stream << "[" << row.SELF_COUNTS << " " << row.CHILDREN_COUNTS << " "
         << row.SELF_CALLS << " " << row.SELF_CALLS << " "
         << row.SELF_PATHS << " " << row.TOTAL_PATHS << "]" << std::endl;
  return stream;
}


template <int ORDER>
struct CompareCallersRow
{
  bool operator()(OtherGProfRow *a, OtherGProfRow *b)
    {
      int64_t callsDiff = ORDER * (a->SELF_COUNTS - b->SELF_COUNTS);
      int64_t cumDiff = ORDER * (a->CHILDREN_COUNTS - b->CHILDREN_COUNTS);
      if (callsDiff) return callsDiff < 0;
      if (cumDiff) return cumDiff < 0;
      return strcmp(a->name(), b->name()) < 0;
    }
};

/* This is the class which represent an entry in the gprof style flat output.
 *
 * CUM is the accumulated counts for that entry, including the count for the children.
 * SELF is the accumulated counts for only that entry, excluding the counts coming from children.
 * KIDS is the accumulated counts for the entries that are called by that entry.
 *
 * NOTE: one should have CUM = SELF+KIDS,  so I don't quite understand why I actually need one of the three.
 *
 * SELF_ALL I don't remember what it is.
 * CUM_ALL I don't remember what it is.
 */

class MainGProfRow : public GProfRow
{
public:
  typedef std::set <OtherGProfRow *, CompareCallersRow<1> > Callers;
  typedef std::set <OtherGProfRow *, CompareCallersRow<-1> > Calls;

  int64_t CUM;
  int64_t SELF;
  int64_t KIDS;
  int64_t SELF_ALL[3];
  int64_t CUM_ALL[3];

  Callers CALLERS;
  Calls CALLS;
};

class GProfMainRowBuilder
{
public:
  GProfMainRowBuilder(TreeMapBuilderFilter *flatMapBuilder,
                      TreeMapBuilderFilter *baselineBuilder,
                      bool diffMode)
    : m_info(0), m_origInfo(0),
      m_row(0), m_callmax(0),
      m_totals(0),
      m_totfreq(0),
      m_diffMode(diffMode),
      m_flatMap(flatMapBuilder->flatMap()),
      m_baselineMap(baselineBuilder ? baselineBuilder->flatMap() : 0)
    {
      flatMapBuilder->getTotals(m_totals, m_totfreq);
      if (baselineBuilder)
      {
        int64_t oldTotals = 0;
        int64_t oldTotfreq = 0;
        baselineBuilder->getTotals(oldTotals, oldTotfreq);
        m_totals -= oldTotals;
        m_totfreq -= oldTotfreq;
      }
    }

  void addCaller(SymbolInfo *callerSymbol)
    {
      assert(m_info);
      assert(m_row);
      FlatInfo *origin = (*m_flatMap)[callerSymbol];
      CallInfo *thisCall = origin->getCallee(m_info->SYMBOL);

      assert(thisCall);
      if (!thisCall->VALUES[0])
        return;
      OtherGProfRow *callrow = new OtherGProfRow();
      callrow->initFromInfo(origin);

      // In diff mode the percentage value is the percentual increment
      // between the two runs.
      // In the normal mode (i.e. no negative counts) it is the
      // fraction of the total counts.
      if (m_diffMode)
      {
        FlatInfo *origOrigin = (*m_baselineMap)[callerSymbol];
        CallInfo * origThisCall = 0;
        if (origOrigin)
          origThisCall = origOrigin->getCallee(m_info->SYMBOL);
        if (origThisCall)
          callrow->PCT = percent(thisCall->VALUES[0], llabs(origThisCall->VALUES[0]));
        else
          callrow->PCT = FLT_MAX;
      }
      else
        callrow->PCT = percent(thisCall->VALUES[0], m_totals);

      callrow->SELF_COUNTS = thisCall->VALUES[0];
      callrow->CHILDREN_COUNTS = origin->CUM_KEY[0];

      callrow->SELF_CALLS = thisCall->VALUES[1];
      callrow->TOTAL_CALLS = origin->CUM_KEY[1];

      callrow->SELF_PATHS = thisCall->VALUES[2];
      callrow->TOTAL_PATHS = origin->CUM_KEY[2];
      m_row->CALLERS.insert(callrow);
    }

  void addCallee(CallInfo *thisCall)
    {
      assert(m_info);
      assert(m_row);
      // calleeInfo is the global information about this symbol
      // thisCall contains the information when this symbol is called by m_info
      FlatInfo *calleeInfo = (*m_flatMap)[thisCall->SYMBOL];

      if (!thisCall->VALUES[0])
        return;

      if (m_callmax < thisCall->VALUES[0])
        m_callmax = thisCall->VALUES[0];

      OtherGProfRow *callrow = new OtherGProfRow();
      assert(calleeInfo);
      callrow->initFromInfo(calleeInfo);
      if (m_diffMode)
      {
        CallInfo *origCall = 0;
        if (m_origInfo)
          origCall = m_origInfo->getCallee(thisCall->SYMBOL);
        if (origCall)
          callrow->PCT = percent(thisCall->VALUES[0], llabs(origCall->VALUES[0]));
        else
          callrow->PCT = FLT_MAX;
      }
      else
        callrow->PCT = percent(thisCall->VALUES[0], m_totals);

      callrow->SELF_COUNTS = thisCall->VALUES[0];
      callrow->CHILDREN_COUNTS = calleeInfo->CUM_KEY[0];

      callrow->SELF_CALLS = thisCall->VALUES[1];
      callrow->TOTAL_CALLS = calleeInfo->CUM_KEY[1];

      callrow->SELF_PATHS = thisCall->VALUES[2];
      callrow->TOTAL_PATHS = calleeInfo->CUM_KEY[2];

      m_row->CALLS.insert(callrow);
      //assert(callrow->SELF_CALLS <= callrow->TOTAL_CALLS);
    }

  void beginEditingWith(FlatInfo *info)
    {
      assert(!m_info);
      assert(!m_row);
      m_info = info;
      m_row = new MainGProfRow();
      m_row->initFromInfo(m_info);

      // In normal mode PCT is the percentage relative to the
      // total counts.
      // In diff mode we need to lookup the original value for
      // the counter and give the percentage of the increment
      // relative to that.
      // In case the old counter is not there, we simply set the
      // percentage to FLT_MAX and later on print "new".
      if (m_diffMode)
      {
        FlatInfoMap::iterator i = m_baselineMap->find(m_info->SYMBOL);
        if (i != m_baselineMap->end())
          m_origInfo = i->second;

        if (m_origInfo && (m_origInfo->CUM_KEY[0] != 0))
          m_row->PCT = percent(m_info->CUM_KEY[0], llabs(m_origInfo->CUM_KEY[0]));
        else
          m_row->PCT = FLT_MAX;

        if (m_origInfo && (m_origInfo->SELF_KEY[0] != 0))
          m_row->SELF_PCT = percent(m_info->SELF_KEY[0], llabs(m_origInfo->SELF_KEY[0]));
        else
          m_row->SELF_PCT = FLT_MAX;

      }
      else
      {
        m_row->PCT = percent(m_info->CUM_KEY[0], m_totals);
        m_row->SELF_PCT = percent(m_info->SELF_KEY[0], m_totals);
      }

      m_row->CUM = m_info->CUM_KEY[0];
      m_row->SELF = m_info->SELF_KEY[0];
    }

  void endEditing(void)
    {
      assert(m_info);
      assert(m_row);
      m_info = 0;
      m_origInfo = 0;
      m_row = 0;
      m_callmax = 0;
    }

  MainGProfRow *build(bool isMax)
    {
      assert(m_row);
      if (isMax)
        m_row->KIDS = m_callmax;
      else
        m_row->KIDS = m_info->CUM_KEY[0] - m_info->SELF_KEY[0];
      memcpy(m_row->CUM_ALL, m_info->CUM_KEY, 3*sizeof(int64_t));
      memcpy(m_row->SELF_ALL, m_info->SELF_KEY, 3*sizeof(int64_t));
      return m_row;
    }

  int64_t totals(void)
    {
      return m_totals;
    }

  int64_t totfreq(void)
    {
      return m_totfreq;
    }
private:
  FlatInfo *m_info;
  FlatInfo *m_origInfo;
  MainGProfRow *m_row;
  int64_t m_callmax;
  int64_t m_totals;
  int64_t m_totfreq;
  bool m_diffMode;
  FlatInfoMap   *m_flatMap;
  FlatInfoMap   *m_baselineMap;
  MainGProfRow  *m_mainCallrow;
};

class HeaderPrinter
{
public:
  HeaderPrinter(bool showpaths, bool showcalls,
                int maxval, int maxcnt, bool diffMode)
    :m_showPaths(showpaths),
     m_showCalls(showcalls),
     m_maxval(maxval),
     m_maxcnt(maxcnt),
     m_diffMode(diffMode)
    {}

  void print(const char *description, const char *kind)
    {
      std::cout << "\n" << std::string(70, '-') << "\n"
                << description << "\n\n";
      if (m_diffMode)
        std::cout << "delta %  ";
      else
        std::cout << "% total  ";
      (AlignedPrinter(m_maxval))(kind);
      if (m_showCalls)
        (AlignedPrinter(m_maxcnt))("Calls");
      if (m_showPaths)
        (AlignedPrinter(m_maxcnt))("Paths");
      std::cout << "Function\n";
    }

private:
  bool m_showPaths;
  bool m_showCalls;
  int  m_maxval;
  int  m_maxcnt;
  bool m_diffMode;
};

int64_t
max(int64_t a, int64_t b)
{
  return a > b ? a : b;
}

class SortRowBySelf
{
public:
  bool operator()(MainGProfRow *a, MainGProfRow *b)
    {
      return llabs(a->SELF) > llabs(b->SELF);
      //if(a->SELF != b->SELF) return a->SELF > b->SELF;
      //if(a->DEPTH != b->DEPTH) return a->DEPTH < b->DEPTH;
      //return a->NAME < b->NAME;
    }
};

/** Helper method which summarizes page information
    contained in flat infos and substitutes them to
    the actual counter values.
  */
void
IgProfAnalyzerApplication::summarizePageInfo(FlatVector &sorted)
{
  for (size_t fii = 0, fie = sorted.size(); fii != fie; ++fii)
  {
    FlatInfo *flatinfo = sorted[fii];
    int64_t selfranges = 0;
    int64_t cumranges = 0;
    for (size_t ri = 0, re = flatinfo->SELF_RANGES.size(); ri != re; ++ri)
      selfranges += flatinfo->SELF_RANGES[ri].size();

    for (size_t ri = 0, re = flatinfo->CUM_RANGES.size(); ri != re; ++ri)
      cumranges += flatinfo->CUM_RANGES[ri].size();

    if (m_showPageRanges)
    {
      flatinfo->SELF_KEY[0] = flatinfo->SELF_RANGES.size();
      flatinfo->CUM_KEY[0] = flatinfo->CUM_RANGES.size();
      for (FlatInfo::Calls::iterator ci = flatinfo->CALLS.begin(), ce = flatinfo->CALLS.end();
           ci != ce; ++ci)
        (*ci)->VALUES[0] = (*ci)->RANGES.size();
    }
    else if (m_showLocalityMetrics)
    {
      if (!flatinfo->SELF_KEY[0])
        flatinfo->SELF_KEY[0] = 1;
      else
        flatinfo->SELF_KEY[0] = 4096 * selfranges / flatinfo->SELF_KEY[0];

      if (!flatinfo->CUM_KEY[0])
        flatinfo->CUM_KEY[0] = 1;
      else
        flatinfo->CUM_KEY[0] = 4096 * selfranges / flatinfo->CUM_KEY[0];

      for (FlatInfo::Calls::iterator ci = flatinfo->CALLS.begin(), ce = flatinfo->CALLS.end();
           ci != ce; ++ci)
      {
        if (!(*ci)->RANGES.size())
          (*ci)->VALUES[0] = 1;
        else
          (*ci)->VALUES[0] = 4096 * (*ci)->RANGES.size() / (*ci)->VALUES[0];
      }
    }
    else if (m_showPages)
    {
      flatinfo->SELF_KEY[0] = selfranges;
      flatinfo->CUM_KEY[0] = cumranges;
      for (FlatInfo::Calls::iterator ci = flatinfo->CALLS.begin(), ce = flatinfo->CALLS.end();
           ci != ce; ++ci)
      {
        int64_t callranges = 0;
        for (size_t ri = 0, re = (*ci)->RANGES.size(); ri != re; ++ri)
          callranges += (*ci)->RANGES[ri].size();
        (*ci)->VALUES[0] = callranges;
      }
    }
  }
}

void
IgProfAnalyzerApplication::tree(ProfileInfo &prof)
{
  prepdata(prof);

  // FIXME: make sure that symremap can be called even without
  //        passing a flatMap.
  verboseMessage("Building call tree map");
  TreeMapBuilderFilter *callTreeBuilder = new TreeMapBuilderFilter(m_keyMax, &prof);
  walk(prof.spontaneous(), m_nodesStorage.size(), callTreeBuilder);
  verboseMessage(0, 0, " done\n");

  // Sorting flat entries
  verboseMessage("Sorting", 0, ".\n");
  int rank = 1;
  FlatVector sorted;
  FlatInfoMap *flatMap = callTreeBuilder->flatMap();

  if (flatMap->empty())
    die("Could not find any information to print.");

  for (FlatInfoMap::const_iterator i = flatMap->begin();
       i != flatMap->end();
       i++)
    sorted.push_back(i->second);

  if (m_showPageRanges || m_showPages || m_showLocalityMetrics)
    summarizePageInfo(sorted);

  sort(sorted.begin(), sorted.end(), FlatInfoComparator(m_config->ordering()));

  for (size_t i = 0, e = sorted.size(); i != e; ++i)
    sorted[i]->setRank(rank++);

  if (m_config->doDemangle() || m_config->useGdb)
  {
    verboseMessage("Resolving symbols", 0, ".\n");
    symremap(prof, sorted, m_config->useGdb, m_config->doDemangle());
  }

  // Actually producing the tree.
  MassifTreeBuilder *treeBuilder = new MassifTreeBuilder(m_config);
  walk(prof.spontaneous(), m_nodesStorage.size(), treeBuilder);
}

void
IgProfAnalyzerApplication::dumpAllocations(ProfileInfo &prof)
{
  // Calculate the amount of allocations required to fill one page of
  // memory. This is a rough indication of fragmentation.
  AllocationsPerPage *fragmentationEvaluator = new AllocationsPerPage();
  walk(prof.spontaneous(), m_nodesStorage.size(), fragmentationEvaluator);

  prepdata(prof);

  // FIXME: make sure that symremap can be called even without
  //        passing a flatMap.
  verboseMessage("Building call tree map");
  TreeMapBuilderFilter *callTreeBuilder = new TreeMapBuilderFilter(m_keyMax, &prof);
  walk(prof.spontaneous(), m_nodesStorage.size(), callTreeBuilder);
  verboseMessage(0, 0, " done\n");

  // Sorting flat entries
  verboseMessage("Sorting", 0, ".\n");
  int rank = 1;
  FlatVector sorted;
  FlatInfoMap *flatMap = callTreeBuilder->flatMap();

  if (flatMap->empty())
  {
    std::cerr << "\nCould not find any information to print." << std::endl;
    exit(1);
  }

  for (FlatInfoMap::const_iterator i = flatMap->begin();
       i != flatMap->end();
       i++)
    sorted.push_back(i->second);

  sort(sorted.begin(), sorted.end(), FlatInfoComparator(m_config->ordering()));

  for (size_t i = 0, e = sorted.size(); i != e; ++i)
    sorted[i]->setRank(rank++);

  if (m_config->doDemangle() || m_config->useGdb)
  {
    verboseMessage("Resolving symbols", 0, ".\n");
    symremap(prof, sorted, m_config->useGdb, m_config->doDemangle());
  }

  // Produce the allocations map information.
  DumpAllocationsFilter dumper(std::cout);
  walk(prof.spontaneous(), m_nodesStorage.size(), &dumper);

  // Dump the symbol information for the first 10 entries.

  // Actually building the top 10.
  verboseMessage("Building top N");
  TopNBuilderFilter *topNFilter = new TopNBuilderFilter(m_topN);
  walk(prof.spontaneous(), m_nodesStorage.size(), topNFilter);
  verboseMessage(0, 0, " done\n");

  for (size_t i = 0; i != m_topN; ++i)
  {
    int64_t value;
    TopNBuilderFilter::StackTrace &nodes = topNFilter->stackTrace(i, value);
    if (!value)
      break;

    int j = 0;
    for(std::vector<NodeInfo *>::reverse_iterator ni = nodes.rbegin(), ne = nodes.rend(); ni != ne; ++ni)
      std::cout << "@("<< i << "," << j++ << ")" << *ni << ":" << (*ni)->symbol()->NAME << "\n";
  }

  generateFlatReport(prof, callTreeBuilder, 0, sorted);
}

void
IgProfAnalyzerApplication::topN(ProfileInfo &prof)
{
  prepdata(prof);

  // FIXME: make sure that symremap can be called even without
  //        passing a flatMap.
  verboseMessage("Building call tree map");
  TreeMapBuilderFilter *callTreeBuilder = new TreeMapBuilderFilter(m_keyMax, &prof);
  walk(prof.spontaneous(), m_nodesStorage.size(), callTreeBuilder);
  verboseMessage(0, 0, " done\n");

  // Sorting flat entries
  verboseMessage("Sorting", 0, ".\n");
  int rank = 1;
  FlatVector sorted;
  FlatInfoMap *flatMap = callTreeBuilder->flatMap();

  if (flatMap->empty())
  {
    std::cerr << "Could not find any information to print." << std::endl;
    exit(1);
  }

  for (FlatInfoMap::const_iterator i = flatMap->begin();
       i != flatMap->end();
       i++)
    sorted.push_back(i->second);

  sort(sorted.begin(), sorted.end(), FlatInfoComparator(m_config->ordering()));

  for (size_t i = 0, e = sorted.size(); i != e; ++i)
    sorted[i]->setRank(rank++);

  if (m_config->doDemangle() || m_config->useGdb)
  {
    verboseMessage("Resolving symbols", 0, ".\n");
    symremap(prof, sorted, m_config->useGdb, m_config->doDemangle());
  }

  if (sorted.empty()) {
    std::cerr << "Could not find any sorted information to print." << std::endl;
    exit(1);
  }
  // Actually building the top 10.
  verboseMessage("Building top N");
  TopNBuilderFilter *topNFilter = new TopNBuilderFilter(m_topN);
  walk(prof.spontaneous(), m_nodesStorage.size(), topNFilter);
  verboseMessage(0, 0, " done\n");

  for (size_t i = 0; i != m_topN; i++)
  {
    std::cout << "## Entry " << i+1 << " (";
    int64_t value;
    std::vector<NodeInfo *> &nodes = topNFilter->stackTrace(i, value);
    if (!value)
      break;
    if (m_isPerfTicks)
      std::cout << thousands(static_cast<double>(value) * m_tickPeriod, 0, 2)
                << " seconds)\n";
    else if (m_showLocalityMetrics)
      std::cout << thousands(value) << " spread factor)\n";
    else if (m_showPages)
      std::cout << thousands(value) << " pages)\n";
    else
      std::cout << thousands(value) << " bytes)\n";

    int j = 0;
    for(std::vector<NodeInfo *>::reverse_iterator i = nodes.rbegin(), e = nodes.rend(); i != e; i++)
      std::cout << "#" << j++ << " " << (*i)->symbol()->NAME << "\n";

    std::cout << std::endl;
  }
}

void
IgProfAnalyzerApplication::analyse(ProfileInfo &prof, TreeMapBuilderFilter *baselineBuilder)
{
  prepdata(prof);
  verboseMessage("Building call tree map");
  TreeMapBuilderFilter *callTreeBuilder = new TreeMapBuilderFilter(m_keyMax, &prof);
  walk(prof.spontaneous(), m_nodesStorage.size(), callTreeBuilder);
  verboseMessage(0, 0, " done\n");

  // Sorting flat entries
  verboseMessage("Sorting", 0, ".\n");
  int rank = 1;
  FlatVector sorted;
  FlatInfoMap *flatMap = callTreeBuilder->flatMap();

  if (flatMap->empty())
  {
    std::cerr << "Could not find any information to print." << std::endl;
    exit(1);
  }

  for (FlatInfoMap::const_iterator i = flatMap->begin();
       i != flatMap->end();
       i++)
    sorted.push_back(i->second);

  if (m_showPages || m_showLocalityMetrics || m_showPageRanges)
    summarizePageInfo(sorted);

  sort(sorted.begin(), sorted.end(), FlatInfoComparator(m_config->ordering()));

  for (FlatVector::const_iterator i = sorted.begin(); i != sorted.end(); i++)
    (*i)->setRank(rank++);

  if (m_config->doDemangle() || m_config->useGdb)
  {
    verboseMessage("Resolving symbols", 0, ".\n");
    symremap(prof, sorted, m_config->useGdb, m_config->doDemangle());
  }

  if (sorted.empty()) {
    std::cerr << "Could not find any sorted information to print." << std::endl;
    exit(1);
  }
  generateFlatReport(prof, callTreeBuilder, baselineBuilder, sorted);
}


/** An helper function to generate a flat profile from the tree information.

    @a callTreeBuilder which is the filter used to accumulate the tree information.

    @a baselineBuilder which is the filter used to accumulate the tree information
     of the baseline.

    @a sorted vector which contains the entries of the flat map, correctly
     ordered.
  */
void
IgProfAnalyzerApplication::generateFlatReport(ProfileInfo & /* prof */,
                                              TreeMapBuilderFilter *callTreeBuilder,
                                              TreeMapBuilderFilter *baselineBuilder,
                                              FlatVector &sorted)
{
  verboseMessage("Generating report", 0, ".\n");

  typedef std::vector <MainGProfRow *> CumulativeSortedTable;
  typedef CumulativeSortedTable FinalTable;
  typedef std::vector <MainGProfRow *> SelfSortedTable;

  FinalTable table;
  GProfMainRowBuilder builder(callTreeBuilder, baselineBuilder, m_config->diffMode());
  int64_t totals = builder.totals();
  int64_t totfreq = builder.totfreq();

  for (FlatVector::const_iterator i = sorted.begin();
       i != sorted.end();
       i++)
  {
    FlatInfo *info = *i;
    if (!info->CUM_KEY[0] && !m_showLocalityMetrics)
      continue;

    // Sort calling and called functions.
    // FIXME: should sort callee and callers
    builder.beginEditingWith(info);

    for (FlatInfo::Callers::const_iterator j = info->CALLERS.begin();
         j != info->CALLERS.end();
         j++)
      builder.addCaller(*j);

    for (FlatInfo::Calls::const_iterator j = info->CALLS.begin();
         j != info->CALLS.end();
         j++)
      builder.addCallee(*j);
    table.push_back(builder.build(m_keyMax));
    builder.endEditing();
  }

  SelfSortedTable selfSortedTable;

  for (FinalTable::const_iterator i = table.begin();
       i != table.end();
       i++)
    selfSortedTable.push_back(*i);

  stable_sort(selfSortedTable.begin(), selfSortedTable.end(), SortRowBySelf());
  bool diffMode = m_config->diffMode();
  PercentagePrinter printPercentage(diffMode);

  if (m_config->outputType() == Configuration::TEXT)
  {
    bool showcalls = m_config->showCalls();
    bool showpaths = m_config->showPaths();
    bool showlibs = m_config->showLib();
    std::cout << "Counter: ";

    if (m_showLocalityMetrics)
      std::cout << m_key << "@SPREAD_FACTOR\n";
    else if (m_showPageRanges)
      std::cout << m_key << "@RANGES\n";
    else if (m_showPages)
      std::cout << m_key << "@PAGES\n";
    else
      std::cout << m_key << "\n";

    int maxcnt=0;
    if (m_isPerfTicks && ! m_config->callgrind())
      maxcnt = max(8, max(thousands(static_cast<double>(totals) * m_tickPeriod, 0, 2).size(),
                          thousands(static_cast<double>(totfreq) * m_tickPeriod, 0, 2).size()));
    else
      maxcnt = max(8, max(thousands(totals).size(),
                          thousands(totfreq).size()));
    int maxval = maxcnt + (m_isPerfTicks ? 1 : 0);

    std::string basefmt = m_isPerfTicks ? "%.2f" : "%s";
    FractionPrinter valfmt(maxval);
    FractionPrinter cntfmt(maxcnt);

    HeaderPrinter hp(showpaths, showcalls, maxval, maxcnt, diffMode);

    if (diffMode)
      hp.print("Flat profile (cumulatively different entries only)", "Total");
    else
      hp.print("Flat profile (cumulative >= 1%)", "Total");

    for (FinalTable::const_iterator i = table.begin();
         i != table.end();
         i++)
    {
      MainGProfRow &row = **i;
      if (m_showLocalityMetrics || m_showPageRanges)
        printPercentage(0., "   -.--  ");
      else
        printPercentage(row.PCT);

      if (m_isPerfTicks && ! m_config->callgrind())
        printf("%*s  ", maxval, thousands(static_cast<double>(row.CUM) * m_tickPeriod, 0, 2).c_str());
      else
        printf("%*s  ", maxval, thousands(row.CUM).c_str());

      PrintIf p(maxcnt);
      p(showcalls, thousands(row.CUM_ALL[1]));
      p(showpaths, thousands(row.SELF_ALL[2]));
      printf("%s [%d]", row.name(), row.rank());
      if (showlibs)
        std::cout << row.filename();
      std::cout << "\n";
      if ((row.PCT < 1. && !diffMode) || row.CUM == 0)
        break;
    }

    std::cout << "\n";

    if (diffMode)
      hp.print("Flat profile (self different entries only)", "Self");
    else
      hp.print("Flat profile (self >= 0.01%)", "Self");

    for (SelfSortedTable::const_iterator i = selfSortedTable.begin();
         i != selfSortedTable.end();
         i++)
    {
      MainGProfRow &row = **i;

      if (m_showLocalityMetrics || m_showPageRanges)
        printPercentage(0., "   -.--  ");
      else
        printPercentage(row.SELF_PCT, "%7.2f  ");

      if (m_isPerfTicks && ! m_config->callgrind())
        printf("%*s  ", maxval, thousands(static_cast<double>(row.SELF) * m_tickPeriod, 0, 2).c_str());
      else
        printf("%*s  ", maxval, thousands(row.SELF).c_str());

      PrintIf p(maxcnt);
      p(showcalls, thousands(row.SELF_ALL[1]));
      p(showpaths, thousands(row.SELF_ALL[2]));
      printf("%s [%d]", row.name(), row.rank());
      if (showlibs)
        std::cout << row.filename();
      std::cout << "\n";
      if ((row.SELF_PCT < 0.01 && !diffMode) || row.SELF == 0)
        break;
    }
    std::cout << "\n\n" << std::string(70, '-') << "\n";
    std::cout << "Call tree profile (cumulative)\n";

    for (FinalTable::const_iterator i = table.begin();
         i != table.end();
         i++)
    {
      int64_t divlen = 34+3*maxval
                       + (showcalls ? 1 : 0)*(2*maxcnt+5)
                       + (showpaths ? 1 : 0)*(2*maxcnt+5);

      std::cout << "\n";
      for (int x = 0 ; x <((1+divlen)/2); x++)
        printf("- ");
      std::cout << std::endl;

      MainGProfRow &mainRow = **i;

      if ((mainRow.rank() % 10) == 1)
      {
        printf("%-8s", "Rank");
        if (diffMode)
          printf("delta %%  ");
        else
          printf("%% total  ");
        (AlignedPrinter(maxval))("Self");
        valfmt("Self", "Children");
        printf("  ");
        if (showcalls)
        {
          cntfmt("Calls", "Total");
          printf("  ");
        }

        if (showpaths)
        {
          cntfmt("Paths", "Total");
          printf("  ");
        }

        printf("Function\n");
      }

      for (MainGProfRow::Callers::const_iterator c = mainRow.CALLERS.begin();
           c != mainRow.CALLERS.end();
           c++)
      {
        OtherGProfRow &row = **c;
        std::cout << std::string(8, ' ');

        if (m_showLocalityMetrics || m_showPageRanges)
          printPercentage(0., "   -.--  ");
        else
          printPercentage(row.PCT);

        assert(maxval);
        std::cout << std::string(maxval, '.') << "  ";
        if (m_isPerfTicks && ! m_config->callgrind())
          valfmt(thousands(static_cast<double>(row.SELF_COUNTS) * m_tickPeriod, 0, 2),
                 thousands(static_cast<double>(row.CHILDREN_COUNTS) * m_tickPeriod, 0, 2));
        else
          valfmt(thousands(row.SELF_COUNTS), thousands(row.CHILDREN_COUNTS));

        printf("  ");
        if (showcalls)
        {
          cntfmt(thousands(row.SELF_CALLS),
                 thousands(row.TOTAL_CALLS));
          printf("  ");
        }

        if (showpaths)
        {
          cntfmt(thousands(row.SELF_PATHS),
                 thousands(row.TOTAL_PATHS));
          printf("  ");
        }
        printf("  %s [%d]", row.name(), row.rank());
        if (showlibs)
          std::cout << "  " << row.filename();
        std::cout << "\n";
      }

      char rankBuffer[256];
      sprintf(rankBuffer, "[%d]", mainRow.rank());
      printf("%-8s", rankBuffer);

      if (m_showLocalityMetrics || m_showPageRanges)
        printPercentage(0., "   -.--  ");
      else
        printPercentage(mainRow.PCT);

      if (m_isPerfTicks && ! m_config->callgrind())
      {
        (AlignedPrinter(maxval))(thousands(static_cast<double>(mainRow.CUM) * m_tickPeriod, 0, 2));
        valfmt(thousands(static_cast<double>(mainRow.SELF) * m_tickPeriod, 0, 2),
               thousands(static_cast<double>(mainRow.KIDS) * m_tickPeriod, 0, 2));
      }
      else
      {
        (AlignedPrinter(maxval))(thousands(mainRow.CUM));
        valfmt(thousands(mainRow.SELF), thousands(mainRow.KIDS));
      }
      printf("  ");
      if (showcalls)
      {
        (AlignedPrinter(maxcnt))(thousands(mainRow.CUM_ALL[1]));
        (AlignedPrinter(maxcnt))(""); printf(" ");
      }
      if (showpaths)
      {
        (AlignedPrinter(maxcnt))(thousands(mainRow.SELF_ALL[2]));
        (AlignedPrinter(maxcnt))(""); printf(" ");
      }

      std::cout << mainRow.name();

      if (showlibs)
        std::cout << mainRow.filename();

      std::cout << "\n";

      for (MainGProfRow::Calls::const_iterator c = mainRow.CALLS.begin();
           c != mainRow.CALLS.end();
           c++)
      {
        OtherGProfRow &row = **c;
        std::cout << std::string(8, ' ');
        if (m_showLocalityMetrics || m_showPageRanges)
          printPercentage(0., "   -.--  ");
        else
          printPercentage(row.PCT);

        std::cout << std::string(maxval, '.') << "  ";

        if (m_isPerfTicks && ! m_config->callgrind())
          valfmt(thousands(static_cast<double>(row.SELF_COUNTS) * m_tickPeriod, 0, 2),
                 thousands(static_cast<double>(row.CHILDREN_COUNTS) * m_tickPeriod, 0, 2));
        else
          valfmt(thousands(row.SELF_COUNTS), thousands(row.CHILDREN_COUNTS));

        printf("  ");

        if (showcalls)
        {
          cntfmt(thousands(row.SELF_CALLS),
                 thousands(row.TOTAL_CALLS));
          printf("  ");
        }
        if (showpaths)
        {
          cntfmt(thousands(row.SELF_PATHS),
                 thousands(row.TOTAL_PATHS));
          printf("  ");
        }
        printf("  %s [%d]", row.name(), row.rank());

        if (showlibs)
          std::cout << "  " << row.filename();
        std::cout << "\n";
      }
    }
  }
  else if (m_config->outputType() == Configuration::SQLITE)
  {
    std::cout << ("PRAGMA journal_mode=OFF;\n"
                  "PRAGMA count_changes=OFF;\n"
                  "DROP TABLE IF EXISTS files;\n"
                  "DROP TABLE IF EXISTS symbols;\n"
                  "DROP TABLE IF EXISTS mainrows;\n"
                  "DROP TABLE IF EXISTS children;\n"
                  "DROP TABLE IF EXISTS parents;\n"
                  "DROP TABLE IF EXISTS summary;\n\n"
                  "CREATE TABLE summary (\n"
                  "counter TEXT,\n"
                  "total_count INTEGER,\n"
                  "total_freq INTEGER,\n"
                  "tick_period REAL\n"
                  ");\n\n"
                  "CREATE TABLE files (\n"
                  "id,\n"
                  "name TEXT\n"
                  ");\n\n"
                  "CREATE TABLE symbols (\n"
                  "id,\n"
                  "name TEXT,\n"
                  "filename_id INTEGER CONSTRAINT file_id_exists REFERENCES files(id)\n"
                  ");\n\n"
                  "CREATE TABLE mainrows (\n"
                  "id INTEGER PRIMARY KEY,\n"
                  "symbol_id INTEGER CONSTRAINT symbol_id_exists REFERENCES symbols(id),\n"
                  "self_count INTEGER,\n"
                  "cumulative_count INTEGER,\n"
                  "kids INTEGER,\n"
                  "self_calls INTEGER,\n"
                  "total_calls INTEGER,\n"
                  "self_paths INTEGER,\n"
                  "total_paths INTEGER,\n"
                  "pct REAL\n"
                  ");\n\n"
                  "CREATE TABLE children (\n"
                  "self_id INTEGER CONSTRAINT self_exists REFERENCES mainrows(id),\n"
                  "parent_id INTEGER CONSTRAINT parent_exists REFERENCES mainrows(id),\n"
                  "from_parent_count INTEGER,\n"
                  "from_parent_calls INTEGER,\n"
                  "from_parent_paths INTEGER,\n"
                  "pct REAL\n"
                  ");\n\n"
                  "CREATE TABLE parents (\n"
                  "self_id INTEGER CONSTRAINT self_exists REFERENCES mainrows(id),\n"
                  "child_id INTEGER CONSTRAINT child_exists REFERENCES mainrows(id),\n"
                  "to_child_count INTEGER,\n"
                  "to_child_calls INTEGER,\n"
                  "to_child_paths INTEGER,\n"
                  "pct REAL\n"
                  ");\nPRAGMA synchronous=OFF;\n\nBEGIN TRANSACTION;\n"
                  "INSERT INTO summary (counter, total_count, total_freq, tick_period) VALUES(\"");
    if (m_showLocalityMetrics)
      std::cout << m_key << "@SPREAD_FACTOR";
    else if (m_showPageRanges)
      std::cout << m_key << "@RANGES";
    else if (m_showPages)
      std::cout << m_key << "@PAGES";
    else
      std::cout << m_key;

    std::cout  << "\", " << totals << ", " << totfreq << ", " << m_tickPeriod << ");\n\n";

    unsigned int insertCount = 0;
    std::set<int> filesDone;
    std::set<int> symbolsDone;

    for (FinalTable::const_iterator i = table.begin();
         i != table.end();
         i++)
    {
      MainGProfRow &mainRow = **i;

      if (filesDone.find(mainRow.fileId()) == filesDone.end())
      {
        filesDone.insert(mainRow.fileId());
        std::cout << "INSERT INTO files VALUES ("
                  << mainRow.fileId() << ", \"" << mainRow.filename() << "\");\n";
      }

      if (symbolsDone.find(mainRow.symbolId()) == symbolsDone.end())
      {
        symbolsDone.insert(mainRow.symbolId());
        std::cout << "INSERT INTO symbols VALUES ("
                  << mainRow.symbolId() << ", \"" << mainRow.name() << "\", "
                  << mainRow.fileId() << ");\n";
      }

      std::cout << "INSERT INTO mainrows VALUES ("
                << mainRow.rank() << ", " << mainRow.symbolId() << ", "
                << mainRow.SELF << ", " << mainRow.CUM << ", " << mainRow.KIDS << ", "
                << mainRow.SELF_ALL[1] << ", " << mainRow.CUM_ALL[1] << ", "
                << mainRow.SELF_ALL[2] << ", " << mainRow.CUM_ALL[2] << ", ";

      // In case we are showing page related information,
      // percentages do not really make sense.
      if (m_showLocalityMetrics || m_showPageRanges)
        printPercentage(0.0);
      else
        printPercentage(mainRow.PCT, "%7.2f", "-101");
      std::cout << ");\n";
      if ((++insertCount % 100000) == 0)
        std::cout << "END TRANSACTION;\nBEGIN TRANSACTION;\n";

      for (MainGProfRow::Callers::const_iterator c = mainRow.CALLERS.begin();
           c != mainRow.CALLERS.end();
           c++)
      {
        OtherGProfRow &row = **c;
        std::cout << "INSERT INTO parents VALUES ("
                  << row.rank() << ", " << mainRow.rank() << ", "
                  << row.SELF_COUNTS << ", " << row.SELF_CALLS << ", " << row.SELF_PATHS << ", ";

        // In case we are showing page related information,
        // percentages do not really make sense.
        if (m_showLocalityMetrics || m_showPageRanges)
          printPercentage(0.0);
        else
          printPercentage(row.PCT, "%7.2f", "-101");
        std::cout << ");\n";
        if ((++insertCount % 100000) == 0)
          std::cout << "END TRANSACTION;\nBEGIN TRANSACTION;\n";
      }

      for (MainGProfRow::Calls::const_iterator c = mainRow.CALLS.begin();
           c != mainRow.CALLS.end();
           c++)
      {
        OtherGProfRow &row = **c;
        std::cout << "INSERT INTO children VALUES("
                  << row.rank() << ", " << mainRow.rank() << ", "
                  << row.SELF_COUNTS << ", "<< row.SELF_CALLS << ", " << row.SELF_PATHS << ", ";

        // In case we are showing page related information,
        // percentages do not really make sense.
        if (m_showLocalityMetrics || m_showPageRanges)
          printPercentage(0.0);
        else
          printPercentage(row.PCT, "%7.2f", "-101");
        std::cout << ");\n";
        if ((++insertCount % 100000) == 0)
          std::cout << "END TRANSACTION;\nBEGIN TRANSACTION;\n";
      }
    }
    std::cout << "END TRANSACTION;\n"
                 "CREATE UNIQUE INDEX fileIndex ON files (id);\n"
                 "CREATE UNIQUE INDEX symbolsIndex ON symbols (id);\n"
                 "CREATE INDEX selfCountIndex ON mainrows(self_count);\n"
                 "CREATE INDEX totalCountIndex ON mainrows(cumulative_count);\n"
              << std::endl;
  }
  else if (m_config->outputType() == Configuration::JSON)
  {
    // Format for the output:
    // {
    //   "summary": {
    //      "counter": <counter>,
    //      "total_counts": <total-counts>,
    //      "total_freq": <total-counts>,
    //      "tick_period": <tick_period>
    //   }
    //   "mainrows": []
    //   "symbols": []
    //   "files": []
    //   "callers": []
    //   "callees": []
    // }

    // Print summary information.
    puts("{");
    const char *extraKey = "";
    if (m_showLocalityMetrics)
      extraKey = "@SPREAD_FACTOR";
    else if (m_showPageRanges)
      extraKey = "@RANGES";
    else if (m_showPages)
      extraKey = "@PAGES";

    printf("\"summary\": {\n"
           "   \"counter\": \"%s%s\",\n"
           "   \"total_counts\": %" PRId64 ",\n"
           "   \"total_freq\": %" PRId64 ",\n"
           "   \"tick_period\": %f\n"
           "},\n", m_key.c_str(), extraKey, totals, totfreq, m_tickPeriod);

    // We first print out the mainrows, then the filenames, then the symbols.
    // Notice we keep track of the order in which files and symbols appear, so
    // that we do not need an extra index to get them.
    puts("\"mainrows\": [");
    std::map<int, int> symbolsMap, filesMap;
    int symbolCount = 0, fileCount = 0;
    bool first = true;

    for (FinalTable::const_iterator i = table.begin(); i != table.end(); i++)
    {
      MainGProfRow &mainRow = **i;
      std::map<int,int>::const_iterator si = symbolsMap.find(mainRow.symbolId());
      std::map<int,int>::const_iterator fi = filesMap.find(mainRow.fileId());
      int symbolIndex, fileIndex;

      if (si != symbolsMap.end())
        symbolIndex = si->second;
      else
      {
        symbolIndex = symbolCount++;
        symbolsMap.insert(std::make_pair(mainRow.symbolId(), symbolIndex));
      }

      if (fi != filesMap.end())
        fileIndex = fi->second;
      else
      {
        fileIndex = fileCount++;
        filesMap.insert(std::make_pair(mainRow.fileId(), fileIndex));
      }

      if (first)
        first = false;
      else
        puts(",");

      printf("[%d, %d, %d, %ld, %ld, %ld, %ld, "
              "%ld, %ld, %ld, ",
                mainRow.rank(), symbolIndex, fileIndex,
                mainRow.SELF, mainRow.CUM, mainRow.KIDS,
                mainRow.SELF_ALL[1], mainRow.CUM_ALL[1],
                mainRow.SELF_ALL[2], mainRow.CUM_ALL[2]);
      // In case we are showing page related information,
      // percentages do not really make sense.
      if (m_showLocalityMetrics || m_showPageRanges)
        printPercentage(0.0);
      else
        printPercentage(mainRow.PCT, "%7.2f", "-101");
      putchar(']');
    }

    // Then we print files, as they appear.
    puts("],\n\"files\": [");
    int lastSeen = -1;
    for (FinalTable::const_iterator i = table.begin(); i != table.end(); i++)
    {
      MainGProfRow &mainRow = **i;
      int id = filesMap[mainRow.fileId()];
      // Already printed.
      if (lastSeen >= id)
        continue;
      // Should increase one-by-one
      assert(lastSeen + 1 == id);
      lastSeen = id;
      if (lastSeen > 0)
        puts(",");
      printf("\"%s\"", mainRow.filename());
    }

    puts("],\n \"symbols\": [");

    // Then we print symbols, as they appear.
    lastSeen = -1;
    for (FinalTable::const_iterator i = table.begin(); i != table.end(); i++)
    {
      MainGProfRow &mainRow = **i;
      int id = symbolsMap[mainRow.symbolId()];

      // Already printed.
      if (lastSeen >= id)
        continue;
      // Should increase one-by-one
      assert(lastSeen + 1 == id);
      lastSeen = id;
      if (lastSeen > 0)
        puts(",");
      printf("\"%s\"", mainRow.name());
    }

    // We print out information for each caller and foreach callee,
    // even if they are substantially the same edges.
    // The information is printed in rank order, so that we can quickly find
    // it. We could even store two indices to find them more easily.
    puts("],\n\"callers\": [");
    first = true;
    for (FinalTable::const_iterator i = table.begin(); i != table.end(); i++)
    {
      MainGProfRow &mainRow = **i;
      for (MainGProfRow::Callers::const_iterator c = mainRow.CALLERS.begin();
           c != mainRow.CALLERS.end();
           c++)
      {
        OtherGProfRow &row = **c;
        if (first)
          first = false;
        else
          puts(",");
        printf("[%d, %d, %ld, %ld, %ld, ",
               mainRow.rank(), row.rank(),
               row.SELF_COUNTS, row.SELF_CALLS, row.SELF_PATHS);

        // In case we are showing page related information,
        // percentages do not really make sense.
        if (m_showLocalityMetrics || m_showPageRanges)
          printPercentage(0.0);
        else
          printPercentage(row.PCT, "%7.2f", "-101");
        putchar(']');
      }
    }
    puts("],\n\"callees\": [");
    first = true;
    for (FinalTable::const_iterator i = table.begin(); i != table.end(); i++)
    {
      MainGProfRow &mainRow = **i;
      for (MainGProfRow::Calls::const_iterator c = mainRow.CALLS.begin();
           c != mainRow.CALLS.end();
           c++)
      {
        OtherGProfRow &row = **c;
        if (first)
          first = false;
        else
          puts(",");
        printf("[%d, %d, %ld, %ld, %ld, ",
               mainRow.rank(), row.rank(),
               row.SELF_COUNTS, row.SELF_CALLS, row.SELF_PATHS);

        // In case we are showing page related information,
        // percentages do not really make sense.
        if (m_showLocalityMetrics || m_showPageRanges)
          printPercentage(0.0);
        else
          printPercentage(row.PCT, "%7.2f", "-101");
        putchar(']');
      }
    }
    puts("]\n}");
  }
  else
  {
    assert(false);
  }
}




void
IgProfAnalyzerApplication::callgrind(ProfileInfo & /* prof */)
{
  assert(false);
}



void
IgProfAnalyzerApplication::run(void)
{
  ArgsList args;
  for (int i = 0; i < m_argc; i++)
    args.push_back(m_argv[i]);

  this->parseArgs(args);

  ProfileInfo *prof = new ProfileInfo;
  TreeMapBuilderFilter *baselineBuilder = 0;

  if (!m_config->baseline().empty())
  {
    verboseMessage("Reading baseline", 0, ".\n");
    readDump(prof, m_config->baseline(), new BaseLineFilter);
    prepdata(*prof);
    verboseMessage("Processing baseline");
    baselineBuilder = new TreeMapBuilderFilter(m_keyMax, prof);
    walk(prof->spontaneous(), m_nodesStorage.size(), baselineBuilder);
    verboseMessage(0, 0, " done\n");
  }

  verboseMessage("Reading profile data", 0, ".\n");
  StackTraceFilter *stackTraceFilter = 0;
  if (m_config->hasHitFilter())
    stackTraceFilter = new HitFilter(*m_config);

  for (size_t i = 0, e = m_inputFiles.size(); i != e; ++i)
    readDump(prof, m_inputFiles[i], stackTraceFilter);

  if (! m_config->isShowCallsDefined())
  {
    if (!strncmp(m_key.c_str(), "MEM_", 4))
      m_config->setShowCalls(true);
    else
      m_config->setShowCalls(false);
  }

  if (m_config->callgrind())
    callgrind(*prof);
  else if (m_topN)
    topN(*prof);
  else if (m_config->tree)
    tree(*prof);
  else if (m_config->dumpAllocations)
    dumpAllocations(*prof);
  else
    analyse(*prof, baselineBuilder);
}


int64_t
parseOptionToInt(const std::string &valueString, const char *msg)
{
  char *endptr;
  int64_t value = strtoll(valueString.c_str(), &endptr, 10);
  if (endptr && *endptr == 0)
    return value;
  std::cerr << "Error, " << msg << " expects an integer." << std::endl;
  exit(1);
}

void
unsupportedOptionDeath(const char *option)
{
  std::cerr << "Option " << option << " is not supported anymore" << std::endl;
  exit(1);
}

void
regexpErrorDeath(std::string re, size_t pos)
{
  std::cerr << "Error in regular expression:\n\n" << re << std::endl;
  std::cerr << std::string(pos, ' ') << "^" << std::endl;
  exit(1);
}

void
unexpectedArgumentDeath(const char *opt, const char *arg, const char *possible)
{
  std::cerr << "Unexpected argument " << arg << " for option" << opt << std::endl;
  if (possible)
    std::cerr << "Possible values: " << possible << std::endl;
  exit(1);
}

void
IgProfAnalyzerApplication::parseArgs(const ArgsList &args)
{

  ArgsList::const_iterator arg = args.begin();

  while (++arg != args.end())
  {
    NameChecker is(*arg);
    ArgsLeftCounter left(args.end());
    if (is("--help"))
      die(USAGE);
    else if (is("--verbose", "-v"))
      m_config->setVerbose(true);
    else if (is("--report", "-r") && left(arg))
    {
      if (!m_key.empty())
      {
        std::cerr << "Cannot specify more that one \"-ri / --report \" option." << std::endl;
        exit(1);
      }
      setKey(*(++arg));
    }
    else if (is("--value") && left(arg) > 1)
    {
      std::string type = *(++arg);
      if (type == "peak")
        m_config->setNormalValue(false);
      else if (type == "normal")
        m_config->setNormalValue(true);
      else
        unexpectedArgumentDeath("--value", type.c_str(), "normal (default), peak");
    }
    else if (is("--merge-libraries", "-ml"))
      m_config->setMergeLibraries(true);
    else if (is("--order", "-o"))
    {
      std::string order = *(arg++);
      if (order == "ascending")
        m_config->setOrdering(Configuration::ASCENDING);
      else if (order == "descending")
        m_config->setOrdering(Configuration::DESCENDING);
      else
        unexpectedArgumentDeath("-o / --order", order.c_str(), "ascending (default), descending");
    }
    else if (is("--filter-file", "-F"))
      unsupportedOptionDeath(arg->c_str());
    else if (is("--filter", "-f"))
      unsupportedOptionDeath(arg->c_str());
    else if (is("--no-filter", "-nf"))
      m_disableFilters = true;
    else if (is("--list-filters", "-lf"))
      unsupportedOptionDeath(arg->c_str());
    else if (is("--libs", "-l"))
      m_config->setShowLib(true);
    else if (is("--callgrind", "-C"))
      unsupportedOptionDeath(arg->c_str());
    else if (is("--text", "-t"))
      m_config->setOutputType(Configuration::TEXT);
    else if (is("--sqlite", "-s"))
      m_config->setOutputType(Configuration::SQLITE);
    else if (is("--json", "-js"))
      m_config->setOutputType(Configuration::JSON);
    else if (is("--top", "-tn"))
      m_topN = parseOptionToInt(*(++arg), "--top / -tn");
    else if (is("--tree", "-T"))
      m_config->tree = true;
    else if (is("--demangle", "-d"))
      m_config->setDoDemangle(true);
    else if (is("--gdb", "-g"))
      m_config->useGdb = true;
    else if (is("--paths", "-p"))
      m_config->setShowPaths(true);
    else if (is("--calls", "-c"))
      m_config->setShowCalls(true);
    else if (is("--merge-regexp", "-mr") && left(arg))
    {
#ifdef PCRE_FOUND
      std::string re = *(++arg);
      const char *regexpOption = re.c_str();
      std::string origRe = re;

      size_t pos = 0;
      while (*regexpOption)
      {
        if (*regexpOption++ != 's')
          regexpErrorDeath(origRe, pos);
        pos++;
        char separator[] = {0, 0};
        separator[0] = *regexpOption++;
        pos++;
        int searchSize = strcspn(regexpOption, separator);
        std::string search(regexpOption, searchSize);
        regexpOption += searchSize;
        pos += searchSize;
        if (*regexpOption++ != separator[0])
          regexpErrorDeath(origRe, pos);
        pos++;
        int withSize = strcspn(regexpOption, separator);
        std::string with(regexpOption, withSize);
        regexpOption += withSize;
        pos += withSize;

        if (*regexpOption++ != separator[0])
          regexpErrorDeath(origRe, pos);
        pos++;
        if (*regexpOption && *regexpOption++ != ';')
          regexpErrorDeath(origRe, pos);
        pos++;
        m_regexps.resize(m_regexps.size() + 1);

        RegexpSpec &spec = m_regexps.back();
        spec.re = search;
        spec.with = with;
    }
#else //NO PCRE
    die("igprof: --merge-regexp / -mr) igprof built without pcre support.\n");
#endif
    }
    else if (is("--merge-ancestors", "-ma") && left(arg))
    {
#ifdef PCRE_FOUND
      // syntax -ma ANCESTOR1>ANCESTOR2...>ANCESTORn/with
      // where ancestors are regular expressions representing parent function
      // names. If function has parent that matches all the regular expressions
      // will be renamded to "with"
      std::string ancestor = *(++arg);
      std::vector<std::string> ancestors;

      // Find ancestors from frist to last - 1
      size_t previous = 0;
      size_t found = ancestor.find_first_of(">", previous);
      while (found != std::string::npos)
      {
        ancestors.push_back(ancestor.substr(previous, found - previous));
        previous = found + 1;
        found = ancestor.find_first_of(">", previous );
      }

      // Get the replace name.
      size_t replace = ancestor.find_first_of('/');
      if (replace == std::string::npos)
        die("igprof: --merge-ancestors / -ma) Check the argument form\n");

      // the last ancestor before /
      ancestors.push_back(ancestor.substr(previous, replace - previous));

      // add ancestors vector and replacement into <AncestorsSpec> vector.
      m_ancestors.ancestors = ancestors;
      m_ancestors.with = ancestor.substr(replace + 1);
#else //NO PCRE
      die("igprof: --merge-ancestors / -ma) igprof built without pcre support.\n");
#endif
    }
    else if (is("--baseline", "-b") && left(arg))
      m_config->setBaseline(*(++arg));
    else if (is("--diff-mode", "-D"))
      m_config->setDiffMode(true);
    else if (is("--max-count-value", "-Mc") && left(arg))
      m_config->maxCountValue = parseOptionToInt(*(++arg), "--max-value / -Mc");
    else if (is("--min-count-value", "-mc"))
      m_config->minCountValue = parseOptionToInt(*(++arg), "--min-value / -mc");
    else if (is("--max-calls-value", "-Mf") && left(arg))
      m_config->maxCallsValue = parseOptionToInt(*(++arg), "--max-calls-value / -Mf");
    else if (is("--min-calls-value", "-mf") && left(arg))
      m_config->minCallsValue = parseOptionToInt(*(++arg), "--min-calls-value / -mf");
    else if (is("--max-average-value", "-Ma") && left(arg))
      m_config->maxAverageValue = parseOptionToInt(*(++arg), "--max-average-value / -Ma");
    else if (is("--min-average-value", "-ma") && left(arg))
      m_config->minAverageValue = parseOptionToInt(*(++arg), "--min-average-value / -ma");
    else if (is("--dump-allocations"))
      m_config->dumpAllocations = true;
    else if (is("--show-locality-metrics"))
    {
      m_showLocalityMetrics = true;
      m_showPages = true;
    }
    else if (is("--show-page-ranges"))
    {
      m_showPageRanges = true;
      m_showPages = true;
    }
    else if (is("--show-pages"))
      m_showPages = true;
    else if (is("--"))
    {
      while (left(arg) - 1)
        m_inputFiles.push_back(*(++arg));
    }
    else if ((*arg)[0] == '-')
    {
      std::cerr << "Unknown option " << (*arg) << std::endl;
      die(USAGE);
    }
    else
      m_inputFiles.push_back(*arg);
  }

  // For the moment we force using MEM_LIVE.
  if (m_showPages)
  {
    if (!m_key.empty())
      die("Option --show-pages / --show-page-ranges cannot be used with -r\n%s", USAGE);
    setKey("MEM_LIVE");
    m_config->setShowCalls(false);
  }

  if (m_config->diffMode() && m_config->baseline().empty())
    die("Option --diff-mode / -D requires --baseline / -b\n%s", USAGE);

  if (m_inputFiles.empty())
    die("ERROR: No input files specified.\n%s", USAGE);
}

void
userAborted(int)
{
  die("\nUser interrupted.");
}

int
main(int argc, const char **argv)
{
  signal(SIGINT, userAborted);
  try
  {
    s_config = new Configuration;
    IgProfAnalyzerApplication *app = new IgProfAnalyzerApplication(s_config, argc, argv);
    app->run();
  }
  catch(std::exception &e) {
    std::cerr << "Internal error: \"" << e.what() << "\".\n"
      "\nOh my, you have found a bug in igprof-analyse!\n"
      "Please file a bug report and some mean to reproduce it to:\n\n"
      "  https://savannah.cern.ch/bugs/?group=cmssw\n\n" << std::endl;
  }
  catch(...)
  {
    std::cerr << "Internal error.\n"
      "Oh my, you have found a bug in igprof-analyse!\n"
      "Please file a bug report and some mean to reproduce it to:\n\n"
      "  https://savannah.cern.ch/bugs/?group=cmssw\n\n" << std::endl;
  }
}
