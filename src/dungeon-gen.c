#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "dungeon.h"
#include "linked_list.h"
#include "rmdDungeonWriter.h"
#include "pngDungeonWriter.h"

// Sizes of dungeons in Link's Awakening
// Tail Cave		25
// Bottle Grotto	26
// Key Cavern		29
// Anglers Tunnel	28
// Catfish's Maw	34
// Face Shrine		40
// Eagle's Tower	34
// Turtle Rock		46

/**
 * @brief Helper function to print instructions and exit
 *
 * @param progName
 */
void printAndExit(char* progName)
{
    fprintf(stderr, "Usage: %s [-w width] [-h height] [-s room_size] [-k key_string] [-c carve_walls]\n", progName);
    fprintf(stderr, "    key_string represents the type and order of keys placed in the map.\n");
    fprintf(stderr, "    key_string may not contain duplicate chars.\n");
    fprintf(stderr, "    key_string is a list of the following chars.\n");
    fprintf(stderr, "        g   = gun\n");
    fprintf(stderr, "        c   = charge\n");
    fprintf(stderr, "        m   = missile\n");
    fprintf(stderr, "        i   = ice\n");
    fprintf(stderr, "        x   = xray\n");
    fprintf(stderr, "        l   = lava\n");
    fprintf(stderr, "        w   = water\n");
    fprintf(stderr, "        0-9 = small key\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief Main function
 *
 * @param argc count of arguments
 * @param argv Array of string arguments
 * @return EXIT_FAILURE if there was an error, EXIT_SUCCESS if all is good
 */
int main(int argc, char** argv)
{
    // Dungeon size
    int width       = 0;
    int height      = 0;
    int roomSize    = 0;
    bool carveWalls = false;
    // Key type and order
    char* keyStr = NULL;

    // Read arguments
    int opt;
    while ((opt = getopt(argc, argv, "w:h:s:k:c")) != -1)
    {
        switch (opt)
        {
            case 'w':
            {
                width = atoi(optarg);
                break;
            }
            case 'h':
            {
                height = atoi(optarg);
                break;
            }
            case 's':
            {
                roomSize = atoi(optarg);
                break;
            }
            case 'k':
            {
                keyStr = optarg;
                break;
            }
            case 'c':
            {
                carveWalls = true;
                break;
            }
            default:
            {
                printAndExit(argv[0]);
            }
        }
    }

    // Make sure all arguments are supplied
    if (0 == width || 0 == height || 0 == roomSize || NULL == keyStr)
    {
        printAndExit(argv[0]);
    }

    // Translate the key string to a list of keys
    int numKeys = strlen(keyStr);
    keyType_t goals[numKeys];
    for (int kIdx = 0; kIdx < numKeys; kIdx++)
    {
        keyStr[kIdx] = tolower(keyStr[kIdx]);
        switch (keyStr[kIdx])
        {
            case 'g':
            {
                // KEY_1 is beam
                goals[kIdx] = KEY_1;
                break;
            }
            case 'c':
            {
                // KEY_1 is charge beam
                goals[kIdx] = KEY_2;
                break;
            }
            case 'm':
            {
                // KEY_2 is missiles
                goals[kIdx] = KEY_3;
                break;
            }
            case 'l':
            {
                // KEY_3 is lava suit
                goals[kIdx] = KEY_4;
                break;
            }
            case 'i':
            {
                // KEY_4 is ice beam
                goals[kIdx] = KEY_5;
                break;
            }
            case 'w':
            {
                // KEY_5 is water suit
                goals[kIdx] = KEY_6;
                break;
            }
            case 'x':
            {
                // KEY_6 is xray visor
                goals[kIdx] = KEY_7;
                break;
            }
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                // Numerals are KEY_8 through KEY_17
                goals[kIdx] = KEY_8 + (keyStr[kIdx] - '0');
                break;
            }
            default:
            {
                printAndExit(argv[0]);
            }
        }
    }

    // Use a different seed value so that we don't get same
    // result each time we run this program
    srand(time(NULL));

    // Create and connect dungeon
    dungeon_t dungeon;
    initDungeon(&dungeon, width, height);
    connectDungeonEllers(&dungeon);

    // Place the start
    coord_t startRoom = {
        .x = width / 2,
        .y = height - 1,
    };
    dungeon.rooms[startRoom.x][startRoom.y].isStart = true;

    // Place locks to partition the dungeon
    placeLocks(&dungeon, goals, numKeys, startRoom);

    // Mark dead ends
    markDeadEnds(&dungeon);

    // Place the keys randomly, in accessible locations
    placeKeys(&dungeon, goals, numKeys);

    // Mark the end, which is the furthest room in the last partition
    markEnd(&dungeon, startRoom, goals[numKeys - 1]);

    // Save the image
    saveDungeonPng(&dungeon);

    // Save as RMD
    saveDungeonRmd(&dungeon, roomSize, carveWalls);

    // Free everything
    freeDungeon(&dungeon);

    // Exit
    exit(EXIT_SUCCESS);
}
