//
// Created by sebastian on 15.11.23.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "chess_rules.h"
#include "common.h"

#define MAX_GAMES 100

static GameStatus* games[MAX_GAMES];

static GameStatus* init_game(char* gameId, Player* firstPlayer);

static bool check_pawn(int fromX, int fromY, int toX, int toY, int color, int board[8][8]);

static bool check_knight(int fromX, int fromY, int toX, int toY, int color, int board[8][8]);

static bool check_bishop(int fromX, int fromY, int toX, int toY, int color, int board[8][8]);

static bool check_rook(int fromX, int fromY, int toX, int toY, int color, int board[8][8]);

static bool check_queen(int fromX, int fromY, int toX, int toY, int color, int board[8][8]);

static bool check_king(int fromX, int fromY, int toX, int toY, int color, int board[8][8]);

static bool is_occupied_by_friendly(int fromX, int fromY, int toX, int toY, int color, int board[8][8]);

GameStatus* init_game(char* gameId, Player* firstPlayer) {
    int gameIndex = rand() % MAX_GAMES;
    int i = gameIndex;
    while (games[++i % MAX_GAMES] != NULL) {
        if (i == gameIndex) {
            printf("No free game slots\n");
            exit(1);
        }
    }

    GameStatus* gameStatus = malloc(sizeof(GameStatus));
    gameStatus->gameId = gameId;
    gameStatus->players[0] = firstPlayer;
    gameStatus->players[1] = NULL;
    gameStatus->currentTurn = WHITE;
    gameStatus->winner = -1;
    init_board(gameStatus->board);

    games[i % MAX_GAMES] = gameStatus;

    return games[i % MAX_GAMES];
}

bool check_pawn(int fromX, int fromY, int toX, int toY, int color, int board[8][8]) {
    if (color == BLACK) {
        if (fromY == 1 && toY == 3 && fromX == toX && board[2][fromX] == EMPTY && board[3][fromX] == EMPTY)
            return true;
        if (toY == fromY + 1 && fromX == toX && board[toY][toX] == EMPTY)
            return true;
        if (toY == fromY + 1 && (toX == fromX + 1 || toX == fromX - 1) && get_color(board[toY][toX]) == WHITE)
            return true;
    } else {
        if (fromY == 6 && toY == 4 && fromX == toX && board[5][fromX] == EMPTY && board[4][fromX] == EMPTY)
            return true;
        if (toY == fromY - 1 && fromX == toX && board[toY][toX] == EMPTY)
            return true;
        if (toY == fromY - 1 && (toX == fromX + 1 || toX == fromX - 1) && get_color(board[toY][toX]) == BLACK)
            return true;
    }

    return false;
}

bool check_knight(int fromX, int fromY, int toX, int toY, int color, int board[8][8]) {
    if (toX == fromX + 1 && toY == fromY + 2)
        return true;
    if (toX == fromX + 2 && toY == fromY + 1)
        return true;
    if (toX == fromX + 2 && toY == fromY - 1)
        return true;
    if (toX == fromX + 1 && toY == fromY - 2)
        return true;
    if (toX == fromX - 1 && toY == fromY - 2)
        return true;
    if (toX == fromX - 2 && toY == fromY - 1)
        return true;
    if (toX == fromX - 2 && toY == fromY + 1)
        return true;
    if (toX == fromX - 1 && toY == fromY + 2)
        return true;

    return false;
}

bool check_bishop(int fromX, int fromY, int toX, int toY, int color, int board[8][8]) {
    if (abs(toX - fromX) != abs(toY - fromY))
        return false;

    int dx = (toX - fromX) / abs(toX - fromX);
    int dy = (toY - fromY) / abs(toY - fromY);

    int x = fromX + dx;
    int y = fromY + dy;
    while (x != toX && y != toY) {
        if (board[y][x] != EMPTY)
            return false;

        x += dx;
        y += dy;
    }

    return true;
}

bool check_rook(int fromX, int fromY, int toX, int toY, int color, int board[8][8]) {
    if (fromX != toX && fromY != toY)
        return false;

    if (fromX == toX) {
        int dy = (toY - fromY) / abs(toY - fromY);
        int y = fromY + dy;
        while (y != toY) {
            if (board[y][fromX] != EMPTY)
                return false;

            y += dy;
        }
    } else {
        int dx = (toX - fromX) / abs(toX - fromX);
        int x = fromX + dx;
        while (x != toX) {
            if (board[fromY][x] != EMPTY)
                return false;

            x += dx;
        }
    }

    return true;
}

bool check_queen(int fromX, int fromY, int toX, int toY, int color, int board[8][8]) {
    return check_bishop(fromX, fromY, toX, toY, color, board) || check_rook(fromX, fromY, toX, toY, color, board);
}

bool check_king(int fromX, int fromY, int toX, int toY, int color, int board[8][8]) {
    if (abs(toX - fromX) > 1 || abs(toY - fromY) > 1)
        return false;

    return true;
}

bool is_occupied_by_friendly(int fromX, int fromY, int toX, int toY, int color, int board[8][8]) {
    if (board[toY][toX] == EMPTY)
        return false;

    return get_color(board[toY][toX]) == color;
}


GameStatus* create_or_join_game(char* gameId) {
    GameStatus* gameStatus = find_game(gameId);

    // game exists, add a player
    if (gameStatus != NULL && gameStatus->players[1] == NULL) {
        Player* p = malloc(sizeof(Player));
        assert(gameStatus->players[0]->color == WHITE);
        bzero(p->playerId, 6);
        sprintf(p->playerId, "%d", rand() % 1024);
        p->color = BLACK;
        p->disconnected = false;
        gameStatus->players[1] = p;
        return gameStatus;
    }

    // game exists, but is full
    if (gameStatus != NULL) {
        return nullptr;
    }

    //game does not exist, create a new one
    Player* p = malloc(sizeof(Player));
    bzero(p->playerId, 6);
    sprintf(p->playerId, "%d", rand() % 1024);
    p->color = WHITE;
    p->disconnected = false;
    return init_game(gameId, p);
}

GameStatus* find_game(char* gameId) {
    int gameIndex = -1;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] != NULL && strcmp(games[i]->gameId, gameId) == 0) {
            gameIndex = i;
            break;
        }
    }
    if (gameIndex == -1)
        return nullptr;

    return games[gameIndex];
}

Player* find_player(GameStatus* gameStatus, char* playerId) {
    if (strcmp(gameStatus->players[0]->playerId, playerId) == 0)
        return gameStatus->players[0];
    if (strcmp(gameStatus->players[1]->playerId, playerId) == 0)
        return gameStatus->players[1];
    return nullptr;
}

bool is_move_valid(GameStatus* gameStatus, int fromX, int fromY, int toX, int toY) {
    int piece = gameStatus->board[fromY][fromX];
    int color = get_color(piece);
    if (color != gameStatus->currentTurn)
        return false;

    int pieceType = get_piece_type(piece);

    if (is_occupied_by_friendly(fromX, fromY, toX, toY, color, gameStatus->board))
        return false;

    switch (pieceType) {
        case PAWN:
            return check_pawn(fromX, fromY, toX, toY, color, gameStatus->board);
        case KNIGHT:
            return check_knight(fromX, fromY, toX, toY, color, gameStatus->board);
        case BISHOP:
            return check_bishop(fromX, fromY, toX, toY, color, gameStatus->board);
        case ROOK:
            return check_rook(fromX, fromY, toX, toY, color, gameStatus->board);
        case QUEEN:
            return check_queen(fromX, fromY, toX, toY, color, gameStatus->board);
        case KING:
            return check_king(fromX, fromY, toX, toY, color, gameStatus->board);
        default:
            return false;
    }
}

void serialize_board(cJSON* root, int gameBoard[8][8]) {
    cJSON* board = cJSON_CreateArray();
    for (int i = 0; i < 8; i++) {
        cJSON* row = cJSON_CreateArray();
        for (int j = 0; j < 8; j++) {
            cJSON_AddItemToArray(row, cJSON_CreateNumber(gameBoard[i][j]));
        }
        cJSON_AddItemToArray(board, row);
    }

    cJSON_AddItemToObject(root, "board", board);
}

void free_game(GameStatus* gameStatus) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] == gameStatus) {
            games[i] = NULL;
            break;
        }
    }

    free(gameStatus->players[0]);
    free(gameStatus->players[1]);
    free(gameStatus->gameId);
    free(gameStatus);
}
