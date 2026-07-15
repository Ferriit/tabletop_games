/*NOTE: This implementation of Blackjack was originally written for very limited systems.
 * Any weird tricks like how stuff is stored or printed was originally to be able to compile without the c standard library.*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

#ifdef _WIN32
    #include <windows.h>

    void sleep_us(unsigned int us)
    {
        if (us == 0)
            return;

        Sleep((us + 999) / 1000); // microseconds -> milliseconds (rounded up)
    }

#else
    #include <unistd.h>

    void sleep_us(unsigned int us)
    {
        usleep(us);
    }

#endif

void render(const char *player, const char *dealer,
            const char *player_score, const char *dealer_score) {

    printf("\x1b[H\x1b[2J");

    printf("PLAYER: %s\n        %s\n\nDEALER: %s\n        %s\n\n", player, player_score, dealer, dealer_score);

    if (global_player_points > global_dealer_points) {
        printf("%s - %s TO PLAYER", c_player_points, c_dealer_points);
    }
    else if (global_dealer_points > global_player_points) {
        printf("%s - %s TO DEALER", c_dealer_points, c_player_points);
    }
    else {
        printf("%s - %s", c_dealer_points, c_player_points);
    }

    printf("\n> Hit or Stand (h/s) (e to exit): ");
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
    strcat(text, buf);

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
        snprintf(dealer_score, sizeof(dealer_score), "%d", best);

        render(player_text, dealer_text, player_score, dealer_score);

        if (best > 21) {
            printf("\nDealer Bust\n");
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
    srand((unsigned int)time(NULL));

    char buf[32];

    char player_text[128];
    player_text[0] = '\0';

    char dealer_text[128];
    dealer_text[0] = '\0';

    value player_v = {0, 0};
    value dealer_v = {0, 0};

    for (int i = 0; i < 52; i++)
        played_cards[i] = 65;

    card_count = 0;

    deal_card(player_text, &player_v, buf);
    deal_card(player_text, &player_v, buf);

    deal_card(dealer_text, &dealer_v, buf);

    char dealer_score_text[32];

    snprintf(dealer_score_text, sizeof(dealer_score_text), "%d", dealer_v.low);
    snprintf(c_dealer_points, sizeof(c_dealer_points), "%d", global_dealer_points);
    snprintf(c_player_points, sizeof(c_player_points), "%d", global_player_points);

    render(player_text, dealer_text, "?", dealer_score_text);

    int dealer_points = 0;

    while (1) {
        // Player score
        if (player_v.low == player_v.high || player_v.high > 21) {
            snprintf(player_score, sizeof(player_score), "%d", player_v.low);
        }
        else {
            snprintf(player_score, sizeof(player_score), "%d/%d", player_v.low, player_v.high);
        }

        // Dealer score
        if (dealer_v.low == dealer_v.high || dealer_v.high > 21) {
            snprintf(dealer_score_text, sizeof(dealer_score_text), "%d", dealer_v.low);
        }
        else {
            snprintf(dealer_score_text, sizeof(dealer_score_text), "%d/%d", dealer_v.low, dealer_v.high);
        }

        render(player_text, dealer_text, player_score, dealer_score_text);

        if (player_v.low > 21) {
            printf("\nPlayer Bust\n");

            global_dealer_points++;

            dealer_points = dealer(player_text, dealer_v, dealer_text);
            break;
        }
        char ans;

        scanf(" %c", &ans);

        if (ans == 's' || ans == 'S') {
            dealer_points = dealer(player_text, dealer_v, dealer_text);
            break;
        }

        if (ans == 'e' || ans == 'E') {
            return 1;
        }

        if (ans == 'h' || ans == 'H') {
            deal_card(player_text, &player_v, buf);
        }
    }

    int player_best = (player_v.high <= 21) ? player_v.high : player_v.low;

    if (dealer_points != 0 && player_best <= 21) {
        if (dealer_points > 21 && player_best > 21) {
            printf("\nBoth Bust\n");
        }
        else if (dealer_points > 21) {
            printf("\nDealer Bust\n");
        }
        else if (dealer_points > player_best) {
            printf("\nDealer Wins\n");
            global_dealer_points++;
        }
        else if (dealer_points < player_best) {
            printf("\nPlayer Wins\n");
            global_player_points++;
        }
        else {
            printf("\nPush (Tie)\n");
        }
    }

    snprintf(c_dealer_points, sizeof(c_dealer_points), "%d", global_dealer_points);
    snprintf(c_player_points, sizeof(c_player_points), "%d", global_player_points);

    printf("Press Enter to continue...");
    getchar(); // consume newline
    getchar(); // wait for enter

    return 0;
}

int main() {
    while (1) {
        if (game_round() == 1)
            break;
    }
    return 0;
}
