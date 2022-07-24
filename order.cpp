#include "order.h"


uint16_t killers[MAX_DEPTH][2];
uint32_t history[64 * 64];


static const uint8_t LogTable256[256] = 
{
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};


void clear_history()
{
    for (uint16_t i = 0; i < 64 * 64; i++) history[i] = 0;
}

void order_moves(MLIST *mlist, uint8_t ply)
{
    //add scores to each move, and sort the moves by score
    //this engine uses insertion sort, which is O(n^2) in the worst case, but works well for small n
    for (int i = 0; i < mlist->count; i++)
    {
        MOVE curmove = mlist->moves[i];

        //add score to move
        //MVV-LVA captures
        if (curmove.flags & F_CAPT)
        {
            curmove.score -= board[curmove.src] & PTYPE;
            curmove.score += (board[curmove.tgt] & PTYPE) << 3;
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
                curmove.score += history[move_id];
                // printf("%x\n", curmove.score);
            }
        }

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
