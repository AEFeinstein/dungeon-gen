#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include "dungeon.h"
#include "linked_list.h"

#define ABS(x) (((x) < 0) ? -(x) : (x))

int main(void)
{
    // Use a different seed value so that we don't get same
    // result each time we run this program
    srand ( time(NULL) );

#ifdef RUN_LINKED_LIST_TESTS
    listTester();
#endif

    // Tail Cave		25
    // Bottle Grotto	26
    // Key Cavern		29
    // Anglers Tunnel	28
    // Catfish's Maw	34
    // Face Shrine		40
    // Eagle's Tower	34
    // Turtle Rock		46

    // Dungeon size
    int mazeW = 8;
    int mazeH = 8;

    // The keys and locks to place
    roomType_t goals[] =
    {
        KEY_7,
        KEY_6,
        KEY_5,
        KEY_4,
        KEY_3,
        KEY_2,
        KEY_1,
    };
    int numKeys = sizeof(goals) / sizeof(goals[0]);

    // Create and connect dungeon
    dungeon_t dungeon;
    initDungeon(&dungeon, mazeW, mazeH);
    connectDungeonEllers(&dungeon);

    // Place the start
    coord_t startRoom = {.x = mazeW / 2, .y = mazeH - 1};
    dungeon.maze[startRoom.x][startRoom.y].type = DUNGEON_START;

    // Place locks by partitioning the maze into roughly equal size chunks
    for(uint8_t i = 0; i < numKeys; i++)
    {
        // Figure out how many rooms are behind each door
        countRoomsAfterDoors(&dungeon, startRoom.x, startRoom.y, true);

        // Each partition in the maze should be about this size
        int tpSize = (dungeon.maze[startRoom.x][startRoom.y].numChildren + 1) /
                    (numKeys + 1 - i);

        // Find the door that best partitions the dungeon
        int bestPartitionDiff = dungeon.w * dungeon.h + 1;
        door_t * bestDoor = NULL;
        for(int d = 0; d < dungeon.numDoors; d++)
        {
            door_t* door = &dungeon.doors[d];
            // Try to get this as close to zero as we can
            int partitionDiff = ABS(tpSize - door->numChildren);
            if(partitionDiff < bestPartitionDiff)
            {
                bestPartitionDiff = partitionDiff;
                bestDoor = door;
            }
        }

        // Lock the door
        bestDoor->type = goals[i];

        // Find the room after the lock
        mazeCell_t * roomAfterLock;
        if(bestDoor->rooms[0]->numChildren < bestDoor->numChildren)
        {
            roomAfterLock = bestDoor->rooms[0];
        }
        else
        {
            roomAfterLock = bestDoor->rooms[1];
        }

        // Mark the partiton of all rooms after the lock
        setPartitions(&dungeon, roomAfterLock, goals[i]);
    }

    // Mark dead ends
    markDeadEnds(&dungeon);

    // Place the keys randomly, in accessible locations
    placeKeys(&dungeon, goals, numKeys);

    // Mark the end, which is the furthest room in the last partition
    clearDungeonDists(&dungeon);
    addDistFromRoom(&dungeon, startRoom.x, startRoom.y, true);
    int greatestDist = 0;
    coord_t end = {.x = 0, .y = 0};
    for(int y = 0; y < dungeon.h; y++)
    {
        for(int x = 0; x < dungeon.w; x++)
        {
            if(dungeon.maze[x][y].partition == goals[0])
            {
                if(dungeon.maze[x][y].dist > greatestDist)
                {
                    greatestDist = dungeon.maze[x][y].dist;
                    end.x = x;
                    end.y = y;
                }
            }
        }
    }
    dungeon.maze[end.x][end.y].type = DUNGEON_END;

    // Save the image
    saveDungeonPng(&dungeon);

    // Free everything
    freeDungeon(&dungeon);
    return 0;
}
