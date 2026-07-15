# tabletop_games

This is a collection of different tabletop games that I've written; either now or a while ago.

## Building
Either run `make` to build everything at once, or `make <game>` to build just one game. The final binary(ies) will be placed in `dist/` by default, however this can be changed in the `Makefile`.

## Per-game instructions
This is an instruction manual over how to use the binaries, not the rules of the games.

### Blackjack
1. Press `[h]` to hit and get anothe card or `[s]` to stand and stop receiving cards.
2. You can quit at any time by pressing `[e]`.

### Chess
1. This binary can be executed, either without flags to play as white, or with the flag `-b` (`--black`) to play as black.
2. Type the moves in the forms `A1B1` where the first square is the starting square and the last square is the destination.
3. Wait for the engine to make a move and keep going.


#### Note
I am actively adding more games to this collection.
