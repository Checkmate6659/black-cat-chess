#include "order.h"


uint16_t killers[MAX_DEPTH][2];

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
            //Killer move handling
            if (MOVE_ID(curmove) == killers[ply][0]) //Primary killer move
            {
                curmove.score = SCORE_KILLER_PRIMARY;
            }
            else if (MOVE_ID(curmove) == killers[ply][1]) //Secondary killer move
            {
                curmove.score = SCORE_KILLER_SECONDARY;
            }
            else
            {
                //TODO: add history and other things
                curmove.score = SCORE_QUIET;
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
