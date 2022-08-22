#include "order.h"


uint16_t killers[MAX_DEPTH][2];
uint32_t history[HIST_LENGTH];
uint32_t conthist[CHIST_LENGTH];


void clear_history()
{
    for (uint16_t i = 0; i < HIST_LENGTH; i++) history[i] = 0;
    for (uint32_t i = 0; i < CHIST_LENGTH; i++) conthist[i] = 0;
}

void score_moves(MLIST *mlist, uint8_t stm, uint16_t hash_move, MOVE prevmove, bool use_see, uint8_t ply)
{
    //add scores to each move, and sort the moves by score (TODO: no longer do that, but pick the moves in the move loop)
    //this engine uses insertion sort, which is O(n^2) in the worst case, but works well for small n
    //TODO: fix switching to selection sort for move picker
    for (int i = 0; i < mlist->count; i++)
    {
        MOVE curmove = mlist->moves[i];

        //add score to move
        //Hash move
        if (MOVE_ID(curmove) == hash_move)
        {
            curmove.score = SCORE_HASH;
        }
        //MVV-LVA captures
        else if (curmove.flags & F_CAPT)
        {
            curmove.score = SCORE_CAPT;
            if (use_see) //SEE
            {
                curmove.score += see(stm, curmove.tgt); //Too slow!!!
            }
            else //MVV-LVA
            {
                curmove.score -= board[curmove.src] & PTYPE;
                curmove.score += (board[curmove.tgt] & PTYPE) << 3;
            }

            // //enemy pawn attack penalty (temp)
            // uint8_t enemy_pawn = (stm >> 3) ^ 3; //enemy pawn's type (1 = white pawn; 2 = black pawn)
            // uint8_t pawn_rank_square = curmove.tgt + (stm << 2) - 48;
            
            // if (!((pawn_rank_square - 1) & OFFBOARD) && (board[pawn_rank_square - 1] & PTYPE) == enemy_pawn) //if the target is attacked by a pawn
            //     curmove.score -= PAWN_ATTACK_PENALTY; //penalize the move
            // else if (!((pawn_rank_square + 1) & OFFBOARD) && (board[pawn_rank_square + 1] & PTYPE) == enemy_pawn) //same on the other side
            //     curmove.score -= PAWN_ATTACK_PENALTY;
        }
        else if (curmove.flags < F_CAPT) //neither capture nor promo
        {
            uint16_t move_id = MOVE_ID(curmove);

            //Killer move handling
            if (move_id == killers[ply][0]) //Primary killer move
            {
                curmove.score = SCORE_KILLER_PRIMARY;
            }
            else if (move_id == killers[ply][1]) //Secondary killer move
            {
                curmove.score = SCORE_KILLER_SECONDARY;
            }
            else
            {
                curmove.score = SCORE_QUIET;
                curmove.score += history[PSQ_INDEX(curmove)];
                curmove.score += conthist[CH_INDEX(prevmove, curmove)];

                //penalize leaving pieces attacked by enemy pawns (TODO: try tuning this! or scrapping the idea)
                // uint8_t enemy_pawn = (stm >> 3) ^ 3; //enemy pawn's type (1 = white pawn; 2 = black pawn)
                // uint8_t pawn_rank_square = curmove.tgt + (stm << 2) - 48;

                // if (!((pawn_rank_square - 1) & OFFBOARD) && (board[pawn_rank_square - 1] & PTYPE) == enemy_pawn) //if the target is attacked by a pawn
                //     curmove.score -= PAWN_ATTACK_PENALTY; //penalize the move
                // else if (!((pawn_rank_square + 1) & OFFBOARD) && (board[pawn_rank_square + 1] & PTYPE) == enemy_pawn) //same on the other side
                //     curmove.score -= PAWN_ATTACK_PENALTY;

                // curmove.score -= (!((pawn_rank_square - 1) & OFFBOARD) && (board[pawn_rank_square - 1] & PTYPE) == enemy_pawn) * PAWN_ATTACK_PENALTY;
                // curmove.score -= (!((pawn_rank_square + 1) & OFFBOARD) && (board[pawn_rank_square + 1] & PTYPE) == enemy_pawn) * PAWN_ATTACK_PENALTY;
            }
        }

        // mlist->moves[i] = curmove;

        //insert move into sorted list
        int j = i;
        while (j > 0 && mlist->moves[j - 1].score > curmove.score)
        {
            mlist->moves[j] = mlist->moves[j - 1];
            j--;
        }
        mlist->moves[j] = curmove;
    }
}

//BUG: function works if list sorted, but not if it isnt (which is the goal in the end)
MOVE pick_move(MLIST *mlist)
{
    //pick the next move from the sorted list, which is the one with the highest score, and swap it with the element mlist->moves[mlist->count - 1]
    uint8_t best_index = 0;
    uint32_t best_score = 0;

    for (uint8_t index = 0; index < mlist->count; index++)
    {
        if (mlist->moves[index].score >= best_score)
        {
            best_index = index;
            best_score = mlist->moves[index].score;
        }
    }

    //Do the "swap": no need to really swap, since mlist->moves[mlist->count - 1] isn't accessed in main search loop anyway and will be irrelevant afterwards
    MOVE best_move = mlist->moves[best_index]; //Move with the best score
    mlist->moves[best_index] = mlist->moves[mlist->count - 1]; //replace best move with last move; its stored in variable anyway
    --mlist->count;

    return best_move;
}

int16_t see(uint8_t stm, uint8_t square)
{
    int16_t value = 0;
    MOVE curmove = get_smallest_attacker_move(stm, square);

    if (curmove.flags) //a capture is possible (otherwise a MOVE with all zeroes will be returned)
    {
        MOVE_RESULT res = make_move(stm, curmove);
        value = std::max(0, SEE_VALUES[res.piece & PTYPE] - see(stm ^ ENEMY, square));
        unmake_move(stm, curmove, res);
    }

    return value;
}
