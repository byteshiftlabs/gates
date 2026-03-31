FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Toggle full project verification during image build.
ARG RUN_VALIDATION=0
ARG BUILD_DIR=/tmp/gates-build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    cppcheck \
    git \
    python3 \
    python3-sphinx \
    python3-sphinx-rtd-theme \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . /workspace

RUN if [ "$RUN_VALIDATION" = "1" ]; then \
      cmake -S /workspace -B "$BUILD_DIR" -DENABLE_TESTING=ON && \
      cmake --build "$BUILD_DIR" --target gates_tests -j"$(nproc)" && \
      ctest --test-dir "$BUILD_DIR" --output-on-failure && \
      cmake --build "$BUILD_DIR" --target gates -j"$(nproc)" && \
      "$BUILD_DIR"/gates --version && \
      cmake --build "$BUILD_DIR" --target docs -j"$(nproc)" && \
      cmake --build "$BUILD_DIR" --target cppcheck; \
    fi

CMD ["/bin/bash"]