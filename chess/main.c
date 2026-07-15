#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define MAX_HISTORY 1024
#define MAX_MOVES 256
#define INF 9999999

#define true 1
#define false 0

#define WHITE 1
#define BLACK 0

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

enum PIECES {
    BLACK_PAWN = 1,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING,

    WHITE_PAWN,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING
};

enum TYPES {
    PAWN = 1,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

typedef struct {
    int start_file, start_rank;
    int end_file, end_rank;
    int piece;
    int promotion;

    int castle;
    int en_passant;
} move;

typedef struct {
    move m;
    int score;
} scored_move;

typedef struct {
    int file, rank;
} position;

typedef struct {
    int board[8][8];
    int white_king_file;
    int white_king_rank;
    int black_king_file;
    int black_king_rank;
    int turn;
} position_history;

position_history history[MAX_HISTORY];
int history_count = 0;

// 0,0 = A1, 0,1 = B1 (file, rank)
int board[8][8];

// controlled[0] for black, controlled[1] for white
int controlled[2][8][8];
position king_positions[2];

int white_king_moved = false;
int black_king_moved = false;

int white_rook_a_moved = false;
int white_rook_h_moved = false;

int black_rook_a_moved = false;
int black_rook_h_moved = false;

move last_move;

int turn = WHITE;
int move_count = 0;

int check_only_attacking = false;

int get_type(int piece) {
    if (piece > BLACK_KING) {
        return piece - BLACK_KING;
    }
    return piece;
}

int get_color(int piece) {
    if (piece < WHITE_PAWN) {
        return BLACK;
    }
    return WHITE;
}

int abs(int a) {
    if (a < 0) {
        return -a;
    }
    return a;
}

void copy_board(int dest[8][8], int src[8][8]) {
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            dest[file][rank] = src[file][rank];
        }
    }
}

void copy_kings(position dest[2], position src[2]){
    dest[0] = src[0];
    dest[1] = src[1];
}

void update_king_position(move m) {
    int piece = board[m.end_file][m.end_rank];

    if (piece == WHITE_KING) {
        king_positions[WHITE].file = m.end_file;
        king_positions[WHITE].rank = m.end_rank;
    }
    else if (piece == BLACK_KING) {
        king_positions[BLACK].file = m.end_file;
        king_positions[BLACK].rank = m.end_rank;
    }
}

void add_move(move moves[], int *count, int sf, int sr, int ef, int er, int piece) {
    moves[*count].start_file = sf;
    moves[*count].start_rank = sr;
    moves[*count].end_file = ef;
    moves[*count].end_rank = er;
    moves[*count].piece = piece;
    moves[*count].promotion = 0;
    moves[*count].castle = false;
    moves[*count].en_passant = false;

    (*count)++;
}

void get_controlled_squares();

int can_castle(int color, int kingside) {
    if (color == WHITE) {
        if (white_king_moved)
            return false;

        if (kingside) {
            if (white_rook_h_moved)
                return false;

            if (board[5][0] || board[6][0])
                return false;

            if (controlled[BLACK][4][0] ||
                controlled[BLACK][5][0] ||
                controlled[BLACK][6][0])
                return false;

            return board[7][0] == WHITE_ROOK;
        }
        else {
            if (white_rook_a_moved)
                return false;

            if (board[1][0] || board[2][0] || board[3][0])
                return false;

            if (controlled[BLACK][4][0] ||
                controlled[BLACK][3][0] ||
                controlled[BLACK][2][0])
                return false;

            return board[0][0] == WHITE_ROOK;
        }
    }

    // Black
    if (black_king_moved)
        return false;

    if (kingside) {
        if (black_rook_h_moved)
            return false;

        // Squares between e8 and h8
        if (board[5][7] || board[6][7])
            return false;

        // Cannot castle through check
        if (controlled[WHITE][4][7] ||
            controlled[WHITE][5][7] ||
            controlled[WHITE][6][7])
            return false;

        return board[7][7] == BLACK_ROOK;
    }
    else {
        if (black_rook_a_moved)
            return false;

        // Squares between e8 and a8
        if (board[1][7] || board[2][7] || board[3][7])
            return false;

        // Cannot castle through check
        if (controlled[WHITE][4][7] ||
            controlled[WHITE][3][7] ||
            controlled[WHITE][2][7])
            return false;

        return board[0][7] == BLACK_ROOK;
    }
}

void save_position() {
    if (history_count >= MAX_HISTORY)
        return;

    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            history[history_count].board[file][rank] = board[file][rank];
        }
    }

    history[history_count].white_king_file = king_positions[WHITE].file;
    history[history_count].white_king_rank = king_positions[WHITE].rank;

    history[history_count].black_king_file = king_positions[BLACK].file;
    history[history_count].black_king_rank = king_positions[BLACK].rank;

    history[history_count].turn = turn;

    history_count++;
}

void populate_board() {
    board[0][0] = WHITE_ROOK;
    board[1][0] = WHITE_KNIGHT;
    board[2][0] = WHITE_BISHOP;
    board[3][0] = WHITE_QUEEN;
    board[4][0] = WHITE_KING;
    board[5][0] = WHITE_BISHOP;
    board[6][0] = WHITE_KNIGHT;
    board[7][0] = WHITE_ROOK;

    for (size_t i = 0; i < 8; i++) {
        board[i][1] = WHITE_PAWN;
    }

    board[0][7] = BLACK_ROOK;
    board[1][7] = BLACK_KNIGHT;
    board[2][7] = BLACK_BISHOP;
    board[3][7] = BLACK_QUEEN;
    board[4][7] = BLACK_KING;
    board[5][7] = BLACK_BISHOP;
    board[6][7] = BLACK_KNIGHT;
    board[7][7] = BLACK_ROOK;

    for (size_t i = 0; i < 8; i++) {
        board[i][6] = BLACK_PAWN;
    }

    king_positions[WHITE].file = 4;
    king_positions[WHITE].rank = 0;

    king_positions[BLACK].file = 4;
    king_positions[BLACK].rank = 7;
}

void promote(char promotion_choice, int file, int rank) {
    int promotion = BLACK_QUEEN;

    switch (promotion_choice) {
        case 'Q': promotion = BLACK_QUEEN; break;
        case 'R': promotion = BLACK_ROOK; break;
        case 'B': promotion = BLACK_BISHOP; break;
        case 'N': promotion = BLACK_KNIGHT; break;
        default: promotion = BLACK_QUEEN; break;
    }

    if (board[file][rank] >= WHITE_PAWN) {
        promotion += BLACK_KING;
    }

    board[file][rank] = promotion;
}

int is_move_valid(int start_file, int start_rank, int end_file, int end_rank);

int is_white_pawn_move(int start_file, int start_rank, int end_file, int end_rank) {
    // Capture
    if (((start_rank == (end_rank - 1)) &&
            ((start_file == (end_file - 1)) ||
            (start_file == (end_file + 1)))) &&
            (board[end_file][end_rank] != 0)) {
        return true;
    }
    
    if (check_only_attacking) {
        return false;
    }

    // Normal move
    if (((start_rank == (end_rank - 1)) && (start_file == end_file)) && (board[end_file][end_rank] == 0)) {
        return true;
    }
    // Two-square pawn advance
    else if ((start_rank == 1) &&
        (end_rank == start_rank + 2) &&
        (start_file == end_file) &&
        (board[start_file][start_rank + 1] == 0) &&
        (board[end_file][end_rank] == 0)) {
        return true;
    }

    // En Passant
    else if (board[end_file][end_rank] == 0 &&
        end_rank == start_rank + 1 &&
        (end_file == start_file - 1 || end_file == start_file + 1) &&

        last_move.piece == BLACK_PAWN &&
        last_move.start_rank == 6 &&
        last_move.end_rank == 4 &&
        last_move.end_file == end_file &&
        last_move.end_rank == start_rank) {
        // Remove the captured pawn.
        board[last_move.end_file][last_move.end_rank] = 0;

        return true;
    }

    return false;
}

int is_black_pawn_move(int start_file, int start_rank, int end_file, int end_rank) {
    // Capture
    if (((start_rank == (end_rank + 1)) &&
             ((start_file == (end_file - 1)) ||
              (start_file == (end_file + 1)))) &&
             (board[end_file][end_rank] != 0)) {
        return true;
    }
    
    if (check_only_attacking) {
        return false;
    }

    // Normal move
    if (((start_rank == (end_rank + 1)) && (start_file == end_file)) &&
        (board[end_file][end_rank] == 0)) {
        return true;
    }

    // Two-square pawn advance
    else if ((start_rank == 6) &&
        (end_rank == start_rank - 2) &&
        (start_file == end_file) &&
        (board[start_file][start_rank - 1] == 0) &&
        (board[end_file][end_rank] == 0)) {
        return true;
    }

    // En Passant
    else if (board[end_file][end_rank] == 0 &&
             end_rank == start_rank - 1 &&
             (end_file == start_file - 1 || end_file == start_file + 1) &&

             last_move.piece == WHITE_PAWN &&
             last_move.start_rank == 1 &&
             last_move.end_rank == 3 &&
             last_move.end_file == end_file &&
             last_move.end_rank == start_rank) {
        // Remove the captured pawn.
        board[last_move.end_file][last_move.end_rank] = 0;

        return true;
    }

    return false;
}

int is_rook_move(int start_file, int start_rank, int end_file, int end_rank) {
    if (start_file == end_file && start_rank == end_rank) {
        return false;
    }

    // Must move horizontally or vertically
    if (start_file != end_file && start_rank != end_rank) {
        return false;
    }

    // Vertical movement
    if (start_file == end_file) {
        int direction = (end_rank > start_rank) ? 1 : -1;

        for (int rank = start_rank + direction;
             rank != end_rank;
             rank += direction) {

            if (board[start_file][rank] != 0) {
                return false;
            }
        }
    }

    // Horizontal movement
    else {
        int direction = (end_file > start_file) ? 1 : -1;

        for (int file = start_file + direction;
             file != end_file;
             file += direction) {

            if (board[file][start_rank] != 0) {
                return false;
            }
        }
    }

    return true;
}

int is_bishop_move(int start_file, int start_rank, int end_file, int end_rank) {
    // Cannot stay in place
    if (start_file == end_file && start_rank == end_rank) {
        return false;
    }

    int file_diff = end_file - start_file;
    int rank_diff = end_rank - start_rank;

    // Must move diagonally
    if (abs(file_diff) != abs(rank_diff)) {
        return false;
    }

    int file_direction = (file_diff > 0) ? 1 : -1;
    int rank_direction = (rank_diff > 0) ? 1 : -1;

    int file = start_file + file_direction;
    int rank = start_rank + rank_direction;

    // Check squares between start and destination
    while (file != end_file && rank != end_rank) {
        if (board[file][rank] != 0) {
            return false;
        }

        file += file_direction;
        rank += rank_direction;
    }

    return true;
}

int is_knight_move(int start_file, int start_rank, int end_file, int end_rank) {
    int file_diff = abs(end_file - start_file);
    int rank_diff = abs(end_rank - start_rank);

    return (file_diff == 2 && rank_diff == 1) || (file_diff == 1 && rank_diff == 2);
}

void get_controlled_squares() {
    check_only_attacking = true;

    for (int color = 0; color < 2; color++) {
        for (int file = 0; file < 8; file++) {
            for (int rank = 0; rank < 8; rank++) {
                controlled[color][file][rank] = false;
            }
        }
    }

    for (int start_file = 0; start_file < 8; start_file++) {
        for (int start_rank = 0; start_rank < 8; start_rank++) {

            if (board[start_file][start_rank] == 0) {
                continue;
            }

            int color = get_color(board[start_file][start_rank]);

            for (int end_file = 0; end_file < 8; end_file++) {
                for (int end_rank = 0; end_rank < 8; end_rank++) {

                    if (is_move_valid(start_file, start_rank,
                                      end_file, end_rank) &&
                        !(end_file == start_file && end_rank == start_rank)) {

                        controlled[color][end_file][end_rank] = true;
                    }
                }
            }
        }
    }

    check_only_attacking = false;
}

int is_king_move(int start_file, int start_rank, int end_file, int end_rank, int color) {
    int file_diff = abs(end_file - start_file);
    int rank_diff = abs(end_rank - start_rank);

    // Normal king move
    if (file_diff <= 1 && rank_diff <= 1) {
        return true;
    }

    // Castling
    if (rank_diff == 0 && file_diff == 2) {
        if (color == WHITE && start_file == 4 && start_rank == 0) {
            return can_castle(color, end_file > start_file);
        }

        if (color == BLACK && start_file == 4 && start_rank == 7) {
            return can_castle(color, end_file > start_file);
        }
    }

    return false;
}

int is_move_valid(int start_file, int start_rank, int end_file, int end_rank) {
    int piece = board[start_file][start_rank];

    if (piece == 0) {
        return false;
    }

    // Cannot capture your own pieces
    if (board[end_file][end_rank] != 0 &&
        get_color(board[end_file][end_rank]) == get_color(piece)) {
        return false;
    }

    int type = get_type(piece);

    if (piece == WHITE_PAWN) {
        return is_white_pawn_move(start_file, start_rank, end_file, end_rank);
    }
    else if (piece == BLACK_PAWN) {
        return is_black_pawn_move(start_file, start_rank, end_file, end_rank);
    }
    else if (type == ROOK) {
        return is_rook_move(start_file, start_rank, end_file, end_rank);
    }
    else if (type == BISHOP) {
        return is_bishop_move(start_file, start_rank, end_file, end_rank);
    }
    else if (type == QUEEN) {
        return is_bishop_move(start_file, start_rank, end_file, end_rank) ||
               is_rook_move(start_file, start_rank, end_file, end_rank);
    }
    else if (type == KNIGHT) {
        return is_knight_move(start_file, start_rank, end_file, end_rank);
    }
    else if (type == KING) {
        return is_king_move(start_file, start_rank, end_file, end_rank, get_color(piece));
    }

    return false;
}

// ENGINE CODE
int piece_values[] = {
    0,      // empty
    100,    // pawn
    320,    // knight
    330,    // bishop
    500,    // rook
    900,    // queen
    20000   // king
};

int knight_table[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

int center_bonus(int file, int rank) {
    int bonus = 0;

    if (file >= 3 && file <= 4 &&
        rank >= 3 && rank <= 4) {
        bonus += 20;
    }

    return bonus;
}

int generate_moves(int color, move moves[]) {
    int count = 0;

    for (int start_file = 0; start_file < 8; start_file++) {
        for (int start_rank = 0; start_rank < 8; start_rank++) {

            int piece = board[start_file][start_rank];

            if (piece == 0)
                continue;

            if (get_color(piece) != color)
                continue;


            for (int end_file = 0; end_file < 8; end_file++) {
                for (int end_rank = 0; end_rank < 8; end_rank++) {
                    if (!is_move_valid(start_file, start_rank, end_file, end_rank))
                        continue;


                    // Promotions
                    if (piece == WHITE_PAWN && end_rank == 7) {
                        int choices[] = {
                            WHITE_QUEEN,
                            WHITE_ROOK,
                            WHITE_BISHOP,
                            WHITE_KNIGHT
                        };

                        for (int i = 0; i < 4; i++) {
                            add_move(moves, &count, start_file, start_rank, end_file, end_rank, piece);

                            moves[count - 1].promotion = choices[i];
                        }
                    }

                    else if (piece == BLACK_PAWN && end_rank == 0) {
                        int choices[] = {
                            BLACK_QUEEN,
                            BLACK_ROOK,
                            BLACK_BISHOP,
                            BLACK_KNIGHT
                        };

                        for (int i = 0; i < 4; i++) {
                            add_move(moves, &count, start_file, start_rank, end_file, end_rank, piece);

                            moves[count - 1].promotion = choices[i];
                        }
                    }

                    else {
                        add_move(moves, &count, start_file, start_rank, end_file, end_rank, piece);
                    }
                }
            }


            // Castling
            if (piece == WHITE_KING &&
                start_file == 4 &&
                start_rank == 0) {

                if (can_castle(WHITE, true)) {
                    add_move(moves, &count, 4, 0, 6, 0, piece);
                    moves[count - 1].castle = true;
                }

                if (can_castle(WHITE, false)) {
                    add_move(moves, &count, 4, 0, 2, 0, piece);
                    moves[count - 1].castle = true;
                }
            }


            if (piece == BLACK_KING &&
                start_file == 4 &&
                start_rank == 7) {

                if (can_castle(BLACK, true)) {
                    add_move(moves, &count, 4, 7, 6, 7, piece);
                    moves[count - 1].castle = true;
                }

                if (can_castle(BLACK, false)) {
                    add_move(moves, &count, 4, 7, 2, 7, piece);
                    moves[count - 1].castle = true;
                }
            }
        }
    }

    return count;
}

int count_legal_moves(int color) {
    move moves[256];

    return generate_moves(color, moves);
}

int is_repetition() {
    int repeats = 0;

    for (int h = 0; h < history_count; h++) {
        if (history[h].turn != turn)
            continue;

        int same = true;

        for (int file = 0; file < 8; file++) {
            for (int rank = 0; rank < 8; rank++) {
                if (history[h].board[file][rank] != board[file][rank]) {
                    same = false;
                    break;
                }
            }
            if (!same)
                break;
        }
        if (same) {
            repeats++;
            if (repeats >= 2)
                return true;
        }
    }
    return false;
}

int evaluate() {
    int score = 0;

    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {

            int piece = board[file][rank];

            if (piece == 0)
                continue;

            int type = get_type(piece);
            int color = get_color(piece);

            int value = piece_values[type];

            // White pieces add, black pieces subtract
            if (color == WHITE) {
                score += value;
            } else {
                score -= value;
            }


            // Piece-square bonuses
            int positional = 0;

            if (type == KNIGHT) {
                positional = knight_table[file][rank];
            }

            positional += center_bonus(file, rank);


            if (color == WHITE) {
                score += positional;
            } else {
                score -= positional;
            }


            // Pawn advancement
            if (type == PAWN) {
                if (color == WHITE) {
                    score += rank * 10;
                } else {
                    score -= (7 - rank) * 10;
                }
            }
        }
    }


    // Mobility
    int old_turn = turn;

    turn = WHITE;
    int white_moves = count_legal_moves(WHITE);

    turn = BLACK;
    int black_moves = count_legal_moves(BLACK);

    turn = old_turn;

    score += (white_moves - black_moves) * 5;


    // King safety
    if (controlled[BLACK][king_positions[WHITE].file][king_positions[WHITE].rank]) {
        score -= 50;
    }

    if (controlled[WHITE][king_positions[BLACK].file][king_positions[BLACK].rank]) {
        score += 50;
    }

    if (is_repetition()) {
        return (turn == WHITE) ? -500 : 500;
    }

    score += (white_moves - black_moves) * 5;

    return score;
}


void apply_move(move m) {
    int temp_white_king_moved = white_king_moved;
    int temp_black_king_moved = black_king_moved;

    int temp_white_rook_a_moved = white_rook_a_moved;
    int temp_white_rook_h_moved = white_rook_h_moved;

    int temp_black_rook_a_moved = black_rook_a_moved;
    int temp_black_rook_h_moved = black_rook_h_moved;

    int piece = board[m.start_file][m.start_rank];

    // Promotion
    if (m.promotion != 0)
        piece = m.promotion;

    board[m.end_file][m.end_rank] = piece;
    board[m.start_file][m.start_rank] = 0;

    // Update king location
    if (get_type(piece) == KING) {
        king_positions[get_color(piece)].file = m.end_file;
        king_positions[get_color(piece)].rank = m.end_rank;
    }

    // Castle rook movement
    if (m.castle) {
        // Kingside
        if (m.end_file == 6) {
            board[5][m.start_rank] =
                board[7][m.start_rank];

            board[7][m.start_rank] = 0;
        }
        // Queenside
        else if (m.end_file == 2) {
            board[3][m.start_rank] =
                board[0][m.start_rank];

            board[0][m.start_rank] = 0;
        }
    }
    white_king_moved = temp_white_king_moved;
    black_king_moved = temp_black_king_moved;

    white_rook_a_moved = temp_white_rook_a_moved;
    white_rook_h_moved = temp_white_rook_h_moved;

    black_rook_a_moved = temp_black_rook_a_moved;
    black_rook_h_moved = temp_black_rook_h_moved;
}

int minimax_score(int depth, int color, int alpha, int beta) {
    if (depth == 0) {
        return evaluate();
    }

    move moves[256];
    int move_count = generate_moves(color, moves);

    if (move_count == 0) {
        if (controlled[!color][king_positions[color].file][king_positions[color].rank]) {
            return (color == WHITE) ? -INF : INF;
        }

        return 0; // stalemate
    }

    if (color == WHITE) {
        int best = -INF;

        for (int i = 0; i < move_count; i++) {
            int temp_board[8][8];
            position temp_positions[2];

            copy_board(temp_board, board);
            copy_kings(temp_positions, king_positions);

            board[moves[i].end_file][moves[i].end_rank] =
                board[moves[i].start_file][moves[i].start_rank];

            board[moves[i].start_file][moves[i].start_rank] = 0;

            update_king_position(moves[i]);

            int score = minimax_score(depth - 1, BLACK, alpha, beta);

            copy_board(board, temp_board);
            copy_kings(king_positions, temp_positions);

            if (score > best)
                best = score;

            if (best > alpha)
                alpha = best;

            // Beta cutoff
            if (beta <= alpha)
                break;
        }

        return best;
    }

    else {
        int best = INF;

        for (int i = 0; i < move_count; i++) {
            int temp_board[8][8];
            position temp_positions[2];

            copy_board(temp_board, board);
            copy_kings(temp_positions, king_positions);

            board[moves[i].end_file][moves[i].end_rank] =
                board[moves[i].start_file][moves[i].start_rank];

            board[moves[i].start_file][moves[i].start_rank] = 0;

            update_king_position(moves[i]);

            int score = minimax_score(depth - 1, WHITE, alpha, beta);

            copy_board(board, temp_board);
            copy_kings(king_positions, temp_positions);

            if (score < best)
                best = score;

            if (best < beta)
                beta = best;

            // Alpha cutoff
            if (beta <= alpha)
                break;
        }

        return best;
    }
}


move minimax(int depth, int color, int difficulty) {
    move moves[256];
    int move_count = generate_moves(color, moves);

    move best_moves[256];
    int best_scores[256];

    int count = 0;

    int best_score = (color == WHITE) ? -INF : INF;

    for (int i = 0; i < move_count; i++) {
        int temp_board[8][8];
        position temp_positions[2];

        copy_board(temp_board, board);
        copy_kings(temp_positions, king_positions);

        apply_move(moves[i]);

        int score = minimax_score(depth - 1, !color, -INF, INF);

        copy_board(board, temp_board);
        copy_kings(king_positions, temp_positions);

        if ((color == WHITE && score > best_score) ||
            (color == BLACK && score < best_score)) {

            best_score = score;
            count = 0;

            best_moves[count] = moves[i];
            best_scores[count] = score;
            count++;
        }
        else if (score == best_score) {
            best_moves[count] = moves[i];
            best_scores[count] = score;
            count++;
        }
    }

    // Difficulty: choose from worse moves occasionally
    int choice_range = difficulty + 1;

    if (choice_range > count)
        choice_range = count;


    return best_moves[rand() % choice_range];
}

move get_engine_move(int color) {
    const static move openings[] = {
        {.start_file=4, .start_rank=1, .end_file=4, .end_rank=3}, // e4
        {.start_file=4, .start_rank=1, .end_file=4, .end_rank=3}, // make e4 more common
        {.start_file=4, .start_rank=1, .end_file=4, .end_rank=3}, // make e4 more common
        {.start_file=3, .start_rank=1, .end_file=3, .end_rank=3}, // d4
        {.start_file=4, .start_rank=1, .end_file=4, .end_rank=2}, // e3
        {.start_file=3, .start_rank=1, .end_file=3, .end_rank=2}, // d3

        // Knight openings
        {.start_file=6, .start_rank=0, .end_file=5, .end_rank=2}, // Nf3
        {.start_file=1, .start_rank=0, .end_file=2, .end_rank=2}, // Nc3
    };

    if (move_count == 0 && color == WHITE) {
        return openings[rand() % ARRAY_SIZE(openings)];
    }

    return minimax(4, color, 0);
}

// GAME CODE

int move_piece(char start_file, char start_rank, char end_file, char end_rank, char promotion_choice) {
    start_file &= ~0x20;
    end_file &= ~0x20;
    promotion_choice &= ~0x20;

    start_file -= 'A';
    end_file -= 'A';

    start_rank -= '1';
    end_rank -= '1';

    if (start_file < 0 || start_file > 7 ||
        end_file < 0 || end_file > 7 ||
        start_rank < 0 || start_rank > 7 ||
        end_rank < 0 || end_rank > 7) {
        return false;
    }

    int piece = board[start_file][start_rank];
    int color = get_color(piece);

    if (color != turn) {
        return false;
    }

    if (!is_move_valid(start_file, start_rank, end_file, end_rank)) {
        return false;
    }

    // Save old state in case the move exposes the king
    int old_piece = board[end_file][end_rank];
    position old_king = king_positions[color];

    // Detect castling
    int castling = false;
    if (get_type(piece) == KING &&
        start_rank == end_rank &&
        abs(end_file - start_file) == 2) {

        castling = true;

        // Move rook too
        if (end_file > start_file) { // Kingside
            board[5][start_rank] = board[7][start_rank];
            board[7][start_rank] = 0;
        }
        else { // Queenside
            board[3][start_rank] = board[0][start_rank];
            board[0][start_rank] = 0;
        }
    }

    // Move the piece
    board[end_file][end_rank] = piece;
    board[start_file][start_rank] = 0;

    if (get_type(piece) == KING) {
        king_positions[color].file = end_file;
        king_positions[color].rank = end_rank;
    }

    get_controlled_squares();

    // Illegal if king is in check
    if (controlled[!color][king_positions[color].file][king_positions[color].rank]) {
        board[start_file][start_rank] = piece;
        board[end_file][end_rank] = old_piece;

        if (castling) {
            if (end_file > start_file) { // Kingside
                board[7][start_rank] = board[5][start_rank];
                board[5][start_rank] = 0;
            }
            else { // Queenside
                board[0][start_rank] = board[3][start_rank];
                board[3][start_rank] = 0;
            }
        }

        king_positions[color] = old_king;
        return false;
    }

    // Update castling rights
    if (piece == WHITE_KING)
        white_king_moved = true;

    if (piece == BLACK_KING)
        black_king_moved = true;

    if (piece == WHITE_ROOK) {
        if (start_file == 0 && start_rank == 0)
            white_rook_a_moved = true;
        if (start_file == 7 && start_rank == 0)
            white_rook_h_moved = true;
    }

    if (piece == BLACK_ROOK) {
        if (start_file == 0 && start_rank == 7)
            black_rook_a_moved = true;
        if (start_file == 7 && start_rank == 7)
            black_rook_h_moved = true;
    }

    // Promotion
    if ((board[end_file][end_rank] == BLACK_PAWN && end_rank == 0) ||
        (board[end_file][end_rank] == WHITE_PAWN && end_rank == 7)) {
        promote(promotion_choice, end_file, end_rank);
    }

    last_move.start_file = start_file;
    last_move.start_rank = start_rank;
    last_move.end_file = end_file;
    last_move.end_rank = end_rank;
    last_move.piece = board[end_file][end_rank];
    last_move.promotion = promotion_choice;

    turn = !turn;
    save_position();
    return true;
}

void render(int start_file, int start_rank, int end_file, int end_rank) {
    // Reversed since most terminals are dark mode
    const char* piece_chars[] = {
        "   ",
        " ♟ ",
        " ♞ ",
        " ♝ ",
        " ♜ ",
        " ♛ ",
        " ♚ ",
        " ♟ ",
        " ♞ ",
        " ♝ ",
        " ♜ ",
        " ♛ ",
        " ♚ "
    };
    printf("\x1b[H\x1b[2J");

    int score = evaluate();

    printf("%d\n", score);

    for (int rank = 0; rank < 8; rank++) {
        printf("%d ", 8 - rank);

        for (int file = 0; file < 8; file++) {
            if ((file + rank) % 2) {
                printf("\x1b[48;5;238m");
            } else {
                printf("\x1b[48;5;245m");
            }

            // QUICK CHECK
            /*
            if (controlled[0][file][rank]) {
                printf("\x1b[0m\x1b[48;5;166m");
            }
            if (controlled[1][file][rank]) {
                printf("\x1b[0m\x1b[48;5;147m");
            }
            if (controlled[1][file][rank] && controlled[0][file][rank]) {
                printf("\x1b[0m\x1b[48;5;118m");
            }
            */

            if ((((7 - rank) == start_rank) && (file == start_file)) || (((7 - rank) == end_rank) && (file == end_file))) {
                printf("\x1b[0m\x1b[48;5;214m");
            }

            if (board[file][7 - rank] < WHITE_PAWN) {
                printf("\x1b[38;5;232m");
            } else {
                printf("\x1b[38;5;255m");
            }

            printf("%s", piece_chars[board[file][7 - rank]]);
        }
        printf("\x1b[0m\n");
    }

    printf("   A  B  C  D  E  F  G  H\n");
}

int main(int argc, char** argv) {
    srand(time(NULL));

    last_move.end_file = 0;
    last_move.end_rank = 0;
    last_move.start_file = 0;
    last_move.start_rank = 0;
    last_move.piece = 0;
    last_move.promotion = 0;

    int player_black = false;

    if (argc > 1) {
        player_black = !strcmp(argv[1], "-b") || !strcmp(argv[1], "--black");
    }
    
    populate_board();
    
    render(-1, -1, -1, -1);

    char input[6];

    if (!player_black) {
        while (1) {
            white_player_input:
            if (scanf("%5s", input) != 1)
                break;
            int valid = move_piece(input[0], input[1], input[2], input[3], input[4]);
            if (!valid) goto white_player_input;

            get_controlled_squares();

            render(-1, -1, -1, -1);

            move m = get_engine_move(BLACK);
            move_piece(m.start_file + 'A', m.start_rank + '1', m.end_file + 'A', m.end_rank + '1', m.promotion);

            render(m.start_file, m.start_rank, m.end_file, m.end_rank);
            printf("%c%c%c%c\n", m.start_file + 'A', m.start_rank + '1', m.end_file + 'A', m.end_rank + '1');
            move_count++;
        }
    }
    else {
        while (1) {
            get_controlled_squares();

            move m = get_engine_move(WHITE);
            move_piece(m.start_file + 'A', m.start_rank + '1', m.end_file + 'A', m.end_rank + '1', m.promotion);

            render(m.start_file, m.start_rank, m.end_file, m.end_rank);
            printf("%c%c%c%c\n", m.start_file + 'A', m.start_rank + '1', m.end_file + 'A', m.end_rank + '1');

            black_player_input:
            if (scanf("%5s", input) != 1)
                break;

            int valid = move_piece(input[0], input[1], input[2], input[3], input[4]);
            if (!valid) goto black_player_input;

            render(-1, -1, -1, -1);
            move_count++;
        }
    }

    return 0;
}
