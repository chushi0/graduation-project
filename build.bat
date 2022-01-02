@echo off
if defined INSTALL_PATH (
    echo=
) else (
    echo Run env.bat first!
    exit /B 1
)

set WORKSPACE=%cd%

del /F /S /Q "%INSTALL_PATH%" > NUL 2> NUL
mkdir %INSTALL_PATH% > NUL 2> NUL

cd %WORKSPACE%\cpp\main
cmd /C build.bat
cd %WORKSPACE%\cpp\qscintilla
cmd /C build.bat

cd %WORKSPACE%\golang\startup
cmd /C build.bat

cd %INSTALL_PATH%
for %%i in (*.dll) do "%QT_BIN%\windeployqt" "%%i"
for %%i in (*.exe) do "%QT_BIN%\windeployqt" "%%i"

cd %WORKSPACE%