#include "eval.h"


#define PSQT_PADDING 0,0,0,0,0,0,0,0,

const int16_t mg_piece_values[] = {
    0, //0 is the empty square
    100, //1 is the white pawn
    100, //2 is the black pawn
    320, //3 is the knight
    -MATE_SCORE / 2, //4 is the king
    330, //5 is the bishop
    500, //6 is the rook
    900, //7 is the queen
};

const int16_t eg_piece_values[] = {
    0, //0 is the empty square
    100, //1 is the white pawn
    100, //2 is the black pawn
    320, //3 is the knight
    -MATE_SCORE / 2, //4 is the king
    330, //5 is the bishop
    500, //6 is the rook
    900, //7 is the queen
};

const uint8_t game_phase[] = {0,
    0, 0, //Pawns don't count in the game phase
    1, //Knights
    0, //Kings don't count
    1, //Bishops
    2, //Rooks
    4, //Queens
};


//On every PSQT, the White side is on the left, and the Black side is on the top.
const int8_t PAWN_PSQT[] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50, -5,-10,-10, 20, 20,-10,-10, -5,
     10, 10, 20, 30, 30, 20, 10, 10, -5,  5, 10,  0,  0, 10,  5, -5,
      5,  5, 10, 25, 25, 10,  5,  5,  0,  0,  0,-20,-20,  0,  0,  0,
      0,  0,  0, 20, 20,  0,  0,  0, -5, -5,-10,-25,-25,-10, -5, -5,
      5, -5,-10,  0,  0,-10, -5,  5,-10,-10,-20,-30,-30,-20,-10,-10,
      5, 10, 10,-20,-20, 10, 10,  5,-50,-50,-50,-50,-50,-50,-50,-50,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

const int8_t KNIGHT_PSQT[] = {
    -50,-40,-30,-30,-30,-30,-40,-50, 50, 40, 30, 30, 30, 30, 40, 50,
    -40,-20,  0,  0,  0,  0,-20,-40, 40, 20,  0, -5, -5,  0, 20, 40,
    -30,  0, 10, 15, 15, 10,  0,-30, 30, -5,-10,-15,-15,-10, -5, 30,
    -30,  5, 15, 20, 20, 15,  5,-30, 30,  0,-15,-20,-20,-15,  0, 30,
    -30,  0, 15, 20, 20, 15,  0,-30, 30, -5,-15,-20,-20,-15, -5, 30,
    -30,  5, 10, 15, 15, 10,  5,-30, 30,  0,-10,-15,-15,-10,  0, 30,
    -40,-20,  0,  5,  5,  0,-20,-40, 40, 20,  0,  0,  0,  0, 20, 40,
    -50,-40,-30,-30,-30,-30,-40,-50, 50, 40, 30, 30, 30, 30, 40, 50,
};

const int8_t BISHOP_PSQT[] = {
    -20,-10,-10,-10,-10,-10,-10,-20, 20, 10, 10, 10, 10, 10, 10, 20,
    -10,  0,  0,  0,  0,  0,  0,-10, 10, -5,  0,  0,  0,  0, -5, 10,
    -10,  0,  5, 10, 10,  5,  0,-10, 10,-10,-10,-10,-10,-10,-10, 10,
    -10,  5,  5, 10, 10,  5,  5,-10, 10,  0,-10,-10,-10,-10,  0, 10,
    -10,  0, 10, 10, 10, 10,  0,-10, 10, -5, -5,-10,-10, -5, -5, 10,
    -10, 10, 10, 10, 10, 10, 10,-10, 10,  0, -5,-10,-10, -5,  0, 10,
    -10,  5,  0,  0,  0,  0,  5,-10, 10,  0,  0,  0,  0,  0,  0, 10,
    -20,-10,-10,-10,-10,-10,-10,-20, 20, 10, 10, 10, 10, 10, 10, 20,
};

const int8_t ROOK_PSQT[] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -5, -5,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,  5,  0,  0,  0,  0,  0,  0,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,  5,  0,  0,  0,  0,  0,  0,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,  5,  0,  0,  0,  0,  0,  0,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,  5,  0,  0,  0,  0,  0,  0,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,  5,  0,  0,  0,  0,  0,  0,  5,
     -5,  0,  0,  0,  0,  0,  0, -5, -5,-10,-10,-10,-10,-10,-10, -5,
      0,  0,  0,  5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

const int8_t QUEEN_PSQT[] = {
    -20,-10,-10, -5, -5,-10,-10,-20, 20, 10, 10,  5,  5, 10, 10, 20,
    -10,  0,  0,  0,  0,  0,  0,-10, 10,  0, -5,  0,  0,  0,  0, 10,
    -10,  0,  5,  5,  5,  5,  0,-10, 10, -5, -5, -5, -5, -5,  0, 10,
     -5,  0,  5,  5,  5,  5,  0, -5,  0,  0, -5, -5, -5, -5,  0,  5,
      0,  0,  5,  5,  5,  5,  0, -5,  5,  0, -5, -5, -5, -5,  0,  5,
    -10,  5,  5,  5,  5,  5,  0,-10, 10,  0, -5, -5, -5, -5,  0, 10,
    -10,  0,  5,  0,  0,  0,  0,-10, 10,  0,  0,  0,  0,  0,  0, 10,
    -20,-10,-10, -5, -5,-10,-10,-20, 20, 10, 10,  5,  5, 10, 10, 20,
};



const int8_t MG_KING_PSQT[] = {
    -30,-40,-40,-50,-50,-40,-40,-30,-20,-30,  0,  0,  0,  0,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,-20,-20,  0,  0,  0,  0,-20,-20,
    -30,-40,-40,-50,-50,-40,-40,-30, 10, 20, 20, 20, 20, 20, 20, 10,
    -30,-40,-40,-50,-50,-40,-40,-30, 20, 30, 30, 40, 40, 30, 30, 20,
    -20,-30,-30,-40,-40,-30,-30,-20, 30, 40, 40, 50, 50, 40, 40, 30,
    -10,-20,-20,-20,-20,-20,-20,-10, 30, 40, 40, 50, 50, 40, 40, 30,
     20, 20,  0,  0,  0,  0, 20, 20, 30, 40, 40, 50, 50, 40, 40, 30,
     20, 30, 10,  0,  0, 10, 30, 20, 30, 40, 40, 50, 50, 40, 40, 30,
};

const int8_t EG_KING_PSQT[] = {
    -50,-40,-30,-20,-20,-30,-40,-50, 50, 30, 30, 30, 30, 30, 30, 50,
    -30,-20,-10,  0,  0,-10,-20,-30, 30, 30,  0,  0,  0,  0, 30, 30,
    -30,-10, 20, 30, 30, 20,-10,-30, 30, 10,-20,-30,-30,-20, 10, 30,
    -30,-10, 30, 40, 40, 30,-10,-30, 30, 10,-30,-40,-40,-30, 10, 30,
    -30,-10, 30, 40, 40, 30,-10,-30, 30, 10,-30,-40,-40,-30, 10, 30,
    -30,-10, 20, 30, 30, 20,-10,-30, 30, 10,-20,-30,-30,-20, 10, 30,
    -30,-30,  0,  0,  0,  0,-30,-30, 30, 20, 10,  0,  0, 10, 20, 30,
    -50,-30,-30,-30,-30,-30,-30,-50, 50, 40, 30, 20, 20, 30, 40, 50,
};


const int8_t *PSQT_MG[] = {
    nullptr,
    PAWN_PSQT,
    PAWN_PSQT,
    KNIGHT_PSQT,
    MG_KING_PSQT,
    BISHOP_PSQT,
    ROOK_PSQT,
    QUEEN_PSQT,
};

const int8_t *PSQT_EG[] = {
    nullptr,
    PAWN_PSQT,
    PAWN_PSQT,
    KNIGHT_PSQT,
    EG_KING_PSQT,
    BISHOP_PSQT,
    ROOK_PSQT,
    QUEEN_PSQT,
};


int16_t evaluate()
{
    uint8_t phase = 0; //Game phase: lower means closer to the endgame (less pieces on board)

    //endgame detection
    for (uint8_t i = 1; i < 32; i++) //calculate the phase of the game
    {
        uint8_t sq = plist[i];
        if (sq != 0xFF) phase += game_phase[board[sq] & PTYPE]; //add to the game phase
    }

    phase = std::min(phase, (uint8_t)TOTAL_PHASE); //by promoting pawns to queens, the game phase could be higher than the total phase

    int16_t midgame_psqt = 0;
    int16_t endgame_psqt = 0;

    //piece_values
    for (uint8_t i = 0; i < 32; i++)
    {
        uint8_t sq = plist[i];
        if (sq == 0xFF) continue; //piece has been captured

        uint8_t piece = board[sq];
        if (!piece) continue;

        uint8_t psqt_index = sq ^ (piece & 16) * 7;
        int16_t perspective = (piece & 16) ? -1 : 1;

        midgame_psqt += perspective * (mg_piece_values[piece & 7] + PSQT_MG[piece & PTYPE][psqt_index]); //midgame material/PSQT
        endgame_psqt += perspective * (eg_piece_values[piece & 7] + PSQT_EG[piece & PTYPE][psqt_index]); //endgame material/PSQT
    }

    int16_t psqt_eval = (midgame_psqt * phase + endgame_psqt * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    return psqt_eval;
}
