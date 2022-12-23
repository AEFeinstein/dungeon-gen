#ifndef _DUNGEON_H_
#define _DUNGEON_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum 
{
    EMPTY_ROOM,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
} keyType_t;

struct _door;
struct _room;

typedef struct _door
{
    /**
     * True if this is a door, false if this is a wall
     */
    bool isDoor;
    /**
     * The type of lock for this door
     */
    keyType_t lock;
    /**
     * temp var, the number of rooms past this door
     */
    int16_t numChildren;
    /**
     * All the rooms connecting to this door
     */
    struct _room * rooms[2];
} door_t;

typedef struct _room
{
    /** true if this is the starting room */
    bool isStart;
    /** true if this is the ending room */
    bool isEnd;
    /** true if this is a dead end */
    bool isDeadEnd;
    /** the type of treasure in this room */
    keyType_t treasure;
    /**
     * The partition this room belongs to after segmenting the dungeon with
     * locked doors
     */
    keyType_t partition;
    /**
     * This room's set, used for Eller's maze generation algorithm
     */
    int32_t set;
    /**
     * All the doors connecting to this room
     */
    door_t * doors[4];
    /**
     * temp var, the distance between this room and some other room
     */
    int16_t dist;
    /**
     * temp var, whether or not some algorithm has visited this room
     */
    bool visited;
    /**
     * temp var, the number of rooms past this room
     */
    int16_t numChildren;
} room_t;

typedef struct
{
    room_t ** rooms;
    door_t * doors;
    int w;
    int h;
    int numDoors;
} dungeon_t;

typedef struct
{
    int x;
    int y;
} coord_t;

void initDungeon(dungeon_t * dungeon, int width, int height);
void freeDungeon(dungeon_t * dungeon);

void connectDungeonEllers(dungeon_t * dungeon);
void connectDungeonRecursive(dungeon_t * dungeon);

void saveDungeonPng(dungeon_t * dungeon);

void clearDungeonDists(dungeon_t * dungeon);
coord_t addDistFromRoom(dungeon_t * dungeon, uint16_t startX, uint16_t startY, bool ignoreLocks);
void countRoomsAfterDoors(dungeon_t * dungeon, uint16_t startX, uint16_t startY);
void setPartitions(dungeon_t * dungeon, room_t * startingRoom, keyType_t partition);
void markDeadEnds(dungeon_t * dungeon);
void placeKeys(dungeon_t * dungeon, const keyType_t * keys, int numKeys);

#endif