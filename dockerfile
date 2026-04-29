# set up environment
FROM arm64v8/debian:bullseye-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV TERM=xterm-256color

# install packages
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

# workspace directory
WORKDIR /workspace

CMD ["/bin/bash"]

# to build
# docker build -t pi-chat-sim:latest .

# to run
# docker run --rm -it --name pi-chat-dev --memory="4g" --cpus="4" -v $(pwd):/workspace \pi-chat-sim:latest