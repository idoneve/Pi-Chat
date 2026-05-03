# Compiler
GPP = gcc

# Compilation flags
GPPFlags = -Wall -Wextra
# -Wall -Wextra -Werror -Wfatal-errors
Debug = -g

SRC := $(wildcard ./src/*.c)

INCLUDE = ./include/

OBJ = $(SRC:.c=.o)
TARGET = client.out

# Library
RAYLIB_MAC = ./lib/raylib-5.5_macos/libraylib.a
RAYLIB_LINUX_64 = ./lib/raylib-5.5_linux_amd64/libraylib.a

# Detect system architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

$(info SRC: $(SRC))

# Select Raylib based on the platform and architecture
ifeq ($(UNAME_S), Darwin)  # macOS
    ifeq ($(UNAME_M), x86_64)
        RAYLIB = $(RAYLIB_MAC)
    endif
endif

ifeq ($(UNAME_S), Linux)  # Linux
    ifeq ($(UNAME_M), x86_64)
        RAYLIB = $(RAYLIB_LINUX_64)
    endif
endif

all: $(TARGET) clean


# Link
$(TARGET): $(OBJ)
	$(GPP) $(GPPFlags) $(OBJ) -o $(TARGET) $(RAYLIB)  -lm

# Compile
%.o: %.c
	$(GPP) $(GPPFlags) -c $< -o $@ -I $(INCLUDE)


.PHONY: clean package

# clean
clean:
	rm -f $(OBJ)
	@echo "Removed all object files."

reset: clean
	rm -f $(TARGET)
	@echo "Removed all output files."

package:
	zip $(TARNAME).zip Makefile $(SRC) $(HEADER)
