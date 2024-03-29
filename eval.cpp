#include "eval.h"


#define X88_PADDING 0,0,0,0,0,0,0,0,


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


const int16_t mg_pawn_table[] = {
      0,   0,   0,   0,   0,   0,  0,   0,   X88_PADDING
     98, 134,  61,  95,  68, 126, 34, -11,   X88_PADDING
     -6,   7,  26,  31,  65,  56, 25, -20,   X88_PADDING
    -14,  13,   6,  21,  23,  12, 17, -23,   X88_PADDING
    -27,  -2,  -5,  12,  17,   6, 10, -25,   X88_PADDING
    -26,  -4,  -4, -10,   3,   3, 33, -12,   X88_PADDING
    -35,  -1, -20, -23, -15,  24, 38, -22,   X88_PADDING
      0,   0,   0,   0,   0,   0,  0,   0,   X88_PADDING
};

const int16_t eg_pawn_table[] = {
      0,   0,   0,   0,   0,   0,   0,   0,   X88_PADDING
    178, 173, 158, 134, 147, 132, 165, 187,   X88_PADDING
     94, 100,  85,  67,  56,  53,  82,  84,   X88_PADDING
     32,  24,  13,   5,  -2,   4,  17,  17,   X88_PADDING
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,   X88_PADDING
      4,   7,  -6,   1,   0,  -5,  -1,  -8,   X88_PADDING
     13,   8,   8,  10,  13,   0,   2,  -7,   X88_PADDING
      0,   0,   0,   0,   0,   0,   0,   0,   X88_PADDING
};

const int16_t mg_knight_table[] = {
    -167, -89, -34, -49,  61, -97, -15, -107,   X88_PADDING
     -73, -41,  72,  36,  23,  62,   7,  -17,   X88_PADDING
     -47,  60,  37,  65,  84, 129,  73,   44,   X88_PADDING
      -9,  17,  19,  53,  37,  69,  18,   22,   X88_PADDING
     -13,   4,  16,  13,  28,  19,  21,   -8,   X88_PADDING
     -23,  -9,  12,  10,  19,  17,  25,  -16,   X88_PADDING
     -29, -53, -12,  -3,  -1,  18, -14,  -19,   X88_PADDING
    -105, -21, -58, -33, -17, -28, -19,  -23,   X88_PADDING
};

const int16_t eg_knight_table[] = {
    -58, -38, -13, -28, -31, -27, -63, -99,   X88_PADDING
    -25,  -8, -25,  -2,  -9, -25, -24, -52,   X88_PADDING
    -24, -20,  10,   9,  -1,  -9, -19, -41,   X88_PADDING
    -17,   3,  22,  22,  22,  11,   8, -18,   X88_PADDING
    -18,  -6,  16,  25,  16,  17,   4, -18,   X88_PADDING
    -23,  -3,  -1,  15,  10,  -3, -20, -22,   X88_PADDING
    -42, -20, -10,  -5,  -2, -20, -23, -44,   X88_PADDING
    -29, -51, -23, -15, -22, -18, -50, -64,   X88_PADDING
};

const int16_t mg_bishop_table[] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,   X88_PADDING
    -26,  16, -18, -13,  30,  59,  18, -47,   X88_PADDING
    -16,  37,  43,  40,  35,  50,  37,  -2,   X88_PADDING
     -4,   5,  19,  50,  37,  37,   7,  -2,   X88_PADDING
     -6,  13,  13,  26,  34,  12,  10,   4,   X88_PADDING
      0,  15,  15,  15,  14,  27,  18,  10,   X88_PADDING
      4,  15,  16,   0,   7,  21,  33,   1,   X88_PADDING
    -33,  -3, -14, -21, -13, -12, -39, -21,   X88_PADDING
};

const int16_t eg_bishop_table[] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,   X88_PADDING
     -8,  -4,   7, -12, -3, -13,  -4, -14,   X88_PADDING
      2,  -8,   0,  -1, -2,   6,   0,   4,   X88_PADDING
     -3,   9,  12,   9, 14,  10,   3,   2,   X88_PADDING
     -6,   3,  13,  19,  7,  10,  -3,  -9,   X88_PADDING
    -12,  -3,   8,  10, 13,   3,  -7, -15,   X88_PADDING
    -14, -18,  -7,  -1,  4,  -9, -15, -27,   X88_PADDING
    -23,  -9, -23,  -5, -9, -16,  -5, -17,   X88_PADDING
};

const int16_t mg_rook_table[] = {
     32,  42,  32,  51, 63,  9,  31,  43,   X88_PADDING
     27,  32,  58,  62, 80, 67,  26,  44,   X88_PADDING
     -5,  19,  26,  36, 17, 45,  61,  16,   X88_PADDING
    -24, -11,   7,  26, 24, 35,  -8, -20,   X88_PADDING
    -36, -26, -12,  -1,  9, -7,   6, -23,   X88_PADDING
    -45, -25, -16, -17,  3,  0,  -5, -33,   X88_PADDING
    -44, -16, -20,  -9, -1, 11,  -6, -71,   X88_PADDING
    -19, -13,   1,  17, 16,  7, -37, -26,   X88_PADDING
};

const int16_t eg_rook_table[] = {
    13, 10, 18, 15, 12,  12,   8,   5,   X88_PADDING
    11, 13, 13, 11, -3,   3,   8,   3,   X88_PADDING
     7,  7,  7,  5,  4,  -3,  -5,  -3,   X88_PADDING
     4,  3, 13,  1,  2,   1,  -1,   2,   X88_PADDING
     3,  5,  8,  4, -5,  -6,  -8, -11,   X88_PADDING
    -4,  0, -5, -1, -7, -12,  -8, -16,   X88_PADDING
    -6, -6,  0,  2, -9,  -9, -11,  -3,   X88_PADDING
    -9,  2,  3, -1, -5, -13,   4, -20,   X88_PADDING
};

const int16_t mg_queen_table[] = {
    -28,   0,  29,  12,  59,  44,  43,  45,   X88_PADDING
    -24, -39,  -5,   1, -16,  57,  28,  54,   X88_PADDING
    -13, -17,   7,   8,  29,  56,  47,  57,   X88_PADDING
    -27, -27, -16, -16,  -1,  17,  -2,   1,   X88_PADDING
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,   X88_PADDING
    -14,   2, -11,  -2,  -5,   2,  14,   5,   X88_PADDING
    -35,  -8,  11,   2,   8,  15,  -3,   1,   X88_PADDING
     -1, -18,  -9,  10, -15, -25, -31, -50,   X88_PADDING
};

const int16_t eg_queen_table[] = {
     -9,  22,  22,  27,  27,  19,  10,  20,   X88_PADDING
    -17,  20,  32,  41,  58,  25,  30,   0,   X88_PADDING
    -20,   6,   9,  49,  47,  35,  19,   9,   X88_PADDING
      3,  22,  24,  45,  57,  40,  57,  36,   X88_PADDING
    -18,  28,  19,  47,  31,  34,  39,  23,   X88_PADDING
    -16, -27,  15,   6,   9,  17,  10,   5,   X88_PADDING
    -22, -23, -30, -16, -16, -23, -36, -32,   X88_PADDING
    -33, -28, -22, -43,  -5, -32, -20, -41,   X88_PADDING
};

const int16_t mg_king_table[] = {
    -65,  23,  16, -15, -56, -34,   2,  13,   X88_PADDING
     29,  -1, -20,  -7,  -8,  -4, -38, -29,   X88_PADDING
     -9,  24,   2, -16, -20,   6,  22, -22,   X88_PADDING
    -17, -20, -12, -27, -30, -25, -14, -36,   X88_PADDING
    -49,  -1, -27, -39, -46, -44, -33, -51,   X88_PADDING
    -14, -14, -22, -46, -44, -30, -15, -27,   X88_PADDING
      1,   7,  -8, -64, -43, -16,   9,   8,   X88_PADDING
    -15,  36,  12, -54,   8, -28,  24,  14,   X88_PADDING
};

const int16_t eg_king_table[] = {
    -74, -35, -18, -18, -11,  15,   4, -17,   X88_PADDING
    -12,  17,  14,  17,  17,  38,  23,  11,   X88_PADDING
     10,  17,  23,  15,  20,  45,  44,  13,   X88_PADDING
     -8,  22,  24,  27,  26,  33,  26,   3,   X88_PADDING
    -18,  -4,  21,  24,  27,  23,   9, -11,   X88_PADDING
    -19,  -3,  11,  21,  23,  16,   7,  -9,   X88_PADDING
    -27, -11,   4,  13,  14,   4,  -5, -17,   X88_PADDING
    -53, -34, -21, -11, -28, -14, -24, -43,   X88_PADDING
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

const uint8_t eg_win_material[] = {0, 5, 5, 2, 0, 3, 5, 5}; //at least 3 knights or 2 bishops; or knight + bishop

//Evaluation function
int16_t evaluate(uint8_t stm)
{
    //do both sides have enough pieces to win in an endgame?
    /* uint8_t win_material_white = 0;
    uint8_t win_material_black = 0;
    for (uint8_t plist_idx = 1; plist_idx < 16; plist_idx++) //iterate through pieces (black king can be skipped ez)
        if (plist[plist_idx] != 0xff) //piece not captured
    		win_material_black += eg_win_material[board[plist[plist_idx]] & PTYPE];
    for (uint8_t plist_idx = 17; plist_idx < 32; plist_idx++) //iterate through pieces (black king can be skipped ez)
        if (plist[plist_idx] != 0xff) //piece not captured
    		win_material_white += eg_win_material[board[plist[plist_idx]] & PTYPE];

    if (win_material_black < 5 && win_material_white < 5) return INSUFFICIENT_MATERIAL; //no side can win! */

    //compute number of each piece
    // uint8_t npiece[16] = {}; //initialize number of pieces to 0
    // for (uint8_t plist_idx = 1; plist_idx < 32; plist_idx++) //iterate through all pieces (black king can be skipped ez)
    //     if (plist[plist_idx] != 0xff) //piece not captured
    // 		npiece[board[plist[plist_idx]] & 15]++; //add 1 to the number of that piece

    //get NNUE result
    int32_t nnue_result = eval_nnue_inc(stm, &stack);
    //clamp the NNUE result depending on who has enough material to win
    nnue_result = (int16_t)std::max(std::min(nnue_result, 9999), -9999);
    //nnue_result = (int16_t)std::max(std::min(nnue_result, 9999 * (win_material_white >= 5)), -9999 * (win_material_black >= 5));

    //compute phase
    // uint8_t phase = TOTAL_PHASE; //phase
    // for (uint8_t plist_idx = 1; plist_idx < 32; plist_idx++) //iterate through all pieces (black king can be skipped ez)
    //     if (plist[plist_idx] != 0xff) //piece not captured
    // 		phase -= game_phase[board[plist[plist_idx]] & PTYPE]; //compute phase
    
    return nnue_result;

    // #uint16_t multiplier = 512 + phase;
    // return (int16_t)std::max(std::min(((int64_t)nnue_result * multiplier) / 512, 9999L), -9999L); //clamp the final eval
}
