//
//  includePtree.cpp
//  util
//
//  Created by Mark Pauley on 6/9/15.
//
//

// Limit instantiation of boost::property_tree::ptree and friends to this translation unit

#pragma GCC diagnostic push
#if __has_warning("-Wextern-c-compat")
#pragma GCC diagnostic ignored "-Wextern-c-compat"
#endif
#if __has_warning("-Wundef")
#pragma GCC diagnostic ignored "-Wundef"
#endif
#if __has_warning("-Wunused-local-typedef")
#pragma GCC diagnostic ignored "-Wunused-local-typedef"
#endif
#include <boost/property_tree/ptree.hpp>

namespace boost
{
  //optionals
  template class optional<std::string>;
  template class optional<std::string&>;
  template class optional<property_tree::basic_ptree<std::string, std::string>>;
  template class optional<property_tree::basic_ptree<std::string, std::string>&>;
  template class optional<bool>;
  template class optional<char>;
  template class optional<int>;
  template class optional<long>;
  template class optional<long long>;
  template class optional<float>;
  template class optional<double>;
  
  template void swap(optional<std::string>& , optional<std::string>&);
  template void swap(optional<bool>& , optional<bool>&);
  template void swap(optional<char>& , optional<char>&);
  template void swap(optional<int>& , optional<int>&);
  template void swap(optional<long>& , optional<long>&);
  template void swap(optional<long long>& , optional<long long>&);
  template void swap(optional<float>& , optional<float>&);
  template void swap(optional<double>& , optional<double>&);
  
  template struct optional_swap_should_use_default_constructor<std::string>;
  template struct optional_swap_should_use_default_constructor<const property_tree::basic_ptree<std::string, std::string>&>;
  template struct optional_swap_should_use_default_constructor<bool>;
  template struct optional_swap_should_use_default_constructor<char>;
  template struct optional_swap_should_use_default_constructor<int>;
  template struct optional_swap_should_use_default_constructor<long>;
  template struct optional_swap_should_use_default_constructor<long long>;
  template struct optional_swap_should_use_default_constructor<float>;
  template struct optional_swap_should_use_default_constructor<double>;
  
  namespace property_tree
  {
    template struct path_of<std::string>;
    template struct translator_between<std::string, std::string>;
    template struct translator_between<std::string, bool>;
    template struct translator_between<std::string, char>;
    template struct translator_between<std::string, int>;
    template struct translator_between<std::string, long>;
    template struct translator_between<std::string, long long>;
    template struct translator_between<std::string, float>;
    template struct translator_between<std::string, double>;
    template struct id_translator<std::string>;
    
    template class  string_path<std::string, id_translator<std::string>>;
    
    template class  basic_ptree<std::string, std::string>;
    
    // getters
    template
    std::string basic_ptree<std::string, std::string>::get<std::string>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    bool basic_ptree<std::string, std::string>::get<bool>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    char basic_ptree<std::string, std::string>::get<char>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    int basic_ptree<std::string, std::string>::get<int>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    long basic_ptree<std::string, std::string>::get<long>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    long long basic_ptree<std::string, std::string>::get<long long>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    float basic_ptree<std::string, std::string>::get<float>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    double basic_ptree<std::string, std::string>::get<double>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    // getters with default
    // const char* default
    template
    std::string basic_ptree<std::string, std::string>::get<char>(const basic_ptree<std::string, std::string>::path_type &path, const char *default_value) const;
    
    // "normal" getters
    template
    std::string basic_ptree<std::string, std::string>::get<std::string>(const basic_ptree<std::string, std::string>::path_type &path, const std::string& default_value) const;
    
    template
    bool basic_ptree<std::string, std::string>::get<bool>(const basic_ptree<std::string, std::string>::path_type &path, const bool& default_value) const;
    
    template
    char basic_ptree<std::string, std::string>::get<char>(const basic_ptree<std::string, std::string>::path_type &path, const char& default_value) const;
    
    template
    int basic_ptree<std::string, std::string>::get<int>(const basic_ptree<std::string, std::string>::path_type &path, const int& default_value) const;
    
    template
    long basic_ptree<std::string, std::string>::get<long>(const basic_ptree<std::string, std::string>::path_type &path, const long& default_value) const;
    
    template
    long long basic_ptree<std::string, std::string>::get<long long>(const basic_ptree<std::string, std::string>::path_type &path, const long long& default_value) const;
    
    template
    float basic_ptree<std::string, std::string>::get<float>(const basic_ptree<std::string, std::string>::path_type &path, const float& default_value) const;
    
    template
    double basic_ptree<std::string, std::string>::get<double>(const basic_ptree<std::string, std::string>::path_type &path, const double& default_value) const;
    
    
    // optional getters
    template
    optional<std::string> basic_ptree<std::string, std::string>::get_optional<std::string>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    optional<bool> basic_ptree<std::string, std::string>::get_optional<bool>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    optional<char> basic_ptree<std::string, std::string>::get_optional<char>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    optional<int> basic_ptree<std::string, std::string>::get_optional<int>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    optional<long> basic_ptree<std::string, std::string>::get_optional<long>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    optional<float> basic_ptree<std::string, std::string>::get_optional<float>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    template
    optional<double> basic_ptree<std::string, std::string>::get_optional<double>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    
    // value getters
    template
    std::string basic_ptree<std::string, std::string>::get_value<std::string>() const;
    
    template
    bool basic_ptree<std::string, std::string>::get_value<bool>() const;
    
    template
    char basic_ptree<std::string, std::string>::get_value<char>() const;
    
    template
    int basic_ptree<std::string, std::string>::get_value<int>() const;
    
    template
    long basic_ptree<std::string, std::string>::get_value<long>() const;
    
    template
    float basic_ptree<std::string, std::string>::get_value<float>() const;
    
    template
    double basic_ptree<std::string, std::string>::get_value<double>() const;
    
    
    // optional value getters
    template
    optional<std::string> basic_ptree<std::string, std::string>::get_value_optional() const;
    
    template
    optional<bool> basic_ptree<std::string, std::string>::get_value_optional<bool>() const;
    
    template
    optional<char> basic_ptree<std::string, std::string>::get_value_optional<char>() const;
    
    template
    optional<int> basic_ptree<std::string, std::string>::get_value_optional<int>() const;
    
    template
    optional<long> basic_ptree<std::string, std::string>::get_value_optional<long>() const;
    
    template
    optional<long long> basic_ptree<std::string, std::string>::get_value_optional<long long>() const;
    
    template
    optional<float> basic_ptree<std::string, std::string>::get_value_optional<float>() const;
    
    template
    optional<double> basic_ptree<std::string, std::string>::get_value_optional<double>() const;
    
    
    // value setters
    template
    void basic_ptree<std::string, std::string>::put_value<std::string>(const std::string &value);
    
    template
    void basic_ptree<std::string, std::string>::put_value<bool>(const bool &value);
    
    template
    void basic_ptree<std::string, std::string>::put_value<char>(const char &value);
    
    template
    void basic_ptree<std::string, std::string>::put_value<int>(const int &value);
    
    template
    void basic_ptree<std::string, std::string>::put_value<long>(const long &value);
    
    template
    void basic_ptree<std::string, std::string>::put_value<long long>(const long long &value);
    
    template
    void basic_ptree<std::string, std::string>::put_value<float>(const float &value);
    
    template
    void basic_ptree<std::string, std::string>::put_value<double>(const double &value);
  }
}

#pragma GCC diagnostic pop

