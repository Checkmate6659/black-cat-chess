ls
g++ -o build/black_cat.out -O3 -Ofast -flto -march=native -s -DNDEBUG -fno-signed-zeros -funroll-loops -fomit-frame-pointer -fno-stack-protector -fgcse-sm -fgcse-las -faggressive-loop-optimizations main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp tt.cpp
# g++ -o build/black_cat.out -O3 -Ofast -march=native -s -DNDEBUG -fno-signed-zeros -funroll-loops -fomit-frame-pointer -fno-stack-protector -fgcse-sm -fgcse-las -faggressive-loop-optimizations main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp tt.cpp
# g++ -o build/black_cat.out -Wall -g3 -fsanitize=address main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp tt.cpp
# g++ -o build/black_cat.out -Wall -g3 -ggdb3 -pg main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp tt.cpp
build/black_cat.out

#test TT using this: position fen 8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -
#mate in 3: position fen 5B2/6P1/1p6/8/1N6/kP6/2K5/8 w - - 0 1
