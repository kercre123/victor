/**
* File: dispatchWorker
*
* Author: Lee Crippen
* Created: 8/30/16
*
* Description: Helper class for parallelizing tasks. Use by declaring a version of the template
* with the desired number of threads and argument types to be used by the task.
*
*
* Example:
*
* using MyDispatchWorker = Util::DispatchWorker<3, const std::string&>;
* MyDispatchWorker::FunctionType loadFileFunc = [this] (const std::string& filePath)
* {
*   LocalLoadFileFunction(filePath);
* };
* MyDispatchWorker myWorker(loadFileFunc);
* const auto& fileList = { "filePath0", "filePath1", "etc..." };
* for (int i = 0; i < fileList.size(); i++)
* {
*   myWorker.PushJob(fileList[i]);
* }
* myWorker.Process();
*
*
* Copyright: Anki, inc. 2016
*
*/
#ifndef __Util_DispatchWorker_DispatchWorker_H_
#define __Util_DispatchWorker_DispatchWorker_H_

#include <array>
#include <functional>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

namespace Anki {
namespace Util {

// template arguments are: number of threads, followed by list of arg types that the processing function expects in its tuple
template<std::size_t TCount, typename... Args>
class DispatchWorker
{
public:
  using FunctionType = std::function<void (Args...)>;
  
  DispatchWorker(FunctionType func) : _function(std::move(func)) { }
  
  // Push in a set of arguments to be processed by a worker thread
  template<typename... FuncArgs>
  void PushJob(FuncArgs&&... args);
  
  // Process all the input arguments that have been pushed
  void Process();
  
private:
  using FunctionArgType = std::tuple<Args...>;
  using ArgumentVector = std::vector<FunctionArgType>;

  std::array<std::thread, TCount>                 _workerThreads{};
  FunctionType                                    _function;
  ArgumentVector                                  _argumentList;
  std::mutex                                      _argsListMutex;
  
  // Use the provided function to process the arguments specified by the input iterators
  void DoThreadWork(typename ArgumentVector::iterator start, typename ArgumentVector::iterator end);

  // Tuple unpacking - from http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
  template<int ...>
  struct seq { };

  template<int N, int ...S>
  struct gens : gens<N-1, N-1, S...> { };

  template<int ...S>
  struct gens<0, S...> {
    typedef seq<S...> type;
  };

  template <int... S>
  void Invoke(FunctionArgType& params, seq<S...>)
  {
    _function(std::forward<typename std::tuple_element<S, FunctionArgType>::type>(std::get<S>(params))...);
  }

}; // end class DispatchWorker
  
  
  
  
// ---------------------- DispatchWorker Implementation ----------------------

template<std::size_t TCount, typename... Args>
template<typename... FuncArgs>
void DispatchWorker<TCount, Args...>::PushJob(FuncArgs&&... args)
{
  std::lock_guard<std::mutex> lockGuard(_argsListMutex);
  _argumentList.emplace_back(std::forward<FuncArgs>(args)...);
}

template<std::size_t TCount, typename... Args>
void DispatchWorker<TCount, Args...>::Process()
{
  std::lock_guard<std::mutex> lockGuard(_argsListMutex);
  const std::size_t size = _argumentList.size();
  const std::size_t numTotalThreads = TCount + 1; // Reserve one slot for the calling thread
  const std::size_t countPerThread = size / numTotalThreads;
  std::size_t remainder = (size % numTotalThreads);
  
  typename ArgumentVector::iterator curIter = _argumentList.begin();
  
  // Hand out work to each of the threads we've allocated
  for (std::size_t i = 0; i < TCount; i++)
  {
    if (0 == countPerThread && 0 == remainder)
    {
      break;
    }
    
    std::size_t sizeForThread = countPerThread;
    if (remainder > 0)
    {
      ++sizeForThread;
      --remainder;
    }
    _workerThreads[i] = std::thread(&DispatchWorker::DoThreadWork, this, curIter, curIter + sizeForThread);
    curIter += sizeForThread;
  }
  
  // Now allow the calling thread to do some work too
  DoThreadWork(curIter, _argumentList.end());
  
  // Wait for all our threads to be done
  for (std::size_t i = 0; i < TCount; i++)
  {
    if (_workerThreads[i].joinable())
    {
      _workerThreads[i].join();
      _workerThreads[i] = std::thread();
    }
  }
  
  _argumentList.clear();
}
  
template<std::size_t TCount, typename... Args>
void DispatchWorker<TCount, Args...>::DoThreadWork(typename ArgumentVector::iterator start, typename ArgumentVector::iterator end)
{
  static const auto paramSeq = typename gens<sizeof...(Args)>::type{};
  while (start != end)
  {
    Invoke(*start++, paramSeq);
  }
}

} // end namespace Util
} // end namespace Anki

#endif //__Util_DispatchWorker_DispatchWorker_H_
