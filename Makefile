CC = clang++ #change to G++ when debugging
WCC = x86_64-w64-mingw32-g++ #TODO: fix windows compiling
EXE = build/black_cat.out
WEXE = build/black_cat.exe
all:
	#G++
	#$(CC) -o $(EXE) *.cpp ./nnue/misc.cpp ./nnue/nnue.cpp ./nnue/eval_data.o -Wall -Wextra -O3 -Ofast -flto -march=native -s -DNDEBUG -fno-signed-zeros -funroll-loops -fomit-frame-pointer -fno-stack-protector -fgcse-sm -fgcse-las -faggressive-loop-optimizations

	#Clang++ (a bit faster than G++)
	$(CC) -o $(EXE) *.cpp ./nnue/misc.cpp ./nnue/nnue.cpp ./nnue/eval_data.o -Wall -Wextra -O3 -Ofast -ffast-math -flto -march=native -s -DNDEBUG -fno-signed-zeros -funroll-loops -fomit-frame-pointer -fno-stack-protector

	#TODO: fix windows compiling
	# $(WCC) -o $(WEXE) *.cpp ./nnue/misc.cpp ./nnue/nnue.cpp ./nnue/eval_data.o -Wall -Wextra -O3 -Ofast -flto -march=native -s -DNDEBUG -fno-signed-zeros -funroll-loops -fomit-frame-pointer -fno-stack-protector -fgcse-sm -fgcse-las -faggressive-loop-optimizations

	#DEBUG (G++)
	# $(CC) -o $(EXE) *.cpp ./nnue/*.cpp -Wall -Wextra -g3 -fsanitize=address #-ggdb3 -pg

