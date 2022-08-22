CC = g++
EXE = build/black_cat.out
all:
	$(CC) -o $(EXE) *.cpp -Wall -Wextra -O3 -Ofast -flto -march=native -s -DNDEBUG -fno-signed-zeros -funroll-loops -fomit-frame-pointer -fno-stack-protector -fgcse-sm -fgcse-las -faggressive-loop-optimizations
	# $(CC) -o $(EXE) *.cpp -Wall -Wextra -g3 -fsanitize=address #-ggdb3 -pg
