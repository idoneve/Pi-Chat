# Compiler
GPP = gcc

# Compilation flags (for developing)
GPPFlags = -Wall -Wextra -g -O0 -std=c11 -D_POSIX_C_SOURCE=199309L

CLIENT_SRC = ./client.c
SERVER_SRC = ./server.c

HEADER = $(wildcard *.h)

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

TARNAME = final_submission

# Target Name
TARGET = a.out

all: client server

client: $(CLIENT_TARGET)

server: $(SERVER_TARGET)

# Link
$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(GPP) $(GPPFlags) $(CLIENT_OBJ) -o $(CLIENT_TARGET) 

$(SERVER_TARGET): $(SERVER_OBJ)
	$(GPP) $(GPPFlags) $(SERVER_OBJ) -o $(SERVER_TARGET) 

# Compile
*.o: %.c
	$(GPP) $(GPPFlags) -c $< -o $@

.PHONY: clean package

# clean
clean:
	rm -f $(TARGET) $(OBJ)
	@echo "Removed all object files."

package:
	zip $(TARNAME).zip Makefile $(SRC) $(HEADER)
