/**
* File: codeGenTags.h
*
* Author: ross
* Created: Jun 24 2018
*
* Description:   Creates a file containing enum classes of message oneof cases
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __CODE_GEN_TAGS_H__
#define __CODE_GEN_TAGS_H__

#include "codeGen/codeCreator.h"
#include "common.h"

namespace Anki {

class CodeGenTags : public CodeCreator 
{
public:
  CodeGenTags( const std::string& protoFile, const std::string& nameSpace, const TagsInfo& tags )
    : CodeCreator( protoFile ) 
    , _namespace( nameSpace )
    , _tags( tags )
    , _protoFile( protoFile )
  { 
    _namespaceDepth = 1 + std::count(_namespace.begin(), _namespace.end(), '.');
    // cpp-ify namespace: Anki.Cozmo.external_interface ==> namespace Anki { namespace ... }
    _namespace = "namespace " + _namespace + " {\n";
    FindAndReplaceAll( _namespace, ".", " {\nnamespace " );
  }
  virtual ~CodeGenTags() = default;

protected:
  virtual bool WriteCode( const std::string& filename,
                          google::protobuf::compiler::GeneratorContext* context,
                          google::protobuf::io::Printer *printer )
  {
    if( _tags.empty() || _namespace.empty() ) {
      return false;
    }
    std::string fileWithoutExt = _protoFile;
    size_t extPos = fileWithoutExt.find(".proto");
    if( extPos != std::string::npos ) {
      fileWithoutExt.resize( extPos );
    }
    printer->Print( ("#ifndef PROTOBUF_" + fileWithoutExt + "Tags_2eproto__INCLUDED\n").c_str() );
    printer->Print( ("#define PROTOBUF_" + fileWithoutExt + "Tags_2eproto__INCLUDED\n\n").c_str() );

    printer->Print( _namespace.c_str() );
    printer->Print( "\n\n" );

    for( const auto& entry : _tags ) {

      std::string type = TypeFromMaxValue( entry.second.size() );
      printer->Print( {{"tagClass", entry.first}, {"underlying", type}}, "enum class $tagClass$ : $underlying$\n" );
      printer->Print( "{\n" );

      printer->Indent();
      for( const auto& value : entry.second ) {
        printer->Print( {{"name",value.first}, {"value",std::to_string(value.second)}},
                        "$name$ = $value$,\n" );
      }
      printer->Outdent();

      printer->Print( "};\n\n" );
    }

    for( int i=0; i<_namespaceDepth; ++i ) {
      printer->Print( "}\n" );
    }
    printer->Print( "\n" );
    printer->Print( "#endif\n" );

    return true;
  }
private:
  std::string TypeFromMaxValue( uint64_t val ) const
  {
    if( val < 65536 ) {
      if( val < 256 ) {
        return "uint8_t";
      } else {
        return "uint16_t";
      }
    } else {
      if( val < 0x100000000L ) {
        return "uint32_t";
      } else {
        return "uint64_t";
      }
    }
  }

  std::string _namespace;
  const TagsInfo& _tags;
  const std::string& _protoFile;
  unsigned int _namespaceDepth;
};

} // namespace

#endif // __CODE_GEN_CONSTRUCTORS_H__
