#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include "dungeon.h"


int main(void)
{
    // Use a different seed value so that we don't get same
    // result each time we run this program
    srand ( time(NULL) );

    // Tail Cave		25
    // Bottle Grotto	26
    // Key Cavern		29
    // Anglers Tunnel	28
    // Catfish’s Maw	34
    // Face Shrine		40
    // Eagle’s Tower	34
    // Turtle Rock		46
    // Dungeon sizes
    int mazeW = 5;
    int mazeH = 5;

    // Create and connect dungeon
    dungeon_t dungeon;
    initDungeon(&dungeon, mazeW, mazeH);
    connectDungeonEllers(&dungeon);

    // Find the start and end
    clearDungeonDists(&dungeon);
    coord_t furthestRoom = distFromRoom(&dungeon, mazeW / 2, mazeH - 1);
    dungeon.maze[mazeW / 2][mazeH - 1].isStart = true;
    dungeon.maze[furthestRoom.x][furthestRoom.y].isEnd = true;

    // Save the image
    saveDungeonPng(&dungeon);

    // Free everything
    freeDungeon(&dungeon);
    return 0;
}
