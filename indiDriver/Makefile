# Compiler and flags
CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++11 -g -fPIC

# INDI include and library paths
INDI_INCLUDE := $(shell indiserver -v 2>&1 | grep "/home/stellarmate/Projects/indi/cmake_modules/FindINDI.cmake" | awk '{print $4}' | sed 's/"//g' | sed 's/,/ /g' | awk '{ for (i=1; i<=NF; i++) print "-I"$i"/include/libindi"}')
INDI_LIBS := -lindi -lpthread -lm -ludev

# Source files
SRC := indi_flatfield.cpp

# Object files
OBJ := $(SRC:.cpp=.o)

# Output driver name
DRIVER := indi_flatfield

all: $(DRIVER)

# Compile object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INDI_INCLUDE) -c $< -o $@

# Link the driver
$(DRIVER): $(OBJ)
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,$(DRIVER) -o $(DRIVER) $(OBJ) $(INDI_LIBS)

# Install (requires sudo)
install: $(DRIVER)
	sudo cp $(DRIVER) /usr/lib/indi/
	sudo chmod +x /usr/lib/indi/$(DRIVER)

# Clean
clean:
	rm -f $(OBJ) $(DRIVER)

.PHONY: all install clean
