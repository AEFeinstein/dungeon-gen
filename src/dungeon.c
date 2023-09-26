//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "rayTypes.h"

#include "linked_list.h"
#include "dungeon.h"

//==============================================================================
// Function prototypes
//==============================================================================

#ifdef DBG_PRINT
void printRow(int y, int width, room_t** rooms);
#endif

void mergeSets(dungeon_t* dungeon, int x, int y);
void fisherYates(int* arr, int len);

//==============================================================================
// Defines
//==============================================================================

#define COORDS_TO_LIST(x, y) ((void*)((intptr_t)(((x) << 16) | (y))))
#define LIST_TO_X(c)         ((((intptr_t)(c)) >> 16) & 0xFFFF)
#define LIST_TO_Y(c)         ((((intptr_t)(c)) >> 0) & 0xFFFF)

//==============================================================================
// Variables
//==============================================================================

// Convenience directions
const coord_t cardinals[DOOR_MAX] = {
    {.x = 0, .y = -1}, // DOOR_UP
    {.x = 0, .y = 1},  // DOOR_DOWN
    {.x = -1, .y = 0}, // DOOR_LEFT
    {.x = 1, .y = 0},  // DOOR_RIGHT
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param dungeon
 * @param width
 * @param height
 */
void initDungeon(dungeon_t* dungeon, int width, int height)
{
    // Save width and height
    dungeon->w = width;
    dungeon->h = height;

    // Allocate rows
    dungeon->rooms = (room_t**)calloc(width, sizeof(room_t*));
    // Allocate columns
    for (int i = 0; i < width; i++)
    {
        dungeon->rooms[i] = (room_t*)calloc(height, sizeof(room_t));
    }

    // Allocate doors
    dungeon->numDoors = (width * (height - 1)) + ((width - 1) * height);
    dungeon->doors    = calloc(dungeon->numDoors, sizeof(door_t));
    int doorIdx       = 0;

    // Connect left/right doors
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width - 1; x++)
        {
            dungeon->rooms[x][y].doors[DOOR_RIGHT]    = &dungeon->doors[doorIdx];
            dungeon->rooms[x + 1][y].doors[DOOR_LEFT] = &dungeon->doors[doorIdx];

            dungeon->doors[doorIdx].rooms[0] = &dungeon->rooms[x][y];
            dungeon->doors[doorIdx].rooms[1] = &dungeon->rooms[x + 1][y];

            doorIdx++;
        }
    }

    // Connect up/down doors
    for (int y = 0; y < height - 1; y++)
    {
        for (int x = 0; x < width; x++)
        {
            dungeon->rooms[x][y].doors[DOOR_DOWN]   = &dungeon->doors[doorIdx];
            dungeon->rooms[x][y + 1].doors[DOOR_UP] = &dungeon->doors[doorIdx];

            dungeon->doors[doorIdx].rooms[0] = &dungeon->rooms[x][y];
            dungeon->doors[doorIdx].rooms[1] = &dungeon->rooms[x][y + 1];

            doorIdx++;
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 */
void freeDungeon(dungeon_t* dungeon)
{
    // Free doors
    free(dungeon->doors);
    // Free columns
    for (int i = 0; i < dungeon->w; i++)
    {
        free(dungeon->rooms[i]);
    }
    // Free rows
    free(dungeon->rooms);
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 */
void connectDungeonEllers(dungeon_t* dungeon)
{
    // Keep track of which set each room belongs to
    int32_t setIdx = 1;

    // For each row of the dungeon
    for (int y = 0; y < dungeon->h; y++)
    {
        // First, assign sets to cells without them
        for (int x = 0; x < dungeon->w; x++)
        {
            if (0 == dungeon->rooms[x][y].set)
            {
                dungeon->rooms[x][y].set = setIdx;
                setIdx++;
            }
        }

#ifdef DBG_PRINT
        printf("Starting row %d\n", y);
        printRow(y, dungeon->w, dungeon->rooms);
#endif

        // Then, randomly create LR walls
        for (int x = 0; x < dungeon->w - 1; x++)
        {
            // 50% chance, but also make walls between cells of the same set to avoid loops
            if ((dungeon->rooms[x][y].set == dungeon->rooms[x + 1][y].set) || (rand() % 2))
            {
                dungeon->rooms[x][y].doors[DOOR_RIGHT]->isDoor = false;

#ifdef DBG_PRINT
                printf("Add LR wall\n");
                printRow(y, dungeon->w, dungeon->rooms);
#endif
            }
            else
            {
                // Mark this as a door
                dungeon->rooms[x][y].doors[DOOR_RIGHT]->isDoor = true;

                // Merge the sets by overwriting all olds with news in this row
                mergeSets(dungeon, x, y);

#ifdef DBG_PRINT
                printf("Add LR door\n");
                printRow(y, dungeon->w, dungeon->rooms);
#endif
            }
        }

        // If this is not the last row
        if (y < (dungeon->h - 1))
        {
            // Keep track of what sets are connected to ensure all are connected
            uint16_t setsWithDoors[dungeon->w];
            uint8_t swdIdx = 0;

            // Randomly create UD walls
            for (int x = 0; x < dungeon->w; x++)
            {
                if (rand() % 2)
                {
                    // If this is a door, mark both cells as part of the same set
                    dungeon->rooms[x][y].doors[DOOR_DOWN]->isDoor = true;
                    dungeon->rooms[x][y + 1].set                  = dungeon->rooms[x][y].set;

                    // Record this set as connected
                    setsWithDoors[swdIdx++] = dungeon->rooms[x][y].set;

#ifdef DBG_PRINT
                    printf("Add UD door\n");
                    printRow(y, dungeon->w, dungeon->rooms);
#endif
                }
                else
                {
                    dungeon->rooms[x][y].doors[DOOR_DOWN]->isDoor = false;

#ifdef DBG_PRINT
                    printf("Add UD wall\n");
                    printRow(y, dungeon->w, dungeon->rooms);
#endif
                }
            }

            // Do a second pass to ensure that all sets are connected somewhere
            for (int x = 0; x < dungeon->w; x++)
            {
                // Check if this set is connected
                bool setHasDoor = false;
                for (int i = 0; i < swdIdx; i++)
                {
                    if (setsWithDoors[i] == dungeon->rooms[x][y].set)
                    {
                        setHasDoor = true;
                        break;
                    }
                }

                // If it is not connected
                if (!setHasDoor)
                {
                    // mark both cells as part of the same set
                    dungeon->rooms[x][y].doors[DOOR_DOWN]->isDoor = true;
                    dungeon->rooms[x][y + 1].set                  = dungeon->rooms[x][y].set;

                    setsWithDoors[swdIdx++] = dungeon->rooms[x][y].set;

#ifdef DBG_PRINT
                    printf("Add UD wall (mandatory)\n");
                    printRow(y, dungeon->w, dungeon->rooms);
#endif
                }
            }
        }
        else
        {
            // Final row, make sure cells are connected
            for (int x = 0; x < dungeon->w - 1; x++)
            {
                if (dungeon->rooms[x][y].set != dungeon->rooms[x + 1][y].set)
                {
                    // Merge the sets by overwriting all olds with news in this row
                    mergeSets(dungeon, x, y);
                    dungeon->rooms[x][y].doors[DOOR_RIGHT]->isDoor = true;

#ifdef DBG_PRINT
                    printf("Add LR wall (last row)\n");
                    printRow(y, dungeon->w, dungeon->rooms);
#endif
                }
            }
        }
#ifdef DBG_PRINT
        printf("Final\n");
        printRow(y, dungeon->w, dungeon->rooms);
        printf("\n\n");
#endif
    }
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 * @param x
 * @param y
 */
void mergeSets(dungeon_t* dungeon, int x, int y)
{
    int newSet, oldSet;
    if (dungeon->rooms[x][y].set < dungeon->rooms[x + 1][y].set)
    {
        newSet = dungeon->rooms[x][y].set;
        oldSet = dungeon->rooms[x + 1][y].set;
    }
    else
    {
        newSet = dungeon->rooms[x + 1][y].set;
        oldSet = dungeon->rooms[x][y].set;
    }

    for (int mergeX = 0; mergeX < dungeon->w; mergeX++)
    {
        if (oldSet == dungeon->rooms[mergeX][y].set)
        {
            dungeon->rooms[mergeX][y].set = newSet;
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param door
 * @return true
 * @return false
 */
bool isLocked(door_t* door)
{
    switch (door->lock)
    {
        case KEY_1:
        case KEY_2:
        case KEY_3:
        case KEY_4:
        case KEY_5:
        case KEY_6:
        case KEY_7:
        case KEY_8:
        case KEY_9:
        case KEY_10:
        case KEY_11:
        case KEY_12:
        case KEY_13:
        case KEY_14:
        case KEY_15:
        case KEY_16:
        {
            return true;
        }
        default:
        {
            return false;
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 */
void clearDungeonDistances(dungeon_t* dungeon)
{
    for (int x = 0; x < dungeon->w; x++)
    {
        for (int y = 0; y < dungeon->h; y++)
        {
            dungeon->rooms[x][y].dist = 0;
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 * @param startX
 * @param startY
 * @param ignoreLocks
 * @return coord_t
 */
coord_t addDistFromRoom(dungeon_t* dungeon, uint16_t startX, uint16_t startY, bool ignoreLocks)
{
    // Mark all cells as not visited
    for (int x = 0; x < dungeon->w; x++)
    {
        for (int y = 0; y < dungeon->h; y++)
        {
            dungeon->rooms[x][y].visited = false;
        }
    }
    // Except for the starting room
    dungeon->rooms[startX][startY].visited = true;

    // Initialize a stack of unvisited rooms
    list_t unvisitedRooms = {0};
    push(&unvisitedRooms, COORDS_TO_LIST(startX, startY));

    // Keep track of the longest distance to the furthest room
    int16_t longestDist  = 0;
    coord_t furthestRoom = {0};

    // For the entire dungeon
    while (unvisitedRooms.length > 0)
    {
        void* poppedVal  = pop(&unvisitedRooms);
        int cX           = LIST_TO_X(poppedVal);
        int cY           = LIST_TO_Y(poppedVal);
        room_t* thisRoom = &(dungeon->rooms[cX][cY]);

        // Check all directions
        for (doorIdx dir = 0; dir < DOOR_MAX; ++dir)
        {
            // If this is an unlocked door
            if (thisRoom->doors[dir] && thisRoom->doors[dir]->isDoor
                && (ignoreLocks || !isLocked(thisRoom->doors[dir])))
            {
                uint16_t nextX   = cX + cardinals[dir].x;
                uint16_t nextY   = cY + cardinals[dir].y;
                room_t* nextRoom = &(dungeon->rooms[nextX][nextY]);
                // If this room hasn't been visited yet
                if (false == nextRoom->visited)
                {
                    // Increment the distance, push it on the stack
                    nextRoom->dist += (thisRoom->dist + 1);
                    nextRoom->visited = true;
                    push(&unvisitedRooms, COORDS_TO_LIST(nextX, nextY));

                    if (nextRoom->dist > longestDist)
                    {
                        longestDist    = nextRoom->dist;
                        furthestRoom.x = nextX;
                        furthestRoom.y = nextY;
                    }
                }
            }
        }
    }
    clear(&unvisitedRooms);

    return furthestRoom;
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 * @param startX
 * @param startY
 * @return int
 */
void countRoomsAfterDoors(dungeon_t* dungeon, uint16_t startX, uint16_t startY)
{
    // Mark all cells as not visited
    for (int x = 0; x < dungeon->w; x++)
    {
        for (int y = 0; y < dungeon->h; y++)
        {
            dungeon->rooms[x][y].visited     = false;
            dungeon->rooms[x][y].numChildren = 0;
        }
    }
    for (int d = 0; d < dungeon->numDoors; d++)
    {
        dungeon->doors[d].numChildren = 0;
    }

    // Build a list of rooms to visit, reverse-depth-first order
    list_t rDepthFirstOrder = {0};
    list_t roomStack        = {0};
    push(&roomStack, &(dungeon->rooms[startX][startY]));
    while (0 != roomStack.length)
    {
        room_t* thisRoom = pop(&roomStack);
        push(&rDepthFirstOrder, thisRoom);

        // Mark this room as visited
        thisRoom->visited = true;

        // Check all directions
        for (doorIdx dir = 0; dir < DOOR_MAX; dir++)
        {
            door_t* door = thisRoom->doors[dir];
            // If this is an unlocked door
            if (door && door->isDoor && !isLocked(door))
            {
                // Get the next room through the door
                room_t* nextRoom = door->rooms[0] == thisRoom ? door->rooms[1] : door->rooms[0];
                // If the next room hasn't been visited yet, push it onto the stack
                if (!nextRoom->visited)
                {
                    push(&roomStack, nextRoom);
                }
            }
        }
    }

    // Mark all cells as not visited, again
    for (int x = 0; x < dungeon->w; x++)
    {
        for (int y = 0; y < dungeon->h; y++)
        {
            dungeon->rooms[x][y].visited = false;
        }
    }

    // Visit all rooms, reverse depth first order
    while (rDepthFirstOrder.length > 0)
    {
        room_t* thisRoom  = pop(&rDepthFirstOrder);
        thisRoom->visited = true;

        // Check all directions
        for (doorIdx dir = 0; dir < DOOR_MAX; dir++)
        {
            door_t* door = thisRoom->doors[dir];
            // If this is an unlocked door
            if (door && door->isDoor && !isLocked(door))
            {
                // Get the other room
                room_t* nextRoom = door->rooms[0] == thisRoom ? door->rooms[1] : door->rooms[0];
                // If it's been visited already (i.e. working backwards)
                if (nextRoom->visited)
                {
                    // Add up the number of children
                    door->numChildren += (1 + nextRoom->numChildren);
                    thisRoom->numChildren += (1 + nextRoom->numChildren);
                }
            }
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 * @param startingRoom
 * @param partition
 */
void setPartitions(dungeon_t* dungeon, room_t* startingRoom, keyType_t partition)
{
    // Mark all cells as not visited
    for (int x = 0; x < dungeon->w; x++)
    {
        for (int y = 0; y < dungeon->h; y++)
        {
            dungeon->rooms[x][y].visited = false;
        }
    }

    list_t roomStack = {0};
    push(&roomStack, startingRoom);

    while (0 != roomStack.length)
    {
        room_t* thisRoom = pop(&roomStack);

        // Mark this room's partition
        thisRoom->partition = partition;
        thisRoom->visited   = true;

        // Check all directions
        for (doorIdx dir = 0; dir < DOOR_MAX; dir++)
        {
            door_t* door = thisRoom->doors[dir];
            // If this is an unlocked door
            if (door && door->isDoor && !isLocked(door))
            {
                // Get the next room through the door
                room_t* nextRoom = door->rooms[0] == thisRoom ? door->rooms[1] : door->rooms[0];
                // If the next room hasn't been visited yet, push it onto the stack
                if (!nextRoom->visited)
                {
                    push(&roomStack, nextRoom);
                }
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param dungeon
 * @param goals
 * @param numKeys
 * @param startRoom
 */
void placeLocks(dungeon_t* dungeon, const keyType_t* goals, int numKeys, coord_t startRoom)
{
    // Place locks by partitioning the dungeon into roughly equal size chunks
    for (uint8_t i = 0; i < numKeys; i++)
    {
        // Figure out how many rooms are behind each door
        countRoomsAfterDoors(dungeon, startRoom.x, startRoom.y);

        // Each partition in the dungeon should be about this size
        int tpSize = (dungeon->rooms[startRoom.x][startRoom.y].numChildren + 1) / (numKeys + 1 - i);

        // Find the door that best partitions the dungeon
        int bestPartitionDiff = dungeon->w * dungeon->h + 1;
        door_t* bestDoor      = NULL;
        for (int d = 0; d < dungeon->numDoors; d++)
        {
            door_t* door = &(dungeon->doors[d]);
            // Try to get this as close to zero as we can
            int partitionDiff = ABS(tpSize - door->numChildren);
            if (partitionDiff < bestPartitionDiff)
            {
                bestPartitionDiff = partitionDiff;
                bestDoor          = door;
            }
        }

        // Lock the door
        bestDoor->lock = goals[numKeys - i - 1];

        // Find the room after the lock
        room_t* roomAfterLock;
        if (bestDoor->rooms[0]->numChildren < bestDoor->numChildren)
        {
            roomAfterLock = bestDoor->rooms[0];
        }
        else
        {
            roomAfterLock = bestDoor->rooms[1];
        }

        // Mark the partition of all rooms after the lock
        setPartitions(dungeon, roomAfterLock, goals[numKeys - i - 1]);
    }
}

/**
 * @brief TODO
 *
 * @param dungeon
 * @param keys
 * @param numKeys
 */
void placeKeys(dungeon_t* dungeon, const keyType_t* keys, int numKeys)
{
    int allRooms[dungeon->w * dungeon->h];
    for (int i = 0; i < dungeon->w * dungeon->h; i++)
    {
        allRooms[i] = i;
    }
    fisherYates(allRooms, dungeon->w * dungeon->h);

    bool keysPlaced[numKeys];
    memset(keysPlaced, 0, sizeof(keysPlaced));

    // Place keys in dead-ends
    for (int i = 0; i < dungeon->w * dungeon->h; i++)
    {
        int x = allRooms[i] % dungeon->w;
        int y = allRooms[i] / dungeon->w;

        room_t* room = &(dungeon->rooms[x][y]);

        // Don't place items in start or end
        if (room->isStart || room->isEnd)
        {
            continue;
        }

        for (int kIdx = 0; kIdx < numKeys; kIdx++)
        {
            if (false == keysPlaced[kIdx])
            {
                if (room->isDeadEnd)
                {
                    bool placeHere = false;
                    if (0 == kIdx)
                    {
                        if (0 == room->partition)
                        {
                            placeHere = true;
                        }
                    }
                    else if (room->partition == keys[kIdx - 1])
                    {
                        placeHere = true;
                    }

                    if (placeHere)
                    {
                        room->treasure   = keys[kIdx];
                        keysPlaced[kIdx] = true;
                        break;
                    }
                }
            }
        }
    }

    // Place keys in non-dead-ends
    for (int i = 0; i < dungeon->w * dungeon->h; i++)
    {
        int x = allRooms[i] % dungeon->w;
        int y = allRooms[i] / dungeon->w;

        room_t* room = &(dungeon->rooms[x][y]);

        // Don't place items in start or end
        if (room->isStart || room->isEnd)
        {
            continue;
        }

        for (int kIdx = 0; kIdx < numKeys; kIdx++)
        {
            if (false == keysPlaced[kIdx])
            {
                bool placeHere = false;
                if (0 == kIdx)
                {
                    if (0 == room->partition)
                    {
                        placeHere = true;
                    }
                }
                else if (room->partition == keys[kIdx - 1])
                {
                    placeHere = true;
                }

                if (placeHere)
                {
                    room->treasure   = keys[kIdx];
                    keysPlaced[kIdx] = true;
                    break;
                }
            }
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param arr
 * @param len
 */
void fisherYates(int* arr, int len)
{
    // Start from the last element and swap one by one. We don't
    // need to run for the first element that's why i > 0
    for (int i = len - 1; i > 0; i--)
    {
        // Pick a random index from 0 to i
        int j = rand() % (i + 1);

        // Swap arr[i] with the element at random index
        int tmp = arr[i];
        arr[i]  = arr[j];
        arr[j]  = tmp;
    }
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 */
void markDeadEnds(dungeon_t* dungeon)
{
    for (int y = 0; y < dungeon->h; y++)
    {
        for (int x = 0; x < dungeon->w; x++)
        {
            int numDoors = 0;
            numDoors += (dungeon->rooms[x][y].doors[DOOR_UP] && dungeon->rooms[x][y].doors[DOOR_UP]->isDoor) ? 1 : 0;
            numDoors
                += (dungeon->rooms[x][y].doors[DOOR_DOWN] && dungeon->rooms[x][y].doors[DOOR_DOWN]->isDoor) ? 1 : 0;
            numDoors
                += (dungeon->rooms[x][y].doors[DOOR_LEFT] && dungeon->rooms[x][y].doors[DOOR_LEFT]->isDoor) ? 1 : 0;
            numDoors
                += (dungeon->rooms[x][y].doors[DOOR_RIGHT] && dungeon->rooms[x][y].doors[DOOR_RIGHT]->isDoor) ? 1 : 0;
            if (1 == numDoors)
            {
                dungeon->rooms[x][y].isDeadEnd = true;
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param dungeon
 * @param startRoom
 * @param finalPartition
 */
void markEnd(dungeon_t* dungeon, coord_t startRoom, keyType_t finalPartition)
{
    clearDungeonDistances(dungeon);
    addDistFromRoom(dungeon, startRoom.x, startRoom.y, true);
    int greatestDist = 0;
    coord_t end      = {
             .x = 0,
             .y = 0,
    };
    for (int y = 0; y < dungeon->h; y++)
    {
        for (int x = 0; x < dungeon->w; x++)
        {
            if (dungeon->rooms[x][y].partition == finalPartition)
            {
                if (dungeon->rooms[x][y].dist > greatestDist)
                {
                    greatestDist = dungeon->rooms[x][y].dist;
                    end.x        = x;
                    end.y        = y;
                }
            }
        }
    }
    dungeon->rooms[end.x][end.y].isEnd = true;
}

#ifdef DBG_PRINT
/**
 * @brief TODO doc
 *
 * @param y
 * @param width
 * @param rooms
 */
void printRow(int y, int width, room_t** rooms)
{
    for (int x = 0; x < width; x++)
    {
        printf("%2d", rooms[x][y].set);
        if (rooms[x][y].doors[DOOR_RIGHT] && !rooms[x][y].doors[DOOR_RIGHT]->isDoor)
        {
            printf("|");
        }
        else
        {
            printf(" ");
        }
    }
    printf("\n");
    for (int x = 0; x < width; x++)
    {
        if (rooms[x][y].doors[DOOR_DOWN] && rooms[x][y].doors[DOOR_DOWN]->isDoor)
        {
            printf("   ");
        }
        else
        {
            printf("---");
        }
    }
    printf("\n");
}
#endif
