# Operating-System-Final-Project

## Docker container setup
To set up the project and dependencies for your base architecture
```sh
docker build -t pi-chat-sim:latest .
docker run --rm -it --name pi-chat-dev --memory="4g" --cpus="4" -v $(pwd):/workspace pi-chat-sim:latest
```

To set up the project and dependencies with arm64 execution (uses emulation on non-arm platforms)
```sh
$ docker buildx build --platform linux/arm64 -t pi-chat-sim:latest .
$ docker run --rm -it --name pi-chat-dev --memory="4g" --cpus="4" --platform linux/arm64 -v $(pwd):/workspace pi-chat-sim:latest
```

## Compilation
Using make, you can build the client with:
```sh
$ make client
```

You can build the server with:
```sh
$ make server
```

You can make both with
```sh
$ make
```
