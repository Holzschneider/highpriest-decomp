# rAthena pre-renewal server that speaks this client's protocol.
#
# The HighPriest client (kRO Sakexe lineage, 2008) uses the zone packet set
# that rAthena calls 2008-04-01aSakexe .. 2008-08-20aSakexe; from 2008-09-10
# on rAthena expects different packet ids. port/probe/packetlen measures this.
FROM debian:12

ARG PACKETVER=20080401
ARG RATHENA_REF=master

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential git ca-certificates \
        libmariadb-dev libmariadb-dev-compat \
        zlib1g-dev libpcre3-dev \
        mariadb-client \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 --branch "${RATHENA_REF}" https://github.com/rathena/rathena.git /rathena \
    && cd /rathena && git rev-parse HEAD > BUILD_COMMIT

WORKDIR /rathena

RUN ./configure --enable-prere --enable-packetver=${PACKETVER} \
    && make -j"$(nproc)" login char map \
    && echo "${PACKETVER}" > BUILD_PACKETVER

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 6900 6121 5121
ENTRYPOINT ["/entrypoint.sh"]
