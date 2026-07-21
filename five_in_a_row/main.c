#include <stdio.h>
#include <stdlib.h>

#define BOARD_HEIGHT 50
#define BOARD_WIDTH 50

enum SYMBOL {
    CROSS, CIRCLE, NONE
};

typedef struct {
    int x, y;
} pos;

int board[BOARD_WIDTH][BOARD_HEIGHT];

int get_valid_moves(int board[BOARD_WIDTH][BOARD_HEIGHT], pos positions[BOARD_WIDTH * BOARD_HEIGHT]) {
    int count = 0;

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[x][y] != NONE)
                continue;

            int nearby = 0;

            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < BOARD_WIDTH &&
                        ny >= 0 && ny < BOARD_HEIGHT &&
                        board[nx][ny] != NONE) {
                        nearby = 1;
                    }
                }
            }

            if (nearby)
                positions[count++] = (pos){x, y};
        }
    }

    return count;
}

char int_to_id(int i) {
    if (i < 10) {
        return i + 48;
    }
    else if (i < 36) {
        return i + 55;
    }
    return i + 61;
}

int id_to_int(char id) {
    if (id < 65) {
        return id - 48;
    }
    else if (id < 96) {
        return id - 55;
    }
    return id  - 61;
}

int is_winning(int board[BOARD_WIDTH][BOARD_HEIGHT]) {
    int directions[4][2] = {
        {1, 0},
        {0, 1},
        {1, 1},
        {1, -1}
    };

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int piece = board[x][y];

            if (piece == NONE)
                continue;

            for (int d = 0; d < 4; d++) {
                int dx = directions[d][0];
                int dy = directions[d][1];

                int count = 0;

                for (int i = 0; i < 5; i++) {
                    int nx = x + dx * i;
                    int ny = y + dy * i;

                    if (nx < 0 || nx >= BOARD_WIDTH ||
                        ny < 0 || ny >= BOARD_HEIGHT)
                        break;

                    if (board[nx][ny] != piece)
                        break;

                    count++;
                }

                if (count == 5) {
                    return piece == CROSS ? 1 : -1;
                }
            }
        }
    }

    return 0;
}

void render() {
    printf("\x1b[H\x1b[2J");

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        printf("\x1b[0m%c ", int_to_id(y));

        for (int x = 0; x < BOARD_WIDTH; x++) {
            enum SYMBOL sym = board[x][y];

            if ((y + x) % 2) {
                printf("\x1b[48;5;232m");
            } else {
                printf("\x1b[48;5;237m");
            }

            if (sym == CROSS) {
                printf("\x1b[38;5;99mX ");
            } else if (sym == CIRCLE) {
                printf("\x1b[38;5;208mO ");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }

    printf("\x1b[0m   ");
    for (int i = 0; i < BOARD_WIDTH; i++) {
        if (i % 2) {
            printf("\x1b[40;37m%c ", int_to_id(i));
        } else {
            printf("\x1b[47;30m%c ", int_to_id(i));
        }
    }
    printf("\n");
}

int minimax(int board[BOARD_WIDTH][BOARD_HEIGHT], int depth, int turn, pos* best_move) {
    int winning = is_winning(board);

    if (winning != 0)
        return winning;

    if (depth == 0)
        return 0;

    pos moves[BOARD_WIDTH * BOARD_HEIGHT];
    int move_count = get_valid_moves(board, moves);

    int best_score = turn == 0 ? -1000000 : 1000000;
    pos best = {-1, -1};

    for (int i = 0; i < move_count; i++) {
        int temp_board[BOARD_WIDTH][BOARD_HEIGHT];

        // Copy board
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                temp_board[x][y] = board[x][y];
            }
        }

        // Make move
        temp_board[moves[i].x][moves[i].y] = turn ? CIRCLE : CROSS;

        // Recurse
        int score = minimax(temp_board, depth - 1, !turn, NULL);

        if (turn == 0) {
            // CROSS maximizes
            if (score > best_score) {
                best_score = score;
                best = moves[i];
            }
        } else {
            // CIRCLE minimizes
            if (score < best_score) {
                best_score = score;
                best = moves[i];
            }
        }
    }

    if (best_move != NULL)
        *best_move = best;

    return best_score;
}

int main() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[x][y] = NONE;
        }
    }

    render();

    char inbuf[3] = "\0\0\0";

    while (1) {
        while (1) {
            printf("\x1b[0m");
            scanf("%2s", inbuf);
            if (board[id_to_int(inbuf[0])][id_to_int(inbuf[1])] == NONE) {
                break;
            }
        }
        board[id_to_int(inbuf[0])][id_to_int(inbuf[1])] = CROSS;

        render();

        pos move;
        int score = minimax(board, 5, CIRCLE, &move);

        board[move.x][move.y] = CIRCLE;

        render();
    }

    return 0;
}
