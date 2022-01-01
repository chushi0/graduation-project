@echo off

mkdir build > NUL 2> NUL
cd build > NUL 2> NUL

cmake .. -G"NMake Makefiles" && nmake

move *.exe %INSTALL_PATH% > NUL 2> NUL
move *.dll %INSTALL_PATH% > NUL 2> NUL

cd .. > NUL 2> NUL