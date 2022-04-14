@echo off

set INCLUDES=-I W:\lib\SDL2\include\ -I W:\lib\SDL2_ttf\include\ -I W:\lib\SDL_FontCache\ -I W:\lib\SDL2_mixer\include\ -I W:\lib\stb\
set LIBRARIES=shell32.lib W:\lib\SDL2\lib\x64\SDL2main.lib W:\lib\SDL2\lib\x64\SDL2.lib W:\lib\SDL2_ttf\lib\x64\SDL2_ttf.lib W:\lib\SDL_FontCache\SDL_FontCache.lib W:\lib\SDL2_mixer\lib\x64\SDL2_mixer.lib
set WARNINGS=-W4 -wd4702 -wd4100 -wd4201 -wd4127 -wd4505
REM Set wd4505 off when release, checks for unused procedures with internal linkage.

IF NOT EXIST W:\lib\SDL_FontCache\SDL_FontCache.lib (
	pushd W:\lib\SDL_FontCache\
	cl  /c /EHsc -MTd -O2 -I W:\lib\SDL2\include\ -I W:\lib\SDL2_ttf\include\ W:\lib\SDL_FontCache\SDL_FontCache.c
	lib SDL_FontCache.obj
	popd
)

IF NOT EXIST W:\build\ (
	mkdir W:\build\
)

pushd W:\build\
set DEBUG=1
if DEFINED DEBUG (
	echo "Debug build"
	del *.pdb > NUL 2> NUL
	echo "LOCK" > LOCK.tmp
	cl -nologo -DDATA_DIR="\"W:/data/\"" -DEXE_DIR="\"W:/build/\"" -DDEBUG=1 -Od -Oi -Z7 -std:c++17 -MTd -GR- -EHsc -EHa- %WARNINGS% %INCLUDES% -LD             W:\src\Antihome.cpp /link -DEBUG:FULL -opt:ref -incremental:no -subsystem:windows %LIBRARIES% -PDB:Antihome_%RANDOM%.pdb -EXPORT:initialize -EXPORT:boot_down -EXPORT:boot_up -EXPORT:update -EXPORT:render
	cl -nologo -DDATA_DIR="\"W:/data/\"" -DEXE_DIR="\"W:/build/\"" -DDEBUG=1 -Od -Oi -Z7 -std:c++17 -MTd -GR- -EHsc -EHa- %WARNINGS% %INCLUDES% -FeAntihome.exe W:\src\platform.cpp               /link -DEBUG:FULL -opt:ref -incremental:no -subsystem:windows %LIBRARIES%
	sleep 0.1
	del LOCK.tmp
)
popd
