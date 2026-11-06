Hey guys I think the ui is pretty good so far but text if u want any changes or need anything

First configure so paste this in terminal:  cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
To open the ui put in terminal:  cmake --build build -j 4
Then after it says 'Built target CrimeDatabase' type:  .\build\bin\CrimeDatabase.exe

Performance page is incomplete still