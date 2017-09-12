#!/usr/bin/env python3

# Copyright (c) 2017 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''
This script will generate a Xamarin/Visual Studio compatable project file suited
for the exported csharp clad files.  It will also inject a version.cs file 
which will identify which version of clad is being exported.
'''

import fnmatch
import os
import re

here = os.path.abspath(os.path.dirname(__file__))

headerText = '''<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <Configuration Condition=" '$(Configuration)' == '' ">Debug</Configuration>
    <Platform Condition=" '$(Platform)' == '' ">x86</Platform>
    <ProjectGuid>{7CFA29E4-65E9-49B5-82AE-39CD0E36796C}</ProjectGuid>
    <OutputType>Library</OutputType>
    <RootNamespace>CladCSharp</RootNamespace>
    <AssemblyName>CladCSharp</AssemblyName>
    <TargetFrameworkVersion>v4.5</TargetFrameworkVersion>
  </PropertyGroup>
  <PropertyGroup Condition=" '$(Configuration)|$(Platform)' == 'Debug|x86' ">
    <DebugSymbols>true</DebugSymbols>
    <DebugType>full</DebugType>
    <Optimize>false</Optimize>
    <OutputPath>bin\Debug</OutputPath>
    <DefineConstants>DEBUG;</DefineConstants>
    <ErrorReport>prompt</ErrorReport>
    <WarningLevel>4</WarningLevel>
    <PlatformTarget>x86</PlatformTarget>
    <NoStdLib>false</NoStdLib>
  </PropertyGroup>
  <PropertyGroup Condition=" '$(Configuration)|$(Platform)' == 'Release|x86' ">
    <Optimize>true</Optimize>
    <OutputPath>bin\Release</OutputPath>
    <ErrorReport>prompt</ErrorReport>
    <WarningLevel>4</WarningLevel>
    <PlatformTarget>x86</PlatformTarget>
    <NoStdLib>false</NoStdLib>
  </PropertyGroup>
  <ItemGroup>
    <Compile Include="version.cs" />
'''

footerText = '''  </ItemGroup>
  <Import Project="$(MSBuildBinPath)\Microsoft.CSharp.targets" />
  <ProjectExtensions>
    <MonoDevelop>
      <Properties>
        <Policies>
          <TextStylePolicy inheritsSet="null" scope="text/x-csharp" />
          <CSharpFormattingPolicy IndentBlock="True" IndentBraces="False" IndentSwitchCaseSection="True" LabelPositioning="OneLess" NewLinesForBracesInTypes="True" NewLinesForBracesInMethods="True" SpaceWithinMethodDeclarationParenthesis="False" SpaceBetweenEmptyMethodDeclarationParentheses="False" SpaceWithinMethodCallParentheses="False" SpaceBetweenEmptyMethodCallParentheses="False" SpaceAfterControlFlowStatementKeyword="True" SpaceWithinExpressionParentheses="False" SpaceWithinCastParentheses="False" SpaceWithinOtherParentheses="False" SpaceAfterCast="False" SpacesIgnoreAroundVariableDeclaration="False" SpaceBetweenEmptySquareBrackets="False" SpaceWithinSquareBrackets="False" SpaceAfterColonInBaseTypeDeclaration="True" SpaceAfterComma="True" SpaceAfterDot="False" SpaceAfterSemicolonsInForStatement="True" SpaceBeforeColonInBaseTypeDeclaration="True" SpaceBeforeComma="False" SpaceBeforeDot="False" SpaceBeforeSemicolonsInForStatement="False" SpacingAroundBinaryOperator="Single" WrappingPreserveSingleLine="True" WrappingKeepStatementsOnSingleLine="True" PlaceSystemDirectiveFirst="True" IndentSwitchSection="True" NewLinesForBracesInProperties="True" NewLinesForBracesInAccessors="True" NewLinesForBracesInAnonymousMethods="True" NewLinesForBracesInControlBlocks="True" NewLinesForBracesInAnonymousTypes="True" NewLinesForBracesInObjectCollectionArrayInitializers="True" NewLinesForBracesInLambdaExpressionBody="True" NewLineForElse="True" NewLineForCatch="True" NewLineForFinally="True" NewLineForMembersInObjectInit="True" NewLineForMembersInAnonymousTypes="True" NewLineForClausesInQuery="True" SpacingAfterMethodDeclarationName="False" SpaceAfterMethodCallName="False" SpaceBeforeOpenSquareBracket="False" inheritsSet="Mono" inheritsScope="text/x-csharp" scope="text/x-csharp" />
          <TextStylePolicy TabsToSpaces="True" RemoveTrailingWhitespace="True" NoTabsAfterNonTabs="False" EolMarker="Native" FileWidth="120" TabWidth="2" IndentWidth="2" inheritsSet="VisualStudio" inheritsScope="text/plain" scope="text/plain" />
        </Policies>
      </Properties>
    </MonoDevelop>
  </ProjectExtensions>
</Project>'''

versionFileText = '''namespace Anki
{
  namespace Cozmo
  {
    public static class CladVersion
    {
      public static readonly string _Data = "CLAD_VERSION";
    }
  } // namespace Cozmo
} // namespace Anki
'''

def walk_directory(f, path, exportedRootPath):
    substringMask = ''.join(path.rsplit(exportedRootPath, 1))
    print(substringMask)
    for root, subFolders, files in os.walk(path):
        relativeRoot = root.replace(substringMask,'').replace('/','\\') + '\\'
        print(relativeRoot)
        for file in files:
            if fnmatch.fnmatch(file, '*.cs'):
                entryText = '    <Compile Include=\"' + relativeRoot + file + '" />\n'
                f.write(entryText)

def generate_project(file_path, source_paths):
    with open(file_path, 'w') as f:
        f.write(headerText)
        for source_path, exported_path in source_paths:
            walk_directory(f, source_path, exported_path)
        f.write(footerText)
        f.close()

def fetch_version():
    try:
        with open(os.path.join(here, 'src', 'cozmoclad', '__init__.py')) as f:
            ns = {}
            exec(f.read(), ns)
            return ns['__version__']
    except (IOError, KeyError):
        return 'local'

def inject_clad_version(file_path):
    clad_version = fetch_version()
    with open(file_path, 'w') as f:
        with open('LICENSE-header-cs.txt','r') as licenseFile:
            for line in licenseFile.readlines():
              f.write(line)
        modified_content = versionFileText.replace('CLAD_VERSION', clad_version)
        f.write(modified_content)

generate_project(os.path.join(here, 'csharp_clad', 'cladcsharp.csproj'),
    [(os.path.join(here, 'csharp_clad', 'clad'), 'clad'),
    (os.path.join(here, 'csharp_clad', 'util'), 'util')])
inject_clad_version(os.path.join(here, 'csharp_clad', 'version.cs'))
