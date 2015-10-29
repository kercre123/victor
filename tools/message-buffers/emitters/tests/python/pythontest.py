
from __future__ import absolute_import
from __future__ import print_function

import math
import sys

from SimpleTest import AnkiTypes, Foo, Bar, Cat, SoManyStrings
from aligned.AutoUnionTest import FunkyMessage, Funky, Monkey, Music

def test_Foo():
    print("Test foo")
    foo1a = Foo()
    foo1b = Foo()
    if foo1a != foo1b:
        sys.exit("foo sanity check failed")
    
    foo2a = Foo()
    foo2b = Foo.unpack(foo2a.pack())
    if foo2a != foo2b:
        sys.exit('foo empty unpack failed\n{0!r}\nvs\n{1!r}'.format(foo2a, foo2b))
    
    foo3a = Foo(isFoo=True, myByte=-5, byteTwo=203, myFloat=13231321, myShort=32000, myNormal=0xFFFFFFFF,
        myFoo=AnkiTypes.AnkiEnum.e3, myString=('laadlsjk' * 10))
    foo3b = Foo.unpack(foo3a.pack())
    if foo3a != foo3b:
        sys.exit('foo constructor check failed\n{0!r}\nvs\n{1!r}'.format(foo3a, foo3b))
    
    foo4a = Foo()
    foo4a.isFoo=True
    foo4a.myByte=31
    foo4a.byteTwo=128
    foo4a.myShort=0
    foo4a.myFloat=1323.12
    foo4a.myNormal=65536
    foo4a.myFoo=AnkiTypes.AnkiEnum.e1
    foo4a.myString=''
    
    foo4b = Foo.unpack(foo4a.pack())
    if foo4a != foo4b:
        sys.exit('foo assign check failed\n{0!r}\nvs\n{1!r}'.format(foo4a, foo4b))
    
    print("Foo contents: {0}".format(foo3a))
    print("Foo length: {0}".format(len(foo3a)))
    
    print("Test foo succeeded")

def test_Bar():
    print("Test bar")
    bar1a = Bar()
    bar1b = Bar()
    if bar1a != bar1b:
        sys.exit('bar sanity check failed')
    
    bar2a = Bar()
    bar2b = Bar.unpack(bar2a.pack())
    if bar2a != bar2b:
        sys.exit('bar empty unpack failed\n{0!r}\nvs\n{1!r}'.format(bar2a, bar2b))
    
    bar3a = Bar(
        (True, True, True),
        [-128, 127, 0, 1, -1],
        (-23201, 23201, 102),
        (),
        (sys.float_info.epsilon, 0, -0, 1, -1, sys.float_info.max, sys.float_info.min,
            sys.float_info.radix, float('inf'), float('-inf')),
        'long' * 256,
        (i ^ 0x1321 for i in range(20)),
        (False for i in range(10)))
    bar3b = Bar.unpack(bar3a.pack())
    if bar3a != bar3b:
        sys.exit('bar constructor check failed\n{0!r}\nvs\n{1!r}'.format(bar3a, bar3b))
    
    bar4a = Bar()
    bar4a.boolBuff = (i % 3 == 0 for i in range(200))
    bar4a.byteBuff = ()
    bar4a.shortBuff = tuple(range(255))
    bar4a.enumBuff = [AnkiTypes.AnkiEnum.e3 for i in range(10)]
    bar4a.doubleBuff = ()
    bar4a.myLongerString = ''.join(str(i) for i in range(10))
    bar4a.fixedBuff = [-1] * 20
    bar4a.fixedBoolBuff = (i % 3 == 0 for i in range(10))
    
    bar4b = Bar.unpack(bar4a.pack())
    if bar4a != bar4b:
        sys.exit('bar constructor check failed\n{0!r}\nvs\n{1!r}'.format(bar4a, bar4b))
    
    
    print("Bar contents: {0}".format(bar4a))
    print("Bar length: {0}".format(len(bar4a)))
        
    print("Test bar succeeded")

def test_SoManyStrings():
    print("Test SoManyStrings")
    somany1a = SoManyStrings()
    somany1b = SoManyStrings()
    if somany1a != somany1b:
        sys.exit('SoManyStrings sanity check failed')
    
    somany2a = SoManyStrings()
    somany2b = SoManyStrings.unpack(somany2a.pack())
    if somany2a != somany2b:
        sys.exit('SoManyStrings empty unpack failed\n{0!r}\nvs\n{1!r}'.format(somany2a, somany2b))
    
    somany3a = SoManyStrings(
        (hex(x*x*x^3423) for x in range(2000)),
        ('na' * i for i in range(3)),
        ('ads', 'fhg', 'jlk'),
        ('Super', 'Troopers'))
    somany3b = SoManyStrings.unpack(somany3a.pack())
    if somany3a != somany3b:
        sys.exit('SoManyStrings constructor check failed\n{0!r}\nvs\n{1!r}'.format(somany3a, somany3b))
    
    somany4a = SoManyStrings()
    somany4a.varStringBuff = (chr(32 + i) for i in range(80))
    somany4a.fixedStringBuff = 'abc'
    somany4a.anotherVarStringBuff = [u'\u1233\u1231 foo', 'asdas', '\xC2\xA2']
    somany4a.anotherFixedStringBuff = ['', '\0']
    
    somany4b = SoManyStrings.unpack(somany4a.pack())
    if somany4a != somany4b:
        sys.exit('SoManyStrings constructor check failed\n{0!r}\nvs\n{1!r}'.format(somany4a, somany4b))
    
    print("SoManyStrings contents: {0}".format(somany4a))
    print("SoManyStrings length: {0}".format(len(somany4a)))
    
    print("Test SoManyStrings succeeded")

def test_MyMessage():
    print("Test Cat.MyMessage")
    msg1a = Cat.MyMessage()
    msg1b = Cat.MyMessage()
    if msg1a != msg1b:
        sys.exit('Cat.MyMessage sanity check failed')
    
    msg2a = Cat.MyMessage(myFoo=Foo()) # can't pack untagged
    msg2b = Cat.MyMessage.unpack(msg2a.pack())
    if msg2a != msg2b:
        sys.exit('Cat.MyMessage empty unpack failed\n{0!r}\nvs\n{1!r}'.format(msg2a, msg2b))
    
    msg3a = Cat.MyMessage(myFoo=Foo(
        False,
        -128,
        127,
        -23201,
        float('inf'),
        102030,
        0,
        'z' * 255))
    msg3b = Cat.MyMessage.unpack(msg3a.pack())
    if msg3a != msg3b:
        sys.exit('Cat.MyMessage constructor check failed\n{0!r}\nvs\n{1!r}'.format(msg3a, msg3b))
    
    msg4a = Cat.MyMessage()
    msg4a.myBar = Bar()
    msg4a.myBar.boolBuff = (True, False)
    msg4a.myBar.byteBuff = ()
    msg4a.myBar.shortBuff = (i ^ 0x1321 for i in range(20))
    msg4a.myBar.enumBuff = [AnkiTypes.AnkiEnum.d3 for i in range(10)]
    msg4a.myBar.doubleBuff = [math.sqrt(x**3) for x in range(100)]
    msg4a.myBar.myLongerString = ''.join(str(i) for i in range(100))
    msg4a.myBar.fixedBuff = [sum(range(i)) for i in range(20)]
    msg4a.myBar.fixedBoolBuff = (True,) * 10
    
    msg4b = Cat.MyMessage.unpack(msg4a.pack())
    if msg4a != msg4b:
        sys.exit('Cat.MyMessage constructor check failed\n{0!r}\nvs\n{1!r}'.format(msg4a, msg4b))
    
    
    print("Cat.MyMessage contents: {0}".format(msg4a))
    print("Cat.MyMessage length: {0}".format(len(msg4a)))
        
    print("Test Cat.MyMessage succeeded")

def test_AutoUnion():
  print("Test AutoUnion: ")
  
  msg = FunkyMessage()
  funky = Funky(AnkiTypes.AnkiEnum.e1, 3)
  aMonkey = Monkey(1331232132, funky)
  
  msg.Monkey = aMonkey
  print("msg = {0} bytes".format(len(msg)))
  
  music = Music((131,), funky)
  msg.Music = music
  
  print("PASS")

if __name__ == '__main__':
    test_Foo()
    test_Bar()
    test_SoManyStrings()
    test_MyMessage()
    test_AutoUnion()

