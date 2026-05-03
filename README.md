# Operating-System-Final-Project

## Docker container setup
To set up the project and dependencies for your base architecture
```sh
docker build -t pi-chat-sim:latest .
docker run --rm -it --name pi-chat-dev --memory="4g" --cpus="4" -v $(pwd):/workspace pi-chat-sim:latest
```

To set up the project and dependencies with arm64 execution (uses emulation on non-arm platforms)

First make sure emulation is set up in docker

```sh
$ docker run --privileged --rm tonistiigi/binfmt --install all
```

Then build and run the image

```sh
$ docker buildx build --platform linux/arm64/v8 -t pi-chat-sim:latest .
$ docker run --rm -it --name pi-chat-dev --memory="4g" --cpus="4" --platform linux/arm64/v8 -v $(pwd):/workspace pi-chat-sim:latest
```

## Compilation
Using make, you can build the binaries with:
```sh
$ make all
```

You can run the server with:
```sh
$ make run_server
```

You can run the client with (IP defaults to localhost):
```sh
$ make run_client IP_ADDRESS=127.0.0.1
```
