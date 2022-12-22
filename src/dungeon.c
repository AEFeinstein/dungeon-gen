//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "linked_list.h"
#include "dungeon.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    DOOR_UP,
    DOOR_DOWN,
    DOOR_LEFT,
    DOOR_RIGHT,
    DOOR_MAX
} doorIdx;

//==============================================================================
// Function prototypes
//==============================================================================

#ifdef DBG_PRINT
void printRow(int y, int mazeW, mazeCell_t ** maze);
#endif

void mergeSets(dungeon_t * dungeon, int x, int y);
void fisherYates(int * arr, int len);
uint32_t roomColor(roomType_t type);

//==============================================================================
// Defines
//==============================================================================

#define COORDS_TO_LIST(x, y) ((void*)((intptr_t)(((x) << 16) | (y))))
#define LIST_TO_X(c) ((((intptr_t)(c)) >> 16) & 0xFFFF)
#define LIST_TO_Y(c) ((((intptr_t)(c)) >>  0) & 0xFFFF)

//==============================================================================
// Variables
//==============================================================================

// Convenience directions
const coord_t cardinals[DOOR_MAX] =
{
    {.x =  0, .y = -1}, // DOOR_UP
    {.x =  0, .y =  1}, // DOOR_DOWN
    {.x = -1, .y =  0}, // DOOR_LEFT
    {.x =  1, .y =  0}  // DOOR_RIGHT
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 * 
 * @param dungeon 
 * @param mazeW 
 * @param mazeH 
 */
void initDungeon(dungeon_t * dungeon, int mazeW, int mazeH)
{
    // Save width and height
    dungeon->w = mazeW;
    dungeon->h = mazeH;

    // Allocate rows
    dungeon->maze = (mazeCell_t**)calloc(mazeW, sizeof(mazeCell_t*));
    // Allocate columns
    for(int i = 0; i < mazeW; i++)
    {
        dungeon->maze[i] = (mazeCell_t*)calloc(mazeH, sizeof(mazeCell_t));
    }

    // Allocate doors
    dungeon->numDoors = (mazeW * (mazeH - 1)) + ((mazeW - 1) * mazeH);
    dungeon->doors = calloc(dungeon->numDoors, sizeof(door_t));
    int doorIdx = 0;

    // Number the rooms
    for(int y = 0; y < mazeH; y++)
    {
        for(int x = 0; x < mazeW; x++)
        {
            dungeon->maze[x][y].x = x;
            dungeon->maze[x][y].y = y;
        }
    }

    // Connect left/right doors
    for(int y = 0; y < mazeH; y++)
    {
        for(int x = 0; x < mazeW - 1; x++)
        {
            dungeon->maze[x    ][y].doors[DOOR_RIGHT] = &dungeon->doors[doorIdx];
            dungeon->maze[x + 1][y].doors[DOOR_LEFT] = &dungeon->doors[doorIdx];

            dungeon->doors[doorIdx].rooms[0] = &dungeon->maze[x    ][y];
            dungeon->doors[doorIdx].rooms[1] = &dungeon->maze[x + 1][y];

            doorIdx++;
        }
    }

    // Connect up/down doors
    for(int y = 0; y < mazeH - 1; y++)
    {
        for(int x = 0; x < mazeW; x++)
        {
            dungeon->maze[x][y    ].doors[DOOR_DOWN] = &dungeon->doors[doorIdx];
            dungeon->maze[x][y + 1].doors[DOOR_UP] = &dungeon->doors[doorIdx];

            dungeon->doors[doorIdx].rooms[0] = &dungeon->maze[x][y    ];
            dungeon->doors[doorIdx].rooms[1] = &dungeon->maze[x][y + 1];

            doorIdx++;
        }
    }
}

/**
 * @brief TODO doc
 * 
 * @param dungeon 
 */
void freeDungeon(dungeon_t * dungeon)
{
    // Free doors
    free(dungeon->doors);
    // Free columns
    for(int i = 0; i < dungeon->w; i++)
    {
        free(dungeon->maze[i]);
    }
    // Free rows
    free(dungeon->maze);
}

/**
 * @brief TODO doc
 * 
 * @param dungeon 
 */
void connectDungeonEllers(dungeon_t * dungeon)
{
    // Keep track of which set each room belongs to
    int32_t setIdx = 1;

    // For each row of the maze
    for(int y = 0; y < dungeon->h; y++)
    {
        // First, assign sets to cells without them 
        for(int x = 0; x < dungeon->w; x++)
        {
            if(0 == dungeon->maze[x][y].set)
            {
                dungeon->maze[x][y].set = setIdx;
                setIdx++;
            }
        }

#ifdef DBG_PRINT
        printf("Starting row %d\n", y);
        printRow(y, dungeon->w, dungeon->maze);
#endif

        // Then, randomly create LR walls
        for(int x = 0; x < dungeon->w - 1; x++)
        {
            // 50% chance, but also make walls between cells of the same set to avoid loops
            if((dungeon->maze[x][y].set == dungeon->maze[x+1][y].set) || (rand() % 2))
            {
                dungeon->maze[x][y].doors[DOOR_RIGHT]->isDoor = false;

#ifdef DBG_PRINT
                printf("Add LR wall\n");
                printRow(y, dungeon->w, dungeon->maze);
#endif
            }
            else
            {
                // Mark this as a door
                dungeon->maze[x][y].doors[DOOR_RIGHT]->isDoor = true;

                // Merge the sets by overwriting all olds with news in this row
                mergeSets(dungeon, x, y);

#ifdef DBG_PRINT
                printf("Add LR door\n");
                printRow(y, dungeon->w, dungeon->maze);
#endif
            }
        }

        // If this is not the last row
        if(y < (dungeon->h - 1))
        {
            // Keep track of what sets are connected to ensure all are connected
            uint16_t setsWithDoors[dungeon->w];
            uint8_t swdIdx = 0;

            // Randomly create UD walls
            for(int x = 0; x < dungeon->w; x++)
            {
                if(rand() % 2)
                {
                    // If this is a door, mark both cells as part of the same set
                    dungeon->maze[x][y].doors[DOOR_DOWN]->isDoor = true;
                    dungeon->maze[x][y + 1].set = dungeon->maze[x][y].set;

                    // Record this set as connected
                    setsWithDoors[swdIdx++] = dungeon->maze[x][y].set;

#ifdef DBG_PRINT
                    printf("Add UD door\n");
                    printRow(y, dungeon->w, dungeon->maze);
#endif
                }
                else
                {
                    dungeon->maze[x][y].doors[DOOR_DOWN]->isDoor = false;

#ifdef DBG_PRINT
                    printf("Add UD wall\n");
                    printRow(y, dungeon->w, dungeon->maze);
#endif
                }
            }

            // Do a second pass to ensure that all sets are connected somewhere
            for(int x = 0; x < dungeon->w; x++)
            {
                // Check if this set is connected
                bool setHasDoor = false;
                for(int i = 0; i < swdIdx; i++)
                {
                    if(setsWithDoors[i] == dungeon->maze[x][y].set)
                    {
                        setHasDoor = true;
                        break;
                    }
                }

                // If it is not connected
                if(!setHasDoor)
                {
                    // mark both cells as part of the same set
                    dungeon->maze[x][y].doors[DOOR_DOWN]->isDoor = true;
                    dungeon->maze[x][y + 1].set = dungeon->maze[x][y].set;

                    setsWithDoors[swdIdx++] = dungeon->maze[x][y].set;

#ifdef DBG_PRINT
                    printf("Add UD wall (mandatory)\n");
                    printRow(y, dungeon->w, dungeon->maze);
#endif
                }
            }
        }
        else
        {
            // Final row, make sure cells are connected
            for(int x = 0; x < dungeon->w - 1; x++)
            {
                if(dungeon->maze[x][y].set != dungeon->maze[x+1][y].set)
                {
                    // Merge the sets by overwriting all olds with news in this row
                    mergeSets(dungeon, x, y);
                    dungeon->maze[x][y].doors[DOOR_RIGHT]->isDoor = true;

#ifdef DBG_PRINT
                    printf("Add LR wall (last row)\n");
                    printRow(y, dungeon->w, dungeon->maze);
#endif
                }
            }
        }
#ifdef DBG_PRINT
        printf("Final\n");
        printRow(y, dungeon->w, dungeon->maze);
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
void mergeSets(dungeon_t * dungeon, int x, int y)
{
	int newSet, oldSet;
	if (dungeon->maze[x][y].set < dungeon->maze[x+1][y].set)
	{
		newSet = dungeon->maze[x][y].set;
		oldSet = dungeon->maze[x+1][y].set;
	}
	else
	{
		newSet = dungeon->maze[x+1][y].set;
		oldSet = dungeon->maze[x][y].set;
	}

	for(int mergeX = 0; mergeX < dungeon->w; mergeX++)
	{
		if(oldSet == dungeon->maze[mergeX][y].set)
		{
			dungeon->maze[mergeX][y].set = newSet;
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
bool isLocked(door_t * door)
{
    switch(door->type)
    {
        case KEY_1:
        case KEY_2:
        case KEY_3:
        case KEY_4:
        case KEY_5:
        case KEY_6:
        case KEY_7:
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
void clearDungeonDists(dungeon_t * dungeon)
{
    for(int x = 0; x < dungeon->w; x++)
    {
        for(int y = 0; y < dungeon->h; y++)
        {
            dungeon->maze[x][y].dist = 0;
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
coord_t addDistFromRoom(dungeon_t * dungeon, uint16_t startX, uint16_t startY, bool ignoreLocks)
{
    // Mark all cells as not visited
    for(int x = 0; x < dungeon->w; x++)
    {
        for(int y = 0; y < dungeon->h; y++)
        {
            dungeon->maze[x][y].visited = false;
        }
    }
    // Except for the starting room
    dungeon->maze[startX][startY].visited = true;

    // Initialize a stack of unvisited rooms
    list_t unvisitedRooms = {0};
    push(&unvisitedRooms, COORDS_TO_LIST(startX, startY));

    // Keep track of the longest distance to the furthest room
    int16_t longestDist = 0;
    coord_t furthestRoom = {0};

    // For the entire maze
    while(unvisitedRooms.length > 0)
    {
        void* poppedVal = pop(&unvisitedRooms);
        int cX = LIST_TO_X(poppedVal);
        int cY = LIST_TO_Y(poppedVal);
        mazeCell_t * thisRoom = &(dungeon->maze[cX][cY]);

        // Check all directions
        for(doorIdx dir = 0; dir < DOOR_MAX; ++dir)
        {
            // If this is an unlocked door
            if(thisRoom->doors[dir] && thisRoom->doors[dir]->isDoor && (ignoreLocks || !isLocked(thisRoom->doors[dir])))
            {
                uint16_t nextX = cX + cardinals[dir].x;
                uint16_t nextY = cY + cardinals[dir].y;
                mazeCell_t * nextRoom = &(dungeon->maze[nextX][nextY]);
                // If this room hasn't been visited yet
                if(false == nextRoom->visited)
                {
                    // Increment the distance, push it on the stack
                    nextRoom->dist += (thisRoom->dist + 1);
                    nextRoom->visited = true;
                    push(&unvisitedRooms, COORDS_TO_LIST(nextX, nextY));

                    if(nextRoom->dist > longestDist)
                    {
                        longestDist = nextRoom->dist;
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
void countRoomsAfterDoors(dungeon_t * dungeon, uint16_t startX, uint16_t startY)
{
    // Mark all cells as not visited
    for(int x = 0; x < dungeon->w; x++)
    {
        for(int y = 0; y < dungeon->h; y++)
        {
            dungeon->maze[x][y].visited = false;
            dungeon->maze[x][y].numChildren = 0;
        }
    }
    for(int d = 0; d < dungeon->numDoors; d++)
    {
        dungeon->doors[d].numChildren = 0;
    }

    // Build a list of rooms to visit, reverse-depth-first order 
    list_t rDepthFirstOrder = {0};
    list_t roomStack = {0};
    push(&roomStack, &(dungeon->maze[startX][startY]));
    while(0 != roomStack.length)
    {
        mazeCell_t * thisRoom = pop(&roomStack);
        push(&rDepthFirstOrder, thisRoom);

        // Mark this room as visited
        thisRoom->visited = true;

        // Check all directions
        for(doorIdx dir = 0; dir < DOOR_MAX; dir++)
        {
            door_t * door = thisRoom->doors[dir];
            // If this is an unlocked door
            if( door && door->isDoor && !isLocked(door))
            {
                // Get the next room through the door
                mazeCell_t * nextRoom = door->rooms[0] == thisRoom ? door->rooms[1] : door->rooms[0];
                // If the next room hasn't been visited yet, push it onto the stack
                if(!nextRoom->visited)
                {
                    push(&roomStack, nextRoom);
                }
            }
        }
    }

    // Mark all cells as not visited, again
    for(int x = 0; x < dungeon->w; x++)
    {
        for(int y = 0; y < dungeon->h; y++)
        {
            dungeon->maze[x][y].visited = false;
        }
    }
    
    // Visit all rooms, reverse depth first order
    while(rDepthFirstOrder.length > 0)
    {
        mazeCell_t * thisRoom = pop(&rDepthFirstOrder);
        thisRoom->visited = true;

        // Check all directions
        for(doorIdx dir = 0; dir < DOOR_MAX; dir++)
        {
            door_t * door = thisRoom->doors[dir];
            // If this is an unlocked door
            if( door && door->isDoor && !isLocked(door))
            {
                // Get the other room
                mazeCell_t * nextRoom = door->rooms[0] == thisRoom ? door->rooms[1] : door->rooms[0];
                // If it's been visited already (i.e. working backwards)
                if(nextRoom->visited)
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
void setPartitions(dungeon_t * dungeon, mazeCell_t * startingRoom, roomType_t partition)
{
    // Mark all cells as not visited
    for(int x = 0; x < dungeon->w; x++)
    {
        for(int y = 0; y < dungeon->h; y++)
        {
            dungeon->maze[x][y].visited = false;
        }
    }

    list_t roomStack = {0};
    push(&roomStack, startingRoom);

    while(0 != roomStack.length)
    {
        mazeCell_t * thisRoom = pop(&roomStack);

        // Mark this room's partition
        thisRoom->partition = partition;
        thisRoom->visited = true;

        // Check all directions
        for(doorIdx dir = 0; dir < DOOR_MAX; dir++)
        {
            door_t * door = thisRoom->doors[dir];
            // If this is an unlocked door
            if( door && door->isDoor && !isLocked(door))
            {
                // Get the next room through the door
                mazeCell_t * nextRoom = door->rooms[0] == thisRoom ? door->rooms[1] : door->rooms[0];
                // If the next room hasn't been visited yet, push it onto the stack
                if(!nextRoom->visited)
                {
                    push(&roomStack, nextRoom);
                }
            }
        }
    }
}

/**
 * @brief
 * 
 * @param dungeon
 * @param keys
 * @param numKeys
 */
void placeKeys(dungeon_t * dungeon, const roomType_t * keys, int numKeys)
{
    int allRooms[dungeon->w * dungeon->h];
    for(int i = 0; i < dungeon->w * dungeon->h; i++)
    {
        allRooms[i] = i;
    }
    fisherYates(allRooms, dungeon->w * dungeon->h);

    bool keysPlaced[numKeys];
    memset(keysPlaced, 0, sizeof(keysPlaced));

    // Place keys in dead-ends
    for(int i = 0; i < dungeon->w * dungeon->h; i++)
    {
        int x = allRooms[i] % dungeon->w;
        int y = allRooms[i] / dungeon->w;

        for(int kIdx = numKeys - 1; kIdx >= 0; kIdx--)
        {
            if(false == keysPlaced[kIdx])
            {
                if((DEAD_END == dungeon->maze[x][y].type) && dungeon->maze[x][y].partition == (keys[kIdx] - 1))
                {
                    dungeon->maze[x][y].type = keys[kIdx];
                    keysPlaced[kIdx] = true;
                    break;
                }
            }
        }
    }

    // Place keys in non-dead-ends
    for(int i = 0; i < dungeon->w * dungeon->h; i++)
    {
        int x = allRooms[i] % dungeon->w;
        int y = allRooms[i] / dungeon->w;

        for(int kIdx = numKeys - 1; kIdx >= 0; kIdx--)
        {
            if(false == keysPlaced[kIdx])
            {
                if(dungeon->maze[x][y].partition < keys[kIdx])
                {
                    dungeon->maze[x][y].type = keys[kIdx];
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
void fisherYates(int * arr, int len)
{
    // Start from the last element and swap one by one. We don't
    // need to run for the first element that's why i > 0
    for (int i = len-1; i > 0; i--)
    {
        // Pick a random index from 0 to i
        int j = rand() % (i+1);

        // Swap arr[i] with the element at random index
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 */
void markDeadEnds(dungeon_t * dungeon)
{
    for(int y = 0; y < dungeon->h; y++)
    {
        for(int x = 0; x < dungeon->w; x++)
        {
            int numDoors = 0;
            numDoors += (dungeon->maze[x][y].doors[DOOR_UP] && dungeon->maze[x][y].doors[DOOR_UP]->isDoor) ? 1 : 0;
            numDoors += (dungeon->maze[x][y].doors[DOOR_DOWN] && dungeon->maze[x][y].doors[DOOR_DOWN]->isDoor) ? 1 : 0;
            numDoors += (dungeon->maze[x][y].doors[DOOR_LEFT] && dungeon->maze[x][y].doors[DOOR_LEFT]->isDoor) ? 1 : 0;
            numDoors += (dungeon->maze[x][y].doors[DOOR_RIGHT] && dungeon->maze[x][y].doors[DOOR_RIGHT]->isDoor) ? 1 : 0;
            if(1 == numDoors && EMPTY_ROOM == dungeon->maze[x][y].type)
            {
                dungeon->maze[x][y].type = DEAD_END;
            }
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param dungeon
 */
void saveDungeonPng(dungeon_t * dungeon)
{
    #define ROOM_SIZE 5
    uint32_t * data = calloc(dungeon->w * dungeon->h * ROOM_SIZE * ROOM_SIZE, sizeof(uint32_t));

    for(int y = 0; y < dungeon->h; y++)
    {
        for(int x = 0; x < dungeon->w; x++)
        {
            int roomIdx = ((y * ROOM_SIZE) * (dungeon->w * ROOM_SIZE)) + (x * ROOM_SIZE);
            for(int roomY = 0; roomY < ROOM_SIZE; roomY++)
            {
                for(int roomX = 0; roomX < ROOM_SIZE; roomX++)
                {
                    int pxIdx = roomIdx + (roomY * (dungeon->w * ROOM_SIZE)) + roomX;
                    if(0 == roomY || 0 == roomX || ROOM_SIZE-1 == roomY || ROOM_SIZE-1 == roomX)
                    {
                        data[pxIdx] = 0xFF000000;
                        if(dungeon->maze[x][y].doors[DOOR_UP] && dungeon->maze[x][y].doors[DOOR_UP]->isDoor) // up
                        {
                            if(0 == roomY && ROOM_SIZE/2 == roomX)
                            {
                                data[pxIdx] = roomColor(dungeon->maze[x][y].doors[DOOR_UP]->type);
                            }
                        }

                        if(dungeon->maze[x][y].doors[DOOR_DOWN] && dungeon->maze[x][y].doors[DOOR_DOWN]->isDoor) // down
                        {
                            if(ROOM_SIZE-1 == roomY && ROOM_SIZE/2 == roomX)
                            {
                                data[pxIdx] = roomColor(dungeon->maze[x][y].doors[DOOR_DOWN]->type);
                            }
                        }

                        if(dungeon->maze[x][y].doors[DOOR_LEFT] && dungeon->maze[x][y].doors[DOOR_LEFT]->isDoor) // left
                        {
                            if(0 == roomX && ROOM_SIZE/2 == roomY)
                            {
                                data[pxIdx] = roomColor(dungeon->maze[x][y].doors[DOOR_LEFT]->type);
                            }
                        }

                        if(dungeon->maze[x][y].doors[DOOR_RIGHT] && dungeon->maze[x][y].doors[DOOR_RIGHT]->isDoor) // right
                        {
                            if(ROOM_SIZE-1 == roomX && ROOM_SIZE/2 == roomY)
                            {
                                data[pxIdx] = roomColor(dungeon->maze[x][y].doors[DOOR_RIGHT]->type);
                            }
                        }
                    }
                    else
                    {
                        data[pxIdx] = roomColor(dungeon->maze[x][y].partition);
                        if(EMPTY_ROOM != dungeon->maze[x][y].type)
                        {
                            if((roomX == ROOM_SIZE / 2) && (roomY == ROOM_SIZE / 2))
                            {
                                data[pxIdx] = roomColor(dungeon->maze[x][y].type);
                            }
                        }
                    }
                }
            }
        }
    }

    stbi_write_png("maze.png", dungeon->w * ROOM_SIZE, dungeon->h * ROOM_SIZE, 4, data, 4 * dungeon->w * ROOM_SIZE);
    free(data);
}

/**
 * @brief TODO doc
 *
 * @param type
 * @return uint32_t
 */
uint32_t roomColor(roomType_t type)
{
    switch(type)
    {
        case EMPTY_ROOM:
        {
            return 0xFFFFFFFF;
        }
        case DUNGEON_START:
        {
            return 0xFFFF0000;
        }
        case DUNGEON_END:
        {
            return 0xFF0000FF;
        }
        case DEAD_END:
        {
            return 0xFF000000;
        }
        case KEY_1:
        {
            return 0xFF7766EE;
        }
        case KEY_2:
        {
            return 0xFF338822;
        }
        case KEY_3:
        {
            return 0xFFAA7744;
        }
        case KEY_4:
        {
            return 0xFF44BBCC;
        }
        case KEY_5:
        {
            return 0xFFEECC66;
        }
        case KEY_6:
        {
            return 0xFF7733AA;
        }
        case KEY_7:
        {
            return 0xFFBBBBBB;
        }
    }
    return 0x00000000;
}

#ifdef DBG_PRINT
/**
 * @brief TODO doc
 * 
 * @param y 
 * @param mazeW 
 * @param maze 
 */
void printRow(int y, int mazeW, mazeCell_t ** maze)
{
    for(int x = 0; x < mazeW; x++)
    {
        printf("%2d", maze[x][y].set);
        if(maze[x][y].doors[DOOR_RIGHT] && !maze[x][y].doors[DOOR_RIGHT]->isDoor)
        {
            printf("|");
        }
        else
        {
            printf(" ");
        }
    }
    printf("\n");
    for(int x = 0; x < mazeW; x++)
    {
        if(maze[x][y].doors[DOOR_DOWN] && maze[x][y].doors[DOOR_DOWN]->isDoor)
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
