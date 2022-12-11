#ifndef _DUNGEON_H_
#define _DUNGEON_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    bool isDoor;
} door_t;

typedef struct
{
    bool visited;
    bool isStart;
    bool isEnd;
    bool isDeadEnd;
    int32_t set;
    door_t * doors[4];
    int16_t dist;
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
coord_t distFromRoom(dungeon_t * dungeon, uint16_t startX, uint16_t startY);

#endif