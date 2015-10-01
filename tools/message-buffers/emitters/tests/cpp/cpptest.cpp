#include "SimpleTest.h"
#include "aligned/AutoUnionTest.h"
#include <iostream>
#include <algorithm>

#pragma GCC diagnostic ignored "-Wmissing-braces"

bool test_AnkiEnum()
{
  std::cout << "Test AnkiEnum: " << std::endl;
  using ae = AnkiTypes::AnkiEnum;
  ae vals[7] = {
    ae::e1,
    ae::e2,
    ae::e3,
    ae::myReallySilly_EnumVal,
    ae::d1,
    ae::d2,
    ae::d3
  };
  for (const auto& e : vals) {
    std::cout << AnkiTypes::AnkiEnumToString(e) << "'s value is " << int(e) << std::endl;
  }
  if (0 != strcmp("e1", AnkiTypes::AnkiEnumToString(ae::e1))) {
    std::cout << "FAIL" << std::endl;
    return false;
  }
  if (0 != strcmp("myReallySilly_EnumVal",
                  AnkiTypes::AnkiEnumToString(ae::myReallySilly_EnumVal))) {
    std::cout << "FAIL" << std::endl;
    return false;
  }
  if (nullptr != AnkiTypes::AnkiEnumToString((ae) (-1))) {
    std::cout << "FAIL" << std::endl;
    return false;
  }
  std::cout << "PASS" << std::endl;
  return true;
}

bool test_Foo()
{
  std::cout << "Test Foo: ";
  Foo myFoo;
  myFoo.isFoo = true;
  myFoo.myByte = 0x0f;
  myFoo.byteTwo = 0x0e;
  myFoo.myShort = 0x0c0a;
  myFoo.myFloat = -1823913982.0f;
  myFoo.myNormal = 0x0eadbeef;
  myFoo.myFoo = AnkiTypes::AnkiEnum::d2;
  myFoo.myString = "Blah Blah Blah";
  size_t length = myFoo.Size();
  std::cout << "myFoo is " << length << " bytes" << std::endl;
  uint8_t* buff = new uint8_t[length];
  myFoo.Pack(&buff[0], length);
  Foo otherFoo;
  otherFoo.Unpack(&buff[0], length);
  delete buff;
  if (myFoo != otherFoo) {
    std::cout << "FAIL" << std::endl;
    return false;
  }
  else {
    std::cout << "PASS" << std::endl;
    return true;
  }
}

bool test_Bar()
{
  std::cout << "Test Bar: ";
  Bar myBar {
        { true, false, false, true, true },
        { 0, 1, 2, 3, 4},
        { 5, 6, 7 },
        { AnkiTypes::AnkiEnum::d1, AnkiTypes::AnkiEnum::e1 },
        { 1, 1, 1 },
        "Foo Bar Baz",
        { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
        { true, true, true, true, true, false, false, false, false, false },
        { AnkiTypes::AnkiEnum::e1, AnkiTypes::AnkiEnum::e3 }
  };
  std::cout << "myBar is " << myBar.Size() << " bytes" << std::endl;
  size_t length = myBar.Size();
  uint8_t* buff = new uint8_t[length];
  Bar otherBar;
  myBar.Pack(buff, length);
  otherBar.Unpack(buff, length);
  delete[] buff;
  if (myBar != otherBar) {
    std::cout << "FAIL" << std::endl;
    return false;
  }
  else {
    std::cout << "PASS" << std::endl;
    return true;
  }
}

bool test_Dog()
{
  std::cout << "Test Dog: ";
  Baz::Dog myDog { AnkiTypes::AnkiEnum::e2, 5 };

  std::cout << "myDog is " << myDog.Size() << " bytes" << std::endl;
  size_t length = myDog.Size();
  uint8_t* buff = new uint8_t[length];
  Baz::Dog otherDog;
  myDog.Pack(buff, length);
  otherDog.Unpack(buff, length);
  delete[] buff;
  if (myDog != otherDog) {
    std::cout << "FAIL" << std::endl;
    return false;
  } else {
    std::cout << "PASS" << std::endl;
    return true;
  }
}

bool test_SoManyStrings()
{
  std::cout << "Test SoManyStrings: ";
  SoManyStrings mySoManyStrings {
    { "one", "two", "three", "four"},
    { "uno", "dos", "tres"},
    { "" },
    { "yi", ""}
  };

  std::cout << "mySoManyStrings is " << mySoManyStrings.Size() << " bytes" << std::endl;
  size_t length = mySoManyStrings.Size();
  uint8_t* buff = new uint8_t[length];
  SoManyStrings otherSoManyStrings;
  mySoManyStrings.Pack(buff, length);
  otherSoManyStrings.Unpack(buff, length);
  delete[] buff;
  if (mySoManyStrings != otherSoManyStrings) {
    std::cout << "FAIL" << std::endl;
    return false;
  } else {
    std::cout << "PASS" << std::endl;
    return true;
  }
}

bool test_od432()
{
  std::cout << "Test od432 (message in a message): ";
  Foo aFoo {false, 1, 2, 3, 1.0, 5555, AnkiTypes::AnkiEnum::e3, "hello"};
  od432 myOD432 { aFoo, 5};

  std::cout << "myOD432 is " << myOD432.Size() << " bytes" << std::endl;
  size_t length = myOD432.Size();
  uint8_t* buff = new uint8_t[length];
  od432 otherOD432;
  myOD432.Pack(buff, length);
  otherOD432.Unpack(buff, length);
  delete[] buff;
  if (myOD432 != otherOD432) {
    std::cout << "FAIL" << std::endl;
    return false;
  } else {
    std::cout << "PASS" << std::endl;
    return true;
  }
}

bool test_od433()
{
  std::cout << "Test od433 (array of messages in a message): ";
  Foo aFoo {false, 1, 2, 3, 1.0, 5555, AnkiTypes::AnkiEnum::e3, "hello"};
  Foo bFoo {true, 3, 2, 1, 5.0, 999, AnkiTypes::AnkiEnum::e1, "world"};
  Foo cFoo {false, 4, 5, 6, 2.0, 4555, AnkiTypes::AnkiEnum::d1, "bye"};
  Foo dFoo {true, 7, 8, 9, 7.0, 989, AnkiTypes::AnkiEnum::d2, "bye"};
  od433 myOD433 { {aFoo, bFoo}, {cFoo, dFoo}, 5};

  std::cout << "myOD433 is " << myOD433.Size() << " bytes" << std::endl;
  size_t length = myOD433.Size();
  uint8_t* buff = new uint8_t[length];
  od433 otherOD433;
  myOD433.Pack(buff, length);
  otherOD433.Unpack(buff, length);
  delete[] buff;
  if (myOD433 != otherOD433) {
    std::cout << "FAIL" << std::endl;
    return false;
  } else {
    std::cout << "PASS" << std::endl;
    return true;
  }
}

bool test_MyMessage()
{
  std::cout << "Test MyMessage: ";
  Cat::MyMessage message;
  std::hash<Cat::MyMessageTag> type_hash_fn;
  Foo myFoo;
  myFoo.isFoo = true;
  myFoo.myByte = 0x0f;
  myFoo.byteTwo = 0x0e;
  myFoo.myShort = 0x0c0a;
  myFoo.myNormal = 0x0eadbeef;
  myFoo.myFloat = -18.02e-33f;
  myFoo.myFoo = AnkiTypes::AnkiEnum::d2;
  myFoo.myString = "Whatever";
  message.Set_myFoo(myFoo);
  std::cout << "msg = " << message.Size() << " bytes" << std::endl;
  std::cout << "msg.Tag = " << Cat::MyMessageTagToString(message.GetTag()) << std::endl;
  std::cout << "hash of msg.Tag = " << type_hash_fn(message.GetTag()) << std::endl;
  CLAD::SafeMessageBuffer  buff(message.Size());
  message.Pack(buff);
  Cat::MyMessage otherMessage(buff);

  if(otherMessage.GetTag() != Cat::MyMessageTag::myFoo
     || message.Get_myFoo() != otherMessage.Get_myFoo()) {
    std::cout << "FAIL" << std::endl;
    return false;
  }
  Bar myBar;
  myBar.boolBuff = std::vector<bool> { true, false, false, true, true, false };
  myBar.byteBuff  = std::vector<int8_t> { 0x0f, 0x0e, 0x0c, 0x0a };
  myBar.shortBuff = std::vector<int16_t> { 0x0fed, 0x0caf, 0x0a2f, 0x0a12 };
  myBar.enumBuff  = std::vector<AnkiTypes::AnkiEnum> { AnkiTypes::AnkiEnum::myReallySilly_EnumVal, AnkiTypes::AnkiEnum::e2 };
  myBar.doubleBuff = std::vector<double> { 3128312.031312e132, 123131e-12, 123 };
  myBar.myLongerString = "SomeLongerStupidString";
  myBar.fixedBuff = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  myBar.fixedBoolBuff = { true, true, true, true, true, false, false, false, false, false };
  myBar.fixedEnumBuff = { AnkiTypes::AnkiEnum::e2, AnkiTypes::AnkiEnum::e3 };
  message.Set_myBar(myBar);
  std::cout << "msg = " << message.Size() << " bytes" << std::endl;
  std::cout << "hash of msg.Tag = " << type_hash_fn(message.GetTag()) << std::endl;
  buff.~SafeMessageBuffer();
  new (&buff) CLAD::SafeMessageBuffer(message.Size());
  message.Pack(buff);
  otherMessage.Unpack(buff);
  buff.~SafeMessageBuffer();

  if(otherMessage.GetTag() != Cat::MyMessageTag::myBar
     || message.Get_myBar() != otherMessage.Get_myBar()) {
    std::cout << "FAIL" << std::endl;
    return false;
  }

  Baz::Dog myDog;
  myDog.a = AnkiTypes::AnkiEnum::e2;
  myDog.b = 55;

  message.Set_myDog(myDog);
  std::cout << "msg = " << message.Size() << " bytes" << std::endl;
  std::cout << "hash of msg.Tag = " << type_hash_fn(message.GetTag()) << std::endl;
  buff.~SafeMessageBuffer();
  new (&buff) CLAD::SafeMessageBuffer(message.Size());
  message.Pack(buff);
  otherMessage.Unpack(buff);
  buff.~SafeMessageBuffer();

  if (otherMessage.GetTag() != Cat::MyMessageTag::myDog
      || message.Get_myDog() != otherMessage.Get_myDog()) {
    std::cout << "FAIL" << std::endl;
    return false;
  }

  od432 myOD432;
  Foo aFoo {false, 1, 2, 3, 1.0, 5555, AnkiTypes::AnkiEnum::e3, "hello"};
  myOD432.aFoo = aFoo;
  myOD432.otherByte = 5;

  message.Set_myOD432(myOD432);
  std::cout << "msg = " << message.Size() << " bytes" << std::endl;
  std::cout << "hash of msg.Tag = " << type_hash_fn(message.GetTag()) << std::endl;
  buff.~SafeMessageBuffer();
  new (&buff) CLAD::SafeMessageBuffer(message.Size());
  message.Pack(buff);
  otherMessage.Unpack(buff);
  buff.~SafeMessageBuffer();

  if (otherMessage.GetTag() != Cat::MyMessageTag::myOD432
      || message.Get_myOD432() != otherMessage.Get_myOD432()) {
    std::cout << "FAIL" << std::endl;
    return false;
  }

  std::cout << "PASS" << std::endl;
  return true;
}

bool test_AutoUnion() {
  std::cout << "Test AutoUnion: " << std::endl;
  FunkyMessage msg;
  Funky funky {AnkiTypes::AnkiEnum::e1, 3};
  Monkey aMonkey {123182931, funky};

  msg.Set_Monkey(aMonkey);
  std::cout << "msg = " << msg.Size() << " bytes" << std::endl;
  std::cout << "msg.Tag = " << FunkyMessageTagToString(msg.GetTag()) << std::endl;

  Music music {{123}, funky};
  msg.Set_Music(music);

  std::cout << "PASS" << std::endl;
  return true;
}

int main()
{
  if(!test_Foo()) {
    return 1;
  }
  if(!test_Bar()) {
    return 2;
  }
  if (!test_Dog()) {
    return 3;
  }
  if (!test_SoManyStrings()) {
    return 4;
  }
  if (!test_od432()) {
    return 5;
  }
  if (!test_od433()) {
    return 6;
  }
  if (!test_AnkiEnum()) {
    return 7;
  }
  if(!test_MyMessage()) {
    return 99;
  }
  if(!test_AutoUnion()) {
    return 100;
  }
  return 0;
}
