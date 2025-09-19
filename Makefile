CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra
TARGET = orderbook
SOURCES = $(wildcard *.cpp)

.PHONY: clean rebuild

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

rebuild:
	make clean && make


