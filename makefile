all:
	gcc ./src/dungeon-gen.c ./src/linked_list.c ./src/dungeon.c -g -Wall -Wextra -o dungeon-gen -lm -std=c99

clean:
	rm -rf dungeon-gen
