//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include "rmdDungeonWriter.h"
#include "rayTypes.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    rayMapCellType_t door;
    rayMapCellType_t key;
} rayPair_t;

typedef struct
{
    doorIdx door;
    int xDoor;
    int yDoor;
} doorCheck_t;

//==============================================================================
// Constant data
//==============================================================================

/// @brief Map between keyType_t rayMapCellType_t. Must be in order
const rayPair_t rayPairs[] = {
    {
        // EMPTY_ROOM
        .door = BG_FLOOR,
        .key  = EMPTY,
    },
    {
        // KEY_1
        .door = BG_DOOR,
        .key  = OBJ_ITEM_BEAM,
    },
    {
        // KEY_2
        .door = BG_DOOR_CHARGE,
        .key  = OBJ_ITEM_CHARGE_BEAM,
    },
    {
        // KEY_3
        .door = BG_DOOR_MISSILE,
        .key  = OBJ_ITEM_MISSILE,
    },
    {
        // KEY_4
        .door = BG_FLOOR_LAVA,
        .key  = OBJ_ITEM_SUIT_LAVA,
    },
    {
        // KEY_5
        .door = BG_DOOR_ICE,
        .key  = OBJ_ITEM_ICE,
    },
    {
        // KEY_6
        .door = BG_FLOOR_WATER,
        .key  = OBJ_ITEM_SUIT_WATER,
    },
    {
        // KEY_7
        .door = BG_DOOR_XRAY,
        .key  = OBJ_ITEM_XRAY,
    },
    {
        // KEY_8
        .door = BG_DOOR_KEY_A,
        .key  = OBJ_ITEM_KEY_A,
    },
    {
        // KEY_9
        .door = BG_DOOR_KEY_B,
        .key  = OBJ_ITEM_KEY_B,
    },
    {
        // KEY_10
        .door = BG_DOOR_KEY_C,
        .key  = OBJ_ITEM_KEY_C,
    },
};

//==============================================================================
// Prototypes
//==============================================================================

static void placeFloor(keyType_t partition, FILE* file);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Convert keyType_t to rayMapCellType_t
 *
 * @param key The type to convert
 * @param isDoor True if this is a door, false if this is a key
 * @return The equivalent background or object
 */
static rayMapCellType_t keyTypeToRayType(keyType_t key, bool isDoor)
{
    if (isDoor)
    {
        if (key < (int)(sizeof(rayPairs) / sizeof(rayPairs[0])))
        {
            return rayPairs[key].door;
        }
        else
        {
            return rayPairs[(sizeof(rayPairs) / sizeof(rayPairs[0])) - 1].door;
        }
    }
    else
    {
        if (key < (int)(sizeof(rayPairs) / sizeof(rayPairs[0])))
        {
            return rayPairs[key].key;
        }
        else
        {
            return rayPairs[(sizeof(rayPairs) / sizeof(rayPairs[0])) - 1].key;
        }
    }
}

/**
 * @brief Save a dungeon as an RMD file
 *
 * @param dungeon The dungeon to save
 * @param roomSize The number of cells for a side of a room. Must be at least 3
 * @param carveWalls true to carve out walls in a partition, false to leave them
 */
void saveDungeonRmd(dungeon_t* dungeon, int roomSize, bool carveWalls)
{
    // Make sure this is at least 3
    if (roomSize < 3)
    {
        roomSize = 3;
    }

    // Array to check doors easier
    doorCheck_t dc[] = {
        {
            .door  = DOOR_UP,
            .xDoor = roomSize / 2,
            .yDoor = 0,
        },
        {
            .door  = DOOR_DOWN,
            .xDoor = roomSize / 2,
            .yDoor = roomSize - 1,
        },
        {
            .door  = DOOR_LEFT,
            .xDoor = 0,
            .yDoor = roomSize / 2,
        },
        {
            .door  = DOOR_RIGHT,
            .xDoor = roomSize - 1,
            .yDoor = roomSize / 2,
        },
    };

    int objIdx = 0;
    // Open a file
    FILE* file = fopen("dungeon.rmd", "wb");
    // Write dimensions
    fputc(dungeon->w * roomSize, file);
    fputc(dungeon->h * roomSize, file);

    for (int y = 0; y < dungeon->h; y++)
    {
        for (int roomY = 0; roomY < roomSize; roomY++)
        {
            for (int x = 0; x < dungeon->w; x++)
            {
                for (int roomX = 0; roomX < roomSize; roomX++)
                {
                    // If this is a boundary
                    if ((roomX == 0) || (roomX == (roomSize - 1)) || (roomY == 0) || (roomY == (roomSize - 1)))
                    {
                        bool doorPlaced = false;
                        for (int d = 0; d < (int)(sizeof(dc) / sizeof(dc[0])); d++)
                        {
                            if (dungeon->rooms[x][y].doors[dc[d].door] &&         //
                                dungeon->rooms[x][y].doors[dc[d].door]->isDoor && //
                                ((dc[d].yDoor == roomY) &&                        //
                                 (dc[d].xDoor == roomX)))
                            {
                                keyType_t key = dungeon->rooms[x][y].doors[dc[d].door]->lock;
                                if (EMPTY_ROOM == key)
                                {
                                    // For empty rooms, place floor according to partition
                                    placeFloor(dungeon->rooms[x][y].partition, file);
                                }
                                else
                                {
                                    // Place the door according ot the lock type
                                    fputc(keyTypeToRayType(key, true), file);
                                }
                                doorPlaced = true;
                                break;
                            }
                        }

                        // If a door wasn't placed
                        if (!doorPlaced)
                        {
                            // If an adjacent cell is part of the same partition, don't draw a wall there
                            bool adjacentIsSamePartition = false;

                            if (carveWalls)
                            {
                                // Left wall
                                if ((0 == roomX) &&                              //
                                    ((0 < roomY) && (roomY < (roomSize - 1))) && //
                                    (x > 0) &&                                   //
                                    (dungeon->rooms[x - 1][y].partition == dungeon->rooms[x][y].partition))
                                {
                                    adjacentIsSamePartition = true;
                                }
                                // Right wall
                                if (((roomSize - 1) == roomX) &&                 //
                                    ((0 < roomY) && (roomY < (roomSize - 1))) && //
                                    (x < (dungeon->w - 1)) &&                    //
                                    (dungeon->rooms[x + 1][y].partition == dungeon->rooms[x][y].partition))
                                {
                                    adjacentIsSamePartition = true;
                                }

                                // Top wall
                                if ((0 == roomY) &&                              //
                                    ((0 < roomX) && (roomX < (roomSize - 1))) && //
                                    (y > 0) &&                                   //
                                    (dungeon->rooms[x][y - 1].partition == dungeon->rooms[x][y].partition))
                                {
                                    adjacentIsSamePartition = true;
                                }
                                // Bottom wall
                                if (((roomSize - 1) == roomY) &&                 //
                                    ((0 < roomX) && (roomX < (roomSize - 1))) && //
                                    (y < (dungeon->h - 1)) &&                    //
                                    (dungeon->rooms[x][y + 1].partition == dungeon->rooms[x][y].partition))
                                {
                                    adjacentIsSamePartition = true;
                                }

                                // Top Left
                                if ((0 == roomX && 0 == roomY) &&                                           //
                                    (x > 0 && y > 0) &&                                                     //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x - 1][y].partition && //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x][y - 1].partition)
                                {
                                    adjacentIsSamePartition = true;
                                }
                                // Bottom Left
                                if ((0 == roomX && (roomSize - 1) == roomY) &&                              //
                                    (x > 0 && y < (dungeon->h - 1)) &&                                      //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x - 1][y].partition && //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x][y + 1].partition)
                                {
                                    adjacentIsSamePartition = true;
                                }

                                // Top Right
                                if (((roomSize - 1) == roomX && 0 == roomY) &&                              //
                                    (x < (dungeon->w - 1) && y > 0) &&                                      //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x + 1][y].partition && //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x][y - 1].partition)
                                {
                                    adjacentIsSamePartition = true;
                                }
                                // Bottom Right
                                if (((roomSize - 1) == roomX && (roomSize - 1) == roomY) &&                 //
                                    (x < (dungeon->w - 1) && y < (dungeon->h - 1)) &&                       //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x + 1][y].partition && //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x][y + 1].partition)
                                {
                                    adjacentIsSamePartition = true;
                                }
                            }

                            // If adjacent cells are the same partition
                            if (adjacentIsSamePartition)
                            {
                                // Put some floor
                                placeFloor(dungeon->rooms[x][y].partition, file);
                            }
                            else
                            {
                                // Put a wall, style based on partition
                                fputc(BG_WALL_1 + (dungeon->rooms[x][y].partition % (BG_WALL_5 - BG_WALL_1 + 1)), file);
                            }
                        }

                        // No object on this tile
                        fputc(EMPTY, file);
                    }
                    else
                    {
                        // Otherwise put some floor
                        placeFloor(dungeon->rooms[x][y].partition, file);

                        // Place an object, maybe
                        if ((roomX == roomSize / 2) && (roomY == roomSize / 2))
                        {
                            rayMapCellType_t itemType = EMPTY;
                            room_t* room              = &dungeon->rooms[x][y];
                            if (EMPTY_ROOM != dungeon->rooms[x][y].treasure)
                            {
                                itemType = keyTypeToRayType(room->treasure, false);
                            }
                            else if (room->isStart)
                            {
                                itemType = OBJ_ENEMY_START_POINT;
                            }
                            else if (room->isEnd)
                            {
                                itemType = OBJ_ITEM_ARTIFACT;
                            }
                            else if (room->isDeadEnd)
                            {
                                itemType = OBJ_ITEM_PICKUP_ENERGY;
                            }

                            fputc(itemType, file);
                            if (EMPTY != itemType)
                            {
                                fputc(objIdx++, file);
                            }
                        }
                        else
                        {
                            // No item
                            fputc(EMPTY, file);
                        }
                    }
                }
            }
        }
    }
    // No scripts
    fputc(0, file);
    fclose(file);
}

/**
 * @brief Place a floor tile according to partition
 *
 * @param partition
 * @param file
 */
static void placeFloor(keyType_t partition, FILE* file)
{
    // Put some floor
    switch (keyTypeToRayType(partition, true))
    {
        case BG_FLOOR_LAVA:
        {
            fputc(BG_FLOOR_LAVA, file);
            break;
        }
        case BG_FLOOR_WATER:
        {
            fputc(BG_FLOOR_WATER, file);
            break;
        }
        default:
        {
            fputc(BG_FLOOR, file);
            break;
        }
    }
}