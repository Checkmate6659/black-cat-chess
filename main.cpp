#include <iostream>
#include <sstream>
#include <time.h>

#include "board.h"
#include "search.h"
#include "tt.h"
#include "time_manager.h"

// #define __TEST_VERSION__
// #define __TEST_NAME__ ""
#define __ENGINE_VERSION__ "2.3-dev"


typedef struct
{
	std::string name;
	std::string type;
	double min, max, val_float;
	std::string val_string;
	//TODO: add action function
} OPTION;

//UCI options
//NOTE: this table is going to be modified to store option values (although this is completely unnecessary for now)
OPTION uci_options[] = {
	{"Hash", "spin", 1, 16384, 16, ""}, //Max hash size 16GB (cannot test!); theoretical max with u32 indices and 14B entries would be 28GB

#ifdef TUNING_MODE
	{"AspiMargin", "spin", 1, 100, (double)aspi_margin, ""},
	{"MaxAspiMargin", "spin", 2, 10000, (double)max_aspi_margin, ""},
	{"AspiMultiplier", "spin", 100, 400, (double)aspi_mul, ""}, //x100
	{"AspiConstant", "spin", 1, 1000, (double)aspi_constant, ""},
	{"RFPMaxDepth", "spin", 1, 100, (double)rfp_max_depth, ""},
	{"RFPMargin", "spin", 0, 1000, (double)rfp_margin, ""},
	{"RFPImpr", "spin", 0, 1000, (double)rfp_impr, ""},
	{"TTFailReductionMinDepth", "spin", 1, 100, (double)iid_reduction_d, ""},
	{"Delta", "spin", 0, 10000, (double)dprune, ""},
	{"NullMoveReductionConstant", "spin", 0, 100, (double)nmp_const, ""},
	{"NullMoveReductionDepth", "spin", 1, 100, (double)nmp_depth, ""},
	{"NullMoveReductionMin", "spin", 0, 100, (double)nmp_evalmin, ""},
	{"NullMoveReductionDiv", "spin", 1, 10000, (double)nmp_evaldiv, ""},
	{"LMRConst", "spin", 0, 20000, (double)lmr_const, ""}, //x10000
	{"LMRMul", "spin", 0, 20000, (double)lmr_mul, ""}, //x10000
	{"LMR_PV", "spin", 0, 1, (double)lmr_do_pv, ""}, //bool; hardcoded
	{"LMR_impr", "spin", 0, 1, (double)lmr_do_impr, ""}, //bool; hardcoded
	{"LMR_kmove", "spin", 0, 1, (double)lmr_do_chk_kmove, ""}, //bool; hardcoded
	{"LMR_killer", "spin", 0, 1, (double)lmr_do_killer, ""}, //bool; hardcoded
	{"LMRHist", "spin", 0, 999999, (double)lmr_history_threshold, ""}, //history score
	{"LMRSqrtMul", "spin", 0, 20000, (double)lmr_sqrt_mul, ""}, //x10000
	{"LMRDistMul", "spin", 0, 20000, (double)lmr_dist_mul, ""}, //x10000
	{"LMRDepthMul", "spin", 0, 20000, (double)lmr_depth_mul, ""}, //x10000
	{"HLPDoImproving", "spin", 0, 1, (double)hlp_do_improving, ""},
	{"HLPMovecount", "spin", 1, 100, (double)hlp_movecount, ""},
	{"HLPReduce", "spin", 0, 999999, (double)hlp_reduce, ""}, //history score
	{"HLPPrune", "spin", 0, 999999, (double)hlp_prune, ""}, //history score
	{"CheckExtensionMinDepth", "spin", 1, 100, (double)chkext_depth, ""},
#endif
};

OPTION uci_defaults[sizeof(uci_options)/sizeof(OPTION)]; //so we need a second table to store the default values

// Benchmarks from Bitgenie
const std::string benchfens[50] = {
    "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
    "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
    "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
    "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
    "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
    "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
    "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
    "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
    "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
    "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
    "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
    "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
    "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
    "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
    "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
    "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
    "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
    "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
    "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
    "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
    "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
    "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
    "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
    "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
    "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
    "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
    "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
    "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
    "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
    "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
    "8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
    "8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
    "8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
    "8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
    "8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54",
    "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
    "8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
    "8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
    "1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
    "4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
    "r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
    "2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
    "6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
    "2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
    "r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
    "6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
    "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
    "2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
    "3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
    "2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93"
};


int main(int argc, char** argv)
{
	init_tt(); //Initialize zobrist keys and allocate/clear transposition table
	init_lmr(); //Fill LMR table

	if (argc == 2) //benchmarking mode: sending bench as command line arg
	{
		node_count = 0;
		benchmark = true; //searches cannot be stopped!
		clock_t start = clock();

		for (int i = 0; i < 50; i++)
		{
			load_fen(benchfens[i]);

			// clear_tt(); //clear the transposition table
			for (RPT_INDEX i = 0; i < RPT_SIZE; i++) //clear the repetition table
				repetition_table[i] = -100;

			// search(board_stm, i < 5 ? 12 : 7, board_last_target, MATE_SCORE, -MATE_SCORE, //do higher depth search on first few fens, to account for potential high-depth search techniques
			// 	board_hash(board_stm, board_last_target) ^ Z_DPP(board_last_target) ^ Z_TURN,  //root key has to be initialized for repetitions before the root
			// 	1, //don't allow NMP at the root, but allow it on subsequent plies
			// 	0, -half_move_clock);

			search_root(-1, i < 5 ? 12 : 7);
		}

		clock_t end = clock();
		double time = (double)(end - start) / CLOCKS_PER_SEC;
		uint64_t nps = (uint64_t)(node_count / time);

		std::cout << node_count << " nodes " << nps << " nps" << std::endl;
		return 0;
	}

	std::cout << "Black Cat v" __ENGINE_VERSION__ " by Enigma\n";

	for (RPT_INDEX i = 0; i < RPT_SIZE; i++) //clear the repetition table
		repetition_table[i] = -100;
	
	for (int option_index = 0; option_index < sizeof(uci_options)/sizeof(OPTION); option_index++) //initalize UCI defaults table
	{
		uci_defaults[option_index] = uci_options[option_index];
	}
	
	
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
			std::cout << "id name Black Cat v" __ENGINE_VERSION__;
#ifdef __TEST_VERSION__
			std::cout << " " __TEST_NAME__;
#endif
			std::cout << "\nid author Enigma\n";

			for (OPTION option : uci_defaults)
			{
				std::cout << "option name " << option.name << " type " << option.type;
				if (option.type == "spin")
					std::cout << " default " << option.val_float << " min " << option.min << " max " << option.max;
				else if (option.type == "string")
					std::cout << " default " << option.val_string;
				else if (option.type == "check")
					std::cout << " default " << (option.val_float != 0 ? "true" : "false");
				std::cout << std::endl;
			}

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
		else if(command == "setoption")
		{
			input_stream >> command; //command = "name"
			input_stream >> command; //fetch first word of option name

			std::string option_name = "";
			std::string option_value = "";

			while (input_stream && command != "value") //no option can have the word "value" in it!
			{
				option_name += command + " ";
				input_stream >> command;
			}

			option_name = option_name.substr(0, option_name.size() - 1); //remove trailing space

			//find UCI option index
			int option_index = 0;
			for (OPTION option : uci_options)
			{
				if (option.name == option_name)
					break;
				option_index++;
			}

			//command now has the value "value"
			input_stream >> command; //fetch first word of the value

			while (input_stream) //go all the way to the end
			{
				option_value += command + " ";
				input_stream >> command;
			}

			option_value = option_value.substr(0, option_value.size() - 1); //remove trailing space

			uci_options[option_index].val_string = option_value;
			uci_options[option_index].val_float = stof(option_value);

			//update the option!
			if (option_name == "Hash") reallocate_tt((TT_INDEX)uci_options[option_index].val_float); //reallocate TT

#ifdef TUNING_MODE
			aspi_margin = uci_options[1].val_float;
			max_aspi_margin = uci_options[2].val_float;
			aspi_mul = uci_options[3].val_float;
			aspi_constant = uci_options[4].val_float;
			rfp_max_depth = uci_options[5].val_float;
			rfp_margin = uci_options[6].val_float;
			rfp_impr = uci_options[7].val_float;
			iid_reduction_d = uci_options[8].val_float;
			dprune = uci_options[9].val_float;
			nmp_const = uci_options[10].val_float;
			nmp_depth = uci_options[11].val_float;
			nmp_evalmin = uci_options[12].val_float;
			nmp_evaldiv = uci_options[13].val_float;
			lmr_const = uci_options[14].val_float;
			lmr_mul = uci_options[15].val_float;
			lmr_do_pv = uci_options[16].val_float;
			lmr_do_impr = uci_options[17].val_float;
			lmr_do_chk_kmove = uci_options[18].val_float;
			lmr_do_killer = uci_options[19].val_float;
			lmr_history_threshold = uci_options[20].val_float;
			lmr_sqrt_mul = uci_options[21].val_float;
			lmr_dist_mul = uci_options[22].val_float;
			lmr_depth_mul = uci_options[23].val_float;
			hlp_do_improving = uci_options[24].val_float;
			hlp_movecount = uci_options[25].val_float;
			hlp_reduce = uci_options[26].val_float;
			hlp_prune = uci_options[27].val_float;
			chkext_depth = uci_options[28].val_float;

			init_lmr();
#endif
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
			search_root(time_ms, MAX_DEPTH);
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
