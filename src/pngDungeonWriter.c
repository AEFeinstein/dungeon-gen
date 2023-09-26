#include <stdlib.h>

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "pngDungeonWriter.h"

#define ROOM_SIZE 5

static uint32_t roomColor(keyType_t type, bool isStart, bool isEnd, bool isDeadEnd);

/**
 * @brief Save a dungeon as a PNG image
 *
 * @param dungeon The dungeon to save
 */
void saveDungeonPng(dungeon_t* dungeon)
{
    uint32_t* data = calloc(dungeon->w * dungeon->h * ROOM_SIZE * ROOM_SIZE, sizeof(uint32_t));

    for (int y = 0; y < dungeon->h; y++)
    {
        for (int x = 0; x < dungeon->w; x++)
        {
            int roomIdx = ((y * ROOM_SIZE) * (dungeon->w * ROOM_SIZE)) + (x * ROOM_SIZE);
            for (int roomY = 0; roomY < ROOM_SIZE; roomY++)
            {
                for (int roomX = 0; roomX < ROOM_SIZE; roomX++)
                {
                    int pxIdx = roomIdx + (roomY * (dungeon->w * ROOM_SIZE)) + roomX;
                    if (0 == roomY || 0 == roomX || ROOM_SIZE - 1 == roomY || ROOM_SIZE - 1 == roomX)
                    {
                        data[pxIdx] = 0xFF000000;
                        if (dungeon->rooms[x][y].doors[DOOR_UP] && dungeon->rooms[x][y].doors[DOOR_UP]->isDoor) // up
                        {
                            if (0 == roomY && ROOM_SIZE / 2 == roomX)
                            {
                                data[pxIdx] = roomColor(dungeon->rooms[x][y].doors[DOOR_UP]->lock, false, false, false);
                            }
                        }

                        if (dungeon->rooms[x][y].doors[DOOR_DOWN]
                            && dungeon->rooms[x][y].doors[DOOR_DOWN]->isDoor) // down
                        {
                            if (ROOM_SIZE - 1 == roomY && ROOM_SIZE / 2 == roomX)
                            {
                                data[pxIdx]
                                    = roomColor(dungeon->rooms[x][y].doors[DOOR_DOWN]->lock, false, false, false);
                            }
                        }

                        if (dungeon->rooms[x][y].doors[DOOR_LEFT]
                            && dungeon->rooms[x][y].doors[DOOR_LEFT]->isDoor) // left
                        {
                            if (0 == roomX && ROOM_SIZE / 2 == roomY)
                            {
                                data[pxIdx]
                                    = roomColor(dungeon->rooms[x][y].doors[DOOR_LEFT]->lock, false, false, false);
                            }
                        }

                        if (dungeon->rooms[x][y].doors[DOOR_RIGHT]
                            && dungeon->rooms[x][y].doors[DOOR_RIGHT]->isDoor) // right
                        {
                            if (ROOM_SIZE - 1 == roomX && ROOM_SIZE / 2 == roomY)
                            {
                                data[pxIdx]
                                    = roomColor(dungeon->rooms[x][y].doors[DOOR_RIGHT]->lock, false, false, false);
                            }
                        }
                    }
                    else
                    {
                        room_t* room = &dungeon->rooms[x][y];
                        data[pxIdx]  = roomColor(room->partition, room->isStart, room->isEnd, false);
                        if ((roomX == ROOM_SIZE / 2) && (roomY == ROOM_SIZE / 2))
                        {
                            if (EMPTY_ROOM != dungeon->rooms[x][y].treasure || room->isStart || room->isEnd
                                || room->isDeadEnd)
                            {
                                data[pxIdx] = roomColor(room->treasure, room->isStart, room->isEnd, room->isDeadEnd);
                            }
                        }
                    }
                }
            }
        }
    }

    stbi_write_png("rooms.png", dungeon->w * ROOM_SIZE, dungeon->h * ROOM_SIZE, 4, data, 4 * dungeon->w * ROOM_SIZE);
    free(data);
}

/**
 * @brief Get a color for a key type
 *
 * @param type The type to get a color for
 * @param isStart true if this is the start of the maze
 * @param isEnd true if this is the end of the maze
 * @param isDeadEnd true if this is a dead end
 * @return A color in 0xAARRGGBB form
 */
static uint32_t roomColor(keyType_t type, bool isStart, bool isEnd, bool isDeadEnd)
{
    if (isStart)
    {
        return 0xFFFF0000;
    }
    else if (isEnd)
    {
        return 0xFF0000FF;
    }
    else
    {
        switch (type)
        {
            case EMPTY_ROOM:
            {
                break;
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
                return 0xFFBBBBBB;
            }
        }

        if (isDeadEnd)
        {
            return 0xFF000000;
        }
    }
    return 0xFFFFFFFF;
}
