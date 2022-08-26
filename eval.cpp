#include "eval.h"


#define PSQT_PADDING 0,0,0,0,0,0,0,0,

//material/psqt from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19
const int16_t mg_piece_values[] = {
    0, //0 is the empty square
    82, //1 is the white pawn
    82, //2 is the black pawn
    337, //3 is the knight
    -MATE_SCORE / 2, //4 is the king
    365, //5 is the bishop
    477, //6 is the rook
    1025, //7 is the queen
};

const int16_t eg_piece_values[] = {
    0, //0 is the empty square
    94, //1 is the white pawn
    94, //2 is the black pawn
    281, //3 is the knight
    -MATE_SCORE / 2, //4 is the king
    297, //5 is the bishop
    512, //6 is the rook
    936, //7 is the queen
};

const uint8_t game_phase[] = {0,
    0, 0, //Pawns don't count in the game phase
    1, //Knights
    0, //Kings don't count
    1, //Bishops
    2, //Rooks
    4, //Queens
};


int16_t mg_pawn_table[] = {
      0,   0,   0,   0,   0,   0,  0,   0,   PSQT_PADDING
     98, 134,  61,  95,  68, 126, 34, -11,   PSQT_PADDING
     -6,   7,  26,  31,  65,  56, 25, -20,   PSQT_PADDING
    -14,  13,   6,  21,  23,  12, 17, -23,   PSQT_PADDING
    -27,  -2,  -5,  12,  17,   6, 10, -25,   PSQT_PADDING
    -26,  -4,  -4, -10,   3,   3, 33, -12,   PSQT_PADDING
    -35,  -1, -20, -23, -15,  24, 38, -22,   PSQT_PADDING
      0,   0,   0,   0,   0,   0,  0,   0,   PSQT_PADDING
};

int16_t eg_pawn_table[] = {
      0,   0,   0,   0,   0,   0,   0,   0,   PSQT_PADDING
    178, 173, 158, 134, 147, 132, 165, 187,   PSQT_PADDING
     94, 100,  85,  67,  56,  53,  82,  84,   PSQT_PADDING
     32,  24,  13,   5,  -2,   4,  17,  17,   PSQT_PADDING
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,   PSQT_PADDING
      4,   7,  -6,   1,   0,  -5,  -1,  -8,   PSQT_PADDING
     13,   8,   8,  10,  13,   0,   2,  -7,   PSQT_PADDING
      0,   0,   0,   0,   0,   0,   0,   0,   PSQT_PADDING
};

int16_t mg_knight_table[] = {
    -167, -89, -34, -49,  61, -97, -15, -107,   PSQT_PADDING
     -73, -41,  72,  36,  23,  62,   7,  -17,   PSQT_PADDING
     -47,  60,  37,  65,  84, 129,  73,   44,   PSQT_PADDING
      -9,  17,  19,  53,  37,  69,  18,   22,   PSQT_PADDING
     -13,   4,  16,  13,  28,  19,  21,   -8,   PSQT_PADDING
     -23,  -9,  12,  10,  19,  17,  25,  -16,   PSQT_PADDING
     -29, -53, -12,  -3,  -1,  18, -14,  -19,   PSQT_PADDING
    -105, -21, -58, -33, -17, -28, -19,  -23,   PSQT_PADDING
};

int16_t eg_knight_table[] = {
    -58, -38, -13, -28, -31, -27, -63, -99,   PSQT_PADDING
    -25,  -8, -25,  -2,  -9, -25, -24, -52,   PSQT_PADDING
    -24, -20,  10,   9,  -1,  -9, -19, -41,   PSQT_PADDING
    -17,   3,  22,  22,  22,  11,   8, -18,   PSQT_PADDING
    -18,  -6,  16,  25,  16,  17,   4, -18,   PSQT_PADDING
    -23,  -3,  -1,  15,  10,  -3, -20, -22,   PSQT_PADDING
    -42, -20, -10,  -5,  -2, -20, -23, -44,   PSQT_PADDING
    -29, -51, -23, -15, -22, -18, -50, -64,   PSQT_PADDING
};

int16_t mg_bishop_table[] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,   PSQT_PADDING
    -26,  16, -18, -13,  30,  59,  18, -47,   PSQT_PADDING
    -16,  37,  43,  40,  35,  50,  37,  -2,   PSQT_PADDING
     -4,   5,  19,  50,  37,  37,   7,  -2,   PSQT_PADDING
     -6,  13,  13,  26,  34,  12,  10,   4,   PSQT_PADDING
      0,  15,  15,  15,  14,  27,  18,  10,   PSQT_PADDING
      4,  15,  16,   0,   7,  21,  33,   1,   PSQT_PADDING
    -33,  -3, -14, -21, -13, -12, -39, -21,   PSQT_PADDING
};

int16_t eg_bishop_table[] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,   PSQT_PADDING
     -8,  -4,   7, -12, -3, -13,  -4, -14,   PSQT_PADDING
      2,  -8,   0,  -1, -2,   6,   0,   4,   PSQT_PADDING
     -3,   9,  12,   9, 14,  10,   3,   2,   PSQT_PADDING
     -6,   3,  13,  19,  7,  10,  -3,  -9,   PSQT_PADDING
    -12,  -3,   8,  10, 13,   3,  -7, -15,   PSQT_PADDING
    -14, -18,  -7,  -1,  4,  -9, -15, -27,   PSQT_PADDING
    -23,  -9, -23,  -5, -9, -16,  -5, -17,   PSQT_PADDING
};

int16_t mg_rook_table[] = {
     32,  42,  32,  51, 63,  9,  31,  43,   PSQT_PADDING
     27,  32,  58,  62, 80, 67,  26,  44,   PSQT_PADDING
     -5,  19,  26,  36, 17, 45,  61,  16,   PSQT_PADDING
    -24, -11,   7,  26, 24, 35,  -8, -20,   PSQT_PADDING
    -36, -26, -12,  -1,  9, -7,   6, -23,   PSQT_PADDING
    -45, -25, -16, -17,  3,  0,  -5, -33,   PSQT_PADDING
    -44, -16, -20,  -9, -1, 11,  -6, -71,   PSQT_PADDING
    -19, -13,   1,  17, 16,  7, -37, -26,   PSQT_PADDING
};

int16_t eg_rook_table[] = {
    13, 10, 18, 15, 12,  12,   8,   5,   PSQT_PADDING
    11, 13, 13, 11, -3,   3,   8,   3,   PSQT_PADDING
     7,  7,  7,  5,  4,  -3,  -5,  -3,   PSQT_PADDING
     4,  3, 13,  1,  2,   1,  -1,   2,   PSQT_PADDING
     3,  5,  8,  4, -5,  -6,  -8, -11,   PSQT_PADDING
    -4,  0, -5, -1, -7, -12,  -8, -16,   PSQT_PADDING
    -6, -6,  0,  2, -9,  -9, -11,  -3,   PSQT_PADDING
    -9,  2,  3, -1, -5, -13,   4, -20,   PSQT_PADDING
};

int16_t mg_queen_table[] = {
    -28,   0,  29,  12,  59,  44,  43,  45,   PSQT_PADDING
    -24, -39,  -5,   1, -16,  57,  28,  54,   PSQT_PADDING
    -13, -17,   7,   8,  29,  56,  47,  57,   PSQT_PADDING
    -27, -27, -16, -16,  -1,  17,  -2,   1,   PSQT_PADDING
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,   PSQT_PADDING
    -14,   2, -11,  -2,  -5,   2,  14,   5,   PSQT_PADDING
    -35,  -8,  11,   2,   8,  15,  -3,   1,   PSQT_PADDING
     -1, -18,  -9,  10, -15, -25, -31, -50,   PSQT_PADDING
};

int16_t eg_queen_table[] = {
     -9,  22,  22,  27,  27,  19,  10,  20,   PSQT_PADDING
    -17,  20,  32,  41,  58,  25,  30,   0,   PSQT_PADDING
    -20,   6,   9,  49,  47,  35,  19,   9,   PSQT_PADDING
      3,  22,  24,  45,  57,  40,  57,  36,   PSQT_PADDING
    -18,  28,  19,  47,  31,  34,  39,  23,   PSQT_PADDING
    -16, -27,  15,   6,   9,  17,  10,   5,   PSQT_PADDING
    -22, -23, -30, -16, -16, -23, -36, -32,   PSQT_PADDING
    -33, -28, -22, -43,  -5, -32, -20, -41,   PSQT_PADDING
};

int16_t mg_king_table[] = {
    -65,  23,  16, -15, -56, -34,   2,  13,   PSQT_PADDING
     29,  -1, -20,  -7,  -8,  -4, -38, -29,   PSQT_PADDING
     -9,  24,   2, -16, -20,   6,  22, -22,   PSQT_PADDING
    -17, -20, -12, -27, -30, -25, -14, -36,   PSQT_PADDING
    -49,  -1, -27, -39, -46, -44, -33, -51,   PSQT_PADDING
    -14, -14, -22, -46, -44, -30, -15, -27,   PSQT_PADDING
      1,   7,  -8, -64, -43, -16,   9,   8,   PSQT_PADDING
    -15,  36,  12, -54,   8, -28,  24,  14,   PSQT_PADDING
};

int16_t eg_king_table[] = {
    -74, -35, -18, -18, -11,  15,   4, -17,   PSQT_PADDING
    -12,  17,  14,  17,  17,  38,  23,  11,   PSQT_PADDING
     10,  17,  23,  15,  20,  45,  44,  13,   PSQT_PADDING
     -8,  22,  24,  27,  26,  33,  26,   3,   PSQT_PADDING
    -18,  -4,  21,  24,  27,  23,   9, -11,   PSQT_PADDING
    -19,  -3,  11,  21,  23,  16,   7,  -9,   PSQT_PADDING
    -27, -11,   4,  13,  14,   4,  -5, -17,   PSQT_PADDING
    -53, -34, -21, -11, -28, -14, -24, -43,   PSQT_PADDING
};


const int16_t *mg_psqt[] = {
    nullptr,
    mg_pawn_table,
    mg_pawn_table,
    mg_knight_table,
    mg_king_table,
    mg_bishop_table,
    mg_rook_table,
    mg_queen_table,
};

const int16_t *eg_psqt[] = {
    nullptr,
    eg_pawn_table,
    eg_pawn_table,
    eg_knight_table,
    eg_king_table,
    eg_bishop_table,
    eg_rook_table,
    eg_queen_table,
};


int16_t evaluate(uint8_t stm)
{
    uint8_t phase = 0; //Game phase: lower means closer to the endgame (less pieces on board)
    int16_t midgame_psqt = 0;
    int16_t endgame_psqt = 0;

    //material/PSQT/game phase loop
    for (uint8_t i = 0; i < 32; i++) //calculate the phase of the game and material/PSQT for mg/eg
    {
        uint8_t sq = plist[i];
        if (sq == 0xFF) continue;
        
        uint8_t piece = board[sq];

        phase += game_phase[piece & PTYPE]; //add to the game phase

        uint8_t psqt_index = sq ^ (piece & 16) * 7;
        int16_t perspective = (piece & 16) ? -1 : 1;

        midgame_psqt += perspective * (mg_piece_values[piece & PTYPE] + mg_psqt[piece & PTYPE][psqt_index]); //midgame material/PSQT
        endgame_psqt += perspective * (eg_piece_values[piece & PTYPE] + eg_psqt[piece & PTYPE][psqt_index]); //endgame material/PSQT
    }

    phase = std::min(phase, (uint8_t)TOTAL_PHASE); //by promoting pawns to queens, the game phase could be higher than the total phase
    int16_t psqt_eval = (midgame_psqt * phase + endgame_psqt * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    return psqt_eval * ((stm & BLACK) ? -1 : 1);
}
