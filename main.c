#include <stdint.h>
#include <stdlib.h>
#include <gb/gb.h>
#include "GameSprites.c"

#define _DEBUG
//#define _RELEASE

//Debug includes
#ifdef _DEBUG
#include <assert.h>
#endif

//Structs
struct Ship
{
    UBYTE spriteIds[2]; //Ship is 16x8px. 2 Sprites
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
};
struct Invader
{
    UBYTE spriteId; //Invader is 8x8px.
    uint8_t x;
    uint8_t y;
};
struct Bullet
{
    UBYTE spriteId;
    uint8_t x;
    uint8_t y;
};

//Global Variables
struct Ship ship;
const uint8_t shipMoveSpeed = 2;

struct Invader invader;
struct Bullet* pBullet = NULL;
const uint8_t bulletSpeed = 3;

//Functions

void PerformantDelay(uint8_t numloops)
{
    for (uint8_t i = 0; i < numloops; i++)
    {
        wait_vbl_done();
    }
}

void MoveShip(struct Ship* pShip, uint8_t x, uint8_t y)
{
    move_sprite(pShip->spriteIds[0], x, y);
    move_sprite(pShip->spriteIds[1], x + 8, y);
}

void InitShip()
{
    ship.x = 80;
    ship.y = 130;
    ship.width = 16;
    ship.height = 8;

    set_sprite_tile(0, 0);
    ship.spriteIds[0] = 0;
    set_sprite_tile(1, 1);
    ship.spriteIds[1] = 1;
    MoveShip(&ship, ship.x, ship.y);
}

void InitInvader()
{
    invader.x = 80;
    invader.y = 30;

    set_sprite_tile(2, 2);
    invader.spriteId = 2;
    move_sprite(invader.spriteId, invader.x, invader.y);
}

void CreateBullet()
{
    //If the bullet is already alive, do nothing
    if (pBullet)
        return;

    pBullet = (struct Bullet*)malloc(sizeof(struct Bullet));

#ifdef _DEBUG
    assert(pBullet);
#endif

    //Initialise the bullet
    pBullet->x = ship.x + 8;
    pBullet->y = ship.y - 8;
    set_sprite_tile(3, 3);
    pBullet->spriteId = 3;
    move_sprite(pBullet->spriteId, pBullet->x, pBullet->y);
}

void DestroyBullet()
{
    free(pBullet);
    pBullet = NULL;
}

void UpdateBullet()
{
    if (!pBullet)
        return;

    //Check if bullet is out of screen
    if (pBullet->y + 8 > 255)
    {
        DestroyBullet();
        return;
    }

    //Move the bullet
    pBullet->y -= bulletSpeed;
    move_sprite(pBullet->spriteId, pBullet->x, pBullet->y);
}

void main()
{
    set_sprite_data(0, 4, GameSprites);
    InitShip();
    InitInvader();

    SHOW_SPRITES;
    DISPLAY_ON;

    while (1)
    {
        //movement
        if (joypad() & J_LEFT)
        {
            ship.x -= shipMoveSpeed;
            MoveShip(&ship, ship.x, ship.y);
        }
        if (joypad() & J_RIGHT)
        {
            ship.x += shipMoveSpeed;
            MoveShip(&ship, ship.x, ship.y);
        }

        // Clamp our x position at the screen ends
        if (ship.x < 8)ship.x = 8;
        if (ship.x > 152)ship.x = 152;

        //Single press
        switch (joypad())
        {
        case J_A:
            CreateBullet();
            break;
        case J_B:
            break;
        }

        //Bullet updating
        UpdateBullet();

        PerformantDelay(2);
    }
}