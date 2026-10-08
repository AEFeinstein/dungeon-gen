#!/bin/bash
set -e

# Make, just in case
make -j16

# Sizes of dungeons in Link's Awakening
# Tail Cave		 25
# Bottle Grotto	 26
# Key Cavern	 29
# Anglers Tunnel 28
# Catfish's Maw	 34
# Face Shrine	 40
# Eagle's Tower	 34
# Turtle Rock	 46

# Generate maps
ROOM_W=14
ROOM_H=12
./dungeon-gen -w 5 -h 5 -x $ROOM_W -y $ROOM_H -s BOTTOM_LEFT  -k 01s23 -n dungeon_1
./dungeon-gen -w 6 -h 5 -x $ROOM_W -y $ROOM_H -s BOTTOM_RIGHT -k 01b23 -n dungeon_2
./dungeon-gen -w 6 -h 6 -x $ROOM_W -y $ROOM_H -s TOP_LEFT     -k 01l23 -n dungeon_3
./dungeon-gen -w 7 -h 6 -x $ROOM_W -y $ROOM_H -s TOP_RIGHT    -k 0123  -n dungeon_4

# Copy to Swadge project
MAP_DIR=../Super-2024-Swadge-FW/assets/tomisQuest/maps
cp dungeon_1.rmd $MAP_DIR/dungeon_1.rmd 
cp dungeon_2.rmd $MAP_DIR/dungeon_2.rmd 
cp dungeon_3.rmd $MAP_DIR/dungeon_3.rmd 
cp dungeon_4.rmd $MAP_DIR/dungeon_4.rmd 
