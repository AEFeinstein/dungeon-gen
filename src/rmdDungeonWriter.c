//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rmdDungeonWriter.h"
#include "rayTypes.h"
#include "linked_list.h"

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

typedef struct
{
    uint8_t x;
    uint8_t y;
} cell_t;

typedef struct
{
    cell_t cam;
    cell_t doors[5];
    uint32_t numDoors;
} doorScript_t;

//==============================================================================
// Constant data
//==============================================================================

/// @brief Map between keyType_t rayMapCellType_t. Must be in order
const rayPair_t rayPairs[] = {
    {
        // EMPTY_ROOM
        .door = BG_FLOOR_DUNGEON,
        .key  = EMPTY,
    },
    {
        // KEY_1
        .door = BG_DOOR_SCRIPT_LOCKED,
        .key  = OBJ_ITEM_SHIELD,
    },
    {
        // KEY_2
        .door = BG_DOOR_SCRIPT_LOCKED,
        .key  = OBJ_ITEM_BOOMERANG,
    },
    {
        // KEY_3
        .door = BG_DOOR_SCRIPT_LOCKED,
        .key  = OBJ_ITEM_LULLABY,
    },
    {
        // KEY_4
        .door = BG_DOOR_R_KEY_LOCKED,
        .key  = OBJ_ITEM_R_KEY,
    },
    {
        // KEY_5
        .door = BG_DOOR_G_KEY_LOCKED,
        .key  = OBJ_ITEM_G_KEY,
    },
    {
        // KEY_6
        .door = BG_DOOR_B_KEY_LOCKED,
        .key  = OBJ_ITEM_B_KEY,
    },
    {
        // KEY_7
        .door = BG_DOOR_K_KEY_LOCKED,
        .key  = OBJ_ITEM_K_KEY,
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
 * @param roomWidth The number of cells for the width of a room. Must be at least 3
 * @param roomHeight The number of cells for the height of a room. Must be at least 3
 * @param carveWalls true to carve out walls in a partition, false to leave them
 * @param name The name to save
 */
void saveDungeonRmd(dungeon_t* dungeon, int roomWidth, int roomHeight, bool carveWalls, const char* name)
{
    // Make sure this is at least 3
    if (roomWidth < 3)
    {
        roomWidth = 3;
    }

    if (roomHeight < 3)
    {
        roomHeight = 3;
    }

    // Array to check doors easier
    doorCheck_t dc[] = {
        {
            .door  = DOOR_UP,
            .xDoor = roomWidth / 2,
            .yDoor = 0,
        },
        {
            .door  = DOOR_DOWN,
            .xDoor = roomWidth / 2,
            .yDoor = roomHeight - 1,
        },
        {
            .door  = DOOR_LEFT,
            .xDoor = 0,
            .yDoor = roomHeight / 2,
        },
        {
            .door  = DOOR_RIGHT,
            .xDoor = roomWidth - 1,
            .yDoor = roomHeight / 2,
        },
    };

    int objIdx = 0;
    // Open a file
    char nameWithSuffix[strlen(name) + 5];
    snprintf(nameWithSuffix, sizeof(nameWithSuffix), "%s.rmd", name);
    FILE* file = fopen(nameWithSuffix, "wb");
    if (NULL == file)
    {
        fprintf(stderr, "Couldn't open %s for writing!\n", nameWithSuffix);
        return;
    }
    // Write dimensions
    fputc(dungeon->w * roomWidth, file);
    fputc(dungeon->h * roomHeight, file);

    // Set up door scripts
    doorScript_t doorScripts[dungeon->w][dungeon->h];
    memset(doorScripts, 0, sizeof(doorScripts));

    for (int y = 0; y < dungeon->h; y++)
    {
        for (int roomY = 0; roomY < roomHeight; roomY++)
        {
            for (int x = 0; x < dungeon->w; x++)
            {
                for (int roomX = 0; roomX < roomWidth; roomX++)
                {
                    // If this is a boundary
                    if ((roomX == 0) || (roomX == (roomWidth - 1)) || (roomY == 0) || (roomY == (roomHeight - 1)))
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
                                if ((EMPTY_ROOM == key)
                                    // TODO place door on side of player progression, if possible?
                                    // || (dungeon->rooms[x][y].doors[DOOR_LEFT]
                                    //     && (EMPTY_ROOM != dungeon->rooms[x][y].doors[DOOR_LEFT]->lock))
                                    // || (dungeon->rooms[x][y].doors[DOOR_UP]
                                    //     && (EMPTY_ROOM != dungeon->rooms[x][y].doors[DOOR_UP]->lock))
                                )
                                {
                                    // For empty rooms or if an adjacent door was already placed,
                                    // place floor according to partition
                                    // placeFloor(dungeon->rooms[x][y].partition, file);
                                    fputc(BG_DOOR_DUNGEON, file);
                                }
                                else
                                {
                                    // Place the door according ot the lock type
                                    fputc(keyTypeToRayType(key, true), file);
                                }
                                doorPlaced = true;

                                doorScript_t* ds          = &doorScripts[x][y];
                                ds->doors[ds->numDoors].x = (x * roomWidth) + roomX;
                                ds->doors[ds->numDoors].y = (y * roomHeight) + roomY;
                                ds->numDoors++;

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
                                if ((0 == roomX) &&                                //
                                    ((0 < roomY) && (roomY < (roomHeight - 1))) && //
                                    (x > 0) &&                                     //
                                    (dungeon->rooms[x - 1][y].partition == dungeon->rooms[x][y].partition))
                                {
                                    adjacentIsSamePartition = true;
                                }
                                // Right wall
                                if (((roomWidth - 1) == roomX) &&                  //
                                    ((0 < roomY) && (roomY < (roomHeight - 1))) && //
                                    (x < (dungeon->w - 1)) &&                      //
                                    (dungeon->rooms[x + 1][y].partition == dungeon->rooms[x][y].partition))
                                {
                                    adjacentIsSamePartition = true;
                                }

                                // Top wall
                                if ((0 == roomY) &&                               //
                                    ((0 < roomX) && (roomX < (roomWidth - 1))) && //
                                    (y > 0) &&                                    //
                                    (dungeon->rooms[x][y - 1].partition == dungeon->rooms[x][y].partition))
                                {
                                    adjacentIsSamePartition = true;
                                }
                                // Bottom wall
                                if (((roomHeight - 1) == roomY) &&                //
                                    ((0 < roomX) && (roomX < (roomWidth - 1))) && //
                                    (y < (dungeon->h - 1)) &&                     //
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
                                if ((0 == roomX && (roomHeight - 1) == roomY) &&                            //
                                    (x > 0 && y < (dungeon->h - 1)) &&                                      //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x - 1][y].partition && //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x][y + 1].partition)
                                {
                                    adjacentIsSamePartition = true;
                                }

                                // Top Right
                                if (((roomWidth - 1) == roomX && 0 == roomY) &&                             //
                                    (x < (dungeon->w - 1) && y > 0) &&                                      //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x + 1][y].partition && //
                                    dungeon->rooms[x][y].partition == dungeon->rooms[x][y - 1].partition)
                                {
                                    adjacentIsSamePartition = true;
                                }
                                // Bottom Right
                                if (((roomWidth - 1) == roomX && (roomHeight - 1) == roomY) &&              //
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
                                // Put a wall
                                if ((roomX == 0) && (roomY == 0))
                                {
                                    fputc(BG_WALL_DUNGEON_UL, file);
                                }
                                else if ((roomX == (roomWidth - 1)) && (roomY == (roomHeight - 1)))
                                {
                                    fputc(BG_WALL_DUNGEON_DR, file);
                                }
                                else if ((roomX == 0) && (roomY == (roomHeight - 1)))
                                {
                                    fputc(BG_WALL_DUNGEON_DL, file);
                                }
                                else if ((roomX == (roomWidth - 1)) && (roomY == 0))
                                {
                                    fputc(BG_WALL_DUNGEON_UR, file);
                                }
                                else if ((roomX == 0) || (roomX == (roomWidth - 1)))
                                {
                                    fputc(BG_WALL_DUNGEON_V, file);
                                }
                                else if ((roomY == 0) || (roomY == (roomHeight - 1)))
                                {
                                    fputc(BG_WALL_DUNGEON_H, file);
                                }
                            }
                        }

                        // No object on this tile
                        fputc(EMPTY, file);
                    }
                    else
                    {
                        // Otherwise put some floor
                        placeFloor(dungeon->rooms[x][y].partition, file);

                        room_t* room = &dungeon->rooms[x][y];

                        // Place an object, maybe
                        if ((roomX == (roomWidth / 2) - 1) && (roomY == roomHeight / 2) && (room->isStart))
                        {
                            // Place exit to left of player at beginning
                            fputc(OBJ_SCENERY_STAIRS, file);
                            fputc(objIdx++, file);
                        }
                        else if ((roomX == roomWidth / 2) && (roomY == roomHeight / 2))
                        {
                            rayMapCellType_t itemType = EMPTY;
                            if (EMPTY_ROOM != dungeon->rooms[x][y].treasure)
                            {
                                itemType = keyTypeToRayType(room->treasure, false);
                            }
                            else if (room->isStart)
                            {
                                itemType = OBJ_ENEMY_START_POINT;

                                // Script camera upon entry
                                doorScript_t* ds          = &doorScripts[x][y];
                                ds->doors[ds->numDoors].x = (x * roomWidth) + roomX;
                                ds->doors[ds->numDoors].y = (y * roomHeight) + roomY;
                                ds->numDoors++;
                            }
                            else if (room->isEnd)
                            {
                                // Place exit at end
                                itemType = OBJ_SCENERY_STAIRS;
                            }
                            else if (room->isDeadEnd)
                            {
                                // Place mpoint at dead ends
                                itemType = OBJ_ITEM_MPOINT_20;
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

    // Count number of actual doors
    uint8_t doorCount = 0;
    for (int d = 0; d < dungeon->numDoors; d++)
    {
        doorCount += (dungeon->doors[d].isDoor) ? 1 : 0;
    }

    // Write camera scripts, one for each room and one for each door
    fputc((dungeon->w * dungeon->h) + doorCount, file);

    // For each door
    for (int d = 0; d < dungeon->numDoors; d++)
    {
        door_t* door = &dungeon->doors[d];

        // If this is a real door
        if (door->isDoor)
        {
            // Find the locations of the rooms between the doors
            // Room 0 is always either above or to the left of room 1
            cell_t room0loc;
            cell_t room1loc;
            for (int y = 0; y < dungeon->h; y++)
            {
                for (int x = 0; x < dungeon->w; x++)
                {
                    if (door->rooms[0] == &dungeon->rooms[x][y])
                    {
                        room0loc.x = x;
                        room0loc.y = y;
                    }
                    else if (door->rooms[1] == &dungeon->rooms[x][y])
                    {
                        room1loc.x = x;
                        room1loc.y = y;
                    }
                }
            }

            bool isHorz = (room0loc.x != room1loc.x);

            // Make the list of cells which trigger door closing
            cell_t doorCells[2];
            cell_t triggerCells[6];
            if (isHorz)
            {
                // Door is on right side of room0
                doorCells[0].x = (roomWidth * room0loc.x) + dc[3].xDoor;
                doorCells[0].y = (roomHeight * room0loc.y) + dc[3].yDoor;

                triggerCells[0].x = doorCells[0].x + 2;
                triggerCells[0].y = doorCells[0].y;

                triggerCells[1].x = doorCells[0].x + 1;
                triggerCells[1].y = doorCells[0].y + 1;

                triggerCells[2].x = doorCells[0].x + 1;
                triggerCells[2].y = doorCells[0].y - 1;

                // Door is on the left side of room 1
                doorCells[1].x = (roomWidth * room1loc.x) + dc[2].xDoor;
                doorCells[1].y = (roomHeight * room1loc.y) + dc[2].yDoor;

                triggerCells[3].x = doorCells[1].x - 2;
                triggerCells[3].y = doorCells[1].y;

                triggerCells[4].x = doorCells[1].x - 1;
                triggerCells[4].y = doorCells[1].y - 1;

                triggerCells[5].x = doorCells[1].x - 1;
                triggerCells[5].y = doorCells[1].y + 1;
            }
            else
            {
                // Door is on bottom of room 0
                doorCells[0].x = (roomWidth * room0loc.x) + dc[1].xDoor;
                doorCells[0].y = (roomHeight * room0loc.y) + dc[1].yDoor;

                triggerCells[0].x = doorCells[0].x;
                triggerCells[0].y = doorCells[0].y - 2;

                triggerCells[1].x = doorCells[0].x - 1;
                triggerCells[1].y = doorCells[0].y - 1;

                triggerCells[2].x = doorCells[0].x + 1;
                triggerCells[2].y = doorCells[0].y - 1;

                // Door is on the top of room 1
                doorCells[1].x = (roomWidth * room1loc.x) + dc[0].xDoor;
                doorCells[1].y = (roomHeight * room1loc.y) + dc[0].yDoor;

                triggerCells[3].x = doorCells[1].x;
                triggerCells[3].y = doorCells[1].y + 2;

                triggerCells[4].x = doorCells[1].x - 1;
                triggerCells[4].y = doorCells[1].y + 1;

                triggerCells[5].x = doorCells[1].x + 1;
                triggerCells[5].y = doorCells[1].y + 1;
            }

            uint8_t numCells = (sizeof(triggerCells) / sizeof(triggerCells[0]));
            uint8_t numDoors = (sizeof(doorCells) / sizeof(doorCells[0]));

            // Two bytes of script length
            uint16_t len = 7 + 2 * (numCells + numDoors);
            fputc((len >> 8) * 0xFF, file);
            fputc(len & 0xFF, file);

            fputc(5, file); // IF ENTER

            fputc(1, file);        // AND/OR (OR = 1)
            fputc(numCells, file); // Number of cells
            // Cell pairs
            for (uint32_t cIdx = 0; cIdx < numCells; cIdx++)
            {
                fputc(triggerCells[cIdx].x, file);
                fputc(triggerCells[cIdx].y, file);
            }
            fputc(1, file); // ORDER (ANY_ORDER = 1)
            fputc(1, file); // ONE TIME (ALWAYS = 1)

            fputc(8, file); // THEN CLOSE

            // One byte of length
            fputc(numDoors, file);

            for (int dIdx = 0; dIdx < numDoors; dIdx++)
            {
                fputc(doorCells[dIdx].x, file); // DOOR X
                fputc(doorCells[dIdx].y, file); // DOOR Y
            }
        }
    }

    // For each room
    for (int y = 0; y < dungeon->h; y++)
    {
        for (int x = 0; x < dungeon->w; x++)
        {
            // Get script
            doorScript_t* ds = &doorScripts[x][y];

            // Set camera target
            ds->cam.x = x * roomWidth;
            ds->cam.y = y * roomHeight;

            // Two bytes of script length
            uint16_t len = 8 + (2 * ds->numDoors);
            fputc((len >> 8) * 0xFF, file);
            fputc(len & 0xFF, file);

            fputc(5, file); // IF ENTER

            fputc(1, file);            // AND/OR (OR = 1)
            fputc(ds->numDoors, file); // Number of cells
            // Cell pairs
            for (uint32_t dIdx = 0; dIdx < ds->numDoors; dIdx++)
            {
                fputc(ds->doors[dIdx].x, file);
                fputc(ds->doors[dIdx].y, file);
            }
            fputc(1, file); // ORDER (ANY_ORDER = 1)
            fputc(1, file); // ONE TIME (ALWAYS = 1)

            fputc(14, file); // THEN CAMERA

            fputc(ds->cam.x, file); // Camera X
            fputc(ds->cam.y, file); // Camera Y
        }
    }

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
        // case BG_FLOOR_LAVA:
        // {
        //     fputc(BG_FLOOR_LAVA, file);
        //     break;
        // }
        // case BG_FLOOR_WATER:
        // {
        //     fputc(BG_FLOOR_WATER, file);
        //     break;
        // }
        default:
        {
            fputc(BG_FLOOR_DUNGEON, file);
            break;
        }
    }
}