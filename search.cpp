#include "search.h"
#include <stdio.h> //DEBUG


clock_t search_end_time = 0; //TODO: iterative deepening!!!

uint64_t node_count = 0;
uint64_t qcall_count = 0;

uint8_t pv_length[MAX_DEPTH];
MOVE pv_table[MAX_DEPTH][MAX_DEPTH];


inline bool check_time() //Returns true if there is no time left or if a keystroke has been detected
{
	if (!(qcall_count & 0x0FFF)) //check every 4096 nodes
	{
		clock_t now = clock(); //if we're past the time limit, return immediately
		if (now > search_end_time)
		{
			//stop searching
			return true;
		}
		else if (!(qcall_count & 0xFFFF)) //check for input available 16 times less, since it's very slow
		{
			if (kbhit())
			{
				search_end_time = 0; //force the search time to be 0, so it won't increase depth infinitely and crash
				return true;
			}
		}
	}

	return false;
}

uint64_t perft(uint8_t stm, uint8_t last_target, uint8_t depth)
{
	if (!depth) //out of depth: return
		return 1;

	//iterate through all legal moves
	MLIST mlist;
	generate_moves(&mlist, stm, last_target); //Generate all the moves! (pseudo-legal, still need to check for legality)

	uint64_t sum = 0;

	while (mlist.count)
	{
		mlist.count--; //only lower mlist.count HERE!

		MOVE curmove = mlist.moves[mlist.count];
		MOVE_RESULT res = make_move(stm, curmove);

		//Failed optimization: if the move is illegal, then we should not count it!
		//Reduced NPS from ~15M to ~10M
		if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) //oh no... this move is illegal!
		{
			unmake_move(stm, curmove, res); //just skip it
			continue;
		}

		sum += perft(stm ^ ENEMY, (curmove.flags & F_DPP) ? curmove.tgt : -2, depth - 1);
		unmake_move(stm, curmove, res);
	}

	return sum;
}

int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint8_t ply)
{
	if (!depth)
	{
		qcall_count++;
		return qsearch(stm, alpha, beta);
	}

	if (check_time())
		return 0; //ran out of time: interrupt search immediately

	bool incheck = sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF;
	uint8_t legal_move_count = 0;

	MLIST mlist;
	generate_moves(&mlist, stm, last_target); //Generate all the moves! (pseudo-legal, still need to check for legality)

	//The king capture should NOT be given a mate score!
	//If i get pruning in the engine, i will have no guarantee the moves im playing are legal
	//except if i do it in a kinda slow way...
	//or spend a bunch of time optimizing movegen
	// if (mlist.count && (board[mlist.moves[mlist.count - 1].tgt] & PTYPE) == KING) //the king is under attack! we must return immediately
	// {
	// 	return -MATE_SCORE - ply; //we win! and the less moves we used, the better!
	// }
	// else if (!mlist.count)
	// {
	// 	//This safety check might not be safe!
	// 	if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) return MATE_SCORE + ply; //our king is under attack: we lose!
	// 	return 0; //stalemate: no legal move, king not under attack
	// }

	//NOTE: the following statement will only be of any use when partial or full move legality will be implemented!
	if (mlist.count == 0) return incheck ? MATE_SCORE + ply : 0; //checkmate or stalemate

	pv_length[ply] = ply; //initialize current PV length

	order_moves(&mlist); //sort the moves by score

	while (mlist.count) //iterate through it backwards
	{
		mlist.count--;

		node_count++;

		MOVE curmove = mlist.moves[mlist.count];
		MOVE_RESULT res = make_move(stm, curmove);

		//Failed optimization: if the move is illegal, then we should not search it!
		if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) //oh no... this move is illegal!
		{
			unmake_move(stm, curmove, res); //just skip it
			continue;
		}

		legal_move_count++;

		int16_t eval = -search(stm ^ ENEMY, depth - 1, (curmove.flags & F_DPP) ? curmove.tgt : -2, -beta, -alpha, ply + 1);

		unmake_move(stm, curmove, res);

		if (eval > alpha) //new best move!
		{
			pv_table[ply][ply] = curmove; //update principal variation (TODO: test doing after beta cutoff to try get extra speed)
			uint8_t next_ply = ply + 1;
			while(pv_length[ply + 1] > next_ply) {
				pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
				next_ply++;
			}
			pv_length[ply] = (pv_length[ply + 1] < ply + 1) ? ply + 1 : pv_length[ply + 1]; //update PV length

			alpha = eval;
			if (alpha >= beta)
				return beta;
		}
	}

	if (legal_move_count == 0) //the position has no legal moves: it is either checkmate or stalemate
		return incheck ? MATE_SCORE + ply : 0; //king in check = checkmate (we lose)

	return alpha;
}

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

	//node_count++;
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

		node_count++;

		MOVE curmove = mlist.moves[mlist.count];
		MOVE_RESULT res = make_move(stm, curmove);

		if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) //oh no... this move is illegal!
		{
			unmake_move(stm, curmove, res); //just skip it
			continue;
		}

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
	for (int i = 0; i < 64; i++) //clear the PV length table to prevent PV table bug
		pv_length[i] = 0;

	search_end_time = clock() + time_ms * CLOCKS_PER_SEC / 1000; //set the time limit (in milliseconds)
	uint8_t depth = 1;

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

	//print out the best move
	std::cout << "bestmove ";
	print_move(best_move);
	std::cout << std::endl;
}
