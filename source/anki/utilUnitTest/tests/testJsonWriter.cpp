/**
 * File: btTestJsonWriter.h
 *
 * Author: bneuman (brad)
 * Created: 9/25/2012
 *
 * Description: Unit tests for the json writer
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#include <string>
#include "util/helpers/includeGTest.h"

#include "util/jsonWriter/jsonWriter.h"

using namespace std;
using namespace Anki::Util;

TEST(JsonWriter, EmptyJson)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.Close();

  string expected = "{\n}\n";
  EXPECT_EQ(expected, ss.str());
}

TEST(JsonWriter, IntValue)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("i", 37);
  jw.Close();

  string exp = "{\n  \"i\": 37\n}\n";
  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, FloatValue)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("i", 3.1415f);
  jw.Close();

  string exp = "{\n  \"i\": 3.1415\n}\n";
  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, SingleValue)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("key", "value");
  jw.Close();

  string exp = "{\n  \"key\": \"value\"\n}\n";
  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, SingleValueStream)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("key")<<"value";
  jw.Close();

  string exp = "{\n  \"key\": \"value\"\n}\n";
  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, CommaAndQuotesTest)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("a")<<1;
  jw.AddEntry("b", "2");
  jw.AddEntry("c")<<3;
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"b\": \"2\"\n"
    " ,\"c\": \"3\"\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, GroupTest)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("a")<<1;
  jw.StartGroup("grp");
  jw.AddEntry("b", "2");
  jw.AddEntry("c")<<3;
  jw.EndGroup();
  jw.AddEntry("d", "4");
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"grp\": {\n"
    "    \"b\": \"2\"\n"
    "   ,\"c\": \"3\"\n"
    "  }\n"
    " ,\"d\": \"4\"\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, SingleEntryList)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("a")<<1;
  jw.StartList("lst");
  jw.NextListItem();
  jw.AddEntry("b", "2");
  jw.AddEntry("c")<<3;
  jw.EndList();
  jw.AddEntry("d", "4");
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"lst\": [\n"
    "    {\n"
    "      \"b\": \"2\"\n"
    "     ,\"c\": \"3\"\n"
    "    }\n"
    "  ]\n"
    " ,\"d\": \"4\"\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}


TEST(JsonWriter, List)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("a")<<1;
  jw.StartList("lst");
  jw.NextListItem();
  jw.AddEntry("b", "2");
  jw.AddEntry("c")<<3;
  jw.NextListItem();
  jw.AddEntry("b", "20");
  jw.AddEntry("c")<<30;
  jw.EndList();
  jw.AddEntry("d", "4");
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"lst\": [\n"
    "    {\n"
    "      \"b\": \"2\"\n"
    "     ,\"c\": \"3\"\n"
    "    }\n"
    "   ,{\n"
    "      \"b\": \"20\"\n"
    "     ,\"c\": \"30\"\n"
    "    }\n"
    "  ]\n"
    " ,\"d\": \"4\"\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, SingleEntryNakedList)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("a")<<1;
  jw.StartList("lst");
  jw.AddRawListEntry("lstVal");
  jw.EndList();
  jw.AddEntry("d", "4");
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"lst\": [\n"
    "    \"lstVal\"\n"
    "  ]\n"
    " ,\"d\": \"4\"\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, SingleEntryNakedListFloat)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("a")<<1;
  jw.StartList("lst");
  jw.AddRawListEntry(1.1f);
  jw.EndList();
  jw.AddEntry("d", "4");
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"lst\": [\n"
    "    1.1\n"
    "  ]\n"
    " ,\"d\": \"4\"\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, NakedListInts)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("a")<<1;
  jw.StartList("lst");
  jw.AddRawListEntry(1);
  jw.AddRawListEntry(2);
  jw.AddRawListEntry(10);
  jw.EndList();
  jw.AddEntry("d", "4");
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"lst\": [\n"
    "    1\n"
    "   ,2\n"
    "   ,10\n"
    "  ]\n"
    " ,\"d\": \"4\"\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, NakedListMixed)
{
  stringstream ss;
  JsonWriter jw(ss);
  jw.AddEntry("a")<<1;
  jw.StartList("lst");
  jw.AddRawListEntry(1.1f);
  jw.AddRawListEntry("asdf");
  jw.AddRawListEntry(2.4);
  jw.AddRawListEntry(10);
  jw.EndList();
  jw.AddEntry("d", "4");
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"lst\": [\n"
    "    1.1\n"
    "   ,\"asdf\"\n"
    "   ,2.4\n"
    "   ,10\n"
    "  ]\n"
    " ,\"d\": \"4\"\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, Complex)
{
  stringstream ss;
  JsonWriter jw(ss);

  jw.AddEntry("a", "1");
  jw.AddEntry("b")<<2;
  jw.StartList("lst");
  jw.NextListItem();
  jw.AddEntry("c")<<3;
  jw.StartList("lst2");
  jw.NextListItem();
  jw.AddEntry("d")<<4;
  jw.AddEntry("e", "5");
  jw.NextListItem();
  jw.AddEntry("d")<<40;
  jw.AddEntry("e")<<50;
  jw.Close();

  string exp =
    "{\n"
    "  \"a\": \"1\"\n"
    " ,\"b\": \"2\"\n"
    " ,\"lst\": [\n"
    "    {\n"
    "      \"c\": \"3\"\n"
    "     ,\"lst2\": [\n"
    "        {\n"
    "          \"d\": \"4\"\n"
    "         ,\"e\": \"5\"\n"
    "        }\n"
    "       ,{\n"
    "          \"d\": \"40\"\n"
    "         ,\"e\": \"50\"\n"
    "        }\n"
    "      ]\n"
    "    }\n"
    "  ]\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}

TEST(JsonWriter, ListNextItem)
{

  stringstream ss;
  JsonWriter jw(ss);
  jw.StartList("lst");
  jw.NextListItem();
  jw.AddEntry("a", "1");
  jw.Close();

  string exp =
    "{\n"
    "  \"lst\": [\n"
    "    {\n"
    "      \"a\": \"1\"\n"
    "    }\n"
    "  ]\n"
    "}\n";

  EXPECT_EQ(exp, ss.str());
}
