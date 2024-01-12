//
// Created by sebastian on 12.11.23.
//

#include "chess_rules.h"

void init_board(int board[8][8]) {
    board[0][0] = ROOK;
    board[0][1] = KNIGHT;
    board[0][2] = BISHOP;
    board[0][3] = QUEEN;
    board[0][4] = KING;
    board[0][5] = BISHOP;
    board[0][6] = KNIGHT;
    board[0][7] = ROOK;

    for (int i = 0; i < 8; i++) {
        set_black(&board[0][i]);
    }

    for (int i = 0; i < 8; i++) {
        board[1][i] = PAWN;
        set_black(&board[1][i]);

        board[6][i] = PAWN;
        set_white(&board[6][i]);
    }

    for (int i = 2; i < 6; i++) {
        for (int j = 0; j < 8; j++) {
            board[i][j] = EMPTY;
        }
    }

    board[7][0] = ROOK;
    board[7][1] = KNIGHT;
    board[7][2] = BISHOP;
    board[7][3] = QUEEN;
    board[7][4] = KING;
    board[7][5] = BISHOP;
    board[7][6] = KNIGHT;
    board[7][7] = ROOK;

    for (int i = 0; i < 8; i++) {
        set_white(&board[7][i]);
    }
}

void set_black(int* piece) {
    *piece |= 0x8;
}

void set_white(int* piece) {
    *piece &= 0x7;
}

int get_color(int piece) {
    return piece >> 3 & 0b1;
}

int get_piece_type(int piece) {
    return piece & 0x7;
}
