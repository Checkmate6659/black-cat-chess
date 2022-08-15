#include <iostream>
#include <sstream>
#include <time.h>

#include "board.h"
#include "search.h"
#include "tt.h"
#include "time_manager.h"

// #define __TEST_VERSION__
// #define __TEST_NAME__ ""
#define __ENGINE_VERSION__ "2.1"


int main()
{
	std::cout << "Black Cat v" __ENGINE_VERSION__ " by Enigma\n";
	init_zobrist(); //Initialize zobrist keys
	init_lmr(); //Fill LMR table
	clear_tt(); //clear the transposition table
	for (RPT_INDEX i = 0; i < RPT_SIZE; i++) //clear the repetition table
		repetition_table[i] = -100;
	
	
	// load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");   					//STARTING POSITION
	// load_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w KQkq e6 0 1");  								//POSITION 3
	// load_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");   	//POSITION 6
	// load_fen("3n4/4Pk2/8/8/8/8/4p3/2KQ4 w - - 0 1");											//CUSTOM POSITION FOR PROMO TESTING
	// load_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");   		//POSITION 2 (Kiwipete)
	// load_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");  					//POSITION 5 (castling test)

	// load_fen("kr6/1q6/3pn3/8/8/1B6/PPP5/1K1R4 w - - 0 1"); //Search test position 1
	// load_fen("5B2/6P1/1p6/8/1N6/kP6/2K5/8 w - - 0 1"); //Search test position 2: HAKMEM 70, mate in 3 after g8=N
	// load_fen("r1bqkbnr/ppp2ppp/2np4/4p3/2BPP3/5N2/PPP2PPP/RNBQK2R b KQkq d3 0 4");  	//ShaheryarSohail test position


	bool loop_running = true;
	while (loop_running)
	{
		std::string input_string;
		std::getline(std::cin, input_string);
		std::stringstream input_stream(input_string);
		std::string command;
		input_stream >> command;

		if(command == "uci")
		{
#ifdef __TEST_VERSION__
			std::cout << "id name " << __TEST_NAME__ << "\nuciok\n";
#else
			std::cout << "id name Black Cat v" __ENGINE_VERSION__ "\n";
			std::cout << "id author Enigma\n";
			std::cout << "uciok\n";
#endif
		}
		//TODO: implement setoption command, and UCI options in general
		else if(command == "isready")
		{
			std::cout << "readyok\n";
		}
		else if(command == "quit")
		{
			loop_running = false;
		}
		else if(command == "display" || command == "print" || command == "disp" || command == "d")
		{
			print_board(board);
		}
		else if(command == "displayfull" || command == "printfull" || command == "dispf" || command == "df")
		{
			print_board_full(board);
		}
		else if(command == "position")
		{
			clear_tt(); //the TT is littered with the previous position results, and that could cause collisions (root hash is ALWAYS 0)
			for (RPT_INDEX i = 0; i < RPT_SIZE; i++) //clear the repetition table
				repetition_table[i] = -100; //invalid ply: game must have been adjudicated previously

			input_stream >> command;
			if (command == "startpos") //just load the starting position
			{
				load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
			}
			else //load from fen
			{
				std::string fen_string, current_element;

				for (uint8_t i = 6; i; i--) //load the 6 "words" of a fen string
				{
					input_stream >> current_element;
					fen_string.append(current_element);
					if (i != 0) //only append the space if there are more words to go
						fen_string.append(" ");
				}
				
				load_fen(fen_string);
			}

			//Make necessary moves to get to the desired position
			input_stream >> command;
			if (command == "moves") //if there are moves to make
			{
				input_stream >> command; //read first move
				while (input_stream)
				{
					bool promo = command.length() == 5; //if the move is a promotion, the UCI representation's length has 5 characters
					uint8_t from_file = command[0] - 'a'; //get the file of the from square
					uint8_t from_rank = '8' - command[1]; //get the rank of the from square
					uint8_t to_file = command[2] - 'a'; //get the file of the to square
					uint8_t to_rank = '8' - command[3]; //get the rank of the to square
					uint8_t src = from_rank * 0x10 + from_file; //get the source square
					uint8_t tgt = to_rank * 0x10 + to_file; //get the target square
					uint8_t promo_piece = 0;
					if (promo)
					{
						uint8_t PROMO_SYMBOLS[] = { //Symbols for promotion moves, table starts at 'b'
							BISHOP, //b = bishop
							0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //c-m = useless
							KNIGHT, //n = knight
							0, 0, //o/p = useless
							QUEEN, //q = queen
							ROOK, //r = rook
						};

						promo_piece = PROMO_SYMBOLS[command[4] - 'b']; //get the promotion piece
					}

					MLIST mlist;
					generate_moves(&mlist, board_stm, board_last_target); //generate all moves from the current position
					MOVE input_move = mlist.moves[0]; //get the first move (just to be safe)
					while (--mlist.count)
					{
						MOVE current_move = mlist.moves[mlist.count]; //get the current move

						if (!promo) //the move is not a promotion
						{
							if (current_move.src == src && current_move.tgt == tgt && !(current_move.flags & F_PROMO)) //if the move is the one we want
							{
								input_move = current_move; //set the input move to the current move
								break; //get out of the loop
							}
						}
						else
						{
							if (current_move.src == src && current_move.tgt == tgt && (current_move.flags & F_PROMO)
								&& current_move.promo == promo_piece) //if the move is the one we want, promoting to the correct piece
							{
								input_move = current_move; //set the input move to the current move
								break; //get out of the loop
							}
						}
					}

					if ((input_move.flags & F_CAPT) || (board[input_move.src] & PTYPE) < 3) //zeroing move: no repetitions possible
						for (RPT_INDEX i = 0; i < RPT_SIZE; i++) //clear the entirety of the repetition table (not the most efficient, but this is UCI anyway)
							repetition_table[i] = -100;

					RPT_INDEX rpt_index = board_hash(board_stm, board_last_target) & RPT_MASK;
					repetition_table[rpt_index] = -half_move_clock;

					make_move(board_stm, input_move); //make the move
					board_stm ^= ENEMY; //switch sides
					board_last_target = (input_move.flags & F_DPP) ? tgt : -2; //set the last target square if we made a double pawn push
					input_stream >> command; //try reading next move
				}

				RPT_INDEX rpt_index = board_hash(board_stm, board_last_target) & RPT_MASK;
				repetition_table[rpt_index] = -100;
			}
		}
		else if (command == "go")
		{
			uint32_t time_ms = 0;
			uint32_t engine_time = DEFAULT_ENGINE_TIME;
			uint32_t increment = 0;
			uint8_t movestogo = DEFAULT_MOVESTOGO;

			while (input_stream)
			{
				std::string cur_info, cur_value;
				input_stream >> cur_info >> cur_value;

				if (!cur_info.length()) break;
				if (cur_info == "movetime") //forcing of the maximum move time
				{
					time_ms = atoi(cur_value.c_str()); //set time in ms
					break; //break out of the loop
				}
				if (cur_info == "movestogo") movestogo = atoi(cur_value.c_str()); //set number of moves to go
				else if (board_stm == WHITE) //engine plays white
				{
					if (cur_info == "wtime") engine_time = atoi(cur_value.c_str());
					else if (cur_info == "winc") increment = atoi(cur_value.c_str());
				}
				else //engine plays black
				{
					if (cur_info == "btime") engine_time = atoi(cur_value.c_str());
					else if (cur_info == "binc") increment = atoi(cur_value.c_str());
				}
			}

			if(!time_ms) time_ms = alloc_time(engine_time, increment, movestogo); //time_ms has not been set by a movetime command, so we need to calculate it
			// printf("time: %u\n", time_ms);
			search_root(time_ms);
		}
		else if (command  == "perft")
		{
			input_stream >> command;
			uint8_t depth = atoi(command.c_str()) - 1;

			uint64_t nodes = 0;
			clock_t start = clock();

			MLIST mlist;
			generate_moves(&mlist, board_stm, board_last_target); //generate all moves from the current position

			while (mlist.count)
			{
				MOVE curmove = mlist.moves[--mlist.count]; //get the last move
				MOVE_RESULT res = make_move(board_stm, curmove); //make the move

				if (sq_attacked(plist[(board_stm & 16) ^ 16], board_stm ^ ENEMY) != 0xFF) //illegal move detected!
				{
					unmake_move(board_stm, curmove, res); //unmake the move
					continue;
				}

				uint64_t cur_move_nodes = perft(board_stm ^ ENEMY, (curmove.flags & F_DPP) ? curmove.tgt : -2, depth);

				nodes += cur_move_nodes;
				print_move(curmove);
				std::cout << cur_move_nodes << std::endl;

				unmake_move(board_stm, curmove, res); //unmake the move
			}

			clock_t end = clock();
			double perft_time = (end - start + 1) * 1000 / (double)CLOCKS_PER_SEC;

			std::cout << "Nodes: " << nodes << std::endl;
			std::cout << "Time: " << perft_time << std::endl;
			std::cout << "NPS: " << (uint64_t)(nodes * 1000 / perft_time) << std::endl;
		}
	}

	return 0;
}
