set "CMAKE_GENERATOR=Visual Studio 17 2022"
call "third-party\common\win\premake\premake5.exe" --file="premake5.lua"  --genproto --dosstub --winrsrc --winsign --os=windows vs2022