/**
* File: codeGenConstructors.h
*
* Author: ross
* Created: Jun 24 2018
*
* Description:   Creates constructor defs and declarations based on a list of params
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __CODE_GEN_CONSTRUCTORS_H__
#define __CODE_GEN_CONSTRUCTORS_H__

#include "codeGen/codeInserter.h"
#include "common.h"

namespace Anki {

class CodeGenConstructors : public CodeInserter
{
public:
  CodeGenConstructors( const std::string& fullName, 
                       const std::string& messageName, 
                       const std::vector<CtorInfo>& varList, 
                       bool separateCtors,
                       bool isDeclaration )
    : CodeInserter( isDeclaration ? fullName : "", 
                    isDeclaration ? "class_scope" : "namespace_scope" ) 
    , _messageName( messageName )
    , _varList( varList )
    , _separateCtors( separateCtors )
    , _isDeclaration( isDeclaration )
  { }
  virtual ~CodeGenConstructors() = default;

protected:
  virtual bool WriteCode( const std::string& filename,
                          google::protobuf::compiler::GeneratorContext* context,
                          google::protobuf::io::Printer *printer )
  {
    if( _varList.size() == 0 ) {
      return false; // should not be using this
    } 
    if( _separateCtors ) {
      GenMultipleCtors( printer );
    } else {
      GenOneCtor( printer );
    }
    return true;
  }
private:

  void GenOneCtor( google::protobuf::io::Printer* printer ) {
    if( _isDeclaration ) {
      if( _varList.size() == 1 ) {
        printer->Print( "explicit " );
      }
    } else {
      printer->Print( (_messageName + "::").c_str() );
    }
    printer->Print( (_messageName + "(").c_str() );
    for( int i=0; i<_varList.size(); ++i ) {
      const auto& entry = _varList[i];
      printer->Print( {{"Type", entry.type}, {"Name", entry.varName}}, " $Type$ $Name$" );
      if( i < _varList.size() - 1 ) {
        printer->Print( "," );
      }
    }
    if( _isDeclaration ) {
      printer->Print( " );\n" );
    } else {
      printer->Print( " )\n" );
      printer->Print( ("  : " + _messageName + "() {\n").c_str() ); // call default ctor
      printer->Indent();
      for( int i=0; i<_varList.size(); ++i ) {
        const auto& entry = _varList[i];
        if( entry.isMessageType ) {
          printer->Print( {{"Name", entry.varName}}, "set_allocated_$Name$( $Name$ );\n" );
        } else {
          printer->Print( {{"Name", entry.varName}}, "set_$Name$( $Name$ );\n" );
        }
      }
      printer->Outdent();
      printer->Print("}\n");
    }
  }

  void GenMultipleCtors( google::protobuf::io::Printer* printer ) {
    if( _isDeclaration ) {
      for( int i=0; i<_varList.size(); ++i ) {
        const auto& entry = _varList[i];
        printer->Print( {{"Name", _messageName}}, "explicit $Name$(" );
        printer->Print( {{"Type", entry.type}, {"Name", entry.varName}}, " $Type$ $Name$" );
        printer->Print( " );\n" );
      }
    } else {
      for( int i=0; i<_varList.size(); ++i ) {
        const auto& entry = _varList[i];
        printer->Print( {{"Name", _messageName}}, "$Name$::$Name$(" );
        printer->Print( {{"Type", entry.type}, {"Name", entry.varName}}, " $Type$ $Name$" );
        printer->Print( " )\n" );
        printer->Print( ("  : " + _messageName + "() {\n").c_str() ); // call default ctor
        printer->Indent();
        if( entry.isMessageType ) {
          printer->Print( {{"Name", entry.varName}}, "set_allocated_$Name$( $Name$ );\n" );
        } else {
          printer->Print( {{"Name", entry.varName}}, "set_$Name$( $Name$ );\n" );
        }
        printer->Outdent();
        printer->Print( "}\n" );
      }
    }
  }
  const std::string& _messageName;
  const std::vector<CtorInfo>& _varList;
  bool _separateCtors;
  bool _isDeclaration;
};

} // namespace

#endif // __CODE_GEN_CONSTRUCTORS_H__
