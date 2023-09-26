.PHONY: all clean format

all:
	gcc ./src/dungeon-gen.c ./src/linked_list.c ./src/dungeon.c ./src/pngDungeonWriter.c ./src/rmdDungeonWriter.c -g -Wall -Wextra -o dungeon-gen -lm -std=c99

clean:
	rm -rf dungeon-gen

format:
	clang-format -i -style=file ./src/dungeon-gen.c ./src/dungeon.c ./src/dungeon.h ./src/linked_list.c ./src/linked_list.h ./src/pngDungeonWriter.c ./src/pngDungeonWriter.h ./src/rayTypes.h ./src/rmdDungeonWriter.c ./src/rmdDungeonWriter.h 