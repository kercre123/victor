#ifndef SYM_CACHE_H
# define SYM_CACHE_H

# include "macros.h"
# include "buffer.h"
# include "profile.h"
# include <limits.h>
# include <stdint.h>

/** A symbol lookup cache.  */
class HIDDEN IgProfSymCache : protected IgProfBuffer
{
  static const unsigned int BINARY_HASH = 128;
  static const unsigned int SYMBOL_HASH = 128*1024;
public:
  struct Binary;
  struct Symbol;
  struct SymCache;

  /// Description of a binary module associated with a symbol.
  struct Binary
  {
    Binary      *next;          //< The next binary in the hash bin chain.
    const char  *name;          //< Name of the executable object if known.
    int         id;             //< Reference ID in final output, -1 if unset.
  };

  /// Description of a symbol behind a call address, linked in hash table.
  struct Symbol
  {
    Symbol      *next;          //< The next symbol in the hash bin chain.
    void        *address;       //< Instruction pointer value.
    const char  *name;          //< Name of the symbol (function) if known.
    long        symoffset;      //< Offset from the beginning of symbol.
    long        binoffset;      //< Offset from the beginning of executable object.
    Binary      *binary;        //< The binary object containing this symbol.
    int         id;             //< Reference ID in final output, -1 if unset.
  };

  /// Hash table cache entry for call address to symbol address mappings.
  struct SymCache
  {
    SymCache    *next;          //< The next cache entry in the hash bin chain.
    void        *calladdr;      //< Instruction pointer value.
    void        *symaddr;       //< Address of the corresponding symbol.
  };

  IgProfSymCache(void);
  ~IgProfSymCache(void);

  Symbol *      get(void *address);

private:
  void *        roundAddressToSymbol(void *address);
  Symbol *      symbolForAddress(void *address);

  Binary        *bintable_[BINARY_HASH]; //< The binaries hash.
  Symbol        *symtable_[SYMBOL_HASH]; //< The symbol hash.
  SymCache      *symcache_[SYMBOL_HASH]; //< The symbol cache hash.

  // Unavailable copy constructor, assignment operator
  IgProfSymCache(IgProfSymCache &);
  IgProfSymCache &operator=(IgProfSymCache &);
};

#endif // SYM_CACHE_H
