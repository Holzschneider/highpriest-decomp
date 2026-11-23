#!/bin/bash
# Waits for the database, imports the schema once, then runs login, char and
# map server in the foreground.
set -u

DB_HOST="${DB_HOST:-db}"
DB_USER="${DB_USER:-ragnarok}"
DB_PASS="${DB_PASS:-ragnarok}"
DB_NAME="${DB_NAME:-ragnarok}"
# Address the char and map server announce to the client. The client connects
# to it directly, so it must be reachable from the machine the client runs on.
PUBLIC_IP="${PUBLIC_IP:-127.0.0.1}"

SQL() { mariadb -h"$DB_HOST" -u"$DB_USER" -p"$DB_PASS" "$@"; }

echo "[init ] waiting for database at ${DB_HOST}..."
until SQL -e 'SELECT 1' >/dev/null 2>&1; do sleep 2; done

if ! SQL -e 'SELECT 1 FROM `login` LIMIT 1' "$DB_NAME" >/dev/null 2>&1; then
    echo "[init ] empty schema, importing rAthena SQL files"
    SQL "$DB_NAME" < sql-files/main.sql
    SQL "$DB_NAME" < sql-files/logs.sql
    if [ -f /seed/seed_accounts.sql ]; then
        SQL "$DB_NAME" < /seed/seed_accounts.sql
        echo "[init ] seeded game accounts"
    fi
else
    echo "[init ] schema present, skipping import"
fi

cd /rathena
# conf/import is generated here so the announced address can come from the
# environment; files mounted at /import-src are copied over it first.
mkdir -p conf/import db/import
cp conf/import-tmpl/* conf/import/ 2>/dev/null
cp -n db/import-tmpl/* db/import/ 2>/dev/null
cp /import-src/* conf/import/ 2>/dev/null
printf '\nchar_ip: %s\n' "$PUBLIC_IP" >> conf/import/char_conf.txt
printf '\nmap_ip: %s\n'  "$PUBLIC_IP" >> conf/import/map_conf.txt

echo "[init ] rAthena $(cut -c1-12 BUILD_COMMIT), PACKETVER=$(cat BUILD_PACKETVER), pre-renewal, announcing ${PUBLIC_IP}"

pids=()
./login-server 2>&1 | sed -u 's/^/[login] /' & pids+=($!)
./char-server  2>&1 | sed -u 's/^/[char ] /' & pids+=($!)
./map-server   2>&1 | sed -u 's/^/[map  ] /' & pids+=($!)

trap 'kill 0' INT TERM
wait -n "${pids[@]}"
echo "[init ] a server exited, shutting down"
kill 0
