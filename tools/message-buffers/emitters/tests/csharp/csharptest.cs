// CSharp message packing test tool
// Author: Mark Pauley
// Date: January 26, 2015

public class HelloWorld {
  public static bool Test_Foo() {
    Foo myFoo = new Foo(true, 0x7f, 0xfe, 0xafe, -123123,
                        0xdeadbeef, AnkiTypes.AnkiEnum.myReallySilly_EnumVal,
                        "Blah Blah Blah");
    Foo otherFoo = new Foo();

    System.Console.WriteLine("myFoo = " + myFoo.Size + " bytes");
    System.IO.MemoryStream stream = new System.IO.MemoryStream();
    myFoo.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherFoo.Unpack(stream);

    return myFoo.Equals(otherFoo);
  }

  public static bool Test_Bar() {
    Bar myBar;
    Bar otherBar;

    myBar = new Bar(new bool[3] { true, false, false },
                    new sbyte[5] { 0, 1, 2, 3, 4 },
                    new short[3] { 5, 6, 7 },
                    new AnkiTypes.AnkiEnum[2] { AnkiTypes.AnkiEnum.d1,
                                                AnkiTypes.AnkiEnum.e1 },
                    new double[] { double.MaxValue, double.PositiveInfinity,
                                   double.MinValue, -0, +0, double.NegativeInfinity, 1, -1 },
                    "Foo Bar Baz",
                    new short[20] {1,2,3,4,5,6,7,8,9,10,
                                   11,12,13,14,15,16,17,18,19,20},
                    new bool[10] {false, false, false, false, false,
                                  true, true, true, true, true},
                    new AnkiTypes.AnkiEnum[2] { AnkiTypes.AnkiEnum.e1,
                                                AnkiTypes.AnkiEnum.e3 });


    System.Console.WriteLine("myBar = " + myBar.Size + " bytes");
    System.IO.MemoryStream stream = new System.IO.MemoryStream();
    myBar.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherBar = new Bar(stream);

    return myBar.Equals(otherBar);
  }

  public static bool Test_Dog() {
    Baz.Dog myDog;
    Baz.Dog otherDog;

    myDog = new Baz.Dog(AnkiTypes.AnkiEnum.e1, 5);
    System.Console.WriteLine("myDog = " + myDog.Size + " bytes");
    System.IO.MemoryStream stream = new System.IO.MemoryStream();
    myDog.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherDog = new Baz.Dog(stream);

    return myDog.Equals(otherDog);
  }

  public static bool Test_od432() {
    od432 myOD432;
    od432 otherOD432;
    Foo myFoo = new Foo(true, 0x7f, 0xfe, 0xafe, -123123, 0xdeadbeef,
                        AnkiTypes.AnkiEnum.myReallySilly_EnumVal,
                        "Blah Blah Blah");
    myOD432 = new od432(myFoo, 5);
    System.Console.WriteLine("myOD432 = " + myOD432.Size + " bytes");
    System.IO.MemoryStream stream = new System.IO.MemoryStream();
    myOD432.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherOD432 = new od432(stream);

    return myOD432.Equals(otherOD432);
  }

  public static bool Test_od433() {
    od433 myOD433;
    od433 otherOD433;
    Foo fooA = new Foo(true, 0x7f, 0xfe, 0xafe, -123123, 0xdeadbeef,
                       AnkiTypes.AnkiEnum.myReallySilly_EnumVal,
                       "Blah Blah Blah");
    Foo fooB = new Foo(false, 0x1f, 0xfc, 0x123, -26874, 0xbeefdead,
                       AnkiTypes.AnkiEnum.e3,
                       "Yeah Yeah Yeah");
    Foo fooC = new Foo(true, 0x3f, 0xae, 0xbfe, -143123, 0xdeedbeef,
                       AnkiTypes.AnkiEnum.d1,
                       "Doh Doh Doh");
    Foo fooD = new Foo(true, 0xf, 0xfd, 0x143, -25874, 0xbeefdeed,
                       AnkiTypes.AnkiEnum.e2,
                       "Yo Yo Yo");
    myOD433 = new od433(new Foo[2] {fooA, fooB}, new Foo[2] {fooC, fooD}, 6);
    System.Console.WriteLine("myOD433 = " + myOD433.Size + " bytes");
    System.IO.MemoryStream stream = new System.IO.MemoryStream();
    myOD433.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherOD433 = new od433(stream);

    return myOD433.Equals(otherOD433);
  }

  public static bool Test_SoManyStrings() {
    SoManyStrings mySoManyStrings;
    SoManyStrings otherSoManyStrings;

    mySoManyStrings = new SoManyStrings(new string[2] { "one", "two" },
                                        new string[3] { "uno", "dos", "tres" },
                                        new string[4] { "yi", "ar", "san", "si" },
                                        new string[2] { "un", "deux" });
    System.Console.WriteLine("mySoManyStrings = " + mySoManyStrings.Size + " bytes");
    System.IO.MemoryStream stream = new System.IO.MemoryStream();
    mySoManyStrings.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherSoManyStrings = new SoManyStrings(stream);

    return mySoManyStrings.Equals(otherSoManyStrings);
  }

  public static bool Test_od434() {
    od434 myOD434;
    od434 otherOD434;

    myOD434 = new od434(new ulong[3] {1UL, 2UL, 3UL});

    System.Console.WriteLine("myOD434 = " + myOD434.Size + " bytes");
    System.IO.MemoryStream stream = new System.IO.MemoryStream();
    myOD434.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherOD434 = new od434(stream);

    return myOD434.Equals(otherOD434);
  }

  public static bool Test_Union() {
    Cat.MyMessage msg = new Cat.MyMessage();
    Cat.MyMessage otherMsg = new Cat.MyMessage();

    Foo myFoo = new Foo();

    myFoo.myByte = 0x7f;
    myFoo.byteTwo = 0xfe;
    myFoo.myShort = 0x0afe;
    myFoo.myFloat = 123.123123e12f;
    myFoo.myNormal = 0x0eadbeef;
    myFoo.myFoo = AnkiTypes.AnkiEnum.myReallySilly_EnumVal;
    myFoo.myString = "Whatever";

    msg.myFoo = myFoo;

    System.Console.WriteLine("msg = " + msg.Size + " bytes");
    System.IO.MemoryStream stream = new System.IO.MemoryStream();
    msg.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherMsg.Unpack(stream);

    if (!msg.Equals(otherMsg)) {
      return false;
    }

    Bar myBar = new Bar();

    myBar.boolBuff = new bool[3] {true, false, true};
    myBar.byteBuff = new sbyte[4] { 0x0f, 0x0e, 0x0c, 0x0a };
    myBar.shortBuff = new short[4] { 0x0fed, 0x0caf, 0x0a2f, 0x0a12};
    myBar.enumBuff = new AnkiTypes.AnkiEnum[2] { AnkiTypes.AnkiEnum.myReallySilly_EnumVal,
                                                 AnkiTypes.AnkiEnum.e2 };
    myBar.doubleBuff = new double[1] { double.NaN };
    myBar.myLongerString = "SomeLongerStupidString";
    myBar.fixedBuff = new short[20] {1,2,3,4,5,6,7,8,9,10,
                                     11,12,13,14,15,16,17,18,19,20};
    myBar.fixedBoolBuff = new bool[10] {false, false, false, false, false,
                                        true, true, true, true, true};


    msg.myBar = myBar;

    System.Console.WriteLine("msg = " + msg.Size + " bytes");
    stream = new System.IO.MemoryStream();
    msg.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherMsg.Unpack(stream);

    if (!msg.Equals(otherMsg)) {
      return false;
    }

    Baz.Dog myDog = new Baz.Dog();

    myDog.a = AnkiTypes.AnkiEnum.e3;
    myDog.b = 9;

    msg.myDog = myDog;
    System.Console.WriteLine("msg = " + msg.Size + " bytes");
    stream = new System.IO.MemoryStream();
    msg.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherMsg.Unpack(stream);

    if (!msg.Equals(otherMsg)) {
      return false;
    }

    SoManyStrings mySoManyStrings = new SoManyStrings();

    mySoManyStrings.varStringBuff = new string[2] { "one", "two" };
    mySoManyStrings.fixedStringBuff =  new string[3] { "uno", "dos", "tres" };
    mySoManyStrings.anotherVarStringBuff = new string[4] { "yi", "ar", "san", "si" };
    mySoManyStrings.anotherFixedStringBuff = new string[2] { "un", "deux" };

    msg.mySoManyStrings = mySoManyStrings;
    System.Console.WriteLine("msg = " + msg.Size + " bytes");
    stream = new System.IO.MemoryStream();
    msg.Pack(stream);
    stream.Seek(0, System.IO.SeekOrigin.Begin);
    otherMsg.Unpack(stream);

    if (!msg.Equals(otherMsg)) {
      return false;
    }

    return true;
  }

  public static void Main() {
    System.Console.Write("Test_Foo: ");
    System.Console.WriteLine(Test_Foo() ? "PASS" : "FAIL");

    System.Console.Write("Test_Bar: ");
    System.Console.WriteLine(Test_Bar() ? "PASS" : "FAIL");

    System.Console.Write("Test_Dog: ");
    System.Console.WriteLine(Test_Dog() ? "PASS" : "FAIL");

    System.Console.Write("Test_od432: ");
    System.Console.WriteLine(Test_od432() ? "PASS" : "FAIL");

    System.Console.Write("Test_od433: ");
    System.Console.WriteLine(Test_od433() ? "PASS" : "FAIL");

    System.Console.Write("Test_SoManyStrings: ");
    System.Console.WriteLine(Test_SoManyStrings() ? "PASS" : "FAIL");

    System.Console.Write("Test_od434: ");
    System.Console.WriteLine(Test_od434() ? "PASS" : "FAIL");

    System.Console.Write("Test_Union: ");
    System.Console.WriteLine(Test_Union() ? "PASS" : "FAIL");
  }
}
