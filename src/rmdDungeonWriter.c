//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include "rmdDungeonWriter.h"
#include "rayTypes.h"

//==============================================================================
// Defines
//==============================================================================

#define RMD_ROOM_SIZE 6

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
        .door = BG_DOOR_CHARGE,
        .key  = OBJ_ITEM_CHARGE_BEAM,
    },
    {
        // KEY_2
        .door = BG_DOOR_MISSILE,
        .key  = OBJ_ITEM_MISSILE,
    },
    {
        // KEY_3
        .door = BG_FLOOR_LAVA,
        .key  = OBJ_ITEM_SUIT_LAVA,
    },
    {
        // KEY_4
        .door = BG_DOOR_ICE,
        .key  = OBJ_ITEM_ICE,
    },
    {
        // KEY_5
        .door = BG_FLOOR_WATER,
        .key  = OBJ_ITEM_SUIT_WATER,
    },
    {
        // KEY_6
        .door = BG_DOOR_XRAY,
        .key  = OBJ_ITEM_XRAY,
    },
    {
        // KEY_7 -> KEY_16
        .door = BG_DOOR_KEY,
        .key  = OBJ_ITEM_KEY,
    },
};

doorCheck_t dc[] = {
    {
        .door  = DOOR_UP,
        .xDoor = RMD_ROOM_SIZE / 2,
        .yDoor = 0,
    },
    {
        .door  = DOOR_DOWN,
        .xDoor = RMD_ROOM_SIZE / 2,
        .yDoor = RMD_ROOM_SIZE - 1,
    },
    {
        .door  = DOOR_LEFT,
        .xDoor = 0,
        .yDoor = RMD_ROOM_SIZE / 2,
    },
    {
        .door  = DOOR_RIGHT,
        .xDoor = RMD_ROOM_SIZE - 1,
        .yDoor = RMD_ROOM_SIZE / 2,
    },
};

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
 */
void saveDungeonRmd(dungeon_t* dungeon)
{
    int objIdx = 0;
    // Open a file
    FILE* file = fopen("dungeon.rmd", "wb");
    // Write dimensions
    fputc(dungeon->w * RMD_ROOM_SIZE, file);
    fputc(dungeon->h * RMD_ROOM_SIZE, file);

    for (int y = 0; y < dungeon->h; y++)
    {
        for (int roomY = 0; roomY < RMD_ROOM_SIZE; roomY++)
        {
            for (int x = 0; x < dungeon->w; x++)
            {
                for (int roomX = 0; roomX < RMD_ROOM_SIZE; roomX++)
                {
                    // If this is a boundary
                    if ((roomX == 0) || (roomX == (RMD_ROOM_SIZE - 1)) || (roomY == 0)
                        || (roomY == (RMD_ROOM_SIZE - 1)))
                    {
                        bool doorPlaced = false;
                        for (int d = 0; d < (int)(sizeof(dc) / sizeof(dc[0])); d++)
                        {
                            if (dungeon->rooms[x][y].doors[dc[d].door] &&         //
                                dungeon->rooms[x][y].doors[dc[d].door]->isDoor && //
                                ((dc[d].yDoor == roomY) &&                        //
                                 (dc[d].xDoor == roomX)))
                            {
                                fputc(keyTypeToRayType(dungeon->rooms[x][y].doors[dc[d].door]->lock, true), file);
                                doorPlaced = true;
                                break;
                            }
                        }

                        // If a door wasn't placed
                        if (!doorPlaced)
                        {
                            // If an adjacent cell is part of the same partition, don't draw a wall there
                            bool adjacentIsSamePartition = false;

                            // Left wall
                            if ((0 == roomX) &&                                   //
                                ((0 < roomY) && (roomY < (RMD_ROOM_SIZE - 1))) && //
                                (x > 0) &&                                        //
                                (dungeon->rooms[x - 1][y].partition == dungeon->rooms[x][y].partition))
                            {
                                adjacentIsSamePartition = true;
                            }
                            // Right wall
                            if (((RMD_ROOM_SIZE - 1) == roomX) &&                 //
                                ((0 < roomY) && (roomY < (RMD_ROOM_SIZE - 1))) && //
                                (x < (dungeon->w - 1)) &&                         //
                                (dungeon->rooms[x + 1][y].partition == dungeon->rooms[x][y].partition))
                            {
                                adjacentIsSamePartition = true;
                            }

                            // Top wall
                            if ((0 == roomY) &&                                   //
                                ((0 < roomX) && (roomX < (RMD_ROOM_SIZE - 1))) && //
                                (y > 0) &&                                        //
                                (dungeon->rooms[x][y - 1].partition == dungeon->rooms[x][y].partition))
                            {
                                adjacentIsSamePartition = true;
                            }
                            // Bottom wall
                            if (((RMD_ROOM_SIZE - 1) == roomY) &&                 //
                                ((0 < roomX) && (roomX < (RMD_ROOM_SIZE - 1))) && //
                                (y < (dungeon->h - 1)) &&                         //
                                (dungeon->rooms[x][y + 1].partition == dungeon->rooms[x][y].partition))
                            {
                                adjacentIsSamePartition = true;
                            }

                            // If adjacent cells are the same partition
                            if (adjacentIsSamePartition)
                            {
                                // Put floor
                                fputc(BG_FLOOR, file);
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
                        fputc(BG_FLOOR, file);

                        // Place an object, maybe
                        if ((roomX == RMD_ROOM_SIZE / 2) && (roomY == RMD_ROOM_SIZE / 2))
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
