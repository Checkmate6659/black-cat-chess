#include <iostream>
#include <sstream>
#include <time.h>

#include "board.h"
#include "search.h"
#include "time_manager.h"

#define __ENGINE_VERSION__ "1.0"


int main(int argc, char** argv)
{
	std::cout << "Black Cat v" __ENGINE_VERSION__ " by Enigma\n";

	
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
			std::cout << "id name Black Cat v" __ENGINE_VERSION__ "\n";
			std::cout << "id author Enigma\n";
			std::cout << "uciok\n";
		}
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
			input_stream >> command;
			if (command == "startpos") //just load the starting position
			{
				load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
			}
			else //load from fen
			{
				std::string fen_string, current_element;

				for (uint8_t i = 5; i; i--) //load the 5 "words" of a fen string
				{
					input_stream >> current_element;
					fen_string.append(current_element);
					if (i != 0) //only append the space if there are more words to go
						fen_string.append(" ");
				}
				
				load_fen(fen_string);
			}

			//TODO: implement moves
		}
		else if (command == "go")
		{
			uint32_t time_ms = 0;
			uint32_t engine_time = 1000;
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
	}

	return 0;
}
