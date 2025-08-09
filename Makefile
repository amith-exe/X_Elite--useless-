# Compiler to use (clang is standard in CS50)
CC = clang

# Compiler flags for creating a shared library
# -shared: Create a shared library
# -fPIC: Generate Position-Independent Code, required for shared libraries
CFLAGS = -Wall -g -std=c11 -shared -fPIC

# The name of our final shared library
TARGET = steganography.so

# The C source file
SRCS = steganography.c

# Default rule: build the library
all: $(TARGET)

# Rule to compile the source file into the shared library
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Rule to clean up compiled files
clean:
	rm -f $(TARGET)
