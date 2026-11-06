#ifndef ITEM_TYPE_H
#define ITEM_TYPE_H

// The four power-ups. Their values are their column in row 4 of
// images/sprites.png, so the icon of an item is just atlasCell(type, 4)
enum class ItemType
{
    BombUp = 0,     // One more bomb at the same time
    FireUp = 1,     // Explosions reach one more tile
    Heart = 2,      // Protects from one hit
    SpeedUp = 3     // Bomberman walks faster
};

#endif
