/**
* File: codeGenTagAccessor.h
*
* Author: ross
* Created: Jun 24 2018
*
* Description:   Adds a GetTag() accessor to a message class
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __CODE_GEN_TAG_ACCESSOR_H__
#define __CODE_GEN_TAG_ACCESSOR_H__

#include "codeGen/codeInserter.h"

namespace Anki {

class CodeGenTagAccessor : public CodeInserter 
{
public:
  CodeGenTagAccessor( const std::string& fullMessageName, const std::string& tagClass )
    : CodeInserter( fullMessageName, "class_scope" ) 
    , _tagClass( tagClass )
  { }
  virtual ~CodeGenTagAccessor() = default;

protected:
  virtual bool WriteCode( const std::string& filename,
                          google::protobuf::compiler::GeneratorContext* context,
                          google::protobuf::io::Printer *printer )
  {
    const char* text = R"(
inline $TagClass$ GetTag() const {
  return static_cast<$TagClass$>( _oneof_case_[0] );
}
)";
    printer->Print( {{"TagClass", _tagClass}}, 1+text ); // +1 skips initial \n
    return true;
  }

private:
  const std::string& _tagClass;
};

} // namespace

#endif // __CODE_GEN_TAG_ACCESSOR_H__
