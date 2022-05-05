@echo off
call vcvarsall.bat x64

REM For finding the `.dll` files.
set PATH=W:\lib\SDL2\lib\x64\;W:\lib\SDL2_ttf\lib\x64\;W:\lib\SDL2_mixer\lib\x64\;W:\lib\SDL2_image\lib\x64\;%PATH%
