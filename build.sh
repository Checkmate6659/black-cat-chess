ls
# g++ -o build/black_cat.out -O3 -Ofast -flto -march=native -s -DNDEBUG -fno-signed-zeros -funroll-loops -fomit-frame-pointer -fno-stack-protector -fgcse-sm -fgcse-las -faggressive-loop-optimizations main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp tt.cpp
# g++ -o build/black_cat.out -Wall -g3 -fsanitize=address main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp tt.cpp
# g++ -o build/black_cat.out -Wall -g3 -ggdb3 -pg -fsanitize=address main.cpp board.cpp search.cpp eval.cpp order.cpp time_manager.cpp posix.cpp tt.cpp
# build/black_cat.out
make ; build/black_cat.out

#test incremental attack tables using this: position fen 8/5pp1/2n5/8/3k4/8/PPP5/2K5 w - - 0 1
#test TT using this (Lasker Reichhelm): position fen 8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -
#mate in 3 (HAKMEM 70): position fen 5B2/6P1/1p6/8/1N6/kP6/2K5/8 w - - 0 1
#WARNING: NMH makes engine fail HAKMEM 70 (stuck on mate in 4 with g8=Q; although shouldn't lower playing strength too much); although it might be because mate scores are stored incorrectly in tt
#Somewhat tricky mate in 4: position fen RNBKQBNR/8/8/8/8/8/pppppppp/rnbqkbnr w - - 3 2
