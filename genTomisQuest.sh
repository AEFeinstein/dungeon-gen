#!/bin/bash
set -e

ROOM_W=14
ROOM_H=12

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

./dungeon-gen -w 5 -h 5 -x $ROOM_W -y $ROOM_H -s BOTTOM_LEFT -k 01s23 -n dungeon_1
./dungeon-gen -w 6 -h 5 -x $ROOM_W -y $ROOM_H -s BOTTOM_LEFT -k 01b23 -n dungeon_2
./dungeon-gen -w 6 -h 6 -x $ROOM_W -y $ROOM_H -s BOTTOM_LEFT -k 01l23 -n dungeon_3
./dungeon-gen -w 7 -h 6 -x $ROOM_W -y $ROOM_H -s BOTTOM_LEFT -k 0123 -n dungeon_4
