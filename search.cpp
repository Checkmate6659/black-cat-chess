#include "search.h" //search.h includes board.h, which also includes iostream

#ifdef TUNING_MODE
int aspi_margin = 22, max_aspi_margin = 2000, aspi_constant = 39;
float aspi_mul = 168; //x100
int rfp_max_depth = 17, rfp_margin = 304, rfp_impr = 146;
int iid_reduction_d = 3;
int dprune = 2069; //DISABLED
int nmp_const = 3, nmp_depth = 16, nmp_evalmin = 44, nmp_evaldiv = 509; //evalmin DISABLED
float lmr_const = 5512, lmr_mul = 0; //x10000; actually the lmr_mul is for log(d)*log(m)

//TODO: do tests instead of tuning for bools, its not good to tune bools with spsa
bool lmr_do_pv = true;
bool lmr_do_impr = true;
bool lmr_do_chk_kmove = false;
bool lmr_do_killer = true;
uint32_t lmr_history_threshold = 6434;

//Extra LMR parameters, for better LMR
float lmr_sqrt_mul = 1262, lmr_dist_mul = 882, lmr_depth_mul = 0; //all x10000

bool hlp_do_improving = true; //do history leaf pruning on improving nodes
uint8_t hlp_movecount = 6; //move count from which we can do it
uint32_t hlp_reduce = 5000, hlp_prune = 2500;

uint8_t chkext_depth = 6; //check extension minimum depth
#endif


clock_t search_end_time = 0;
bool panic = false;
bool benchmark = false;

uint64_t node_count = 0;
uint64_t qcall_count = 0;
uint64_t tt_hits = 0;
// uint64_t collisions = 0;

uint8_t lmr_table[MAX_DEPTH][MAX_MOVE];
uint8_t lmp_table[LMP_MAXDEPTH][2];

int16_t eval_stack[MAX_DEPTH];

uint8_t pv_length[MAX_DEPTH];
MOVE pv_table[MAX_DEPTH][MAX_DEPTH];

int8_t repetition_table[RPT_SIZE];


inline bool check_time() //Returns true if there is no time left or if a keystroke has been detected
{
	if (!(qcall_count & 0x0FFF)) //check every 4096 nodes
	{
		if (benchmark) return false; //don't panic! you can't run out of time it's a fucking benchmark!
		
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
void init_search() //Initialize the late move reduction table
{
	for (uint8_t depth = 0; depth < MAX_DEPTH; depth++) //prev ply
		for (uint8_t move = 0; move < MAX_MOVE; move++)
		{
			if (depth < 1) lmr_table[depth][move] = 0; //no reduction at too little depth (prev LMR_MINDEPTH)
			else if (move < 1) lmr_table[depth][move] = 0; //no reduction for first few moves (prev LMR_THRESHOLD)
			else
			{
				//TODO: TUNE ALL THIS SHIT!

				//Conventional LMR
				// lmr_table[ply][move] = 1; //constant reduction
				// lmr_table[ply][move] = ply / 3; //linear reduction (senpai); TODO: experiment with different denoms
				// lmr_table[ply][move] = (uint8_t)(sqrt((double)ply - 1) + sqrt((double)move - 1)); //square root reduction (fruit reloaded)
				// lmr_table[ply][move] = (uint8_t)(log((double)ply)*log((double)move)); //log reduction (stockfish)

				//Non-conventional LMR
				// lmr_table[ply][move] = (uint8_t)(log(ply * move)); //logarithmic
				// lmr_table[ply][move] = move / 3; //move-linear reduction (default 3)
				// lmr_table[ply][move] = (uint8_t)(sqrt((ply - LMR_MINDEPTH) * ply - LMR_MINDEPTH) + (move - LMR_THRESHOLD) * (move - LMR_THRESHOLD)); //distance
				// lmr_table[ply][move] = (uint8_t)(sqrt((double)ply - LMR_MINDEPTH) + sqrt((double)move - LMR_THRESHOLD)); //RELATIVE square root reduction
				// lmr_table[ply][move] = (uint8_t)(log((double)ply - LMR_MINDEPTH)*log((double)move - LMR_THRESHOLD)); //RELATIVE log reduction

				//Ethereal LMR
				// lmr_table[depth][move] = (uint8_t)(0.75 + log((double)depth)*log((double)move)/2.25); //Ethereal log reduction
				lmr_table[depth][move] = (uint8_t)(LMR_CONST + log((double)depth)*log((double)move)*LMR_MUL); //Derived version

				//Tunable LMR
				lmr_table[depth][move] = (uint8_t)(LMR_CONST + log((double)depth)*log((double)move)*LMR_MUL + (sqrt((double)depth - 1) + sqrt((double)move - 1))*LMR_SQRT_MUL + sqrt(depth*depth + move*move)*LMR_DIST_MUL + depth*LMR_DEPTH_MUL); //Max-tunable LMR
			}
		}
	
	for (uint8_t depth = 1; depth < LMP_MAXDEPTH; depth++) //don't need at 0 depth, since it will be unused
	{
		lmp_table[depth][0] = std::min(2.5 + 2 * depth * depth / 4.5, 255.0);
		lmp_table[depth][1] = std::min(4.0 + 4 * depth * depth / 4.5, 255.0);
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
	TT_ENTRY entry = get_entry(key, ply); //Try to get a TT entry

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
				return entry.eval; //FAIL SOFT
			
			//FAIL HARD
			// else if (entry.flag == HF_BETA && entry.eval >= beta) //beta hit: return if beats beta (fail high)
			// 	return beta;
			// else if (entry.flag == HF_ALPHA && entry.eval <= alpha) //alpha hit: return if lower than current alpha (fail low)
			// 	return alpha;
			
			//FAIL SOFT
			if (entry.flag == HF_BETA)
				alpha = std::max(alpha, entry.eval);
			else if (entry.flag == HF_ALPHA)
				beta = std::min(beta, entry.eval);
			if (alpha >= beta)
				return entry.eval;

			tt_hits--;
		}

		hash_move = entry.move;
	}

	if (!depth || ply == MAX_DEPTH)
	{
		qcall_count++;
		return qsearch(stm, alpha, beta);
	}

	if (!entry.flag && beta - alpha > 1 && depth >= TT_FAIL_REDUCTION_MINDEPTH && ply) depth--; //reduce depth if no TT hit, not root, and PV-node (alternative IID)

	RPT_INDEX rpt_index = key & RPT_MASK; //get the index to repetition hash table

	if (repetition_table[rpt_index] == last_zeroing_ply && ply) //twofold repetition (checking of last_zeroing_ply to diminish risk of collision) and not at root
		return REPETITION; //adjudicate

	bool incheck = sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF; //is the king under attack?
	bool improving = false;

	int16_t static_eval = evaluate(stm);
	if (!incheck && ply >= 2) //if in check, improving stays false; also don't do this close to the root (otherwise buffer overflow)
	{
		//if possible, use TT to improve on static eval
		// if (entry.flag & (HF_EXACT | (entry.eval > static_eval ? HF_BETA : HF_ALPHA))) static_eval = entry.eval;
		// else set_entry(key, HF_EXACT, 0, static_eval, MOVE {0, 0, 0, 0, 0}); //store static eval in TT

		improving = static_eval > eval_stack[ply - 2]; //check if static eval is improving
	}
	eval_stack[ply] = static_eval;

	uint8_t legal_move_count = 0;
	uint8_t lmr_move_count = 0;

	if (incheck && depth > CHKEXT_MINDEPTH) depth++; //check extension
	else if (beta - alpha == 1) //Non-PV node, not extended in this node (TODO: not extended in entire line)
	{
		//Reverse futility pruning/static null move pruning (TODO: try with !incheck)
		if (/* depth <= RFP_MAX_DEPTH */ true											//RFP max depth shown to be useless! uncapped RFP
		&& !IS_MATE(beta) //don't rfp when beta is a mate value
		&& static_eval - RFP_MARGIN * depth - RFP_IMPR * improving >= beta) //sufficient margin
		// && static_eval - RFP_MARGIN * depth >= beta) //basic impl (test this first!)
			// return beta; //FAIL HARD
			return static_eval; //FAIL SOFT (consider using return static_eval - margin)

		//Null move pruning (TODO: tune enhancements)
		uint8_t reduction;
		// reduction = NULL_MOVE_REDUCTION_CONST; //const reduction
		// reduction = std::min((static_eval - beta) / 147, 5) + depth/3 + NULL_MOVE_REDUCTION_CONST; //SF-ish (uses const 4)

		//reduction = NULL_MOVE_REDUCTION_CONST + depth/NULL_MOVE_REDUCTION_DEPTH + std::min(NULL_MOVE_REDUCTION_MIN, (static_eval - beta) / NULL_MOVE_REDUCTION_DIV); //Ethereal-ish (uses const 4)
		reduction = NULL_MOVE_REDUCTION_CONST + depth/NULL_MOVE_REDUCTION_DEPTH + (static_eval - beta) / NULL_MOVE_REDUCTION_DIV; //uncapping eval-beta param probably good idea

		reduction = std::min((uint8_t)(depth - 1), reduction); //don't reduce past depth (depth - 1 to account for the -1 in the search call)
		if (nullmove <= 0 && !incheck && nullmove_safe(stm)) //No NMH when in check or when in late endgame; also need to have enough depth
		{
			//Try search with null move (enemy's turn, ply + 1)
			int16_t null_move_val = -search(stm ^ ENEMY, depth - 1 - reduction, -2, -beta, -beta + 1, hash ^ Z_NULLMOVE, 1, ply + 1, ply);

			// if (panic) return 0; //check if we ran out of time (NOTE: this may not be useful)

			if (null_move_val >= beta) //Null move beat beta
				// return beta; //fail high, but hard
				return std::min(null_move_val, (int16_t)NULLMOVE_MATE); //fail soft: lazy way of not returning erroneous mate scores
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

	score_moves(&mlist, stm, hash_move, ply); //sort the moves by score (with the hash move)

	repetition_table[rpt_index] = last_zeroing_ply; //mark this position as seen before for upcoming searches

	while (mlist.count) //iterate through it backwards
	{
		// MOVE curmove = mlist.moves[mlist.count];
		MOVE curmove = pick_move(&mlist);
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

		//SEE pruning at shallow depth
		int16_t seeMargin = (curmove.flags & F_CAPT) ? SEE_NOISY * depth * depth : SEE_QUIET * depth; //compute SEE margin
		if (depth <= SEE_MAX_DEPTH && SEE_VALUES[res.piece & PTYPE] + seeMargin < see(stm ^ ENEMY, curmove.tgt)) //Lost material exceeds captured material (trades are not included)
		{
			unmake_move(stm, curmove, res); //skip move: loses material in static exchange
			continue;
		}

		int8_t updated_last_zeroing_ply = last_zeroing_ply;
		if ((res.prev_state & PTYPE) < 3 || (curmove.flags & F_CAPT)) updated_last_zeroing_ply = ply; //if we moved a pawn, or captured, the HMC is reset

		//LMR implementation close to Ethereal (TODO: tune)
		int8_t lmr = lmr_table[depth][lmr_move_count]; //fetch LMR

#ifdef TUNING_MODE //WARNING: tuning bools doesn't work! testing would be a much better solution
		if(lmr_do_pv) lmr += (beta - alpha == 1); //reduce less for PV (LMR is a fail-low/loss-seeking heuristic)
		if(lmr_do_impr) lmr += !improving; //reduce less for improving
		if(lmr_do_chk_kmove) lmr += incheck && !(board[curmove.tgt | 8] & 15); //reduce for King moves that escape checks
		if(lmr_do_killer) lmr -= curmove.score >= SCORE_KILLER_SECONDARY; //reduce less for killer moves
#else //king moves in check, very suspicious
		lmr += (beta - alpha == 1); //reduce less for PV (LMR is a fail-low/loss-seeking heuristic)
		lmr += !improving; //reduce less for improving
		lmr += incheck && !(board[curmove.tgt | 8] & 15); //reduce for King moves that escape checks
		lmr -= curmove.score >= SCORE_KILLER_SECONDARY; //reduce less for killer moves
#endif
		lmr -= curmove.score >= SCORE_QUIET + LMR_HISTORY_THRESHOLD; //reduce less for moves with good history

		//history leaf pruning/reduction
		if (beta - alpha == 1 && !incheck && legal_move_count >= HLP_MOVECOUNT //make sure its not a PV node, we're not in check, and a move has been found
#ifdef TUNING_MODE
		&& (!improving || hlp_do_improving)
#else
		&& !improving
#endif
		)
		{
			if (curmove.score < SCORE_QUIET + HLP_REDUCE) //poor history score
			{
				lmr++; //reduce moves with poor histories
				if (curmove.score < SCORE_QUIET + HLP_PRUNE) //very poor history score
				{
					unmake_move(stm, curmove, res); //history leaf pruning
					continue;
				}
			}
		}

		lmr = std::max((int8_t)0, std::min(lmr, (int8_t)(depth - 1))); //make sure it's not dropping into qsearch or extending

		if (!incheck && curmove.score < LMR_MAXSCORE) lmr_move_count++; //variable doesn't get increased if in check or if there are tactical moves
		else lmr = 0; //don't do LMR in check or on tactical moves

		node_count++;
		curmove_hash ^= move_hash(stm, curmove);

		int16_t eval;

		if (/* legal_move_count */ lmr) //LMR implementation, merged with PVS (switch lmr to legal_move_count to enable PVS)
		{
			//search with null window and potentially reduced depth
			eval = -search(stm ^ ENEMY, depth - 1 - lmr, (curmove.flags & F_DPP) ? curmove.tgt : -2, -alpha - 1, -alpha, hash ^ curmove_hash, nullmove - 1, ply + 1, updated_last_zeroing_ply);
			// if (panic) //check if we ran out of time
			// {
			// 	unmake_move(stm, curmove, res); //we have to unmake the moves!
			// 	return 0;
			// }

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
					repetition_table[rpt_index] = -100; //mark this entry as not being seen before (if collision, this is going to erase previous entry); set BEFORE checking panic!

					if (panic) return 0; //should NOT set TT entries when out of time!

					//Handle hash entry: upper bound (fail high)
					set_entry(key, HF_BETA, depth, beta, curmove, ply);

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

					// return beta; //FAIL HARD
					return eval;  //FAIL SOFT
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
		set_entry(key, HF_EXACT, depth, alpha, best_move, ply); //exact score
	else
		set_entry(key, HF_ALPHA, depth, alpha, best_move, ply); //lower bound (fail low)

	// return alpha; //FAIL HARD
	return best_score; //FAIL SOFT
}

int16_t qsearch(uint8_t stm, int16_t alpha, int16_t beta)
{
	//if stand pat causes a beta cutoff, return before generating moves
	int16_t stand_pat = evaluate(stm); //stand pat score
	if (stand_pat >= beta)
		return beta;
	else alpha = std::max(alpha, stand_pat);
	//tuning revealed delta pruning probably BAD!
	// if (stand_pat <= alpha - DELTA) //delta pruning
	// 	return alpha;


	MLIST mlist;
	generate_loud_moves(&mlist, stm); //Generate all the "loud" moves! (pseudo-legal, still need to check for legality)
	if (mlist.count == 0) return alpha; //No captures available: return alpha

	score_moves(&mlist, stm, 0, MAX_DEPTH - 1); //sort the moves by score (ply is set to maximum available ply)

	while (mlist.count) //iterate through the move list backwards
	{
		// MOVE curmove = mlist.moves[mlist.count];
		MOVE curmove =  pick_move(&mlist);
		MOVE_RESULT res = make_move(stm, curmove);

		if (sq_attacked(plist[(stm & 16) ^ 16], stm ^ ENEMY) != 0xFF) //oh no... this move is illegal!
		{
			unmake_move(stm, curmove, res); //just skip it
			continue;
		}

		node_count++;

		//SEE pruning in qsearch
		if (SEE_VALUES[res.piece & PTYPE] < see(stm ^ ENEMY, curmove.tgt)) //Lost material exceeds captured material (trades are not included)
		{
			unmake_move(stm, curmove, res); //skip move: loses material in static exchange
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


void search_root(uint32_t time_ms, uint8_t fixed_depth)
{
	search_end_time = clock() + time_ms * CLOCKS_PER_SEC / 1000; //set the time limit (in milliseconds)

	for (int i = 0; i < MAX_DEPTH; i++) //clear the PV length table
		pv_length[i] = 0;

	clear_history(); //clear history (otherwise risk of saturation, which makes history useless)

	int16_t alpha = MATE_SCORE;
	int16_t beta = -MATE_SCORE;
	int16_t eval = qsearch(board_stm, alpha, beta); //first guess at the score is just qsearch = depth 0
	
	MOVE best_move;

	//iterative deepening loop
	for (uint8_t depth = 1; depth < std::min(fixed_depth, (uint8_t)MAX_DEPTH); depth++) //increase depth until MAX_DEPTH reached
	{
		panic = false; //reset panic flag
		if(!benchmark) node_count = 0; //reset node and qsearch call count
		qcall_count = 0;
		tt_hits = 0; //reset TT hit count
		// collisions = 0; //reset COLLISION count

		//do search and measure elapsed time
		clock_t start = clock();

		uint16_t alpha_margin = ASPI_MARGIN;
		uint16_t beta_margin = ASPI_MARGIN;

		//set alpha and beta
		alpha = (alpha_margin < MAX_ASPI_MARGIN) ? (eval - alpha_margin) : MATE_SCORE;
		beta = (beta_margin < MAX_ASPI_MARGIN) ? (eval + beta_margin) : -MATE_SCORE;

		//Simplified aspiration windows
		// if (depth < 4) eval = alpha; //first few iterations don't aspirate search
		// else eval = search(board_stm, depth, board_last_target, alpha, beta,
		// 	board_hash(board_stm, board_last_target) ^ Z_DPP(board_last_target) ^ Z_TURN,  //root key has to be initialized for repetitions before the root
		// 	1, //don't allow NMP at the root, but allow it on subsequent plies
		// 	0, -half_move_clock);

		// if (eval <= alpha || eval >= beta) //Aspirated search failed: re-search on full window
		// {
		// 	alpha = MATE_SCORE;
		// 	beta = -MATE_SCORE;

		// 	eval = search(board_stm, depth, board_last_target, alpha, beta,
		// 		board_hash(board_stm, board_last_target) ^ Z_DPP(board_last_target) ^ Z_TURN,  //root key has to be initialized for repetitions before the root
		// 		1, //don't allow NMP at the root, but allow it on subsequent plies
		// 		0, -half_move_clock);
		// }


		//More complex aspiration windows
		//Aspiration window loop: do-while to avoid skipping search entirely
		do {
			while(eval <= alpha || eval >= beta)
			{
				if (depth >= 4)
				{
					if (eval <= alpha) alpha_margin = alpha_margin * ASPI_MULTIPLIER + ASPI_CONSTANT;
					else if (eval >= beta) beta_margin = beta_margin * ASPI_MULTIPLIER + ASPI_CONSTANT;

					alpha = (alpha_margin < MAX_ASPI_MARGIN) ? (eval - alpha_margin) : MATE_SCORE;
					beta = (beta_margin < MAX_ASPI_MARGIN) ? (eval + beta_margin) : -MATE_SCORE;
				}
				else //Full-window search at low depth
				{
					alpha = MATE_SCORE;
					beta = -MATE_SCORE;
				}
			}

			eval = search(board_stm, depth, board_last_target, alpha, beta,
				board_hash(board_stm, board_last_target) ^ Z_DPP(board_last_target) ^ Z_TURN,  //root key has to be initialized for repetitions before the root
				1, //don't allow NMP at the root, but allow it on subsequent plies
				0, -half_move_clock);
		} while(eval <= alpha || eval >= beta);


		clock_t end = clock();

		if (panic) break; //we do not have any more time!

		best_move = pv_table[0][0]; //save the best move

		if (!benchmark)
		{
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
	}	

	//print out the best move at the end of the search
	if (!benchmark)
	{
		std::cout << "bestmove ";
		print_move(best_move);
		std::cout << std::endl;
	}
}
