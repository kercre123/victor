CFLAGS = -Wall -Wextra -std=gnu++11

all: zipReaderTest

testmin: zipReaderTest
	./zipReaderTest min.zip

clean:
	rm -rf zipReaderTest zipReaderTest.dSYM

zipReaderTest: zipReaderTest.cpp zipReader.cpp zipReader.h
	g++ $(CFLAGS) -g -o zipReaderTest zipReaderTest.cpp zipReader.cpp
