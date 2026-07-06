FROM debian:13

COPY docker_setup.sh /tmp/

RUN chmod +x /tmp/docker_setup.sh && \
    /tmp/docker_setup.sh

WORKDIR /project

CMD mkdir -p build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=/opt/ortools && \
    make -j"$(nproc)"