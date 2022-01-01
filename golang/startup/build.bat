@echo off

go build -ldflags="-H windowsgui"
move *.exe %INSTALL_PATH% > NUL 2> NUL