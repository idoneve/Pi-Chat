# C Version
CSTD ?= c11

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
         -Wcast-qual -D_POSIX_C_SOURCE=199309L
  RAYLIB = ./lib/raylib-5.5_macos/libraylib.a
else
  CC = gcc
  CFLAGS = -D_FORTIFY_SOURCE=2 -fno-diagnostics-show-option \
         -fstack-protector-all -g -$(OPTIMIZER) -std=$(CSTD) \
         -Walloc-zero -Wpedantic -Wduplicated-cond \
         -Wduplicated-branches -Wextra -Winit-self \
         -Wshadow -Wunused-const-variable=1 -Wlogical-op \
         -Wpointer-arith -Wcast-qual -Wconversion \
         -Wstrict-prototypes -Wmissing-prototypes \
         -Wsign-conversion -Wdouble-promotion -D_POSIX_C_SOURCE=199309L
  RAYLIB = ./lib/raylib-5.5_linux_amd64/libraylib.a
endif

TARGET = bin/client
IP_ADDRESS ?= 127.0.0.1

SRC := $(wildcard .src/*.c)
OBJ = $(SRC:.c=.o)

# Headers
LOCAL_INCLUDE = ./include/
SHARED_INCLUDE = ./../include/

all: $(TARGET)

# bin/client: $(CLIENT_SRC) | bin
# 	$(CC) $(CFLAGS) -I$(LOCAL_INCLUDE) -I$(SHARED_INCLUDE) -o $@ $(CLIENT_SRC)

# Link
$(TARGET): $(OBJ)
	$(GPP) $(GPPFlags) $(OBJ) -o $(TARGET) $(RAYLIB)  -lm

# Compile
%.o: %.c
	$(GPP) $(GPPFlags) -c $< -o $@ -I$(INCLUDE) -I$(SHARED_INCLUDE)

bin:
	mkdir -p bin

run_client: $(TARGET)
	./$(TARGET) $(IP_ADDRESS)

clean:
	rm -rf bin
	rm -f $(OBJ)

.PHONY: all run_client clean
