#include <stdio.h>

#define true 1
#define false 0

int board[8][8];

enum SYM {
    WHITE_MAN,
    BLACK_MAN,
    WHITE_KING,
    BLACK_KING,
    NONE
};

enum COL {
    WHITE,
    BLACK
};

typedef struct {
    int start_x, start_y, end_x, end_y;
} move;

int abs(int n) {
    if (n < 0)
        return -n;
    return n;
}

move input_to_move(char input[5]) {
    int start_x = input[0] - 65;
    int start_y = 56 - input[1];
    int end_x = input[2] - 65;
    int end_y = 56 - input[3];

    return (move){.start_x=start_x, .start_y=start_y, .end_x=end_x, .end_y=end_y};
}

void populate_board() {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            board[x][y] = NONE;
        }
    }
    
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (((x + y) % 2) && (y > 4)) {
                board[x][y] = WHITE_MAN;
            }
            else if (((x + y) % 2) && (y < 3)) {
                board[x][y] = BLACK_MAN;
            }
        }
    }
}

int is_move_valid(int start_x, int start_y, int end_x, int end_y, int color) {
    int dx = end_x - start_x;
    int dy = end_y - start_y;

    int piece = board[start_x][start_y];
    int is_king = (piece == WHITE_KING || piece == BLACK_KING);

    // Normal move
    if (abs(dx) == 1) {
        if (is_king)
            return abs(dy) == 1;

        switch (color) {
            case WHITE:
                return dy == -1;

            case BLACK:
                return dy == 1;
        }
    }

    // Capture
    if (abs(dx) == 2) {
        int middle_x = start_x + dx / 2;
        int middle_y = start_y + dy / 2;

        int opponent = NONE;

        if (color == WHITE) {
            opponent = (board[middle_x][middle_y] == BLACK_MAN ||
                        board[middle_x][middle_y] == BLACK_KING);
        } else {
            opponent = (board[middle_x][middle_y] == WHITE_MAN ||
                        board[middle_x][middle_y] == WHITE_KING);
        }

        if (!opponent)
            return false;

        if (is_king)
            return abs(dy) == 2;

        switch (color) {
            case WHITE:
                return dy == -2;

            case BLACK:
                return dy == 2;
        }
    }

    return false;
}

void render() {
    const char* background_colors[] = {
        "\x1b[48;5;235m", "\x1b[48;5;232m"
    };

    const char* symbols[] = {
        "\x1b[37m● ", "\x1b[37m○ ", "\x1b[37m◉ ", "\x1b[30m◉ ", "\x1b[39m  "
    };

    printf("\x1b[H\x1b[2J");

    for (int y = 0; y < 8; y++) {
        printf("\x1b[0m%d ", 8 - y);

        for (int x = 0; x < 8; x++) {
            printf("%s", background_colors[(x + y) % 2]);
            printf("%s", symbols[board[x][y]]);
        }
        printf("\n");
    }
    printf("\x1b[0m  A B C D E F G H\n");
}

void apply_move(move m, int color) {
    int dx = m.end_x - m.start_x;
    int dy = m.end_y - m.start_y;

    if (abs(dx) == 1) {
        board[m.end_x][m.end_y] = board[m.start_x][m.start_y];
        board[m.start_x][m.start_y] = NONE;
    } else if (abs(dx) == 2) {
        board[m.end_x][m.end_y] = board[m.start_x][m.start_y];
        board[m.start_x][m.start_y] = NONE;
        board[m.end_x - dx / 2][m.end_y - dy / 2] = NONE;
    }

    if ((color == WHITE) && (m.end_y == 0)) {
        board[m.end_x][m.end_y] = WHITE_KING;
    } else if ((color == BLACK) && (m.end_y == 7)) {
        board[m.end_x][m.end_y] = BLACK_KING;
    }
}

int main() {
    populate_board();
    render();

    int turn = 0;

    while (1) {
        char input[5];
    get_input:
        printf("Move: ");
        scanf("%4s", input);

        move m = input_to_move(input);

        if (!is_move_valid(m.start_x, m.start_y, m.end_x, m.end_y, turn % 2)) 
            goto get_input;

        apply_move(m, turn % 2);

        render();

        turn++;
    }
}
