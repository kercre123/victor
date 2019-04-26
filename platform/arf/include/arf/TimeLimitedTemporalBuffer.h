#pragma once

#include <algorithm>
#include <functional>
#include <map>

namespace ARF {

// Stores a bounded number of items, indexable by time.  Not thread-safe.
template <typename DataT, typename TimeT> class TimeLimitedTemporalBuffer {
public:
  TimeLimitedTemporalBuffer(TimeT max_age)
      : _maxAge(max_age) {}

  void Insert(const TimeT &t, const DataT &d) {
    _buffer[t] = d;
    Prune();
  }

  template <typename... Args> void Insert(const TimeT &t, Args &&... args) {
    _buffer.emplace(std::piecewise_construct, std::forward_as_tuple(t),
                    std::forward_as_tuple(args...));
    Prune();
  }

  void Remove(const TimeT &t) {
    _buffer.erase(t);
    // NOTE Should never have to Prune after a remove operation
  }

  // Strictly before
  void RemoveAllBefore(const TimeT &t) {
    while (!_buffer.empty()) {
      typename MapType::iterator it = _buffer.begin();
      if (it->first < t) {
        _buffer.erase(it);
      } else {
        break;
      }
    }
  }

  void RemoveOldest() {
    if (!_buffer.empty()) {
      _buffer.erase(_buffer.begin());
    }
  }

  bool RetrieveOldest(DataT &foundItem, TimeT &foundTime) const {
    if (_buffer.empty()) {
      return false;
    }
    auto item = _buffer.begin();
    foundTime = item->first;
    foundItem = item->second;
    return true;
  }

  void SetMaxAge(const TimeT &a) { _maxAge = a; }

  void SetMaxSize(size_t s) { _maxSize = s; }

  TimeT GetAge() const {
    if (_buffer.size() < 2) {
      return TimeT();
    }

    return _buffer.rbegin()->first - _buffer.begin()->first;
  }

  TimeT GetLatestTime() const {
    if (_buffer.empty()) {
      return TimeT();
    } else {
      return _buffer.rbegin()->first;
    }
  }

  size_t Size() const { return _buffer.size(); }

  void GetAllTimes(std::vector<TimeT> &times) {
    times.clear();
    times.reserve(_buffer.size());
    for (auto &item : _buffer) {
      times.push_back(item.first);
    }
  }

  using MapType = std::map<TimeT, DataT>;
  const MapType& GetBuffer() const { return _buffer; }

  // Strictly newer than.
  MapType GetValuesNewerThan(const TimeT &t) const {
    MapType ret;
    typename MapType::const_iterator it = _buffer.upper_bound(t);
    while (it != _buffer.end()) {
      ret[it->first] = it->second;
      ++it;
    }
    return ret;
  }

private:
  MapType _buffer;

  TimeT _maxAge;
  size_t _maxSize = std::numeric_limits<size_t>::max();

  void Prune() {
    while (GetAge() > _maxAge || Size() > _maxSize) {
      _buffer.erase(_buffer.begin());
    }
  }
};

} // namespace ARF