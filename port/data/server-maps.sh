#!/bin/bash
# Regenerates port/server/import/maps_enterable.conf, the map list cut of the
# test server, and build/maps.txt, the maps the client can enter.
#
#   port/data/server-maps.sh
#
# Reads the server's own map list and map caches out of the running container
# (read only), or, when it is not running, from rAthena's repository at the
# commit the Dockerfile pins. Then grfaudit.py decides per map from the game
# folder as the client reads it (data/ first, then rdata.grf, data.grf).
# Applying the result needs a restart of the server (docker compose restart
# rathena); this script does not touch the server.
set -euo pipefail
cd "$(dirname "$0")/../.."

GAME=${BF_CLIENT_DIR:-$HOME/Games/Bifrost}
OUT=build/port/rathena
mkdir -p "$OUT"
FILES=(conf/maps_athena.conf db/pre-re/map_cache.dat db/map_cache.dat)
COMPOSE=(docker compose -f port/server/docker-compose.yml)

if "${COMPOSE[@]}" exec -T rathena true 2>/dev/null; then
    echo "[maps] server files from the running container"
    for f in "${FILES[@]}"; do "${COMPOSE[@]}" exec -T rathena cat "$f" > "$OUT/$(echo "$f" | tr / _)"; done
else
    REF=$(sed -n 's/^ARG RATHENA_REF=//p' port/server/Dockerfile)
    echo "[maps] server files from rathena/rathena at $REF"
    for f in "${FILES[@]}"; do
        curl -fsSL "https://raw.githubusercontent.com/rathena/rathena/$REF/$f" -o "$OUT/$(echo "$f" | tr / _)"
    done
fi

python3 port/tools/grfaudit.py "$GAME/rdata.grf" "$GAME/data.grf" --folder "$GAME/data" \
    --write-maps build/maps.txt \
    --rathena-maps "$OUT/conf_maps_athena.conf" \
    --rathena-cache "$OUT/db_pre-re_map_cache.dat" --rathena-cache "$OUT/db_map_cache.dat" \
    --write-server-conf port/server/import/maps_enterable.conf | sed -n '/^maps:/,$p'
