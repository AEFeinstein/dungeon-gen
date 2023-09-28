#pragma once

//==============================================================================
// Defines
//==============================================================================

// Bits used for tile type construction, topmost bit
#define BG  0x00
#define OBJ 0x80
// Types of background, next two top bits
#define META  0x00
#define FLOOR 0x20
#define WALL  0x40
#define DOOR  0x60
// Types of objects, next two top bits
#define ITEM    0x00
#define ENEMY   0x20
#define BULLET  0x40
#define SCENERY 0x60

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Tile types. The top three bits are metadata, the bottom five bits are
 * a unique number per that metadata
 */
typedef enum __attribute__((packed))
{
    // Special empty type
    EMPTY = 0, // Equivalent to (BG | META | 0),
               // Special delete tile, only used in map editor
    DELETE = (BG | META | 1),
    // Background tiles
    BG_FLOOR        = (BG | FLOOR | 1),
    BG_FLOOR_WATER  = (BG | FLOOR | 2),
    BG_FLOOR_LAVA   = (BG | FLOOR | 3),
    BG_CEILING      = (BG | FLOOR | 4),
    BG_WALL_1       = (BG | WALL | 1),
    BG_WALL_2       = (BG | WALL | 2),
    BG_WALL_3       = (BG | WALL | 3),
    BG_WALL_4       = (BG | WALL | 4),
    BG_WALL_5       = (BG | WALL | 5),
    BG_DOOR         = (BG | DOOR | 1),
    BG_DOOR_CHARGE  = (BG | DOOR | 2),
    BG_DOOR_MISSILE = (BG | DOOR | 3),
    BG_DOOR_ICE     = (BG | DOOR | 4),
    BG_DOOR_XRAY    = (BG | DOOR | 5),
    BG_DOOR_SCRIPT  = (BG | DOOR | 6),
    BG_DOOR_KEY_A   = (BG | DOOR | 7),
    BG_DOOR_KEY_B   = (BG | DOOR | 8),
    BG_DOOR_KEY_C   = (BG | DOOR | 9),
    // Enemies
    OBJ_ENEMY_START_POINT = (OBJ | ENEMY | 1),
    OBJ_ENEMY_NORMAL      = (OBJ | ENEMY | 2),
    OBJ_ENEMY_STRONG      = (OBJ | ENEMY | 3),
    OBJ_ENEMY_ARMORED     = (OBJ | ENEMY | 4),
    OBJ_ENEMY_FLAMING     = (OBJ | ENEMY | 5),
    OBJ_ENEMY_HIDDEN      = (OBJ | ENEMY | 6),
    OBJ_ENEMY_BOSS        = (OBJ | ENEMY | 7),
    // Power-ups
    OBJ_ITEM_BEAM        = (OBJ | ITEM | 1),
    OBJ_ITEM_CHARGE_BEAM = (OBJ | ITEM | 2),
    OBJ_ITEM_MISSILE     = (OBJ | ITEM | 3),
    OBJ_ITEM_ICE         = (OBJ | ITEM | 4),
    OBJ_ITEM_XRAY        = (OBJ | ITEM | 5),
    OBJ_ITEM_SUIT_WATER  = (OBJ | ITEM | 6),
    OBJ_ITEM_SUIT_LAVA   = (OBJ | ITEM | 7),
    OBJ_ITEM_ENERGY_TANK = (OBJ | ITEM | 8),
    // Permanent non-power-items
    OBJ_ITEM_KEY_A    = (OBJ | ITEM | 9),
    OBJ_ITEM_KEY_B    = (OBJ | ITEM | 10),
    OBJ_ITEM_KEY_C    = (OBJ | ITEM | 11),
    OBJ_ITEM_ARTIFACT = (OBJ | ITEM | 12),
    // Transient items
    OBJ_ITEM_PICKUP_ENERGY  = (OBJ | ITEM | 13),
    OBJ_ITEM_PICKUP_MISSILE = (OBJ | ITEM | 14),
    // Bullets
    OBJ_BULLET_NORMAL  = (OBJ | BULLET | 15),
    OBJ_BULLET_CHARGE  = (OBJ | BULLET | 16),
    OBJ_BULLET_ICE     = (OBJ | BULLET | 17),
    OBJ_BULLET_MISSILE = (OBJ | BULLET | 18),
    OBJ_BULLET_XRAY    = (OBJ | BULLET | 19),
    // Scenery
    OBJ_SCENERY_TERMINAL = (OBJ | SCENERY | 1),
    OBJ_SCENERY_PORTAL   = (OBJ | SCENERY | 2),
} rayMapCellType_t;
