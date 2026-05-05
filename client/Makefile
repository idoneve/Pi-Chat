# C Version
CSTD ?= c23

# File names
MAKENAME ?= Makefile
README ?= $(wildcard README.*)

# Variables
OPTIMIZER ?= O2
ARGS ?=

# Flags
ifeq ($(shell uname -s),Darwin)
  CC = clang
  CFLAGS = -D_FORTIFY_SOURCE=2 -fstack-protector-all -g -$(OPTIMIZER) -std=$(CSTD) \
         -Wall -Wextra -Wpedantic -Wshadow -Winit-self -Wpointer-arith \
         -Wcast-qual -D_POSIX_C_SOURCE=200809L
  RAYLIB = ./lib/raylib-5.5_macos/libraylib.a
else
  ifeq ($(shell uname -m),x86_64)
    CC = gcc
    CFLAGS = -D_FORTIFY_SOURCE=2 -fno-diagnostics-show-option \
           -fstack-protector-all -g -$(OPTIMIZER) -std=$(CSTD) \
           -Walloc-zero -Wpedantic -Wduplicated-cond \
           -Wduplicated-branches -Wextra -Winit-self \
           -Wshadow -Wunused-const-variable=1 -Wlogical-op \
           -Wpointer-arith -Wcast-qual -Wconversion \
           -Wstrict-prototypes -Wmissing-prototypes \
           -Wsign-conversion -Wdouble-promotion -D_POSIX_C_SOURCE=200809L
    RAYLIB = ./lib/raylib-5.5_linux_amd64/libraylib.a
  else ifeq ($(shell uname -m),aarch64)
    CC = gcc

    CFLAGS = -D_FORTIFY_SOURCE=2 -fstack-protector-all -g -$(OPTIMIZER) -std=$(CSTD) \
    		-Wall -Wextra -Wpedantic -Wshadow -Winit-self -Wpointer-arith \
    		-Wcast-qual -D_POSIX_C_SOURCE=200809L

    RAYLIB = ./lib/raylib-5.5_linux_aarch64/libraylib.a 
  endif
endif

TARGET = bin/client
IP_ADDRESS ?= 127.0.0.1

SRC := $(wildcard ./src/*.c)
OBJ = $(SRC:.c=.o)

# Headers
LOCAL_INCLUDE = ./include/

SHARED_LIBRARY = ./lib/chat.a
SHARED_SRC = ../lib/chat.c
SHARED_INCLUDE = ../include/

all: reset $(TARGET) clean

# Link
$(TARGET): $(SHARED_LIBRARY) $(OBJ) | bin
	$(CC) $(CFLAGS) $(OBJ) $(SHARED_LIBRARY) $(RAYLIB) -o $(TARGET) -lm  -I$(SHARED_INCLUDE) -I$(LOCAL_INCLUDE)
	rm -f $(OBJ)

$(SHARED_LIBRARY): $(SHARED_SRC)
	$(CC) $(CFLAGS) -c $< -o $@ -I $(SHARED_INCLUDE)

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(SHARED_INCLUDE) -I$(LOCAL_INCLUDE)

bin:
	mkdir -p bin

run_client: $(TARGET)
	./$(TARGET) $(IP_ADDRESS)

reset:
	rm -rf bin

clean:
	rm -f $(OBJ) $(SHARED_LIBRARY)

.PHONY: all run_client clean reset
