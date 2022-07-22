ls
# g++ -o build/black_cat.out -O3 -Ofast -flto -march=native -s -DNDEBUG -fno-signed-zeros -funroll-loops -fno-stack-protector main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp
g++ -o build/black_cat.out -g3 main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp
build/black_cat.out
