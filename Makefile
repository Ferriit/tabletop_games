CC := gcc
C_FLAGS := -O3 -s

DIST_DIR := dist/

default: all

all: chess blackjack gomoku connect-4

clean:
	rm -rf ${DIST_DIR}

dist_dir:
	mkdir -p ${DIST_DIR}

chess: dist_dir
	${CC} chess/main.c ${C_FLAGS} -o ${DIST_DIR}chess

blackjack: dist_dir
	${CC} blackjack/main.c ${C_FLAGS} -o ${DIST_DIR}blackjack

gomoku: dist_dir
	${CC} gomoku/main.c ${C_FLAGS} -o ${DIST_DIR}gomoku

connect-4: dist_dir
	${CC} connect-4/main.c ${C_FLAGS} -o ${DIST_DIR}connect-4

checkers: dist_dir
	${CC} checkers/main.c ${C_FLAGS} -o ${DIST_DIR}checkers
