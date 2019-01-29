#include "Profile.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Anki {
Profile::Profile(const std::string& name) 
  : _name(name)
{

}

Profile::Entry::Entry(const std::string& name)
  : name(name)
  , min(UINT64_MAX)
  , max(0)
  , sum(0)
  , count(0)
  , average(0)
{
}

uint32_t Profile::append(const std::string& name)
{
  auto iter = _indexes.find(name);
  if (iter != _indexes.end())
    throw std::runtime_error("Name already exists: "+name); 
  uint32_t index = _entries.size();
  _indexes.insert({name, index});
  _entries.push_back(Entry(name));
  return index;
}

void Profile::add(uint32_t index, uint64_t elapsed)
{
  Entry& entry = _entries.at(index);
  if (elapsed )
  entry.min = std::min(entry.min, elapsed);
  entry.max = std::max(entry.max, elapsed);
  entry.sum += elapsed;
  entry.count += 1;
  entry.average = entry.sum / (double)(entry.count);
}

void Profile::add(const std::string& name, uint64_t elapsed)
{
  auto iter = _indexes.find(name);
  if (iter == _indexes.end())
  {
    throw std::runtime_error("No such event: "+name); 
  }
  add(iter->second, elapsed);
}

uint32_t Profile::num_events()
{
  return _entries.size();
}

std::string Profile::to_string() const
{
  std::stringstream ss;
  ss<<_name<<std::endl;
  ss<<std::setw(32)<<"Name"
    <<std::setw(15)<<"Min"
    <<std::setw(14)<<"Max"
    <<std::setw(14)<<"Avg"
    <<std::setw(14)<<"Count"
    <<std::endl;
    
  for (const auto& entry : _entries){
    double min_s = entry.min / 1000000000.0;
    double max_s = entry.max / 1000000000.0;
    double avg_s = entry.average / 1000000000.0;
    ss<<std::setw(32)<<entry.name<<":"
      <<std::setprecision(6) // shows milliseconds
      <<std::fixed
      <<std::setw(14)<<min_s
      <<std::setw(14)<<max_s
      <<std::setw(14)<<avg_s
      <<std::setw(14)<<entry.count
      <<std::endl;
  }
  return ss.str();
}

} /* namespace Anki */