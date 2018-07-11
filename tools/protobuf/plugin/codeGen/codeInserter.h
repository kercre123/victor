/**
* File: codeInserter.h
*
* Author: ross
* Created: Jun 24 2018
*
* Description:   Inserts code in an existing file based on placeholder locations
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __CODE_INSERTER_H__
#define __CODE_INSERTER_H__

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>

namespace Anki {

class CodeInserter 
{
public:
  CodeInserter( const std::string& message_name, const std::string insertion_name );
  virtual ~CodeInserter() = default;
  bool Write( const std::string& filename, google::protobuf::compiler::GeneratorContext* context );
protected:
  virtual bool WriteCode( const std::string& filename,
                          google::protobuf::compiler::GeneratorContext* context,
                          google::protobuf::io::Printer *printer ) = 0;
  std::string _messageName;
  std::string _insertionName;
  std::string _insertionPoint;
};

} // namespace

#endif // __CODE_INSERTER_H__
