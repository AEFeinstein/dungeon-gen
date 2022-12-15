#ifndef _DUNGEON_H_
#define _DUNGEON_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum 
{
    EMPTY_ROOM,
    DUNGEON_START,
    KEY_1,
    KEY_2,
    DUNGEON_END,
    DEAD_END
} roomType_t;

typedef struct
{
    bool isDoor;
    roomType_t type;
} door_t;

typedef struct
{
    bool visited;
    roomType_t type;
    int32_t set;
    door_t * doors[4];
    int16_t dist;
    bool isOnPath;
} mazeCell_t;

typedef struct
{
    int w;
    int h;
    mazeCell_t ** maze;
    door_t * doors;
} dungeon_t;

typedef struct
{
    int x;
    int y;
} coord_t;

void initDungeon(dungeon_t * dungeon, int mazeW, int mazeH);
void freeDungeon(dungeon_t * dungeon);

void connectDungeonEllers(dungeon_t * dungeon);
void connectDungeonRecursive(dungeon_t * dungeon);

void saveDungeonPng(dungeon_t * dungeon);

void clearDungeonDists(dungeon_t * dungeon);
coord_t distFromRoom(dungeon_t * dungeon, uint16_t startX, uint16_t startY, uint16_t * longestDist);
bool markPath(dungeon_t * dungeon, int x0, int y0, int x1, int y1, int pathLen, roomType_t type);

#endif