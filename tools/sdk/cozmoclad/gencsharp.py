# Copyright (c) 2016 Anki, Inc.
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
Cozmo, by Anki.

Cozmo is a small robot with a big personality.

This library provides a low-level protocol library used by the
cozmo SDK package.
'''

import fnmatch
import os.path

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

with open(os.path.join(here, 'csharp_clad', 'cladcsharp.csproj'), 'w') as f:
    f.write(headerText)
    walk_directory(f, os.path.join(here, 'csharp_clad', 'clad'), 'clad')
    f.write(footerText)
    f.close()
