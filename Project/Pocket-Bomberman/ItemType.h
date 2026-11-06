#ifndef ITEM_TYPE_H
#define ITEM_TYPE_H

// Power-ups in row 4 of images/sprites.png
enum class ItemType
{
    BombUp = 0,     // One more bomb at the same time
    FireUp = 1,     // Explosions reach one more tile
    Heart = 2,      // Protects from one hit
    SpeedUp = 3     // Bomberman walks faster
};

#endif
