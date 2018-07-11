/**
* File: codeGenIncluder.h
*
* Author: ross
* Created: Jun 24 2018
*
* Description:   Adds a #include to an existing file
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __CODE_GEN_INCLUDER_H__
#define __CODE_GEN_INCLUDER_H__

#include "codeGen/codeInserter.h"

namespace Anki {

class CodeGenIncluder : public CodeInserter 
{
public:
  explicit CodeGenIncluder( const std::string& includeFile )
    : CodeInserter( "", "includes" ) 
    , _includeFile( includeFile )
  { }
  virtual ~CodeGenIncluder() = default;

protected:
  virtual bool WriteCode( const std::string& filename,
                          google::protobuf::compiler::GeneratorContext* context,
                          google::protobuf::io::Printer *printer )
  {
    printer->Print( {{"Filename", _includeFile}}, "#include \"$Filename$\"\n" );
    return true;
  }

private:
  const std::string& _includeFile;
};

} // namespace

#endif // __CODE_GEN_INCLUDER_H__
