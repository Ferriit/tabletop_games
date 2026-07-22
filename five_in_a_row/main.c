#include <stdio.h>
#include <stdlib.h>

#define BOARD_HEIGHT 25
#define BOARD_WIDTH 25

enum SYMBOL {
    CROSS, CIRCLE, NONE
};

typedef struct {
    int x, y;
} pos;

int board[BOARD_WIDTH][BOARD_HEIGHT];

int score = 0;

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
                    return piece == CIRCLE ? 1 : -1;
                }
            }
        }
    }

    return 0;
}

void render() {
    printf("\x1b[H\x1b[2J\x1b[0m%d\n", score);

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int i = 0; i < 2; i++) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                enum SYMBOL sym = board[x][y];

                if ((y + x) % 2) {
                    printf("\x1b[48;5;232m\x1b[38;5;237m");
                } else {
                    printf("\x1b[48;5;237m\x1b[38;5;232m");
                }

                if (sym == CROSS) {
                    switch (i) {
                        case 0: printf("\x1b[38;5;99m\\__/"); break;
                        case 1: printf("\x1b[38;5;99m/  \\"); break;
                    }
                } else if (sym == CIRCLE) {
                    switch (i) {
                        case 0: printf("\x1b[38;5;208m/‾‾\\"); break;
                        case 1: printf("\x1b[38;5;208m\\__/"); break;
                    }
                } else {
                    if (i == 0) {
                        printf("%c%c  ", int_to_id(x), int_to_id(y));
                    } else {
                        printf("    ");
                    }
                }
            }
            printf("\x1b[0m\n");
        }
    }
}

int evaluate_window(int board[BOARD_WIDTH][BOARD_HEIGHT], int x, int y, int dx, int dy) {
    int cross = 0;
    int circle = 0;
    int empty = 0;

    for (int i = 0; i < 5; i++) {
        int nx = x + dx * i;
        int ny = y + dy * i;

        if (nx < 0 || nx >= BOARD_WIDTH ||
            ny < 0 || ny >= BOARD_HEIGHT)
            return 0;

        if (board[nx][ny] == CROSS)
            cross++;
        else if (board[nx][ny] == CIRCLE)
            circle++;
        else
            empty++;
    }

    if (cross && circle)
        return 0;

    // Winning
    if (circle == 5)
        return 10000000;

    if (cross == 5)
        return -10000000;

        // Blocking opponent
    if (cross == 4 && empty == 1)
        return -1000000;

    // Winning opportunity
    if (circle == 4 && empty == 1)
        return 1000000;


    // Strong threats
    if (circle == 3 && empty == 2)
        return 50000;

    if (cross == 3 && empty == 2)
        return -50000;


    if (circle == 2 && empty == 3)
        return 5000;

    if (cross == 2 && empty == 3)
        return -5000;


    return 0;
}

int evaluate_connections(int board[BOARD_WIDTH][BOARD_HEIGHT]) {
    int score = 0;

    int directions[8][2] = {
        {1,0}, {-1,0},
        {0,1}, {0,-1},
        {1,1}, {-1,-1},
        {1,-1}, {-1,1}
    };

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[x][y] == NONE)
                continue;

            int friends = 0;

            for (int d = 0; d < 8; d++) {
                int nx = x + directions[d][0];
                int ny = y + directions[d][1];

                if (nx >= 0 && nx < BOARD_WIDTH &&
                    ny >= 0 && ny < BOARD_HEIGHT &&
                    board[nx][ny] == board[x][y]) {
                    friends++;
                }
            }

            if (board[x][y] == CIRCLE)
                score += friends * 50;
            else
                score -= friends * 50;
        }
    }

    return score;
}

int evaluate(int board[BOARD_WIDTH][BOARD_HEIGHT]) {
    int score = 0;

    int directions[4][2] = {
        {1,0},
        {0,1},
        {1,1},
        {1,-1}
    };

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            for (int d = 0; d < 4; d++) {
                score += evaluate_window(
                    board, x, y,
                    directions[d][0],
                    directions[d][1]
                );
            }
        }
    }

    score += evaluate_connections(board);

    return score;
}

int minimax(int board[BOARD_WIDTH][BOARD_HEIGHT], int depth, int turn, int alpha, int beta, pos* best_move) {
    int winning = is_winning(board);

    if (winning != 0) {
        return winning * 10000000 + depth;
    }

    if (depth == 0)
        return evaluate(board);

    pos moves[BOARD_WIDTH * BOARD_HEIGHT];
    int move_count = get_valid_moves(board, moves);

    if (move_count == 0)
        return 0;

    int best_score;
    if (turn == CIRCLE)
        best_score = -100000000; // maximize
    else
        best_score = 100000000;  // minimize

    pos best = {-1, -1};

    for (int i = 0; i < move_count; i++) {
        int x = moves[i].x;
        int y = moves[i].y;

        // Make move
        board[x][y] = turn;

        int score = minimax(board, depth - 1,
                            turn == CIRCLE ? CROSS : CIRCLE,
                            alpha, beta, NULL);

        // Undo move
        board[x][y] = NONE;


        if (turn == CIRCLE) {
            // AI tries to maximize score
            if (score > best_score) {
                best_score = score;
                best = moves[i];
            }

            if (best_score > alpha)
                alpha = best_score;

        } else {
            // Player tries to minimize score
            if (score < best_score) {
                best_score = score;
                best = moves[i];
            }

            if (best_score < beta)
                beta = best_score;
        }

        // Alpha-beta pruning
        if (beta <= alpha)
            break;
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
        printf("Turn: ");
        while (1) {
            printf("\x1b[0m");
            scanf("%2s", inbuf);
            if (board[id_to_int(inbuf[0])][id_to_int(inbuf[1])] == NONE) {
                break;
            }
        }
        board[id_to_int(inbuf[0])][id_to_int(inbuf[1])] = CROSS;

        render();
        if (is_winning(board) != 0)
            return 0;

        pos move;
        score = minimax(board, 3, CIRCLE, -100000000, 100000000, &move);

        board[move.x][move.y] = CIRCLE;

        render();
        if (is_winning(board) != 0)
            return 0;
    }

    return 0;
}
