/**
 * File: jsonWriter.h
 *
 * Author: Brad Neuman (bneuman)
 * Created: 30/12/12
 *
 * Description: A simple on-line json writer.
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#ifndef BASESTATION_UTILS_JSON_WRITER_H_
#define BASESTATION_UTILS_JSON_WRITER_H_

#include "util/helpers/includeFstream.h"
#include <vector>
#include <string>


namespace Anki {
namespace Util {

class JsonWriter {
public:

  // creates json writer that outputs to given filename. File is
  // automatically opened and closed
  JsonWriter(const std::string& filename);

  // Creates a json writer that writed to the given stream. No file
  // will be closed in this case
  JsonWriter(std::ostream& stream);

  // MUST be called to close file or file will be invalid
  void Close();

  // starts or ends a new group {}
  void StartGroup(const std::string& key);

  void EndGroup(); // throws if not in a group

  // starts or ends a new list with the given key. For simplicity, all
  // elements in a list are groups
  void StartList(const std::string& key);

  void EndList(); // throws if not in a list

  // This must be called before every list entry, including the first. Creates an unnamed group for the next
  // list item
  void NextListItem();

  // Instead of calling NextListItem(), use this to create a "naked" list entry, which is just a value. Either
  // pass in the value as a string, or use the stream operator
  void AddRawListEntry(const std::string& value);
  void AddRawListEntry(int value);
  void AddRawListEntry(float value);
  void AddRawListEntry(double value);

  // Adds a simple key-value pair, using the underlying file stream operator.  two ways to do it. NOTE: the
  // stream operator will always wrap values in quotes!
  // jsonWriter.AddEntry("key", "valueString");
  // jsonWriter.AddEntry("key", 3.1415);
  // jsonWriter.AddEntry("key")<<value;

  // NOTE: this is "Wrong" if you stream any non-string value. It will put it in quotes, which works in ptree,
  // but isn't valid json, so doesn't work in jsoncpp

  // N.B. the value you print better not have any " in it!!!
  // TODO: could validate by inheriting from stream and returning this instead of returning underlying stream
  std::ostream& AddEntry(const std::string& key);

  void AddEntry(const std::string& key, const std::string& value);
  void AddEntry(const std::string& key, int value);
  void AddEntry(const std::string& key, float value);
  void AddEntry(const std::string& key, double value);

  std::ostream& stream_;

private:

  // a vector of chars representing opening } ] or " that haven't yet
  // been closed. Or it can be 'v' which means a value was put
  // there. The outermost {} is implied
  std::vector<char> stack_;

  unsigned int level_;

  std::ofstream file_;
  bool closeFile_;

  // helpers
  void indent();

  void comma(); // inserts comma if needed
  void clearQuotes();

  void End(char c);

  void addval();

};

} // end namespace Util
} // end namespace Anki
#endif
