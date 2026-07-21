#include <stdio.h>

#ifdef _WIN32
#include <windows.h>

void sleep_ms(int milliseconds) {
    Sleep(milliseconds);
}

#else
#include <time.h>

void sleep_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    
    nanosleep(&ts, NULL);
}

#endif

enum SYM {
    P1,
    P2,
    NONE
};

int board[7][6];

void render() {
    printf("\x1b[H\x1b[2J");
    for (int i = 0; i < 7; i++) {
        printf(" %2d  ", i+1);
    }

    printf("\n");

    for (int y = 0; y < 6; y++) {
        printf("|");
        for (int x = 0; x < 7; x++) {
            if (board[x][5 - y] == P1) {
                printf("\x1b[31m ██ \x1b[0m|");
            } else if (board[x][5 - y] == P2) {
                printf("\x1b[33m ██ \x1b[0m|");
            } else {
                printf("\x1b[0m    |");
            }
        }
        printf("\n+");

        for (int i = 0; i < 7; i++) {
            printf("----+");
        }
        printf("\n");
    }
}

void step_down() {
    for (int y = 5; y > 0; y--) {
        for (int x = 0; x < 7; x++) {
            if (board[x][y - 1] == NONE) {
                board[x][y - 1] = board[x][y];
                board[x][y] = NONE;
            }
        }
        sleep_ms(100);
        render();
    }
}

int place_piece(enum SYM color, int position) {
    if (position > 6)
        return 1;
    if (position < 0)
        return 1;

    if (board[position][5] != NONE)
        return 1;

    board[position][5] = color;

    render();
    step_down();

    return 0;
}

int is_winning(int board[7][6]) {
    int directions[4][2] = {
        {1, 0},
        {0, 1},
        {1, 1},
        {1, -1}
    };

    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 7; x++) {
            int color = board[x][y];

            if (color == NONE)
                continue;

            for (int d = 0; d < 4; d++) {
                int dx = directions[d][0];
                int dy = directions[d][1];

                int count = 0;

                for (int i = 0; i < 4; i++) {
                    int nx = x + dx * i;
                    int ny = y + dy * i;

                    if (nx < 0 || nx >= 7 ||
                        ny < 0 || ny >= 6)
                        break;

                    if (board[nx][ny] != color)
                        break;

                    count++;
                }
                if (count == 4) {
                    return color == P2 ? 1 : -1;
                }
            }
        }
    }

    return 0;
}

int get_valid_moves(int board[7][6], int moves[7]) {
    int count = 0;

    for (int x = 0; x < 7; x++) {
        if (board[x][5] == NONE) {
            moves[count++] = x;
        }
    }

    return count;
}


int evaluate_window(int p2, int p1, int empty) {
    int score = 0;

    // AI winning threats
    if (p2 == 4)
        score += 100000;

    else if (p2 == 3 && empty == 1)
        score += 1000;

    else if (p2 == 2 && empty == 2)
        score += 100;


    // Human threats
    if (p1 == 4)
        score -= 100000;

    else if (p1 == 3 && empty == 1)
        score -= 1000;

    else if (p1 == 2 && empty == 2)
        score -= 100;

    return score;
}


int evaluate(int board[7][6]) {
    int score = 0;

    // Prefer center column
    for (int y = 0; y < 6; y++) {
        if (board[3][y] == P2)
            score += 50;
        else if (board[3][y] == P1)
            score -= 50;
    }


    int directions[4][2] = {
        {1, 0},
        {0, 1},
        {1, 1},
        {1, -1}
    };

    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 7; x++) {
            for (int d = 0; d < 4; d++) {
                int p2 = 0;
                int p1 = 0;
                int empty = 0;

                for (int i = 0; i < 4; i++) {
                    int nx = x + directions[d][0] * i;
                    int ny = y + directions[d][1] * i;

                    if (nx < 0 || nx >= 7 ||
                        ny < 0 || ny >= 6)
                        break;

                    if (board[nx][ny] == P2)
                        p2++;
                    else if (board[nx][ny] == P1)
                        p1++;
                    else
                        empty++;
                }

                // Ignore mixed windows
                if (p1 == 0 || p2 == 0)
                    score += evaluate_window(p2, p1, empty);
            }
        }
    }

    return score;
}


int minimax(int board[7][6], int depth, int turn, int alpha, int beta, int* best_move) {
    int winning = is_winning(board);

    if (winning != 0) {
        return winning * 10000000 + depth;
    }

    if (depth == 0)
        return evaluate(board);

    int moves[7];
    int move_count = get_valid_moves(board, moves);

    if (move_count == 0)
        return 0;

    int best_score;
    int move = -1;

    if (turn == P2) {
        // AI maximizes
        best_score = -100000000;

        for (int i = 0; i < move_count; i++) {
            int col = moves[i];

            // Find lowest empty row
            int row = 0;
            while (board[col][row] != NONE)
                row++;

            board[col][row] = P2;

            int score = minimax(board, depth - 1, P1, alpha, beta, NULL);

            board[col][row] = NONE;

            if (score > best_score) {
                best_score = score;
                move = col;
            }

            if (best_score > alpha)
                alpha = best_score;

            if (beta <= alpha)
                break;
        }

    } else {
        // Human minimizes
        best_score = 100000000;

        for (int i = 0; i < move_count; i++) {
            int col = moves[i];

            int row = 0;
            while (board[col][row] != NONE)
                row++;

            board[col][row] = P1;

            int score = minimax(board, depth - 1, P2, alpha, beta, NULL);

            board[col][row] = NONE;

            if (score < best_score) {
                best_score = score;
                move = col;
            }

            if (best_score < beta)
                beta = best_score;

            if (beta <= alpha)
                break;
        }
    }

    if (best_move != NULL)
        *best_move = move;

    return best_score;
}

int main() {
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 7; x++) {
            board[x][y] = NONE;
        }
    }

    render();
    while (1) {
        char input;
    input_label:
        scanf("%c", &input);

        if (place_piece(P1, input - 49) != 0)
            goto input_label;

        if (is_winning(board) != 0)
            return 0;

        int move;
        minimax(board, 8, P2, -100000000, 100000000, &move);
        place_piece(P2, move);

        if (is_winning(board) != 0)
            return 0;
    }

    return 0;
}

