CXX = g++
CXXFLAGS = -std=c++11 -pthread  # C++11 standard and pthread library flag
SOURCES = main.cpp helper.cpp WriteOutput.cpp  
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o simulator 

clean:
	rm -f $(OBJECTS) simulator  

%.o: %.cpp  # Compile each source file into an object file
	$(CXX) $(CXXFLAGS) -c $< -o $@

