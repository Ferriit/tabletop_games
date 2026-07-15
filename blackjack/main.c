#include "lib.h"

enum suit {
    CLUB,
    SPADE,
    HEART,
    DIAMOND
};

enum card {
    ACE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    TEN,
    JACK,
    QUEEN,
    KING
};

typedef struct {
    int low, high;
} value;

int played_cards[52];
int card_count = 0;

int global_dealer_points = 0;
int global_player_points = 0;

char c_dealer_points[4];
char c_player_points[4];

char player_score[32];

void render(const char *player, const char *dealer,
            const char *player_score, const char *dealer_score) {

    print("\x1b[H\x1b[2J");

    print("PLAYER: ");
    print(player);
    print("\n        ");
    print(player_score);
    print("\n\n");

    print("DEALER: ");
    print(dealer);
    print("\n        ");
    print(dealer_score);
    print("\n\n");

    if (global_player_points > global_dealer_points) {
        print(c_player_points);
        print(" - ");
        print(c_dealer_points);
        print(" TO PLAYER\n");
    }
    else if (global_dealer_points > global_player_points) {
        print(c_dealer_points);
        print(" - ");
        print(c_player_points);
        print(" TO DEALER\n");
    }
    else {
        print(c_dealer_points);
        print(" - ");
        print(c_player_points);
        print("\n");
    }

    print("> Hit or Stand (h/s) (e to exit): ");
}

void get_card(char buf[32], enum card card_num, enum suit card_suit) {
    static const char *cards[] = {
        "A \x1b[0m", "2 \x1b[0m", "3 \x1b[0m", "4 \x1b[0m", "5 \x1b[0m", "6 \x1b[0m",
        "7 \x1b[0m", "8 \x1b[0m", "9 \x1b[0m", "10 \x1b[0m",
        "J \x1b[0m", "Q \x1b[0m", "K \x1b[0m"
    };

    static const char *suits[] = {
        "\x1b[39m♠", "\x1b[31m♥", "\x1b[39m♣", "\x1b[31m♦"
    };

    int i = 0;

    const char *s = suits[card_suit];
    const char *c = cards[card_num];

    while (*s) buf[i++] = *s++;
    buf[i++] = ' ';
    while (*c) buf[i++] = *c++;

    buf[i] = 0;
}

value get_value(enum card card_num) {
    if (card_num == ACE)
        return (value){1, 11};

    if (card_num >= JACK)
        return (value){10, 10};

    int v = card_num + 1;
    return (value){v, v};
}

int contains(int arr[], int target) {
    for (int i = 0; i < 52; i++) {
        if (arr[i] == target)
            return 1;
    }
    return 0;
}

void deal_card(char *text, value *v, char *buf) {
    int id;
    int card;
    enum suit s;

    do {
        card = rand() % 13;
        s = rand() % 4;
        id = card + s * 13;
    } while (contains(played_cards, id));

    played_cards[card_count++] = id;

    get_card(buf, card, s);
    c_str_append(text, buf);

    value c = get_value((enum card)card);
    v->low += c.low;
    v->high += c.high;
}

int dealer(char *player_text, value v, char* dealer_text) {
    sleep_us(100 * 1000);

    char buf[32];

    while (1) {
        deal_card(dealer_text, &v, buf);

        int best = (v.high <= 21) ? v.high : v.low;

        char dealer_score[32];
        c_int_to_str(best, dealer_score);

        render(player_text, dealer_text, player_score, dealer_score);

        if (best > 21) {
            print("\nDealer Bust\n");
            global_player_points++;
            return 0;
        }

        if (best >= 17) {
            return best;
        }

        sleep_us(100 * 1000);
    }
}

int game_round() {
    srand(fast_seed());

    char buf[32];

    char player_text[128];
    player_text[0] = 0;

    char dealer_text[128];
    dealer_text[0] = 0;

    value player_v = {0, 0};
    value dealer_v = {0, 0};

    for (int i = 0; i < 52; i++)
        played_cards[i] = 65;
    card_count = 0;

    deal_card(player_text, &player_v, buf);
    deal_card(player_text, &player_v, buf);

    deal_card(dealer_text, &dealer_v, buf);

    char dealer_score_text[32];
    c_int_to_str(dealer_v.low, dealer_score_text);

    c_int_to_str(global_dealer_points, c_dealer_points);
    c_int_to_str(global_player_points, c_player_points);

    render(player_text, dealer_text, "?", dealer_score_text);

    int dealer_points = 0;

    while (1) {
        if (player_v.low == player_v.high || player_v.high > 21)
            c_int_to_str(player_v.low, player_score);
        else {
            char a[16], b[16];
            c_int_to_str(player_v.low, a);
            c_int_to_str(player_v.high, b);

            int i = 0;
            for (int j = 0; a[j]; j++) player_score[i++] = a[j];
            player_score[i++] = '/';
            for (int j = 0; b[j]; j++) player_score[i++] = b[j];
            player_score[i] = 0;
        }

        if (dealer_v.low == dealer_v.high || dealer_v.high > 21)
            c_int_to_str(dealer_v.low, dealer_score_text);
        else {
            char a[16], b[16];
            c_int_to_str(dealer_v.low, a);
            c_int_to_str(dealer_v.high, b);

            int i = 0;
            for (int j = 0; a[j]; j++) dealer_score_text[i++] = a[j];
            dealer_score_text[i++] = '/';
            for (int j = 0; b[j]; j++) dealer_score_text[i++] = b[j];
            dealer_score_text[i] = 0;
        }

        render(player_text, dealer_text, player_score, dealer_score_text);

        if (player_v.low > 21) {
            print("\nPlayer Bust\n");
            global_dealer_points++;
            dealer_points = dealer(player_text, dealer_v, dealer_text);
            break;
        }

        char ans;
        scan_c(&ans);

        if (ans == 's' || ans == 'S') {
            dealer_points = dealer(player_text, dealer_v, dealer_text);
            break;
        }

        if (ans == 'e' || ans == 'E')
            return 1;

        deal_card(player_text, &player_v, buf);
    }

    int player_best = (player_v.high <= 21) ? player_v.high : player_v.low;

    if (dealer_points != 0 && player_best <= 21) {
        if (dealer_points > 21 && player_best > 21) {
            print("\nBoth Bust\n");
        }
        else if (dealer_points > 21) {
            print("\nDealer Bust\n");
        }
        else if (dealer_points > player_best) {
            print("\nDealer Wins\n");
            global_dealer_points++;
        }
        else if (dealer_points < player_best) {
            print("\nPlayer Wins\n");
            global_player_points++;
        }
        else
            print("\nPush (Tie)\n");
    }

    c_int_to_str(global_dealer_points, c_dealer_points);
    c_int_to_str(global_player_points, c_player_points);

    print("Press Enter to continue...");
    wait_enter();

    return 0;
}

void _start() {
    while (1) {
        if (game_round() == 1)
            break;
    }
    exit(0);
}
