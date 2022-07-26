#include "search.h" //search.h includes board.h, which also includes iostream


clock_t search_end_time = 0; //TODO: iterative deepening!!!
bool panic = false;

uint64_t node_count = 0;
uint64_t qcall_count = 0;
uint64_t tt_hits = 0;

uint8_t pv_length[MAX_DEPTH];
MOVE pv_table[MAX_DEPTH][MAX_DEPTH];


inline bool check_time() //Returns true if there is no time left or if a keystroke has been detected
{
	if (!(qcall_count & 0x0FFF)) //check every 4096 nodes
	{
		clock_t now = clock();
		if (now > search_end_time) //if we're past the time limit, return immediately
		{
			//stop searching
			panic = true;
		}
		else if (!(qcall_count & 0x3FFF)) //don't always check for input available, since it's very slow
		{
			if (kbhit())
			{
				//stop searching
				search_end_time = 0; //force the search time to be 0, so it won't increase depth infinitely and crash
				panic = true;
			}
		}
	}

	return panic;
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

int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint64_t hash, uint8_t ply)
{
	if (panic || check_time())
		return 0; //ran out of time, or keystroke: interrupt search immediately

	uint64_t dpp_hash = Z_DPP(last_target);
	hash ^= Z_TURN ^ dpp_hash; //switch sides in the hash function and account for double pawn pushes *before* probing table

	TT_ENTRY entry = get_entry(hash); //Try to get a TT entry
	uint16_t hash_move = 0;

	if (entry.flag && is_acceptable(entry.move)) //TT hit
	{
		//TODO: check for collisions! (is the move legal, or pseudo-legal)
		//and if there are collisions, get around them somehow
		//Idea: write a function that can tell if a move ID is orthodox, or pseudo-legal

		if (depth <= entry.depth) //sufficient depth, and move is not a collision
		{
			tt_hits++;

			if (entry.flag == HF_EXACT) //exact hit: always return eval
				return entry.eval;
			if (entry.flag == HF_BETA && entry.eval >= beta) //beta hit: return if beats beta (fail high)
				return beta;
			if (entry.flag == HF_ALPHA && entry.eval <= alpha) //alpha hit: return if lower than current alpha (fail low)
				return alpha;
			
			tt_hits--;
		}

		hash_move = entry.move;
	}

	if (!depth)
	{
		qcall_count++;
		return qsearch(stm, alpha, beta);
	}

	bool incheck = sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF;
	uint8_t legal_move_count = 0;

	MLIST mlist;
	generate_moves(&mlist, stm, last_target); //Generate all the moves! (pseudo-legal, still need to check for legality)

	//NOTE: the following statement will only be of any use when partial or full move legality will be implemented!
	if (mlist.count == 0) return incheck ? MATE_SCORE + ply : 0; //checkmate or stalemate

	//needed for TT
	int16_t old_alpha = alpha; //save an old alpha for handling exact TT scores
	MOVE best_move; //we need to know the best move even if we have not beaten alpha
	int16_t best_score = MATE_SCORE;

	pv_length[ply] = ply; //initialize current PV length
	order_moves(&mlist, hash_move, ply); //sort the moves by score (with the hash move)

	while (mlist.count) //iterate through it backwards
	{
		mlist.count--;
		node_count++;

		MOVE curmove = mlist.moves[mlist.count];
		uint64_t curmove_hash = move_hash(curmove) ^ dpp_hash; //cancel out DPP hash, since en passant will be illegal next move

		// if (!legal_move_count && hash_move && curmove.score != SCORE_HASH) printf("COLLISION\n");

		MOVE_RESULT res = make_move(stm, curmove);

		//Failed optimization: if the move is illegal, then we should not search it!
		if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) //oh no... this move is illegal!
		{
			// if (!legal_move_count && hash_move) printf("ILLEGAL HASH\n");

			unmake_move(stm, curmove, res); //just skip it
			continue;
		}

		legal_move_count++;
		curmove_hash ^= move_hash(curmove);

		int16_t eval = -search(stm ^ ENEMY, depth - 1, (curmove.flags & F_DPP) ? curmove.tgt : -2, -beta, -alpha, hash ^ curmove_hash, ply + 1);

		unmake_move(stm, curmove, res);

		//needed for TT: get the best move even if alpha hasn't been beaten
		//TODO: optimize this
		if (eval > best_score)
		{
			best_score = eval;
			best_move = curmove;
		}

		if (eval > alpha) //new best move!
		{
			alpha = eval;

			pv_table[ply][ply] = curmove; //update principal variation (TODO: test doing after beta cutoff to try get extra speed)
			uint8_t next_ply = ply + 1;
			while(pv_length[ply + 1] > next_ply) {
				pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
				next_ply++;
			}
			pv_length[ply] = (pv_length[ply + 1] < ply + 1) ? ply + 1 : pv_length[ply + 1]; //update PV length

			uint16_t move_id = MOVE_ID(curmove);
			if (curmove.flags < F_CAPT)
			{
				//Add history bonus
				uint16_t hindex = PSQ_INDEX(curmove);
				history[hindex] = std::min(history[hindex] + depth * depth, MAX_HISTORY); //Quadratic incrementation scheme
			}

			if (alpha >= beta) //beta cutoff
			{
				if (panic) return 0; //should NOT set TT entries when out of time!

				//Handle hash entry: upper bound (fail high)
				set_entry(hash, HF_BETA, depth, beta, curmove);

				//Killer move: not a capture nor a promotion
				if (curmove.flags < F_CAPT)
				{
					//Handle killer moves
					if(move_id != killers[ply][0]) //don't overwrite killers if the move is *already* a killer
					{
						killers[ply][1] = killers[ply][0];
						killers[ply][0] = move_id;
					}
				}

				return beta;
			}
		}
	}

	if (legal_move_count == 0) //the position has no legal moves: it is either checkmate or stalemate
		return incheck ? MATE_SCORE + ply : 0; //king in check = checkmate (we lose)
	
	//Handle hash entry
	if (panic) return 0; //should NOT set TT entries when out of time!
	else if (alpha > old_alpha)
		set_entry(hash, HF_EXACT, depth, alpha, best_move); //exact score
	else
		set_entry(hash, HF_ALPHA, depth, alpha, best_move); //lower bound (fail low)

	return alpha;
}

int16_t qsearch(uint8_t stm, int16_t alpha, int16_t beta)
{
	//if stand pat causes a beta cutoff, return before generating moves
	int16_t stand_pat = evaluate() * ((stm & BLACK) ? -1 : 1); //stand pat score
	if (stand_pat >= beta)
		return beta;
	else alpha = std::max(alpha, stand_pat);
	if (stand_pat <= alpha - DELTA) //delta pruning
		return alpha;


	MLIST mlist;
	generate_loud_moves(&mlist, stm); //Generate all the "loud" moves! (pseudo-legal, still need to check for legality)
	if (mlist.count == 0) return alpha; //No captures available: return alpha

	order_moves(&mlist, 0, MAX_DEPTH - 1); //sort the moves by score (ply is set to maximum available ply)

	while (mlist.count) //iterate through the move list backwards
	{
		mlist.count--;
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

		alpha = std::max(alpha, eval);
		if (alpha >= beta)
			return beta;
	}

	return alpha;
}


void search_root(uint32_t time_ms)
{
	MOVE best_move;

	for (int i = 0; i < 64; i++) //clear the PV length table
		pv_length[i] = 0;

	clear_history(); //clear history (otherwise risk of saturation, which makes history useless)

	search_end_time = clock() + time_ms * CLOCKS_PER_SEC / 1000; //set the time limit (in milliseconds)

	//iterative deepening loop
	for (uint8_t depth = 1; depth < MAX_DEPTH; depth++) //increase depth until MAX_DEPTH reached
	{
		panic = false; //reset panic flag
		node_count = 0; //reset node and qsearch call count
		qcall_count = 0;
		tt_hits = 0; //reset TT hit count

		//do search and measure elapsed time
		clock_t start = clock();
		int16_t eval = search(board_stm, depth, board_last_target, MATE_SCORE, -MATE_SCORE, 0, 0);
		clock_t end = clock();

		if (check_time()) break; //we do not have any more time!


		if (pv_length[0] == 0) //PV length = 0: the root node is in the TT
		{
			TT_ENTRY root_entry = get_entry(Z_TURN); //get root entry
			if (root_entry.flag)
			{
				MLIST mlist;
				generate_moves(&mlist, board_stm, board_last_target);

				while (mlist.count--)
					if (MOVE_ID(mlist.moves[mlist.count]) == root_entry.move)
						pv_table[0][0] = mlist.moves[mlist.count]; //get best move from TT
				
				pv_length[0] = 1; //we have 1 TT move in the PV
			}

			//TODO: complete the PV with TT moves
		}

		best_move = pv_table[0][0]; //save the best move

		std::cout << "info score cp " << eval;
		std::cout << " depth " << (int)depth;
		std::cout << " nodes " << node_count;
		std::cout << " time " << (end - start) * 1000 / CLOCKS_PER_SEC;
		std::cout << " nps " << node_count * CLOCKS_PER_SEC / (end - start + 1); //HACK: Adding 1 clock cycle to avoid division by 0
		// std::cout << " tthits " << tt_hits; //echoing TT entry hit count

		std::cout << " pv ";
		int c = pv_length[0];
		for (uint8_t i = 0; i < c; i++) {
			MOVE move = pv_table[0][i];
			print_move(move);
		}
		std::cout << std::endl;
	}	

	//print out the best move at the end of the search
	std::cout << "bestmove ";
	print_move(best_move);
	std::cout << std::endl;
}
