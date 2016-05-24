//
//  includePtree.h
//  includes ptree into current traslation unit
//
//  Created by damjan stulic on 5/22/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef basestation_util_ptree_includePtree_h
#define basestation_util_ptree_includePtree_h

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

// Manual extern instantiation of the worst offending templates
//  to reduce the number of duplicated symbols caused by boost.
namespace boost
{
  //optionals
  extern template class optional<std::string>;
  extern template class optional<std::string&>;
  extern template class optional<property_tree::basic_ptree<std::string, std::string>>;
  extern template class optional<property_tree::basic_ptree<std::string, std::string>&>;
  extern template class optional<bool>;
  extern template class optional<char>;
  extern template class optional<int>;
  extern template class optional<long>;
  extern template class optional<long long>;
  extern template class optional<float>;
  extern template class optional<double>;
  
  extern template void swap(optional<std::string>& , optional<std::string>&);
  extern template void swap(optional<bool>& , optional<bool>&);
  extern template void swap(optional<char>& , optional<char>&);
  extern template void swap(optional<int>& , optional<int>&);
  extern template void swap(optional<long>& , optional<long>&);
  extern template void swap(optional<long long>& , optional<long long>&);
  extern template void swap(optional<float>& , optional<float>&);
  extern template void swap(optional<double>& , optional<double>&);
  
  extern template struct optional_swap_should_use_default_constructor<std::string>;
  extern template struct optional_swap_should_use_default_constructor<const property_tree::basic_ptree<std::string, std::string>&>;
  extern template struct optional_swap_should_use_default_constructor<bool>;
  extern template struct optional_swap_should_use_default_constructor<char>;
  extern template struct optional_swap_should_use_default_constructor<int>;
  extern template struct optional_swap_should_use_default_constructor<long>;
  extern template struct optional_swap_should_use_default_constructor<long long>;
  extern template struct optional_swap_should_use_default_constructor<float>;
  extern template struct optional_swap_should_use_default_constructor<double>;

  
  namespace property_tree
  {
    // classes / structs / dependent types
    extern template struct path_of<std::string>;
    extern template struct translator_between<std::string, std::string>;
    extern template struct translator_between<std::string, bool>;
    extern template struct translator_between<std::string, char>;
    extern template struct translator_between<std::string, int>;
    extern template struct translator_between<std::string, long>;
    extern template struct translator_between<std::string, long long>;
    extern template struct translator_between<std::string, float>;
    extern template struct translator_between<std::string, double>;
    extern template struct id_translator<std::string>;
    
    extern template class  string_path<std::string, id_translator<std::string> >;
    
    extern template class  basic_ptree<std::string, std::string>;
    extern template struct basic_ptree<std::string, std::string>::subs;
    extern template class  basic_ptree<std::string, std::string>::iterator;
    extern template class  basic_ptree<std::string, std::string>::const_iterator;
    
    extern template class  basic_ptree<std::string, std::string>::const_assoc_iterator;
    extern template class  basic_ptree<std::string, std::string>::reverse_iterator;
    extern template class  basic_ptree<std::string, std::string>::const_reverse_iterator;
    
    
    // getters
    extern template
    std::string basic_ptree<std::string, std::string>::get<std::string>(const basic_ptree<std::string, std::string>::path_type &path) const;

    extern template
    bool basic_ptree<std::string, std::string>::get<bool>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    char basic_ptree<std::string, std::string>::get<char>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    int basic_ptree<std::string, std::string>::get<int>(const basic_ptree<std::string, std::string>::path_type &path) const;
   
    extern template
    long basic_ptree<std::string, std::string>::get<long>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    long long basic_ptree<std::string, std::string>::get<long long>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    float basic_ptree<std::string, std::string>::get<float>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    double basic_ptree<std::string, std::string>::get<double>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    // getters with default
    // const char* default
    extern template
    std::string basic_ptree<std::string, std::string>::get<char>(const basic_ptree<std::string, std::string>::path_type &path, const char *default_value) const;
    
    // "normal" getters
    extern template
    std::string basic_ptree<std::string, std::string>::get<std::string>(const basic_ptree<std::string, std::string>::path_type &path, const std::string& default_value) const;
    
    extern template
    bool basic_ptree<std::string, std::string>::get<bool>(const basic_ptree<std::string, std::string>::path_type &path, const bool& default_value) const;
    
    extern template
    char basic_ptree<std::string, std::string>::get<char>(const basic_ptree<std::string, std::string>::path_type &path, const char& default_value) const;
    
    extern template
    int basic_ptree<std::string, std::string>::get<int>(const basic_ptree<std::string, std::string>::path_type &path, const int& default_value) const;
    
    extern template
    long basic_ptree<std::string, std::string>::get<long>(const basic_ptree<std::string, std::string>::path_type &path, const long& default_value) const;
    
    extern template
    long long basic_ptree<std::string, std::string>::get<long long>(const basic_ptree<std::string, std::string>::path_type &path, const long long& default_value) const;
    
    extern template
    float basic_ptree<std::string, std::string>::get<float>(const basic_ptree<std::string, std::string>::path_type &path, const float& default_value) const;
    
    extern template
    double basic_ptree<std::string, std::string>::get<double>(const basic_ptree<std::string, std::string>::path_type &path, const double& default_value) const;
    
    
    // optional getters
    extern template
    optional<std::string> basic_ptree<std::string, std::string>::get_optional<std::string>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    optional<bool> basic_ptree<std::string, std::string>::get_optional<bool>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    optional<char> basic_ptree<std::string, std::string>::get_optional<char>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    optional<int> basic_ptree<std::string, std::string>::get_optional<int>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    optional<long> basic_ptree<std::string, std::string>::get_optional<long>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    optional<float> basic_ptree<std::string, std::string>::get_optional<float>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    extern template
    optional<double> basic_ptree<std::string, std::string>::get_optional<double>(const basic_ptree<std::string, std::string>::path_type &path) const;
    
    
    // value getters
    extern template
    std::string basic_ptree<std::string, std::string>::get_value<std::string>() const;
    
    extern template
    bool basic_ptree<std::string, std::string>::get_value<bool>() const;
    
    extern template
    char basic_ptree<std::string, std::string>::get_value<char>() const;
    
    extern template
    int basic_ptree<std::string, std::string>::get_value<int>() const;
    
    extern template
    long basic_ptree<std::string, std::string>::get_value<long>() const;
    
    extern template
    float basic_ptree<std::string, std::string>::get_value<float>() const;
    
    extern template
    double basic_ptree<std::string, std::string>::get_value<double>() const;
    
    
    // optional value getters
    extern template
    optional<std::string> basic_ptree<std::string, std::string>::get_value_optional() const;
    
    extern template
    optional<bool> basic_ptree<std::string, std::string>::get_value_optional<bool>() const;
   
    extern template
    optional<char> basic_ptree<std::string, std::string>::get_value_optional<char>() const;
    
    extern template
    optional<int> basic_ptree<std::string, std::string>::get_value_optional<int>() const;
    
    extern template
    optional<long> basic_ptree<std::string, std::string>::get_value_optional<long>() const;
   
    extern template
    optional<long long> basic_ptree<std::string, std::string>::get_value_optional<long long>() const;
   
    extern template
    optional<float> basic_ptree<std::string, std::string>::get_value_optional<float>() const;
   
    extern template
    optional<double> basic_ptree<std::string, std::string>::get_value_optional<double>() const;
    
    
    // value setters
    extern template
    void basic_ptree<std::string, std::string>::put_value<std::string>(const std::string &value);
   
    extern template
    void basic_ptree<std::string, std::string>::put_value<bool>(const bool &value);
    
    extern template
    void basic_ptree<std::string, std::string>::put_value<char>(const char &value);
    
    extern template
    void basic_ptree<std::string, std::string>::put_value<int>(const int &value);
    
    extern template
    void basic_ptree<std::string, std::string>::put_value<long>(const long &value);
    
    extern template
    void basic_ptree<std::string, std::string>::put_value<long long>(const long long &value);
    
    extern template
    void basic_ptree<std::string, std::string>::put_value<float>(const float &value);
   
    extern template
    void basic_ptree<std::string, std::string>::put_value<double>(const double &value);
    
  }

}

#pragma GCC diagnostic pop

#endif //basestation_util_ptree_includePtree_h
