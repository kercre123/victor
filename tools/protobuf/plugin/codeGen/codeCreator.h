/**
* File: codeCreator.h
*
* Author: ross
* Created: Jun 24 2018
*
* Description:   Creates a new file (replacing if it exists)
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __CODE_CREATOR_H__
#define __CODE_CREATOR_H__

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>

namespace Anki {

class CodeCreator 
{
public:
  explicit CodeCreator( const std::string& protoFile );
  virtual ~CodeCreator() = default;

  bool Write( const std::string& filename, google::protobuf::compiler::GeneratorContext* context );
protected:
  virtual bool WriteCode( const std::string& filename,
                          google::protobuf::compiler::GeneratorContext* context,
                          google::protobuf::io::Printer *printer ) = 0;
private:
  const std::string& _protoFile;
};

} // namespace

#endif // __CODE_CREATOR_H__
