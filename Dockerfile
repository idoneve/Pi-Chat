FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV TERM=xterm-256color

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc \
    make \
    libncurses-dev \
    netcat-openbsd \
    vim \
    git \
    apt-utils \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
CMD ["/bin/bash"]

# docker build -t pi-chat-sim:latest .
# docker run --rm -it --name pi-chat-dev --memory="4g" --cpus="4" -v $(pwd):/workspace pi-chat-sim:latest
