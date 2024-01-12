//
// Created by sebastian on 15.11.23.
//

#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <cjson/cJSON.h>

typedef struct {
    char playerId[6];
    int color;
    bool disconnected;
} Player;

typedef struct {
    char* gameId;
    Player* players[2];
    int currentTurn;
    int board[8][8];
    int winner;
} GameStatus;

GameStatus* create_or_join_game(char* gameId);

GameStatus* find_game(char* gameId);

Player* find_player(GameStatus* gameStatus, char* playerId);
bool is_move_valid(GameStatus* gameStatus, int fromX, int fromY, int toX, int toY);
void serialize_board(cJSON* root, int gameBoard[8][8]);
void free_game(GameStatus* gameStatus);

#endif //SERVER_GAME_H
