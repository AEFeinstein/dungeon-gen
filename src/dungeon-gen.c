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
    // Catfish's Maw	34
    // Face Shrine		40
    // Eagle's Tower	34
    // Turtle Rock		46
    // Dungeon sizes
    int mazeW = 4;
    int mazeH = 4;

    // Create and connect dungeon
    dungeon_t dungeon;
    initDungeon(&dungeon, mazeW, mazeH);
    connectDungeonEllers(&dungeon);

    // The order in which to place things
    roomType_t goals[] = 
    {
        DUNGEON_END,
        KEY_2,
        KEY_1
    };

    dungeon.maze[mazeW / 2][mazeH - 1].type = DUNGEON_START;

    // Place locks and keys and such
    for(uint8_t i = 0; i < (sizeof(goals) / sizeof(goals[0])); i++)
    {
        clearDungeonDists(&dungeon);
        uint16_t longestDist;
        coord_t furthestRoom = distFromRoom(&dungeon, mazeW / 2, mazeH - 1, &longestDist);
        // Place a key in this room
        dungeon.maze[furthestRoom.x][furthestRoom.y].type = goals[i];
        // Place the next lock
        if(i + 1u < (sizeof(goals) / sizeof(goals[0])))
        {
            markPath(&dungeon, mazeW / 2, mazeH - 1, furthestRoom.x, furthestRoom.y, longestDist, goals[i + 1]);
        }
    }

    // Save the image
    saveDungeonPng(&dungeon);

    // Free everything
    freeDungeon(&dungeon);
    return 0;
}
