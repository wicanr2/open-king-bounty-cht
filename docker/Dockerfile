# openkb 中文化:可重現 build / 執行環境 (SDL 1.2 baseline)
# 用途:編譯 upstream openkb (free 模組) 並做 headless baseline 截圖。
# SDL2 化 (P1) 後會另開 Dockerfile.sdl2。
FROM debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq && apt-get install -y -qq --no-install-recommends \
      build-essential autoconf automake pkg-config git ca-certificates wget curl \
      libsdl1.2-dev libsdl-image1.2-dev libsdl-net1.2-dev libpng-dev \
      xvfb imagemagick \
    && rm -rf /var/lib/apt/lists/*

# git:// (GitHub 已停用) → https://
RUN git config --global url."https://github.com/".insteadOf "git://github.com/"

WORKDIR /src
