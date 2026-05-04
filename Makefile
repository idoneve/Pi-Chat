# Targets
SERVER_TARGET = bin/server
CLIENT_TARGET = bin/client

all: $(SERVER_TARGET) $(CLIENT_TARGET)

bin/server: | bin
	make -C ./server/
	cp ./server/bin/server ./bin/server

bin/client: | bin
	make -C ./client/
	cp ./client/bin/client ./bin/client

bin:
	mkdir -p bin

run_server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

run_client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET) $(IP_ADDRESS)

# clean
clean:
	rm -rf bin
	make clean -C ./server/
	make clean -C ./client/

.PHONY: all run_server run_client clean
