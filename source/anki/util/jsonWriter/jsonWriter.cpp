/**
 * File: jsonWriter.cpp
 *
 * Author: Brad Neuman (bneuman)
 * Created: 30/12/12
 *
 * Description: A simple on-line json writer.
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#include "util/jsonWriter/jsonWriter.h"
#include "util/logging/logging.h"
#include <exception>

using namespace std;

namespace Anki {
namespace Util {

JsonWriter::JsonWriter(const string& filename) :
  stream_(file_),
  file_(filename.c_str(), ios_base::out),
  closeFile_(true)
{
  stream_<<"{\n";
  level_ = 1;
}

JsonWriter::JsonWriter(ostream& stream) :
  stream_(stream),
  closeFile_(false)
{
  stream_<<"{\n";
  level_ = 1;
}

void JsonWriter::Close()
{
  while(!stack_.empty()) {
    switch(stack_.back()) {
    case '\"': stream_<<"\"\n"; break;
    case '}': level_--; indent(); stream_<<" }\n"; break;
    case ']': level_--; indent(); stream_<<" ]\n"; break;
    }
    stack_.pop_back();
  }

  stream_<<"}\n";

  if(closeFile_) {
    file_.close();
  }
}

void JsonWriter::StartGroup(const string& key)
{
  clearQuotes();
  indent();
  comma();
  stream_<<"\""<<key<<"\": {\n";
  addval();
  stack_.push_back('}');
  level_++;
}

void JsonWriter::EndGroup()
{
  End('}');
}

void JsonWriter::StartList(const string& key)
{
  clearQuotes();
  indent();
  comma();
  stream_<<"\""<<key<<"\": [\n";
  addval();
  stack_.push_back(']');
  level_++;
}

void JsonWriter::NextListItem()
{
  clearQuotes();
  if(stack_.back() != ']') { // this not the first entry
    End('}');
  }
  indent();
  comma();
  stream_<<"{\n";
  addval();
  stack_.push_back('}');
  level_++;
}

void JsonWriter::EndList()
{
  // go backwards through the stack to figure out if this was a grouped list or a raw list
  size_t stackSize = stack_.size();
  for(size_t i=stackSize; i>0; --i) {
    if( stack_[i-1] == ']' ) {
      // if we hit ] first, we must be in a raw list, so break out
      break;
    }
    if( stack_[i-1] == '}' ) {
      // we hit a } before ], so we must be in a grouped list. End the group now
      End('}');
      break;
    }
  }

  End(']');
}

ostream& JsonWriter::AddEntry(const string& key)
{
  clearQuotes();
  indent();
  comma();
  stream_<<"\""<<key<<"\": \"";
  addval();
  stack_.push_back('\"');

  // return the file stream so they can output the value directly to it.
  return stream_;
}

void JsonWriter::AddEntry(const string& key, const string& value)
{
  clearQuotes();
  indent();
  comma();
  stream_<<"\""<<key<<"\": \""<<value<<"\"\n";
  addval();
}

void JsonWriter::AddEntry(const string& key, int value)
{
  clearQuotes();
  indent();
  comma();
  stream_<<"\""<<key<<"\": "<<value<<"\n";
  addval();
}

void JsonWriter::AddEntry(const string& key, float value)
{
  clearQuotes();
  indent();
  comma();
  stream_<<"\""<<key<<"\": "<<value<<"\n";
  addval();
}

void JsonWriter::AddEntry(const string& key, double value)
{
  clearQuotes();
  indent();
  comma();
  stream_<<"\""<<key<<"\": "<<value<<"\n";
  addval();
}

void JsonWriter::AddRawListEntry(const std::string& value)
{
  clearQuotes();
  indent();
  comma();
  stream_<<"\""<<value<<"\"\n";
  addval();
}

void JsonWriter::AddRawListEntry(int value)
{
  clearQuotes();
  indent();
  comma();
  stream_<<value<<"\n";
  addval();
}

void JsonWriter::AddRawListEntry(float value)
{
  clearQuotes();
  indent();
  comma();
  stream_<<value<<"\n";
  addval();
}

void JsonWriter::AddRawListEntry(double value)
{
  clearQuotes();
  indent();
  comma();
  stream_<<value<<"\n";
  addval();
}

void JsonWriter::addval()
{
  if(stack_.empty() || stack_.back() != 'v') {
    stack_.push_back('v');
  }
}

void JsonWriter::End(char c)
{
  clearQuotes();

  if(stack_.empty()) {
    PRINT_NAMED_ERROR("jsonWriter.inconsistentStack", "End called but stack was empty");
    throw std::runtime_error("jsonWriter.inconsistentStack.End");
  }

  // if there was a value here, we don't care anymore
  if(stack_.back() == 'v') {
    stack_.pop_back();
  }

  if(stack_.back() != c) {
    PRINT_NAMED_ERROR("jsonWriter.inconsistentStack", "Invalid stack value '%c' (%d), looking for '%c'", stack_.back(), stack_.back(), c);
    throw std::runtime_error("jsonWriter.inconsistentStack.InvalidStack");
  }

  stack_.pop_back();

  level_--;
  indent();
  stream_<<' '<<c<<endl;
}

void JsonWriter::clearQuotes()
{
  if(!stack_.empty() && stack_.back() == '\"') {
    stream_<<"\"\n";
    stack_.pop_back();
  }
}

void JsonWriter::indent()
{
  for(unsigned int i=1; i<level_; i++) {
    stream_<<"  ";
  }
  stream_<<' ';
}

void JsonWriter::comma()
{
  if(!stack_.empty() && stack_.back() == 'v') {
    stream_<<',';
  }
  else {
    stream_<<' ';
  }
}


} // end namespace Util
} // end namespace Anki
