//NNUE wrapping functions

#include "board.h"
#include "./nnue/nnue.h"
#include "nnue.h"


const uint8_t NNUE_PIECE_INDEX[16] = { //NNUE pieces
    blank,
    wpawn, bpawn,
    bknight,
    bking,
    bbishop,
    brook,
    bqueen,

    blank,
    wpawn, bpawn,
    wknight,
    wking,
    wbishop,
    wrook,
    wqueen,
};

void init_nnue (char *fname)
{
    nnue_init(fname);
}

int eval_nnue (uint8_t stm)
{
    int pieces[33], squares[32]; //squares can probably be just 32 elements
    uint8_t npieces = 2;

    pieces[0] = wking;
    pieces[1] = bking;
    squares[0] = ((plist[16] + (plist[16] & 7)) >> 1) ^ 56;
    squares[1] = ((plist[0] + (plist[0] & 7)) >> 1) ^ 56;

    for (uint8_t i = 1; i < 32; i++)
    {
        if (i == 16) continue; //skip white king
        uint8_t sq = plist[i];
        if (sq == 0xFF) continue;
        uint8_t piece = board[sq];
        assert(piece);

        uint8_t nnue_sq = ((sq + (sq & 7)) >> 1) ^ 56;
        uint8_t nnue_piece = NNUE_PIECE_INDEX[piece & 15];

        pieces[npieces] = nnue_piece;
        squares[npieces] = nnue_sq;
        npieces++;
    }

    pieces[npieces] = blank;

    return nnue_evaluate((stm == WHITE) ? white : black, (int*)pieces, (int*)squares);
}

