/**
* File: main.cpp
*
* Author: ross
* Created: Jun 24 2018
*
* Description: runs the plugin for giving the proto generated code some nice features from CLAD
*
* Copyright: Anki, Inc. 2018
*/

#include "ankiProtoGenerator.h"
#include <google/protobuf/compiler/plugin.h>

using namespace Anki;

int main(int argc, char** argv) {
  AnkiProtoGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
