mkdir build
cd build
call "%VS100COMNTOOLS%\vsvars32.bat"
..\configure.py
build.py
pause
cd ..