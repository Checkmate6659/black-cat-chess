#include "search.h"
#include <stdio.h> //DEBUG


clock_t search_end_time = 0; //TODO: iterative deepening!!!

uint64_t node_count = 0;
uint64_t qcall_count = 0;

uint8_t pv_length[MAX_DEPTH];
MOVE pv_table[MAX_DEPTH][MAX_DEPTH];

uint64_t perft(uint8_t stm, uint8_t last_target, uint8_t depth)
{
	if (depth)
	{
		--depth; //we only need to subtract once, and it is right here that we will do it
		//printf("ENTERED PERFT\n");

		//iterate through all legal moves
		MLIST mlist;
		generate_moves(&mlist, stm, last_target); //Generate all the moves! (pseudo-legal, still need to check for legality)

		//if(board[mlist.moves[mlist.count - 1].tgt]) printf("%x\n", board[mlist.moves[mlist.count - 1].tgt]); //gives all 0s in startpos
		//if last move is king capture, then the position is illegal and should NOT count!
		if (mlist.count && (board[mlist.moves[mlist.count - 1].tgt] & PTYPE) == KING)
		{
			//printf("ILLEGAL MOVE DETECTED\n");
			return 0;
		}

		uint64_t sum = 0;

		while (mlist.count)
		{
			mlist.count--; //only lower mlist.count HERE!

			MOVE curmove = mlist.moves[mlist.count];

			/* if ((board[curmove.tgt] & PTYPE) == KING) //we're taking a king! that's illegal!
			{
				printf("ILLEGAL MOVE DETECTED 2 ELECTRIC BOOGALOO\n");
				return 0;
			} */

			/*if (curmove.flags & F_DPP && (curmove.src ^ curmove.tgt) != 0x20) //BAD DPP
			{
				printf("BAD DPP DETECTED %x:%x\n", curmove.src, curmove.tgt);
				print_board_full(board);
			}*/

			//make/unmake the move!
			/*if ( curmove.flags & F_EP) {
				// printf("EP %x:%x %x LT%x\n", curmove.src, curmove.tgt, stm, last_target);
				print_board(board);
			} */
			MOVE_RESULT res = make_move(stm, curmove);
			//curmove.tgt n'est PAS SUR L'ECHIQUIER!
			//if (curmove.flags & F_DPP) printf("%x:%x\n", curmove.src, curmove.tgt);
			/* if (curmove.flags & F_EP){
				print_board(board);
			} */
			sum += perft(stm ^ ENEMY, (curmove.flags & F_DPP) ? curmove.tgt : -2, depth);
			unmake_move(stm, curmove, res);

			//printf("OK %x:%x\n", curmove.src, curmove.tgt);
		}

		return sum;
	}

	//otherwise, we just return a single node (but only if the position is legal!)
	if (sq_attacked(plist[stm & 16], stm) != 0xFF) //0 is the king's pos in the piece list (for debug only)
		return 0;

	return 1;
}

int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint8_t ply)
{
	if (!(qcall_count & 0x0FFF)) //check every 4096 nodes
	{
		//TODO: implement key stroke stuff (when you press a key during search, it will stop the search)
		clock_t now = clock(); //if we're past the time limit, return immediately
		if (now > search_end_time)
		{
			//stop searching
			// depth = 0; //if i want to use the newly searched moves (later optimization)
			return 0;
		}
	}
	if (depth)
	{
		--depth;

		MLIST mlist;
		generate_moves(&mlist, stm, last_target); //Generate all the moves! (pseudo-legal, still need to check for legality)

		//The king capture should NOT be given a mate score!
		//If i get pruning in the engine, i will have no guarantee the moves im playing are legal
		//except if i do it in a kinda slow way...
		//or spend a bunch of time optimizing movegen
		if (mlist.count && (board[mlist.moves[mlist.count - 1].tgt] & PTYPE) == KING) //the king is under attack! we must return immediately
		{
			return -MATE_SCORE - ply; //we win! and the less moves we used, the better!
		}
		else if (!mlist.count)
		{
			//This safety check might not be safe!
			//if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) return -MATE_SCORE; //king under attack: checkmate!
			return 0; //stalemate!
		}

		pv_length[ply] = ply; //initialize current PV length

		order_moves(&mlist); //sort the moves by score

		while (mlist.count) //iterate through it backwards
		{
			mlist.count--;

			MOVE curmove = mlist.moves[mlist.count];
			MOVE_RESULT res = make_move(stm, curmove);

			int16_t eval = -search(stm ^ ENEMY, depth, (curmove.flags & F_DPP) ? curmove.tgt : -2, -beta, -alpha, ply + 1);

			unmake_move(stm, curmove, res);

			if (eval > alpha) //new best move!
			{
				pv_table[ply][ply] = curmove; //update principal variation (TODO: test doing after beta cutoff to try get extra speed)
				uint8_t next_ply = ply + 1;
				while(pv_length[ply + 1] > next_ply) {
					pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
					next_ply++;
				}
				// pv_length[ply] = pv_length[ply + 1]; //update PV length
				pv_length[ply] = (pv_length[ply + 1] < ply + 1) ? ply + 1 : pv_length[ply + 1]; //update PV length (modified)
				// assert(pv_length[ply]);

				alpha = eval;
				if (alpha >= beta)
					return beta;
			}
		}

		return alpha;
	}
	qcall_count++;
	return qsearch(stm, alpha, beta);
	// return evaluate() * ((stm & BLACK) ? -1 : 1);
}

//BUG HERE: if i switch the evaluations around, it sometimes works better or worse
int16_t qsearch(uint8_t stm, int16_t alpha, int16_t beta)
{
	MLIST mlist;
	generate_loud_moves(&mlist, stm); //Generate all the "loud" moves! (pseudo-legal, still need to check for legality)

	if (mlist.count && (board[mlist.moves[mlist.count - 1].tgt] & PTYPE) == KING) //the king is under attack! we must return immediately
	{
		return -MATE_SCORE; //we win!
	}
	else if (!mlist.count)
	{
		return evaluate() * ((stm & BLACK) ? -1 : 1); //evaluate
	}

	order_moves(&mlist); //sort the moves by score

	node_count++;
	int16_t stand_pat = evaluate() * ((stm & BLACK) ? -1 : 1); //stand pat score
	if (stand_pat >= beta)
		return beta;
	else if (stand_pat >= alpha) //couldn't figure out why <algorithm> wasn't compiling properly
		alpha = stand_pat;
	else if (stand_pat <= alpha - DELTA) //delta pruning
		return alpha;

	uint8_t remaining_moves = QSEARCH_LMP;
	while (mlist.count) //iterate through the move list backwards
	{
		if (remaining_moves == 0) //late move pruning: break and fail low
			break;

		mlist.count--;
		remaining_moves--;

		MOVE curmove = mlist.moves[mlist.count];
		MOVE_RESULT res = make_move(stm, curmove);

		int16_t eval = -qsearch(stm ^ ENEMY, -beta, -alpha);

		unmake_move(stm, curmove, res);

		if (eval > alpha)
		{
			alpha = eval;
			if (alpha >= beta)
				return beta;
		}
	}

	return alpha;
}


void search_root(uint32_t time_ms)
{
	search_end_time = clock() + time_ms * CLOCKS_PER_SEC / 1000; //set the time limit (in milliseconds)
	uint8_t depth = 3; //initial depth of 3 ply

	MOVE best_move;

	while (clock() < search_end_time) //while we still have time
	{
		best_move = pv_table[0][0]; //best move = previous iteration's first PV move (search gives irrelevant result if ran out of time)

		node_count = 0; //reset node and qsearch call count
		qcall_count = 0;

		clock_t start = clock();
		int16_t eval = search(board_stm, depth, board_last_target, MATE_SCORE, -MATE_SCORE, 0);

		if (clock() < search_end_time) //only print out info if it's relevant
		{
			clock_t end = clock();

			std::cout << "info score cp " << eval;
			std::cout << " depth " << (int)depth;
			std::cout << " nodes " << node_count;
			std::cout << " time " << (end - start) * 1000 / CLOCKS_PER_SEC;
			std::cout << " nps " << node_count * CLOCKS_PER_SEC / (end - start);
			// std::cout << " nps " << qcall_count * CLOCKS_PER_SEC / (end - start);

			std::cout << " pv ";
			int c = pv_length[0];
			for (uint8_t i = 0; i < c; i++) {
				MOVE move = pv_table[0][i];
				print_move(move);
			}
			std::cout << std::endl;
		}

		depth++; //increase the depth
	}

	// for (uint8_t i=0; i<MAX_DEPTH; i++)
	// {
	// 	printf("%d ", pv_length[i]);
	// }
	// for (uint8_t i = 0; i<MAX_DEPTH; i++) {
   	// 	MOVE move = pv_table[0][i];
	// 	print_move(move);
	// }

	//print out the best move
	std::cout << "bestmove ";
	print_move(best_move);
	std::cout << std::endl;
}
