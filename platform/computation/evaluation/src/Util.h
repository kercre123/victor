#ifndef __EVALUATOR_UTIL_H__
#define __EVALUATOR_UTIL_H__

#include <string>
#include <vector>

namespace Anki
{

void directory_create(const std::string& dirname);
void directory_list(const std::string& dirname, std::vector<std::string>& filenames);

template<typename ...T>
std::string path_join(const std::string& base, const T& ... parts)
{
  std::string result = base;
  for (const auto& part : {parts...}){
    result = result + "/" + part;
  }
  return result;
}

bool path_has_suffix(const std::string &str, const std::string &suffix);

std::vector<std::string> split(const std::string& value, const std::string& delimiter=" ");
std::string join(const std::vector<std::string>& values, const std::string& delimiter=" ");

const char * cl_error_to_string(int error);

}

#endif