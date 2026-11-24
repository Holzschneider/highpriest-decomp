"""Test harness of the macOS port: drives the client through its control
socket (port/native/control/PROTOCOL.md) and confirms what it did in the test
server's SQL logs. Python 3 standard library only."""

PROTOCOL_VERSION = 0
