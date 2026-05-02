GPP = gcc
GPPFlags = -Wall -Wextra -g -O0 -std=c11 -D_POSIX_C_SOURCE=199309L

HEADER = $(wildcard *.h)

CLIENT_SRC = ./client.c
SERVER_SRC = ./server.c
SIGNAL_SRC = ./signal.c

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
SIGNAL_OBJ = $(SIGNAL_SRC:.c=.o)

TARNAME = final_submission

all: client server

client: $(CLIENT_TARGET)

server: $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(GPP) $(GPPFlags) $(CLIENT_OBJ) $(SIGNAL_OBJ) -o $(CLIENT_TARGET) 

$(SERVER_TARGET): $(SERVER_OBJ)
	$(GPP) $(GPPFlags) $(SERVER_OBJ) $(SIGNAL_OBJ) -o $(SERVER_TARGET) 

*.o: %.c
	$(GPP) $(GPPFlags) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

package:
	zip $(TARNAME).zip Makefile $(SRC) $(HEADER)

.PHONY: clean package
