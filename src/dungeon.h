#ifndef _DUNGEON_H_
#define _DUNGEON_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum 
{
    EMPTY_ROOM,
    DEAD_END,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    DUNGEON_START,
    DUNGEON_END,
} roomType_t;

struct _door;
struct _room;

typedef struct _door
{
    bool isDoor;
    roomType_t type;
    int numChildren;
    bool isOnPath;
    struct _room * rooms[2];
} door_t;

typedef struct _room
{
    bool visited;
    roomType_t type;
    roomType_t partition;
    int32_t set;
    door_t * doors[4];
    int16_t dist;
    bool isOnPath;
    int numChildren;
    int x;
    int y;
} mazeCell_t;

typedef struct
{
    int w;
    int h;
    mazeCell_t ** maze;
    int numDoors;
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
coord_t addDistFromRoom(dungeon_t * dungeon, uint16_t startX, uint16_t startY, bool ignoreLocks);
int countRoomsAfterDoors(dungeon_t * dungeon, uint16_t startX, uint16_t startY, bool isInit);
void setPartitions(dungeon_t * dungeon, mazeCell_t * startingRoom, roomType_t partition);
void markDeadEnds(dungeon_t * dungeon);
void placeKeys(dungeon_t * dungeon, const roomType_t * keys, int numKeys);

#endif