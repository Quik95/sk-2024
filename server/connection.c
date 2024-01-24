#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <threads.h>
#include "connection.h"
#include "common.h"
#include "game.h"
#include "chess_rules.h"

static char HTTP_HEADER[] = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n";
#define MAX_MESSAGE_LENGTH 2048

enum MESSAGE_TYPE_OUT {
    WAIT_FOR_OTHER_PLAYER = 0,
    GAME_STARTED,
    GAME_STATE_RESPONSE,
    MOVE_ACCEPTED,
    GAME_ENDED,
    PLAYER_DISCONNECTED,
    OPPONENT_DISCONNECTED
};

enum MESSAGE_TYPE_IN {
    JOIN_GAME = 128,
    GAME_STATE_REQUEST,
    MOVE_PIECE,
    DISCONNECT,
    EXIT_SERVER = 42069
};

static ssize_t readfull_header(int descriptor, char* buffer, int sizetoread);

static int extract_content_length(char* buffer);

static char* readfull_body(int descriptor, char* buffer, int sizetoread);

static void write_http_response(int conn_fd, char* response);

static int extract_string(cJSON* root, char* key, char* value);

static cJSON* prepare_json(int messageType, char* gameId, char* playerId);

static void handle_join_game(int conn_fd, cJSON* message);

static void handle_sync_state(int conn_fd, cJSON* root);

static void handle_move_piece(int conn_fd, cJSON* root);
static void send_game_ended(int conn_fd, GameStatus* g);
static void handle_disconnect(int conn_fd, cJSON* root);
static void send_opponent_disconnected(int conn_fd, GameStatus* g);

ssize_t readfull_header(int descriptor, char* buffer, int sizetoread) {
    ssize_t offset = 0;
    while (strstr(buffer, "\r\n\r\n") == NULL || offset == sizetoread) {
        ssize_t read = recv(descriptor, buffer + offset, sizetoread - offset, MSG_PEEK);
        if (read < 1) {
            return offset;
        }
        offset += read;
    }
    int header_length = strstr(buffer, "\r\n\r\n") - buffer + 4;
    recv(descriptor, buffer, header_length, 0);
    return offset;
}

int extract_content_length(char* buffer) {
    char* body_length_str = strstr(buffer, "Content-Length: ");
    char* body_length_end = strstr(body_length_str, "\r\n");
    body_length_str += strlen("Content-Length: ");
    int body_length = strtol(body_length_str, &body_length_end, 10);

    return body_length;
}

char* readfull_body(int descriptor, char* buffer, int sizetoread) {
    ssize_t offset = 0;
    while (offset < sizetoread) {
        ssize_t read = recv(descriptor, buffer + offset, sizetoread - offset, 0);
        if (read < 1) {
            return buffer;
        }
        offset += read;
    }
    return buffer;
}

void handle_join_game(int conn_fd, cJSON* message) {
    char* game_id = malloc(sizeof(char) * 6);
    if (extract_string(message, "game_id", game_id) < 0) {
        printf("error parsing game_id\n");
        exit(1);
    }

    GameStatus* g = create_or_join_game(game_id);
    if (g == NULL) {
        printf("Attempted to join a full game\n");
        return;
    }

    cJSON* resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "gameId", game_id);

    char id[6];
    if (g->players[1] == NULL) {
        cJSON_AddNumberToObject(resp, "messageType", WAIT_FOR_OTHER_PLAYER);
        sprintf(id, "%s", g->players[0]->playerId);
        cJSON_AddStringToObject(resp, "playerId", id);
        cJSON_AddNumberToObject(resp, "playerColor", g->players[0]->color);
    } else {
        cJSON_AddNumberToObject(resp, "messageType", GAME_STARTED);
        sprintf(id, "%s", g->players[1]->playerId);
        cJSON_AddStringToObject(resp, "playerId", id);
        cJSON_AddNumberToObject(resp, "playerColor", g->players[1]->color);
    }

    char* marshalled = cJSON_Print(resp);
    write_http_response(conn_fd, marshalled);

    cJSON_Delete(resp);
    cJSON_free(marshalled);
}

void write_http_response(int conn_fd, char* response) {
    char buffer[150] = {0};
    sprintf(buffer, HTTP_HEADER, strlen(response));
    send(conn_fd, buffer, strlen(buffer), 0);
    send(conn_fd, response, strlen(response), 0);
}

void handle_sync_state(int conn_fd, cJSON* root) {
    char game_id[6] = {0};
    cJSON* game_id_json = cJSON_GetObjectItem(root, "gameId");
    if (game_id_json == NULL) {
        printf("error parsing game_id\n");
        exit(1);
    }
    strcpy(game_id, game_id_json->valuestring);

    char player_id[6] = {0};
    cJSON* player_id_json = cJSON_GetObjectItem(root, "playerId");
    if (player_id_json == NULL) {
        printf("error parsing player_id\n");
        exit(1);
    }
    strcpy(player_id, player_id_json->valuestring);

    GameStatus* g = find_game(game_id);
    if (g == NULL) {
        printf("Attempted to check status of a non-existent game\n");
        return;
    }

    if (g->winner != -1) {
        send_game_ended(conn_fd, g);
        return;
    }

    Player* p = find_player(g, player_id);
    p->lastHeartbeat = time(NULL);

    Player* other = get_the_other_player(g, p);
    if (other != NULL && other->disconnected) {
        send_opponent_disconnected(conn_fd, g);
        printf("Discovered disconnected opponent, sending message\n");
        return;
    }

    if (g->players[0] == nullptr || g->players[1] == nullptr) {
        cJSON* resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "messageType", WAIT_FOR_OTHER_PLAYER);
        cJSON_AddStringToObject(resp, "gameId", g->gameId);
        cJSON_AddStringToObject(resp, "playerId", player_id);
        char* marshalled = cJSON_Print(resp);
        write_http_response(conn_fd, marshalled);

        cJSON_Delete(resp);
        cJSON_free(marshalled);

        return;
    }

    cJSON* resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "messageType", GAME_STATE_RESPONSE);
    cJSON_AddStringToObject(resp, "gameId", g->gameId);
    cJSON_AddStringToObject(resp, "playerId", player_id);
    cJSON_AddNumberToObject(resp, "currentTurn", g->currentTurn);

    cJSON_AddNumberToObject(resp, "playerColor", p->color);

    serialize_board(resp, g->board);

    char* marshalled = cJSON_Print(resp);
    write_http_response(conn_fd, marshalled);

    cJSON_Delete(resp);
    cJSON_free(marshalled);
}

void handle_move_piece(int conn_fd, cJSON* root) {
    char game_id[6] = {0};
    if (extract_string(root, "gameId", game_id) < 0) {
        printf("error parsing game_id\n");
        return;
    }
    GameStatus* g = find_game(game_id);
    if (g == nullptr) {
        printf("game not found\n");
        return;
    }

    char player_id[6] = {0};
    if (extract_string(root, "playerId", player_id) < 0) {
        printf("error parsing playerId\n");
        return;
    }

    Player* p = find_player(g, player_id);
    p->lastHeartbeat = time(NULL);
    if (p->color != g->currentTurn) {
        printf("wrong player turn\n");
        return;
    }

    cJSON* move = cJSON_GetObjectItem(root, "move");
    cJSON* from = cJSON_GetObjectItem(move, "from");
    cJSON* to = cJSON_GetObjectItem(move, "to");

    int from_x = cJSON_GetArrayItem(from, 0)->valueint;
    int from_y = cJSON_GetArrayItem(from, 1)->valueint;

    int to_x = cJSON_GetArrayItem(to, 0)->valueint;
    int to_y = cJSON_GetArrayItem(to, 1)->valueint;

    if (!is_move_valid(g, from_x, from_y, to_x, to_y)) {
        cJSON *resp = prepare_json(GAME_STATE_RESPONSE, game_id, player_id);

        serialize_board(resp, g->board);
        cJSON_AddNumberToObject(resp, "playerColor", p->color);
        cJSON_AddNumberToObject(resp, "currentTurn", g->currentTurn);

        char* marshalled = cJSON_Print(resp);
        write_http_response(conn_fd, marshalled);

        cJSON_Delete(resp);
        cJSON_free(marshalled);

        return;
    }

    int piece = g->board[from_y][from_x];
    int targetPiece = g->board[to_y][to_x];
    g->board[from_y][from_x] = EMPTY;
    g->board[to_y][to_x] = piece;
    g->currentTurn = !g->currentTurn;

    if (get_piece_type(targetPiece) == KING) {
        g->winner = p->color;
        send_game_ended(conn_fd, g);
        return;
    }

    cJSON* resp = prepare_json(MOVE_ACCEPTED, game_id, player_id);
    char* marshalled = cJSON_Print(resp);
    write_http_response(conn_fd, marshalled);

    cJSON_Delete(resp);
    cJSON_free(marshalled);
}

int extract_string(cJSON* root, char* key, char* value) {
    cJSON* json = cJSON_GetObjectItem(root, key);
    if (json == NULL) {
        printf("error parsing %s\n", key);
        return -1;
    }
    strcpy(value, json->valuestring);
    return 1;
}

cJSON* prepare_json(int messageType, char* gameId, char* playerId) {
    cJSON* resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "messageType", messageType);
    cJSON_AddStringToObject(resp, "gameId", gameId);
    cJSON_AddStringToObject(resp, "playerId", playerId);

    return resp;
}

void send_game_ended(int conn_fd, GameStatus* g) {
    cJSON* resp = prepare_json(GAME_ENDED, g->gameId, g->players[0]->playerId);
    cJSON_AddNumberToObject(resp, "winner", g->winner);
    serialize_board(resp, g->board);
    char* marshalled = cJSON_Print(resp);
    write_http_response(conn_fd, marshalled);

    cJSON_Delete(resp);
    cJSON_free(marshalled);
}

void handle_disconnect(int conn_fd, cJSON* root) {
    char game_id[6] = {0};
    if (extract_string(root, "gameId", game_id) < 0) {
        printf("error parsing game_id\n");
        return;
    }
    GameStatus* g = find_game(game_id);
    if (g == nullptr) {
        printf("game not found\n");
        return;
    }

    char player_id[6] = {0};
    if (extract_string(root, "playerId", player_id) < 0) {
        printf("error parsing playerId\n");
        return;
    }

    Player* p = find_player(g, player_id);
    p->disconnected = true;

    if (g->players[0]->disconnected && g->players[1]->disconnected) {
        printf("Both players disconnected, deleting game\n");
        free_game(g);
    }

    cJSON* resp = prepare_json(PLAYER_DISCONNECTED, game_id, player_id);
    char* marshalled = cJSON_Print(resp);
    write_http_response(conn_fd, marshalled);

    cJSON_Delete(resp);
    cJSON_free(marshalled);
}

void send_opponent_disconnected(int conn_fd, GameStatus* g) {
    cJSON* resp = prepare_json(OPPONENT_DISCONNECTED, g->gameId, g->players[0]->playerId);
    char* marshalled = cJSON_Print(resp);
    write_http_response(conn_fd, marshalled);

    cJSON_Delete(resp);
    cJSON_free(marshalled);
}

void handle_connection(int conn_fd) {
    char buffer[MAX_MESSAGE_LENGTH] = {0};
    printf("Accepted connection.\n");

    int n = readfull_header(conn_fd, buffer, MAX_MESSAGE_LENGTH);

    if (n < 0) {
        perror("receive has failed");
        close(conn_fd);
        return;
    }

    int body_length = extract_content_length(buffer);
    printf("Body length: %d\n", body_length);

    bzero(buffer, MAX_MESSAGE_LENGTH);
    char* request_body = readfull_body(conn_fd, buffer, body_length);
    printf("HTTP request body: %s\n", request_body);

    cJSON* root = cJSON_Parse(request_body);
    if (root == NULL) {
        printf("error parsing JSON body\n");
        exit(1);
    }

    cJSON* message_type = cJSON_GetObjectItem(root, "messageType");
    if (message_type == NULL) {
        printf("malformed message: %s\n", request_body);
        exit(1);
    }

    switch (message_type->valueint) {
        case JOIN_GAME:
            handle_join_game(conn_fd, root);
            break;
        case GAME_STATE_REQUEST:
            handle_sync_state(conn_fd, root);
            break;
        case MOVE_PIECE:
            handle_move_piece(conn_fd, root);
            break;
        case DISCONNECT:
            handle_disconnect(conn_fd, root);
            break;
        case EXIT_SERVER:
            printf("Exiting server\n");
            exit(0);
        default:
            printf("unknown message type: %d\n", message_type->valueint);
            exit(1);
    }

    mark_disconnected_players();

    cJSON_Delete(root);

    shutdown(conn_fd, SHUT_RDWR);
}
