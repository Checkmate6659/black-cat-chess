#include "search.h" //search.h includes board.h, which also includes iostream


clock_t search_end_time = 0; //TODO: iterative deepening!!!
bool panic = false;

uint64_t node_count = 0;
uint64_t qcall_count = 0;
uint64_t tt_hits = 0;
// uint64_t collisions = 0;

uint8_t lmr_table[MAX_DEPTH][MAX_MOVE];

uint8_t pv_length[MAX_DEPTH];
MOVE pv_table[MAX_DEPTH][MAX_DEPTH];

int8_t repetition_table[RPT_SIZE];


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

//LMR test results:
//http://www.open-chess.org/viewtopic.php?f=3&t=435
//http://www.talkchess.com/forum3/viewtopic.php?t=65273
void init_lmr() //Initialize the late move reduction table
{
	for (uint8_t ply = 0; ply < MAX_DEPTH; ply++)
		for (uint8_t move = 0; move < MAX_MOVE; move++)
		{
			if (ply < LMR_MINDEPTH) lmr_table[ply][move] = 0; //no reduction at too little depth
			else if (move < LMR_THRESHOLD) lmr_table[ply][move] = 0; //no reduction for first few moves
			else
			{
				//Conventional LMR
				// lmr_table[ply][move] = 1; //constant reduction
				// lmr_table[ply][move] = ply / 3; //linear reduction (senpai); TODO: experiment with different denoms
				// lmr_table[ply][move] = (uint8_t)(sqrt((double)ply - 1) + sqrt((double)move - 1)); //square root reduction (fruit reloaded)
				// lmr_table[ply][move] = (uint8_t)(log((double)ply)*log((double)move)); //log reduction (stockfish)

				//Non-conventional LMR
				// lmr_table[ply][move] = (uint8_t)(log(ply * move)); //logarithmic
				// lmr_table[ply][move] = move / 3; //move-linear reduction (default 3)
				// lmr_table[ply][move] = (uint8_t)(sqrt((ply - LMR_MINDEPTH) * ply - LMR_MINDEPTH) + (move - LMR_THRESHOLD) * (move - LMR_THRESHOLD)); //distance
				// lmr_table[ply][move] = (uint8_t)(sqrt((double)ply - LMR_MINDEPTH) + sqrt((double)move - LMR_THRESHOLD)); //RELATIVE square root reduction (fruit reloaded)
				lmr_table[ply][move] = (uint8_t)(log1p((double)ply - LMR_MINDEPTH)*log1p((double)move - LMR_THRESHOLD)); //RELATIVE log reduction (stockfish)
			}
		}
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

bool nullmove_safe(uint8_t stm)
{
    uint8_t nullmove_material[] = {0, 0, 0, 1, 0, 1, 2, 2}; //Null move safety values for each piece type
    int8_t nullmove_counter = 2; //minimum count is 2 (major piece or 2 minor pieces); otherwise null move is not safe

    for (uint8_t plist_idx = (stm & 16) ^ 17; plist_idx & 15; plist_idx++) //iterate through all friendly pieces (except for the king)
    {
        nullmove_counter -= nullmove_material[board[plist_idx] & PTYPE]; //decrement counter for each piece
        if (nullmove_counter <= 0) return true; //Null move is safe: zugzwang unlikely
    }

    return false; //Null move is dangerous: late endgame, zugzwang likely
}

//TODO: implement draw by insufficient material
int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint64_t hash, int8_t nullmove, uint8_t ply, int8_t last_zeroing_ply)
{
	if (check_time())
		return 0; //ran out of time, or keystroke: interrupt search immediately

	pv_length[ply] = ply; //initialize current PV length

	if (ply - last_zeroing_ply >= 100) return FIFTY_MOVE; //if we've gone over 100 ply without zeroing, then the game is a draw

	hash ^= Z_TURN; //switch sides
	uint64_t key = hash ^ Z_DPP(last_target); //hash is just the pieces position and castling rights, we need to complement it with turn/dpp
	if (last_target == (uint8_t)-2) key ^= Z_DPP(last_target);

	//Using a board hash function gives a big search depth, but a garbage result (loses every time to prev version, still giving a ton of illegal PV moves)
	//Also triggers COLLISION and ILLEGAL HASH printf-lines (when enabled) a whole bunch (depth 4+ on startpos; gives 1 collision at depths 2 and 3)
	// key = board_hash(stm, last_target); //DEBUG: not give a shit to whatever there was before, just use the board_hash function

	uint16_t hash_move = 0;
	TT_ENTRY entry = get_entry(key); //Try to get a TT entry

	//if i dont check for the move being acceptable, the depth just skyrockets all the way up to 127!!!
	if (entry.flag) //TT hit
	{
		//TODO: check for collisions! (is the move legal, or pseudo-legal)
		//and if there are collisions, get around them somehow
		//Idea: write a function that can tell if a move ID is orthodox, or pseudo-legal

		if (ply && depth <= entry.depth && is_acceptable(entry.move)) //sufficient depth, and node is not root
		{
			tt_hits++;

			if (entry.flag == HF_EXACT) //exact hit: always return eval
				return entry.eval;
			
			//FAIL HARD
			else if (entry.flag == HF_BETA && entry.eval >= beta) //beta hit: return if beats beta (fail high)
				return beta;
			else if (entry.flag == HF_ALPHA && entry.eval <= alpha) //alpha hit: return if lower than current alpha (fail low)
				return alpha;
			
			//FAIL SOFT
			// if (entry.flag == HF_BETA)
			// 	alpha = std::max(alpha, entry.eval);
			// else if (entry.flag == HF_ALPHA)
			// 	beta = std::min(beta, entry.eval);
			// if (alpha >= beta)
			// 	return entry.eval;

			tt_hits--;
		}

		hash_move = entry.move;
	}

	if (!depth || ply == MAX_DEPTH)
	{
		qcall_count++;
		return qsearch(stm, alpha, beta);
	}

	RPT_INDEX rpt_index = key & RPT_MASK; //get the index to repetition hash table

	if (repetition_table[rpt_index] == last_zeroing_ply && ply) //twofold repetition (checking of last_zeroing_ply to diminish risk of collision) and not at root
		return REPETITION; //adjudicate

	bool incheck = sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF;
	uint8_t legal_move_count = 0;
	uint8_t lmr_move_count = 0;

	if (beta - alpha == 1) //Non-PV node
	{
		//Null move pruning
		if (depth > NULL_MOVE_REDUCTION && nullmove <= 0 && !incheck && nullmove_safe(stm)) //No NMH when in check or when in late endgame; also need to have enough depth
		{
			//Try search with null move (enemy's turn, ply + 1)
			int16_t null_move_val = -search(stm ^ ENEMY, depth - 1 - NULL_MOVE_REDUCTION, -2, -beta, -beta + 1, hash ^ Z_NULLMOVE, NULL_MOVE_COOLDOWN, ply + 1, ply);

			if (panic) return 0; //check if we ran out of time (NOTE: this may not be useful)

			if (null_move_val >= beta) //Null move beat beta
				return beta; //fail high
		}
	}

	// depth = std::min(depth + incheck, MAX_DEPTH); //check extension (add safety to avoid overflow)

	MLIST mlist;
	generate_moves(&mlist, stm, last_target); //Generate all the moves! (pseudo-legal, still need to check for legality)

	//NOTE: the following statement will only be of any use when partial or full move legality will be implemented!
	if (mlist.count == 0) return incheck ? MATE_SCORE + ply : STALEMATE; //checkmate or stalemate

	//needed for TT
	int16_t old_alpha = alpha; //save an old alpha for handling exact TT scores
	MOVE best_move; //we need to know the best move even if we have not beaten alpha
	int16_t best_score = MATE_SCORE;

	order_moves(&mlist, stm, hash_move, SEE_SEARCH, ply); //sort the moves by score (with the hash move)

	repetition_table[rpt_index] = last_zeroing_ply; //mark this position as seen before for upcoming searches

	while (mlist.count) //iterate through it backwards
	{
		mlist.count--;

		MOVE curmove = mlist.moves[mlist.count];
		uint64_t curmove_hash = move_hash(stm, curmove);

		// if (!legal_move_count && hash_move && curmove.score != SCORE_HASH) collisions++;

		MOVE_RESULT res = make_move(stm, curmove);

		//If the move is illegal, then we should not search it!
		if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) //oh no... this move is illegal!
		{
			// if (!legal_move_count && hash_move) collisions++;

			unmake_move(stm, curmove, res); //just skip it
			continue;
		}

		int8_t updated_last_zeroing_ply = last_zeroing_ply;
		if ((res.prev_state & PTYPE) < 3 || (curmove.flags & F_CAPT)) updated_last_zeroing_ply = ply; //if we moved a pawn, or captured, the HMC is reset

		//LMR can always be applied if lmr_move_count != 0, which means no if statement is needed here
		uint8_t lmr = lmr_table[depth][lmr_move_count]; //fetch LMR
		if (depth >= LMR_MINDEPTH && depth < lmr + LMR_MINDEPTH) //the reduction is too high, and would get below LMR_MINDEPTH (only possible if LMR > 0)
			lmr = depth - LMR_MINDEPTH; //don't reduce so much that depth would be below LMR_MINDEPTH (TODO: experiment with subtracting 1)
	
		if (!incheck && curmove.score < LMR_MAXSCORE) lmr_move_count++; //variable doesn't get increased if in check or if there are tactical moves
		else lmr = 0; //don't do LMR in check or on tactical moves

		node_count++;
		curmove_hash ^= move_hash(stm, curmove);

		int16_t eval;

		if (legal_move_count /* lmr */) //LMR implementation, merged with PVS (switch lmr to legal_move_count to enable PVS)
		{
			//search with null window and potentially reduced depth
			eval = -search(stm ^ ENEMY, depth - 1 - lmr, (curmove.flags & F_DPP) ? curmove.tgt : -2, -alpha - 1, -alpha, hash ^ curmove_hash, nullmove - 1, ply + 1, updated_last_zeroing_ply);
			if (panic) //check if we ran out of time
			{
				unmake_move(stm, curmove, res); //we have to unmake the moves!
				return 0;
			}

			if (eval > alpha) //beat alpha: re-search with full window and no reduction
				eval = -search(stm ^ ENEMY, depth - 1, (curmove.flags & F_DPP) ? curmove.tgt : -2, -beta, -alpha, hash ^ curmove_hash, nullmove - 1, ply + 1, updated_last_zeroing_ply);
		}
		else
			eval = -search(stm ^ ENEMY, depth - 1, (curmove.flags & F_DPP) ? curmove.tgt : -2, -beta, -alpha, hash ^ curmove_hash, nullmove - 1, ply + 1, updated_last_zeroing_ply);

		legal_move_count++; //only implement legal_move_count here, so that its equal to 0 on the first move in the PVS condition

		unmake_move(stm, curmove, res);

		//needed for TT: get the best move even if alpha hasn't been beaten
		if (eval > best_score) //new best move!
		{
			best_score = eval;
			best_move = curmove;

			if (eval > alpha) //beat alpha
			{
				alpha = eval;

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
					set_entry(key, HF_BETA, depth, beta, curmove);

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

					repetition_table[rpt_index] = -100; //mark this entry as not being seen before (if collision, this is going to erase previous entry)
					return beta; //FAIL HARD
					// return eval;  //FAIL SOFT
				}

				pv_table[ply][ply] = curmove; //update principal variation
				uint8_t next_ply = ply + 1;
				while(pv_length[ply + 1] > next_ply) {
					pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
					next_ply++;
				}
				pv_length[ply] = (pv_length[ply + 1] < ply + 1) ? ply + 1 : pv_length[ply + 1]; //update PV length
			}
		}
	}

	repetition_table[rpt_index] = -100; //mark this entry as not being seen before (if collision, this is going to erase previous entry)

	if (legal_move_count == 0) //the position has no legal moves: it is either checkmate or stalemate
		return incheck ? MATE_SCORE + ply : STALEMATE; //king in check = checkmate (we lose); otherwise stalemate
	
	//Handle hash entry
	if (panic) return 0; //should NOT set TT entries when out of time!
	else if (alpha > old_alpha)
		set_entry(key, HF_EXACT, depth, alpha, best_move); //exact score
	else
		set_entry(key, HF_ALPHA, depth, alpha, best_move); //lower bound (fail low)

	return alpha; //FAIL HARD
	// return best_score; //FAIL SOFT
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

	order_moves(&mlist, stm, 0, SEE_QSEARCH, MAX_DEPTH - 1); //sort the moves by score (ply is set to maximum available ply)

	while (mlist.count) //iterate through the move list backwards
	{
		mlist.count--;

		MOVE curmove = mlist.moves[mlist.count];
		MOVE_RESULT res = make_move(stm, curmove);

		if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) //oh no... this move is illegal!
		{
			unmake_move(stm, curmove, res); //just skip it
			continue;
		}

		node_count++;

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
	search_end_time = clock() + time_ms * CLOCKS_PER_SEC / 1000; //set the time limit (in milliseconds)

	for (int i = 0; i < MAX_DEPTH; i++) //clear the PV length table
		pv_length[i] = 0;

	clear_history(); //clear history (otherwise risk of saturation, which makes history useless)

	int16_t alpha = MATE_SCORE;
	int16_t beta = -MATE_SCORE;
	MOVE best_move;
	
	//iterative deepening loop
	for (uint8_t depth = 1; depth < MAX_DEPTH; depth++) //increase depth until MAX_DEPTH reached
	{
		panic = false; //reset panic flag
		node_count = 0; //reset node and qsearch call count
		qcall_count = 0;
		tt_hits = 0; //reset TT hit count
		// collisions = 0; //reset COLLISION count

		//do search and measure elapsed time
		clock_t start = clock();
		int16_t eval = search(board_stm, depth, board_last_target, alpha, beta,
			board_hash(board_stm, board_last_target) ^ Z_DPP(board_last_target) ^ Z_TURN,  //root key has to be initialized for repetitions before the root
			1, //don't allow NMP at the root, but allow it on subsequent plies
			0, -half_move_clock);
		clock_t end = clock();

		if (panic) break; //we do not have any more time!

		best_move = pv_table[0][0]; //save the best move

		std::cout << "info score cp " << eval;
		std::cout << " depth " << (int)depth;
		std::cout << " nodes " << node_count;
		std::cout << " time " << (end - start) * 1000 / CLOCKS_PER_SEC;
		std::cout << " nps " << node_count * CLOCKS_PER_SEC / (end - start + 1); //HACK: Adding 1 clock cycle to avoid division by 0
		std::cout << " tthits " << tt_hits; //echoing TT entry hit count
		// std::cout << " COLLISIONS " << collisions; //echoing number of type 1 (key) collisions

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
