/**
 * File: jsonConfigTree.h
 *
 * Author: Brad Neuman
 * Created: 2014-01-06
 *
 * Description: Config tree implementation using jsoncpp library
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/common/basestation/configTree.h"
#include "json/json.h"
#include <fstream>

namespace Anki
{

ConfigTree::~ConfigTree()
{
  delete tree_;
  tree_ = NULL;
}

ConfigTree::ConfigTree()
{
  tree_ = new Json::Value;
}

ConfigTree::ConfigTree(Json::Value tree)
{
  tree_ = new Json::Value(tree);
}

bool ConfigTree::ReadJson(const std::string& filename)
{
  Json::Reader reader;
  std::ifstream configFileStream(filename);

  return reader.parse(configFileStream, *tree_);
}

std::string ConfigTree::AsString()
{
  return tree_->asString();
}

int ConfigTree::AsInt()
{
  return tree_->asInt();
}

float ConfigTree::AsFloat()
{
  return tree_->asFloat();
}

bool ConfigTree::AsBool()
{
  return tree_->asBool();
}

bool ConfigTree::GetValueOptional(const std::string& key, std::string& value)
{
  Json::Value child((*tree_)[key]);
  if(child.isNull())
    return false;

  value = child.asString();
  return true;
}

bool ConfigTree::GetValueOptional(const std::string& key, int& value)
{
  Json::Value child((*tree_)[key]);
  if(child.isNull())
    return false;

  value = child.asInt();
  return true;
}

bool ConfigTree::GetValueOptional(const std::string& key, float& value)
{
  Json::Value child((*tree_)[key]);
  if(child.isNull())
    return false;

  value = child.asFloat();
  return true;
}

bool ConfigTree::GetValueOptional(const std::string& key, bool& value)
{
  Json::Value child((*tree_)[key]);
  if(child.isNull())
    return false;

  value = child.asBool();
  return true;
}

std::string ConfigTree::GetValue(const std::string& key, const std::string& defaultValue)
{
  return tree_->get(key, defaultValue).asString();
}

std::string ConfigTree::GetValue(const std::string& key, const char* defaultValue)
{
  return tree_->get(key, defaultValue).asString();
}


int ConfigTree::GetValue(const std::string& key, const int defaultValue)
{
  return tree_->get(key, defaultValue).asInt();
}

float ConfigTree::GetValue(const std::string& key, const float defaultValue)
{
  return tree_->get(key, defaultValue).asFloat();
}

bool ConfigTree::GetValue(const std::string& key, const bool defaultValue)
{
  return tree_->get(key, defaultValue).asBool();
}


ConfigTree ConfigTree::operator[](const std::string& key)
{
  return (*tree_)[key];
}

ConfigTree ConfigTree::operator[](unsigned int index)
{
  return (*tree_)[index];
}

unsigned int ConfigTree::GetNumberOfChildren() const
{
  return tree_->size();
}

}

