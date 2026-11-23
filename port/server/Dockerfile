# rAthena pre-renewal server that speaks this client's protocol.
#
# The client (kRO Sakexe lineage, 2008) uses the zone packet set
# that rAthena calls 2008-04-01aSakexe .. 2008-08-20aSakexe; from 2008-09-10
# on rAthena expects different packet ids. port/probe/packetlen measures this.
FROM debian:12

ARG PACKETVER=20080401
# A fixed commit, so that every build of the test server is the same server.
ARG RATHENA_REF=4cd08b739035721b2cbffa0310dbbf0eac081d9d

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential git ca-certificates \
        libmariadb-dev libmariadb-dev-compat \
        zlib1g-dev libpcre3-dev \
        mariadb-client \
    && rm -rf /var/lib/apt/lists/*

RUN git init -q /rathena && cd /rathena \
    && git remote add origin https://github.com/rathena/rathena.git \
    && git fetch -q --depth 1 origin "${RATHENA_REF}" && git checkout -q FETCH_HEAD \
    && git rev-parse HEAD > BUILD_COMMIT

WORKDIR /rathena

RUN ./configure --enable-prere --enable-packetver=${PACKETVER} \
    && (make -j"$(nproc)" login char map || make login char map) \
    && echo "${PACKETVER}" > BUILD_PACKETVER

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 6900 6121 5121
ENTRYPOINT ["/entrypoint.sh"]
