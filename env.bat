@echo off
set WORKSPACE=%cd%

set QT_DIR=D:\IDE\qt-6.2\6.2.2\msvc2019_64\lib\cmake\Qt6
set QT_BIN=D:\IDE\qt-6.2\6.2.2\msvc2019_64\bin

set CMAKE_BUILD_TYPE=Release
set INSTALL_PATH=%WORKSPACE%\output
set CMAKE_PREFIX_PATH=D:/IDE/qt-6.2/6.2.2/msvc2019_64
set CMAKE_PROJECT_INCLUDE_BEFORE=D:\IDE\qt-6.2\Tools\QtCreator\share\qtcreator\package-manager\auto-setup.cmake

%VS_VARS_64%