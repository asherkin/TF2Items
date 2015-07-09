mkdir build
cd build
call "%VS120COMNTOOLS%\vsvars32.bat"
..\configure.py
build.py
pause
cd ..