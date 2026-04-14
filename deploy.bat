:: C:\devkitPro\msys2\msys2.exe deploy.bat

@REM clear && make clean && make 
clear && make clean && make && 3dslink.exe --address 192.168.2.146 itchy.3dsx
