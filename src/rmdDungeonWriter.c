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
                        if (dungeon->rooms[x][y].doors[DOOR_UP] && dungeon->rooms[x][y].doors[DOOR_UP]->isDoor
                            && (0 == roomY && RMD_ROOM_SIZE / 2 == roomX)) // up
                        {
                            fputc(keyTypeToRayType(dungeon->rooms[x][y].doors[DOOR_UP]->lock, true), file);
                        }
                        else if (dungeon->rooms[x][y].doors[DOOR_DOWN] && dungeon->rooms[x][y].doors[DOOR_DOWN]->isDoor
                                 && (RMD_ROOM_SIZE - 1 == roomY && RMD_ROOM_SIZE / 2 == roomX)) // down
                        {
                            fputc(keyTypeToRayType(dungeon->rooms[x][y].doors[DOOR_DOWN]->lock, true), file);
                        }
                        else if (dungeon->rooms[x][y].doors[DOOR_LEFT] && dungeon->rooms[x][y].doors[DOOR_LEFT]->isDoor
                                 && (0 == roomX && RMD_ROOM_SIZE / 2 == roomY)) // left
                        {
                            fputc(keyTypeToRayType(dungeon->rooms[x][y].doors[DOOR_LEFT]->lock, true), file);
                        }
                        else if (dungeon->rooms[x][y].doors[DOOR_RIGHT]
                                 && dungeon->rooms[x][y].doors[DOOR_RIGHT]->isDoor
                                 && (RMD_ROOM_SIZE - 1 == roomX && RMD_ROOM_SIZE / 2 == roomY)) // right
                        {
                            fputc(keyTypeToRayType(dungeon->rooms[x][y].doors[DOOR_RIGHT]->lock, true), file);
                        }
                        else
                        {
                            // Put a wall
                            fputc(BG_WALL_1, file);
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
