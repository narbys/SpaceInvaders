#include <stdint.h>
#include <stdbool.h>
// #include <stdlib.h>
#include <gb/gb.h>
#include "GameSprites.c"
#include "BkgTiles.c"

#define _DEBUG
//#define _RELEASE

//Debug includes
#ifdef _DEBUG
#include <assert.h>
#include <stdio.h>
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
    bool isActive;
    UBYTE spriteId; //Invader is 8x8px.
    uint8_t x;
    uint8_t y;
    int8_t slide;
};
struct Bullet
{
    UBYTE spriteId;
    uint8_t x;
    uint8_t y;
    bool isActive;
};

//Global Variables
struct Ship ship;
const uint8_t shipMoveSpeed = 2;

struct Invader invaders[40];
int8_t slideDir;
uint8_t invaderMoveTimer;
bool hasInvaderReachedScreenedge;

struct Bullet bullet;
const uint8_t bulletSpeed = 3;

//General helper Functions
void PerformantDelay(uint8_t numloops)
{
    for (uint8_t i = 0; i < numloops; i++)
    {
        wait_vbl_done();
    }
}

//Player (ship) functions
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

//Enemies (invaders) functions
void InitInvaders()
{
    for (uint8_t i = 0;i < 40;i++)
    {
        invaders[i].x = (i % 8) * 2 + 2;
        invaders[i].y = (i / 8) + 2;
        invaders[i].isActive = true;
        invaders[i].spriteId = 1;
        invaders[i].slide = 0;
        set_bkg_tile_xy(invaders[i].x, invaders[i].y, invaders[i].spriteId);
    }


    // invaders[5].isActive = false;

    slideDir = -1;
    invaderMoveTimer = 0;
    hasInvaderReachedScreenedge = false;
}

//Drawing code for moving the invader on the background
void UpdateinvaderTiles(uint8_t i)
{

    if (invaders[i].slide == 0)
    {
        set_bkg_tile_xy(invaders[i].x, invaders[i].y, invaders[i].spriteId);
        set_bkg_tile_xy(invaders[i].x - slideDir, invaders[i].y, 0);
        set_bkg_tile_xy(invaders[i].x + slideDir, invaders[i].y, 0);
    }
    else if (invaders[i].slide > 0)
    {
        set_bkg_tile_xy(invaders[i].x, invaders[i].y, invaders[i].spriteId + 16 - invaders[i].slide);
        set_bkg_tile_xy(invaders[i].x + 1, invaders[i].y, invaders[i].spriteId + 8 - invaders[i].slide);
    }
    else if (invaders[i].slide < 0)
    {

        set_bkg_tile_xy(invaders[i].x, invaders[i].y, invaders[i].spriteId - invaders[i].slide);
        set_bkg_tile_xy(invaders[i].x - 1, invaders[i].y, invaders[i].spriteId + 8 - invaders[i].slide);
    }
}

void UpdateInvaders()
{
    invaderMoveTimer++;
    if (invaderMoveTimer < 16) //16
        return;
    else
    {
        invaderMoveTimer = 0;
    }

    if (hasInvaderReachedScreenedge)
    {
        hasInvaderReachedScreenedge = false;

        for (uint8_t i = 0;i < 40;i++)
        {
            invaders[i].y++;
        }

        slideDir = -slideDir;
        fill_bkg_rect(0, invaders[0].y - 1, 20, 1, 0);
    }

    for (uint8_t i = 0;i < 40;i++)
    {
        if (invaders[i].isActive == false)
        {
            // Just draw blank
            set_bkg_tile_xy(invaders[i].x, invaders[i].y, 0);
            set_bkg_tile_xy(invaders[i].x + slideDir, invaders[i].y, 0);
        }
        else
        {
            //Move the invader
            UpdateinvaderTiles(i);
        }
        invaders[i].slide += slideDir;

        //If we shifted 7 times, reset.
        if ((invaders[i].slide > 7) || (invaders[i].slide < -7))
        {
            invaders[i].slide = 0;
            invaders[i].x += slideDir;
        }

        //If we are on the edge
        if (((invaders[i].x == 0) || (invaders[i].x == 19)) && (invaders[i].slide == 0))
        {
            hasInvaderReachedScreenedge = true;
        }
    }
}


//Bullet Functions
void InitBullet()
{
    bullet.spriteId = 2;
    bullet.x = 0;
    bullet.y = 0;
    bullet.isActive = false;
}
void CreateBullet()
{
    //If the bullet is already alive, do nothing
    if (bullet.isActive)
        return;

    //Initialise the bullet
    bullet.x = ship.x + 8;
    bullet.y = ship.y - 8;
    bullet.isActive = true;
    set_sprite_tile(2, 2);
    bullet.spriteId = 2;
    move_sprite(bullet.spriteId, bullet.x, bullet.y);
}
void DestroyBullet()
{
    set_sprite_tile(3, NULL);
    bullet.isActive = false;
}
void UpdateBullet()
{
    if (!bullet.isActive)
        return;

    //Check if bullet is out of screen
    if (bullet.y >= 255)
    {
        DestroyBullet();
        return;
    }

    //Move the bullet
    bullet.y -= bulletSpeed;
    move_sprite(bullet.spriteId, bullet.x, bullet.y);
}

//Main function
void main()
{
    set_sprite_data(0, 4, GameSprites);
    set_bkg_data(0, 17, BkgTiles);
    init_bkg(0);

    InitShip();
    InitInvaders();
    InitBullet();

    DISPLAY_ON;
    SHOW_SPRITES;
    SHOW_BKG;
    SPRITES_8x8;

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

        //Updating
        UpdateInvaders();
        UpdateBullet();

        PerformantDelay(2);
    }
}