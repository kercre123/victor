#ifndef __EVALUATOR_PROFILE_H__
#define __EVALUATOR_PROFILE_H__

#include <vector>
#include <map>
#include <string>

namespace Anki {

class Profile {
public:
  Profile(const std::string& name);

  //! Append a new event, returns the index
  uint32_t append(const std::string& name);
  
  //! Add the elapsed time to the entry
  void add(uint32_t index, uint64_t elapsed);
  void add(const std::string& name, uint64_t elapsed);
  
  //! Get the number of events
  uint32_t num_events();

  std::string to_string() const;

private:
  struct Entry {
    Entry(const std::string& name);

    std::string name;
    uint64_t min;
    uint64_t max;
    uint64_t sum;
    uint64_t count;
    double average;
  };

  std::string _name;
  std::map<std::string, uint32_t> _indexes;
  std::vector<Entry> _entries;
};

} /* namespace Anki */

#endif /* __EVALUATOR_PROFILE_H__ */