all: monkeys.c
	gcc -std=c99 -Wall -Wextra -O3 -o monkeys monkeys.c
