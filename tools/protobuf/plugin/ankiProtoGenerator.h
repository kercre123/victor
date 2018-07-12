/**
* File: ankiProtoGenerator.h
*
* Author: ross
* Created: Jun 24 2018
*
* Description: plugin for giving the proto generated code some nice features from CLAD
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __ANKI_PROTO_GENERATOR_H__
#define __ANKI_PROTO_GENERATOR_H__

#include "common.h"
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include <google/protobuf/descriptor.h>

#include <string>
#include <unordered_map>

namespace Anki {

class AnkiProtoGenerator : public google::protobuf::compiler::CodeGenerator 
{
public:
  virtual ~AnkiProtoGenerator() = default;

  virtual bool Generate( const google::protobuf::FileDescriptor* file,
                         const std::string& parameter,
                         google::protobuf::compiler::GeneratorContext* context,
                         std::string* error ) const override;

protected:
  
  bool GenerateConstructors( const google::protobuf::FileDescriptor* file,
                             const google::protobuf::Descriptor* message,
                             google::protobuf::compiler::GeneratorContext* context ) const;

  bool GenerateTagAccessors( const google::protobuf::FileDescriptor* file,
                             const google::protobuf::Descriptor* message,
                             google::protobuf::compiler::GeneratorContext* context,
                             std::string& outTagClass,
                             TagList& outTagList ) const;

  bool GenerateTags( const google::protobuf::FileDescriptor* file,
                     const std::string& nameSpace,
                     const std::unordered_map<std::string,TagList>& tags,
                     google::protobuf::compiler::GeneratorContext* context ) const;
};

} // namespace

#endif // __ANKI_PROTO_GENERATOR_H__
