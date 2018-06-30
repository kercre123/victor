/**
* File: ankiProtoGenerator.cpp
*
* Author: ross
* Created: Jun 24 2018
*
* Description: plugin for giving the proto generated code some nice features from CLAD
*
* Copyright: Anki, Inc. 2018
*/

#include "ankiProtoGenerator.h"
#include "codeGen/codeGenConstructors.h"
#include "codeGen/codeGenTagAccessor.h"
#include "codeGen/codeGenTags.h"
#include "codeGen/codeGenIncluder.h"

#include <string>
#include <iostream>
#include <unordered_map>

using namespace google::protobuf;

namespace Anki {

void FindAndReplaceAll( std::string& str, const std::string& searchStr, const std::string& replaceStr );
std::string GetVariableType( const FieldDescriptor* message );

bool AnkiProtoGenerator::Generate( const FileDescriptor* file,
                                   const std::string& parameter,
                                   compiler::GeneratorContext* context,
                                   std::string* error ) const
{
  TagsInfo oneOfTags;

  for( int i = 0; i < file->message_type_count(); i++ ) {
    const Descriptor* message = file->message_type(i);
    // Todo: we might want to consider generating custom code only if the message is annotated in
    // some way. custom protobuf options (extentions) don't play well with optimize_for LITE_RUNTIME, 
    // but we could annotate messages with "// @@anki_gen_code=true" and look for that in
    // SourceLocation::leading_comments. For now, generate code for all the messages!
    
    // Check the # of oneofs in the message. 
    // Generate tags only for a single oneof but no other params.
    const int numOneOfs = message->oneof_decl_count();
    const int numFields = message->field_count();
    bool shouldGenTags = false;
    if( numOneOfs == 1 ) {
      const int numOneOfFields = message->oneof_decl(0)->field_count();
      shouldGenTags = (numOneOfFields == numFields);
    }
    // Generate constructors if there's either no one-of, or a single oneof but no other params,
    // and no repeated fields (we could support repeated fields with vectors, but forcing users
    // to use the pure protobuf methods will result in fewer copies)
    bool shouldGenCtors = shouldGenTags || (numOneOfs == 0 && numFields > 0);
    if( shouldGenCtors ) {
      for( int i=0; i<message->field_count(); ++i ) {
        if( message->field(i)->is_repeated() ) {
          shouldGenCtors = false;
          break;
        }
      }
    }

    // generate code

    if( shouldGenCtors ) {
      const bool success = GenerateConstructors( file, message, context );
      if( !success ) {
        return false;
      }
    }

    if( shouldGenTags ) {
      TagList tags;
      std::string tagClass;
      const bool success = GenerateTagAccessors( file, message, context, tagClass, tags );
      if( !success ) {
        return false;
      } else {
        oneOfTags.emplace( tagClass, std::move(tags) );
      }
    }
    
  }

  if( !oneOfTags.empty() ) {
    // create the tags file
    std::string nameSpace = file->package();
    const bool success = GenerateTags( file, nameSpace, oneOfTags, context );
    if( !success ) {
      return false;
    }
  }

  return true;
}


bool AnkiProtoGenerator::GenerateConstructors( const FileDescriptor* file,
                                               const Descriptor* message,
                                               compiler::GeneratorContext* context ) const
{
  std::vector<CtorInfo> list;
  const int fieldCount = message->field_count();
  const std::string messageName = message->name();
  const int numOneOfs = message->oneof_decl_count();
  for( int i=0; i<fieldCount; ++i ) {
    list.push_back({});
    CtorInfo& info = list.back();
    info.varName = message->field(i)->name();
    info.type = GetVariableType( message->field(i) );
    info.isMessageType = (message->field(i)->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE);
  }
  const bool separateCtors = (numOneOfs > 0);
  const bool isDecl = true;
  CodeGenConstructors ctorDeclWriter{ message->full_name(), messageName, list, separateCtors, isDecl };
  std::string headerFilename = file->name();
  FindAndReplaceAll( headerFilename, ".proto", ".pb.h");
  const bool wrote = ctorDeclWriter.Write( headerFilename, context );

  if( !wrote ) {
    return false;
  }

  CodeGenConstructors ctorDefWriter{ message->full_name(), messageName, list, separateCtors, !isDecl };
  std::string cppFilename = file->name();
  FindAndReplaceAll( cppFilename, ".proto", ".pb.cc");
  return ctorDefWriter.Write( cppFilename, context );
}

bool AnkiProtoGenerator::GenerateTagAccessors( const FileDescriptor* file,
                                               const Descriptor* message,
                                               compiler::GeneratorContext* context,
                                               std::string& outTagClass,
                                               TagList& outTagList ) const
{
  std::string filename = file->name();
  FindAndReplaceAll( filename, ".proto", ".pb.h");
  outTagClass = message->name() + std::string{"Tag"};
  std::string tagClass = message->full_name();
  FindAndReplaceAll( tagClass, ".", "::" );
  tagClass = "::" + tagClass + "Tag";
  CodeGenTagAccessor genTagAccessor{ message->full_name(), tagClass };
  const bool wrote = genTagAccessor.Write( filename, context );

  if( wrote ) {
    // get tag values
    const int fieldCount = message->oneof_decl(0)->field_count();
    outTagList.resize( 1 + fieldCount );
    for( int i=0; i<fieldCount; ++i ) {
      // match proto's format
      std::string camelCase = message->oneof_decl(0)->field(i)->camelcase_name();
      camelCase[0] = std::toupper(static_cast<unsigned char>(camelCase[0]));
      outTagList[i].first = "k" + camelCase;
      outTagList[i].second = message->oneof_decl(0)->field(i)->number();
    }
    outTagList[fieldCount].first = "ONEOF_MESSAGE_TYPE_NOT_SET";
    outTagList[fieldCount].second = 0; // proto3 will guarantee that this is not used already
  }
  return wrote;
}

bool AnkiProtoGenerator::GenerateTags( const FileDescriptor* file,
                                       const std::string& nameSpace,
                                       const TagsInfo& tags,
                                       google::protobuf::compiler::GeneratorContext* context ) const
{

  std::string filename = file->name();
  FindAndReplaceAll( filename, ".proto", "Tags.pb.h");
  CodeGenTags tagWriter{ file->name(), nameSpace, tags };
  const bool wrote = tagWriter.Write( filename, context );
  if( !wrote ) {
    return false;
  }
  CodeGenIncluder includer{ filename };
  std::string insertionFile = file->name();
  FindAndReplaceAll( insertionFile, ".proto", ".pb.h");
  return includer.Write( insertionFile, context );
}

std::string GetVariableType( const FieldDescriptor* message ) 
{
  const auto cppType = message->cpp_type();
  if( cppType == FieldDescriptor::CPPTYPE_MESSAGE ) {
    return message->message_type()->name() + std::string{"*"};
  } else if( cppType == FieldDescriptor::CPPTYPE_ENUM ) {
    std::string enumName = message->enum_type()->full_name();
    FindAndReplaceAll( enumName, ".", "::" );
    return std::string{"::"} + enumName;
  } else if( cppType == FieldDescriptor::CPPTYPE_STRING ) {
    return "const std::string&"; // todo: make move ctors also
  } else if( (cppType == FieldDescriptor::CPPTYPE_INT32)
          || (cppType == FieldDescriptor::CPPTYPE_INT64)
          || (cppType == FieldDescriptor::CPPTYPE_UINT32)
          || (cppType == FieldDescriptor::CPPTYPE_UINT64) ) 
  {
    return std::string{"::google::protobuf::"} + FieldDescriptor::CppTypeName( cppType );
  } else {
    return FieldDescriptor::CppTypeName( cppType );
  }
}

} // namespace
