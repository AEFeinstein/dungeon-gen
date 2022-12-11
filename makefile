all:
	gcc dungeon-gen.c linked_list.c dungeon.c -g -Wall -Wextra -o dungeon-gen -lm

clean:
	rm -rf dungeon-gen
