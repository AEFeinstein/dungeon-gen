//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>

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

void printRow(int y, int mazeW, mazeCell_t ** maze);
void printStack(list_t * stack);
void fisherYates(doorIdx * arr, int len);
void mergeSets(dungeon_t * dungeon, int x, int y);

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
    int numDoors = (mazeW * (mazeH - 1)) + ((mazeW - 1) * mazeH);
    dungeon->doors = calloc(numDoors, sizeof(door_t));
    int doorIdx = 0;

    // Connect left/right doors
    for(int y = 0; y < mazeH; y++)
    {
        for(int x = 0; x < mazeW - 1; x++)
        {
            dungeon->maze[x    ][y].doors[DOOR_RIGHT] = &dungeon->doors[doorIdx];
            dungeon->maze[x + 1][y].doors[DOOR_LEFT] = &dungeon->doors[doorIdx];
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
 * @param dungeon 
 */
void connectDungeonRecursive(dungeon_t * dungeon)
{
    list_t roomStack = {0};
    coord_t cRoom = {.x = dungeon->w / 2, .y = dungeon->h - 1};
    dungeon->maze[cRoom.x][cRoom.y].type = DUNGEON_START;

    int roomsVisited = 0;
    int distFromStart = 0;
    int furthestDist = 0;
    coord_t furtherstRoom;
    while(roomsVisited < dungeon->w * dungeon->h)
    {
        // Visit this room
        push(&roomStack, (void*) (intptr_t)((cRoom.y * dungeon->w) + cRoom.x) );
        printStack(&roomStack);
        if(false == dungeon->maze[cRoom.x][cRoom.y].visited)
        {
            dungeon->maze[cRoom.x][cRoom.y].visited = true;
            roomsVisited++;
        }

        // Find the next room
        doorIdx choices[] = {DOOR_UP, DOOR_DOWN, DOOR_LEFT, DOOR_RIGHT};
        fisherYates(choices, sizeof(choices) / sizeof(choices[0]));
        bool roomMoved = false;
        for(size_t i = 0; i < sizeof(choices) / sizeof(int); i++)
        {
            switch (choices[i])
            {
                case DOOR_UP:
                {
                    // Go up
                    if(cRoom.y > 0)
                    {
                        if(!dungeon->maze[cRoom.x][cRoom.y - 1].visited)
                        {
                            dungeon->maze[cRoom.x][cRoom.y].doors[choices[i]]->isDoor = true;
                            cRoom.y--;
                            roomMoved = true;
                        }
                    }
                    break;
                }
                case DOOR_DOWN:
                {
                    // Go down
                    if(cRoom.y < dungeon->h - 1)
                    {
                        if(!dungeon->maze[cRoom.x][cRoom.y + 1].visited)
                        {
                            dungeon->maze[cRoom.x][cRoom.y].doors[choices[i]]->isDoor = true;
                            cRoom.y++;
                            roomMoved = true;
                        }
                    }
                    break;
                }
                case DOOR_LEFT:
                {
                    // Go left
                    if(cRoom.x > 0)
                    {
                        if(!dungeon->maze[cRoom.x - 1][cRoom.y].visited)
                        {
                            dungeon->maze[cRoom.x][cRoom.y].doors[choices[i]]->isDoor = true;
                            cRoom.x--;
                            roomMoved = true;
                        }
                    }
                    break;
                }
                case DOOR_RIGHT:
                {
                    // Go right
                    if(cRoom.x < dungeon->w - 1)
                    {
                        if(!dungeon->maze[cRoom.x + 1][cRoom.y].visited)
                        {
                            dungeon->maze[cRoom.x][cRoom.y].doors[choices[i]]->isDoor = true;
                            cRoom.x++;
                            roomMoved = true;
                        }
                    }
                    break;
                }
                case DOOR_MAX:
                {
                    break;
                }
            }

            if(roomMoved)
            {
                // New room was found, so stop looping
                distFromStart++;

                if(distFromStart > furthestDist)
                {
                    furthestDist = distFromStart;
                    furtherstRoom = cRoom;
                }
                break;
            }
        }

        if(!roomMoved)
        {
            distFromStart--;
            // No valid rooms to move to, recursively do stuff
            // Pop this room
            pop(&roomStack);
            // Pop the previous room to move to it
            intptr_t roomIdx = (intptr_t)pop(&roomStack);
            printStack(&roomStack);
            // (intptr_t)peekLast(&roomStack);
            cRoom.x = roomIdx % dungeon->w;
            cRoom.y = roomIdx / dungeon->w;
        }
    }

    dungeon->maze[furtherstRoom.x][furtherstRoom.y].type = DUNGEON_END;

    clear(&roomStack);
}

/**
 * @brief TODO doc
 * 
 * @param maze 
 * @param width 
 * @param height 
 */
void clearDungeonDists(dungeon_t * dungeon)
{
    for(int x = 0; x < dungeon->w; x++)
    {
        for(int y = 0; y < dungeon->h; y++)
        {
            dungeon->maze[x][y].dist = -1;
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
 * @param startX 
 * @param startY 
 * @param longestDist 
 * @return coord_t 
 */
coord_t distFromRoom(dungeon_t * dungeon, uint16_t startX, uint16_t startY, uint16_t * longestDist)
{
    // Initialize a stack of unvisited rooms
    list_t unvisitedRooms = {0};
    push(&unvisitedRooms, COORDS_TO_LIST(startX, startY));
    dungeon->maze[startX][startY].dist = 0;

    // Keep track of the longest distance to the furthest room
    *longestDist = 0;
    coord_t furthestRoom = {0};

    // For the entire maze
    while(unvisitedRooms.length > 0)
    {
        void* poppedVal = pop(&unvisitedRooms);
        int cX = LIST_TO_X(poppedVal);
        int cY = LIST_TO_Y(poppedVal);
        mazeCell_t * thisRoom = &(dungeon->maze[cX][cY]);

        // Check all directions
        for(doorIdx dir = 0; dir < DOOR_MAX; dir++)
        {
            // If this is a door
            if(thisRoom->doors[dir] && thisRoom->doors[dir]->isDoor && !isLocked(thisRoom->doors[dir]))
            {
                uint16_t nextX = cX + cardinals[dir].x;
                uint16_t nextY = cY + cardinals[dir].y;
                mazeCell_t * nextRoom = &(dungeon->maze[nextX][nextY]);
                // If this room hasn't been visited yet
                if(nextRoom->dist < 0)
                {
                    // Assign a distance, push it on the stack
                    nextRoom->dist = thisRoom->dist + 1;
                    push(&unvisitedRooms, COORDS_TO_LIST(nextX, nextY));

                    if(nextRoom->dist > *longestDist)
                    {
                        *longestDist = nextRoom->dist;
                        furthestRoom.x = nextX;
                        furthestRoom.y = nextY;
                    }
                }
            }
        }
    }
    clear(&unvisitedRooms);

    printf("Longest dist %d\n", *longestDist);

    return furthestRoom;
}

/**
 * @brief TODO doc
 * 
 * @param dungeon 
 * @param x0 
 * @param y0 
 * @param x1 
 * @param y1 
 * @param pathLen 
 * @param type 
 * @return true 
 * @return false 
 */
bool markPath(dungeon_t * dungeon, int x0, int y0, int x1, int y1, int pathLen, roomType_t type)
{
    // Start at the end, walk backwards
    int cX = x1;
    int cY = y1;

    // Place a locked door halfway on the path
    int lockPos = pathLen / 2 + 1;
    int pathIdx = 0;

    // Keep walking until you get to the start
    while(cX != x0 || cY != y0)
    {
        mazeCell_t * thisRoom = &dungeon->maze[cX][cY];

        // If this happens, it's impossible to walk a path
        if(thisRoom->dist < 0)
        {
            return false;
        }

        // Check all directions
        for(doorIdx dir = 0; dir < DOOR_MAX; dir++)
        {
            // If this is a door
            if(thisRoom->doors[dir] && thisRoom->doors[dir]->isDoor)
            {
                uint16_t nextX = cX + cardinals[dir].x;
                uint16_t nextY = cY + cardinals[dir].y;
                mazeCell_t * nextRoom = &(dungeon->maze[nextX][nextY]);
                // If this room is closer to the start than we are now
                if(nextRoom->dist < thisRoom->dist)
                {
                    // Increment the path length
                    pathIdx++;
                    // Check if a lock should be placed
                    if(pathIdx == lockPos)
                    {
                        thisRoom->doors[dir]->type = type;
                    }

                    // Mark this room on the path
                    nextRoom->isOnPath = true;
                    cX = nextX;
                    cY = nextY;
                    break;
                }
            }
        }
    }
    return true;
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
            return 0xFF00FF00;
        }
        case KEY_1:
        {
            return 0xFFFF8040;
        }
        case KEY_2:
        {
            return 0xFF4080FF;
        }
    }
    return 0x00000000;
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
            int numDoors = 0;
            numDoors += (dungeon->maze[x][y].doors[DOOR_UP] && dungeon->maze[x][y].doors[DOOR_UP]->isDoor) ? 1 : 0;
            numDoors += (dungeon->maze[x][y].doors[DOOR_DOWN] && dungeon->maze[x][y].doors[DOOR_DOWN]->isDoor) ? 1 : 0;
            numDoors += (dungeon->maze[x][y].doors[DOOR_LEFT] && dungeon->maze[x][y].doors[DOOR_LEFT]->isDoor) ? 1 : 0;
            numDoors += (dungeon->maze[x][y].doors[DOOR_RIGHT] && dungeon->maze[x][y].doors[DOOR_RIGHT]->isDoor) ? 1 : 0;
            if(1 == numDoors && EMPTY_ROOM == dungeon->maze[x][y].type)
            {
                dungeon->maze[x][y].type = DEAD_END;
            }

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
                        data[pxIdx] = roomColor(dungeon->maze[x][y].type);
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

/**
 * @brief TODO doc
 * 
 * @param stack 
 */
void printStack(list_t * stack)
{
    node_t * node = stack->first;
    while(node)
    {
        printf("%" PRIxPTR ", ", (intptr_t)node->val);
        node = node->next;
    }
    printf("\n");
}

/**
 * @brief TODO doc
 * 
 * @param arr 
 * @param len 
 */
void fisherYates(doorIdx * arr, int len)
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