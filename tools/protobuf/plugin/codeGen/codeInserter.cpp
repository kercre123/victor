/**
* File: codeInserter.cpp
*
* Author: ross
* Created: Jun 24 2018
*
* Description:   Inserts code in an existing file based on placeholder locations
*
* Copyright: Anki, Inc. 2018
*/

#include "codeGen/codeInserter.h"
#include <google/protobuf/io/zero_copy_stream.h>
#include <memory>

using namespace google::protobuf;

namespace Anki {

CodeInserter::CodeInserter( const std::string& messageName, const std::string insertionName )
  : _messageName( messageName )
  , _insertionName( insertionName )
  , _insertionPoint( insertionName ) 
{
  if( !_messageName.empty() ) {
    _insertionPoint = _insertionName + ":" + _messageName;
  }
}

bool CodeInserter::Write( const string& filename, compiler::GeneratorContext* context )
{
  std::unique_ptr<io::ZeroCopyOutputStream> output( context->OpenForInsert(filename, _insertionPoint) );
  io::Printer printer( output.get(), '$' );

  printer.Print( "\n// Begin code inserted by Anki plugin.\n" );
  if( !WriteCode( filename, context, &printer ) ) {
    return false;
  }
  printer.Print( "// End code inserted by Anki plugin.\n\n" );

  return !printer.failed();
}

} // namespace
