//
// Created by sebastian on 12.11.23.
//

#ifndef SERVER_CHESS_RULES_H
#define SERVER_CHESS_RULES_H

enum Piece {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};

#define WHITE 0
#define BLACK 1

void init_board(int board[8][8]);

void set_black(int* piece);

void set_white(int* piece);

int get_color(int piece);
int get_piece_type(int piece);

#endif //SERVER_CHESS_RULES_H
