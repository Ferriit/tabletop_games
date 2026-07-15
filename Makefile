CC := gcc
C_FLAGS := -Oz -s

DIST_DIR := dist/

default: all

all: chess blackjack

clean:
	rm -rf ${DIST_DIR}

dist_dir:
	mkdir -p ${DIST_DIR}

chess: dist_dir
	${CC} chess/main.c ${C_FLAGS} -o ${DIST_DIR}chess

blackjack: dist_dir
	${CC} blackjack/main.c ${C_FLAGS} -o ${DIST_DIR}blackjack

