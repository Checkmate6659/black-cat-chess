#include "board.h"

//The board
//Piece encoding:
//piece & 7
//0: EMPTY
//1: white pawn (moves up = lower index)
//2: black pawn (moves down = raise index)
//3: knight
//4: king
//5: bishop
//6: rook
//7: queen
//piece & 7 = piece type
//piece & 7 > 4 = slider
//piece ^ 24 = enemy equivalent (be careful for PAWNS tho!)
//(piece ^ other_piece) & 24 = enemies?
//PTYPE = 7
//WHITE = 8
//BLACK = 16
//ENEMY = 24
//MOVED = 32
//Right side: piece list indexes (don't forget to update those!)
uint8_t board[] = {
  22, 19, 21, 23, 20, 21, 19, 22,   9, 10, 11, 12,  0, 13, 14, 15,
  18, 18, 18, 18, 18, 18, 18, 18,   1,  2,  3,  4,  5,  6,  7,  8,
   0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
   9,  9,  9,  9,  9,  9,  9,  9,  17, 18, 19, 20, 21, 22, 23, 24,
  14, 11, 13, 15, 12, 13, 11, 14,  25, 26, 27, 28, 16, 29, 30, 31,
};

//Piece lists! The thing that will make this entire thing fast
//To index the list, just index it directly! (EZPZ)
uint8_t plist[] = {
	0x04, //black king
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, //black pawns
	0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, //black pieces
	0x74, //white king
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, //white pawns
	0x70, 0x71, 0x72, 0x73, 0x75, 0x76, 0x77, //white pieces
};

uint8_t board_stm = WHITE;
uint8_t board_last_target = -2;


//and here comes 41 bytes (could be way less) of some crazy compression & branchless-ification (could be useful for some kind of variant program as well)
//number of offsets for each piece
const uint8_t noffsets[] = {
	0, 3, 3, 8, 8, 4, 4, 8,
};

//index of offset starts for each piece (first index for pawns is non-capt)
const uint8_t offset_idx[] = {
	0, 3, 16, 8, 0, 4, 0, 0, //king/queen/rook are 0 (first moves are lateral), knight is 8, bishop is 4, white pawn is 3, black pawn is 16
};

//all the offsets of all the pieces in a jumbled mess. to get around here, use the 2 other arrays!
//these could also be seen as the single most important law of the universe and everything
const int8_t offsets[] = {
	0x01, -0x01, 0x10, -0x10, //orthogonal (last is white pawn forward)
	-0x11, -0x0F, 0x11, 0x0F, //diagonal (first group of 2 is white pawn capture)
	0x21, 0x1F, 0x12, 0x0E, -0x21, -0x1F, -0x12, -0x0E, //knight moves
	0x10, 0x11, 0x0F, //black pawn
};


void print_board(uint8_t *b)
{
	char symbols[17] = ".?pnkbrq?P?NKBRQ";

	for (uint8_t i = 0; i < 120; i++)
	{
		if (i & OFFBOARD)
		{
			if (!(i & 7)) std::cout << std::endl;
		}
		else std::cout << symbols[b[i] & 15] << ' ';
	}

	std::cout << "\n\n";
}

void print_board_full(uint8_t* b)
{
	char symbols[17] = ".?pnkbrq?P?NKBRQ";

	for (uint8_t i = 0; i < 128; i++)
	{
		if (i & OFFBOARD)
		{
			if (!(i & 7)) std::cout << " | ";

			if (b[i & 0x77]) printf("%c ", 64 + b[i]);
			else printf(". ");
			if ((i & 7) == 7) std::cout << std::endl;
		}
		else std::cout << symbols[b[i] & 15] << (((b[i] & MOVED) || !b[i]) ? ' ' : '*');
	}

	for (uint8_t i = 0; i < 32; i++)
	{
		printf("%2x ", plist[i]);
		if ((i & 7) == 7) std::cout << std::endl;
	}

	std::cout << "\n\n";
}

void print_move(MOVE move)
{
	char promo_symbols[] = "???n?brq";
	if (move.flags & F_PROMO)
		printf("%c%d%c%d%c ", (move.src & 7) + 'a', 8 - (move.src >> 4), (move.tgt & 7) + 'a', 8 - (move.tgt >> 4), promo_symbols[move.promo & PTYPE]);
	else
		printf("%c%d%c%d ", (move.src & 7) + 'a', 8 - (move.src >> 4), (move.tgt & 7) + 'a', 8 - (move.tgt >> 4));

}

//TODO: implement halfmove counter
void load_fen(std::string fen)
{
	char position[80];
	char turn;
	char castling[5], enpassant[3];
	int halfmove, fullmove;
	sscanf(fen.c_str(), "%s %c %s %s %d %d", position, &turn, castling, enpassant, &halfmove, &fullmove);
	// std::cout << position << std::endl;
	// std::cout << turn << std::endl;
	// std::cout << castling << std::endl;
	// std::cout << enpassant << std::endl;
	// std::cout << halfmove << std::endl;
	// std::cout << fullmove << std::endl;

	uint8_t sq = 0;
	uint8_t white_plist_idx = 0, black_plist_idx = 0;
	for (uint8_t i = 0; position[i]; i++)
	{
		char c = position[i];

		if (c == '\0') break; //END OF STRING
		else if (c == '/')
		{
			sq |= 0x0f; //skip line
		}
		else if (c >= '1' && c <= '8')
		{
			c -= '0';
			while(c)
			{
				board[sq] = 0;
				board[sq | 8] = 0;
				sq++;
				c--;
			}
			sq--;
		}
		else
		{
			switch(c)
			{
				case 'P': board[sq] = 011; break;
				case 'p': board[sq] = 022; break;
				case 'N': board[sq] = 013; break;
				case 'n': board[sq] = 023; break;
				case 'K': board[sq] = 014; break;
				case 'k': board[sq] = 024; break;
				case 'B': board[sq] = 015; break;
				case 'b': board[sq] = 025; break;
				case 'R': board[sq] = 016; break;
				case 'r': board[sq] = 026; break;
				case 'Q': board[sq] = 017; break;
				case 'q': board[sq] = 027; break;
			}

			if (c < 'a')
			{
				board[sq | 8] = black_plist_idx + 0x10;
				plist[0x10 + black_plist_idx++] = sq;
			}
			else
			{
				board[sq | 8] = white_plist_idx;
				plist[white_plist_idx++] = sq;
			}
		}
		sq++;
	}

	//fill up rest of piece list with "captured" pieces
	while (white_plist_idx < 16) plist[white_plist_idx++] = -1;
	while (black_plist_idx < 16) plist[0x10 + black_plist_idx++] = -1;

	// the kings have to be at the front of the plist
	for(sq = 0; sq < 120; sq++)
	{
		if (sq & OFFBOARD) continue;
		// printf("SQ %2x; ", sq);
		if (board[sq] == 024) //Black King Found!
		{
			uint8_t temp;

			temp = plist[0]; //swap in piece list
			plist[0] = plist[board[sq | 8]];
			plist[board[sq | 8]] = temp;

			// printf("BLACK KING AT %x; TMP %x\n", sq, temp);

			board[temp | 8] = board[plist[0] | 8]; //swap plist pointers
			board[plist[0] | 8] = 0;
		}
		else if (board[sq] == 014) //White King Found!
		{
			uint8_t temp;

			temp = plist[16]; //swap in piece list
			plist[16] = plist[board[sq | 8]];
			plist[board[sq | 8]] = temp;

			board[temp | 8] = board[plist[16] | 8]; //swap plist pointers
			board[plist[16] | 8] = 16;
		}
	}

	if (board[0x74]) board[0x74] |= MOVED; //White King
	if (board[0x77]) board[0x77] |= MOVED; //White Rooks
	if (board[0x70]) board[0x70] |= MOVED;

	if (board[0x04]) board[0x04] |= MOVED; //Black King
	if (board[0x07]) board[0x07] |= MOVED; //Black Rooks
	if (board[0x00]) board[0x00] |= MOVED;

	for (uint8_t i = 0; castling[i]; i++)
	{
		if (castling[i] == 'K')
		{
			board[0x74] &= ~MOVED;
			board[0x77] &= ~MOVED;
		}
		else if (castling[i] == 'Q')
		{
			board[0x74] &= ~MOVED;
			board[0x70] &= ~MOVED;
		}
		else if (castling[i] == 'k')
		{
			board[0x04] &= ~MOVED;
			board[0x07] &= ~MOVED;
		}
		else if (castling[i] == 'q')
		{
			board[0x04] &= ~MOVED;
			board[0x00] &= ~MOVED;
		}
	}

	board_stm = (turn == 'w') ? WHITE : BLACK;
	if (enpassant[0] == '-')
		board_last_target = -2;
	else
	{
		board_last_target = enpassant[0] + (board_stm << 1) - 0x41;
	}
}


uint8_t sq_attacked(uint8_t target, uint8_t attacker)
{
	//quite a lot of extra instructions for doing black first white second in the piece lists
	uint8_t end_value = (attacker ^ ENEMY) << 1; //start value + 16 pieces

	for (uint8_t plist_idx = (attacker & 16) ^ 16; plist_idx < end_value; plist_idx++) //iterate through the attacker's pieces
	{
		uint8_t cur_sq = plist[plist_idx];

		if (cur_sq & OFFBOARD) continue; //the piece has been captured: signaled by an off-the-board square (could improve that in the future)

		uint8_t rel = 0x77 + target - cur_sq;
		uint8_t ptype = board[cur_sq] & PTYPE; //The piece type
		uint8_t ray = RAYS[rel] & RAYMSK[ptype];

		if (!ray) continue; //no way that's attacking!

		if (ptype < 5) return cur_sq; //There is an attack from a leaper: cannot be blocked

		//now we have to iterate through ALL the squares in that direction!
		int8_t offset = RAY_OFFSETS[rel];

		cur_sq -= offset; //i fucked up: offset is always the wrong sign

		while (cur_sq != target) //don't have to check for off-the-board, since the ray can only be in there
		{
			//printf("%x,%x,%x\n", cur_sq, board[cur_sq], offset);
			if (board[cur_sq]) goto square_attacked_abort;

			cur_sq -= offset;
		}

		//Nothing has been hit: WE ARE UNDER ATTACK!!!
		return plist[plist_idx]; //cur_sq has been modified, so we must retake from the piece list (im sure that can be optimized)

	square_attacked_abort:; //Aborted square attack loop: hit a piece in-between
	}

	return -1; //NOTHING TO SEE HERE!
}


void generate_moves(MLIST *mlist, uint8_t stm, uint8_t last_target)
{
	mlist->count = 0; //just being extra-safe here, and diminishing code size
	
	uint8_t king_sq = plist[(stm & 16) ^ 16];
	uint8_t checking_piece = sq_attacked(king_sq, stm ^ ENEMY); //first piece to attack the king (in case of double check its just the first one)
	bool in_check = checking_piece != 0xFF; //are we in check?
	// printf("CHECK %x %x\n", in_check, stm);

	//quite a lot of extra instructions for doing black first white second in the piece lists
	uint8_t end_value = (stm ^ ENEMY) << 1; //start value + 16 pieces

	for (uint8_t plist_idx = (stm & 16) ^ 16; plist_idx < end_value; plist_idx++) //iterate through the attacker's pieces
	{
		uint8_t sq = plist[plist_idx];
		//printf("%x\n", sq);

		if (sq & OFFBOARD) continue; //the piece has been captured: signaled by an off-the-board square (could improve that in the future)

		//all the movegen code HERE
		uint8_t ptype = board[sq] & PTYPE;

		for (uint8_t i = noffsets[ptype]; i;)
		{
			i--; //do it HERE! not at the top! otherwise it's always off by 1!

			uint8_t abort_dpp = i; //abort trying to double-push immediately if trying to capture (abort_dpp cannot get lower)
			abort_dpp += (sq >> 4) % 5 != 1; //if pawn not on 2nd/7th rank, don't try to double-push
			
			int8_t offset = offsets[i + offset_idx[ptype]]; //current offset to TEST
			uint8_t current_target = sq + offset;

			//printf("%x,%x      => %x %d\n", sq, current_target, i, offset);

			//iterate through ray
			while (!((current_target & OFFBOARD) || (board[current_target]))) //go till we hit something
			{
				//printf("%x,%x,%x,%x\n", sq, piece, i, current_target);
				if (ptype > 2 || !i) //pawns can't move diagonally without taking! so filter 'em out!
				{
					//here we do horses, kings and straight-moving pawns
					mlist->moves[mlist->count] = MOVE{ sq, current_target, 0, 0, 0 };

					if (ptype < 3) //it's a pawn
					{
						//Had to add first condition, otherwise a bunch of "fake DPPs" would come
						if ((sq ^ current_target) == 0x20 && abort_dpp) mlist->moves[mlist->count].flags = F_DPP; //can use = since prev flag was 0
						else if ((current_target >> 4) % 7 == 0)
						{
							mlist->moves[mlist->count].flags = F_PROMO; //promotion
							mlist->moves[mlist->count].promo = QUEEN; //promotion to queen
							mlist->moves[mlist->count].score = SCORE_PROMO[0];

							for (uint8_t j = 1; j < 4; j++)
							{
								uint8_t promo_piece = PIECE_PROMO[j]; //ROOK, BISHOP, KNIGHT
								mlist->moves[mlist->count + j] = MOVE{ sq, current_target, F_PROMO, promo_piece, SCORE_PROMO[j] };
							}

							mlist->count += 3; //account for extra promos
						}
					}

					mlist->count++;
				}

				current_target += offset;

				//THE NEXT LINE COULD BE IMPROVED!
				//don't bother about pawns that just moved FORWARD 1 square (they can move up to 2 squares!)
				//!i is to test if the forward offset (idx+0) is tested
				//current_target has to be 3 or 4 for the pawn to go 2 squares
				if (ptype < 3 && !abort_dpp && !(offset & 0x0F))
				{
					abort_dpp = 1; //stop trying to double-pawn-push
					continue; //do another loop, without exiting the loop
				}
				if (ptype < 5) //if it's a leaper, abort after 1 iteration (or 2 for pawns, as written above), without erasing target info
				{
					if (ptype < 3) //dont wanna lose target info (problem is we added an extra offset)
					{
						current_target -= offset; //remove the extra offset we added
						current_target ^= 0x80; //exiting loop
					}
					else current_target = -1; //knights can jump 2 squares up/down; this might not be necessary
				}
			}

			//generate capture
			if (!(current_target & OFFBOARD) && ((board[current_target] ^ stm) & ENEMY) && (i || ptype > 2)) //ENEMY NEUTRALIZED (exception for pawns moving forward)
			{
				mlist->moves[mlist->count] = MOVE{ sq, current_target, F_CAPT, 0, SCORE_CAPT }; //thanks for the free stuff bro
				mlist->count++;

				if ((board[current_target] & PTYPE) == KING) return; //HEY! THAT'S ILLEGAL! (king captured)
				else if (ptype < 3 && (current_target >> 4) % 7 == 0) //pawn promotion
				{
					mlist->count--; //FASTER!!! (and lazier)

					mlist->moves[mlist->count].flags = F_PROMO | F_CAPT; //promotion AND capture!
					mlist->moves[mlist->count].promo = QUEEN; //promotion to queen
					mlist->moves[mlist->count].score = SCORE_PROMO[0];

					for (uint8_t j = 1; j < 4; j++)
					{
						//BUG HERE!!!!!! Promo piece has to be 6, 5 then 3 and NOT 4!
						uint8_t promo_piece = PIECE_PROMO[j]; //ROOK, BISHOP, KNIGHT
						mlist->moves[mlist->count + j] = MOVE{ sq, current_target, F_PROMO | F_CAPT, promo_piece, SCORE_PROMO[j] };
					}

					mlist->count += 4; //account for all 4 promos
				}
			}
			//TODO: try to get rid of the safety check at the beginning (except if it makes things run better)
			//not doing anything!
			else if ((last_target & OFFBOARD) == 0 && (current_target ^ 0x80) == (last_target ^ 0x10) && ptype < 3 && i) //En Passant Babe Lets Go!
			{
				// printf("EN PASSANT BABE LETS GO\n");

				current_target &= 0x77; //force the current target back on the real square (clear 0x80 loop interruption flag for leapers)

				mlist->moves[mlist->count] = MOVE{ sq, current_target, F_CAPT | F_EP, 0, SCORE_CAPT }; //NINJA MODE ACTIVATED!
				mlist->count++;
			}
		}
	}
	if (/* king_sq % 56 == 4 && */ !(board[king_sq] & MOVED) && !in_check) //our king hasn't moved, and not in check: castling can be legal
	{
		// printf("CASTLING! %x KING %x\n", stm, plist[(stm & 16) ^ 16]);

		//kingside castling
		if (board[king_sq + 1] == 0 && //empty square on f-file
			board[king_sq + 2] == 0 && //empty square on g-file
			board[king_sq + 3] == (stm | ROOK) && //a virgin rook on h-file
			sq_attacked(king_sq + 1, stm ^ ENEMY) == 0xFF) //transit square not attacked
		{
			uint8_t target = king_sq + 2;
			mlist->moves[mlist->count] = MOVE{ king_sq, target, F_KCASTLE, 0, SCORE_CASTLE }; //Ooooooh yeah! Don't watch kids!
			mlist->count++;
		}

		//queenside castling
		if (board[king_sq - 1] == 0 && //empty square on d-file
			board[king_sq - 2] == 0 && //empty square on c-file
			board[king_sq - 3] == 0 && //empty square on b-file
			board[king_sq - 4] == (stm | ROOK) && //a virgin rook on a-file
			sq_attacked(king_sq - 1, stm ^ ENEMY) == 0xFF) //transit square not attacked
		{
			uint8_t target = king_sq - 2;
			mlist->moves[mlist->count] = MOVE{ king_sq, target, F_QCASTLE, 0, SCORE_CASTLE }; //Ooooooh yeah! Don't watch kids!
			mlist->count++;
		}
	}
}

void generate_loud_moves(MLIST *mlist, uint8_t stm)
{
	mlist->count = 0; //just being extra-safe here, and diminishing code size
	
	// uint8_t king_sq = plist[(stm & 16) ^ 16];
	// uint8_t checking_piece = sq_attacked(king_sq, stm ^ ENEMY); //first piece to attack the king (in case of double check its just the first one)
	// bool in_check = checking_piece != 0xFF; //are we in check?
	// // printf("CHECK %x %x\n", in_check, stm);

	//quite a lot of extra instructions for doing black first white second in the piece lists
	uint8_t end_value = (stm ^ ENEMY) << 1; //start value + 16 pieces

	for (uint8_t plist_idx = (stm & 16) ^ 16; plist_idx < end_value; plist_idx++) //iterate through the attacker's pieces
	{
		uint8_t sq = plist[plist_idx];
		//printf("%x\n", sq);

		if (sq & OFFBOARD) continue; //the piece has been captured: signaled by an off-the-board square (could improve that in the future)

		//all the movegen code HERE
		uint8_t ptype = board[sq] & PTYPE;

		for (uint8_t i = noffsets[ptype]; i;)
		{
			i--; //do it HERE! not at the top! otherwise it's always off by 1!

			int8_t offset = offsets[i + offset_idx[ptype]]; //current offset to TEST
			uint8_t current_target = sq + offset;

			//printf("%x,%x      => %x %d\n", sq, current_target, i, offset);

			//iterate through ray
			while (!((current_target & OFFBOARD) || board[current_target])) //go till we hit something
			{
				if (ptype < 5) //if it's a leaper, abort after 1 iteration (or 2 for pawns, as written above), without erasing target info
				{
					break; //break the loop
				}

				current_target += offset; //move on to the next square
			}

			//generate capture
			if (!(current_target & OFFBOARD) && ((board[current_target] ^ stm) & ENEMY) && (i || ptype > 2)) //ENEMY NEUTRALIZED (exception for pawns moving forward)
			{
				mlist->moves[mlist->count] = MOVE{ sq, current_target, F_CAPT, 0, SCORE_CAPT }; //thanks for the free stuff bro
				mlist->count++;

				if ((board[current_target] & PTYPE) == KING) return; //HEY! THAT'S ILLEGAL! (king captured)
				// else if (ptype < 3 && (current_target >> 4) % 7 == 0) //pawn promotion
				// {
				// 	mlist->count--; //FASTER!!! (and lazier)

				// 	mlist->moves[mlist->count].flags = F_PROMO | F_CAPT; //promotion AND capture!
				// 	mlist->moves[mlist->count].promo = QUEEN; //promotion to queen
				// 	mlist->moves[mlist->count].score = SCORE_PROMO[0];

				// 	for (uint8_t j = 1; j < 4; j++)
				// 	{
				// 		//BUG HERE!!!!!! Promo piece has to be 6, 5 then 3 and NOT 4!
				// 		uint8_t promo_piece = PIECE_PROMO[j]; //ROOK, BISHOP, KNIGHT
				// 		mlist->moves[mlist->count + j] = MOVE{ sq, current_target, F_PROMO | F_CAPT, promo_piece, SCORE_PROMO[j] };
				// 	}

				// 	mlist->count += 4; //account for all 4 promos
				// }
			}
		}
	}
}

MOVE_RESULT make_move(uint8_t stm, MOVE move)
{
	//Information to give:
	//MOVE_RESULT = {piece type, plist index}

	//The result that will be given away at the end
	//Contains the piece type, and then its index in the big piece list
	MOVE_RESULT result = MOVE_RESULT{ board[move.tgt], board[move.tgt | 8], board[move.src] };

	// result.prev_state = board[move.src]; //store old piece type (for promotions and recovery of castling rights)

	//move the piece
	board[move.tgt] = board[move.src] | MOVED; //add MOVED bit
	board[move.src] = 0;

	//move the piece list index
	board[move.tgt | 8] = board[move.src | 8];
	//board[move.src | 8] = 0; //we don't really need this line, since the square will be empty anyways

	//update piece list
	plist[board[move.tgt | 8]] = move.tgt;

	if (move.flags & F_EP) //EN PASSANT
	{
		//calculating the new target
		//maybe we could do this more efficiently?
		move.tgt -= (stm - 12) << 2;

		//recalculate the result
		result = MOVE_RESULT{ board[move.tgt], board[move.tgt | 8], result.prev_state };

		board[move.tgt] = 0; //we just take it!
		plist[result.plist_idx] = -1; //update piece list
	}
	else
	{
		if (result.piece) plist[result.plist_idx] = -1; //Signal captured piece by off-the-board coordinate in piece list

		if (move.flags & F_CASTLE)
		{
			if (move.flags & F_KCASTLE) //WARNING: only implemented for white so far!
			{
				board[move.src + 1] = board[move.tgt] | ROOK; //compiler will optimize that together, don't worry; we use that king | rook = rook
				board[move.src + 3] = 0;

				board[move.src + 9] = board[move.src + 11]; //we know the rook hasn't moved, so it's constant = optim?
				plist[board[move.src + 9]] = move.src + 1; //same goes here (don't have to unset that, and using index 0x7F OR 0x7D is the same)
			}
			else
			{
				board[move.src - 1] = board[move.tgt] | ROOK; //compiler will optimize that together, don't worry
				board[move.src - 4] = 0;

				board[move.src + 7] = board[move.src + 4]; //we know the rook hasn't moved, so it's constant = optim?
				plist[board[move.src + 7]] = move.src - 1; //same goes here (don't have to unset that, and using index 0x78 OR 0x7B is the same)
			}
		}
		else if (move.flags & F_PROMO)
		{
			board[move.tgt] &= 0b111000; //keep the color bits, but clear the piece bits
			board[move.tgt] |= move.promo; //change the piece!
		}
	}

	return result;
}

void unmake_move(uint8_t stm, MOVE move, MOVE_RESULT move_result)
{
	//Information to use:
	//MOVE = {source, target, flags, promo, score}
	//MOVE_RESULT = {piece type, plist index, prev_state}

	//1st half
	board[move.tgt] = move_result.prev_state; //restore the piece type (test)

	board[move.src] = board[move.tgt];
	// board[move.src] = move_result.prev_state; //Undo a promotion, or recover castling rights
	board[move.src | 8] = board[move.tgt | 8]; //Piece list index

	if (move.flags & F_EP) //en passant
	{
		board[move.tgt] = 0; //target clear
		move.tgt -= (stm - 12) << 2; //readjusting target
	}

	//2nd half
	board[move.tgt] = move_result.piece; //Retrieve the piece (in case of a non-capturing move, the piece is 0 anyway)
	board[move.tgt | 8] = move_result.plist_idx; //Retrieve the piece list index

	//Update piece list
	plist[board[move.src | 8]] = move.src; //Move back the piece, again
	if(move_result.piece) plist[move_result.plist_idx] = move.tgt; //And "revive" the captured enemy piece (if one has been captured) could be optimized tho

	if (move.flags & F_CASTLE) //We castlin' babe! Or rather, we are "uncastling"
	{
		board[move.src] &= ~MOVED; //Let's not talk about our castling. Shhhhhht! Just act like it never happened!

		if (move.flags & F_KCASTLE)
		{
			board[move.src + 3] = board[move.src] | ROOK; //compiler will optimize that together, don't worry
			board[move.src + 1] = 0;

			board[move.src + 11] = board[move.src + 9]; //we know what rook we castled with, so it's constant = optim?
			plist[board[move.src + 11]] = move.src + 3; //same goes here
		}
		else
		{
			board[move.src - 4] = board[move.src] | ROOK; //compiler will optimize that together, don't worry
			board[move.src - 1] = 0;

			board[move.src + 4] = board[move.src + 7]; //we know what rook we castled with, so it's constant = optim?
			plist[board[move.src + 4]] = move.src - 4; //same goes here
		}
	}
}
