#include <stdint.h>
#include <stdbool.h>
#include <gb/gb.h>
#include <gbdk/font.h>
#include "GameSprites.c"
#include "BkgTiles.c"

//#define _HITBOXDEBUG
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
    UBYTE spriteId; //bkg tile id in this case
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

struct EnemyBullet
{
    UBYTE spriteId;
    uint8_t x;
    uint8_t y;
    bool isActive;
};

#ifdef _HITBOXDEBUG
struct DebugSpriteObj
{
    UBYTE spriteId;
    uint8_t x;
    uint8_t y;
    uint8_t val;
};
struct DebugSpriteObj dbgSpr;
#endif

//Global Variables
//General
bool GameRunning;

//player
struct Ship ship;
const uint8_t shipMoveSpeed = 2;
uint8_t lives;
uint16_t score;
uint8_t scoreSplit[5];

struct Bullet bullet;
const uint8_t bulletSpeed = 3;

//invader
struct Invader invaders[40];
int8_t slideDir;
uint8_t invaderMoveTimer;
bool hasInvaderReachedScreenedge;
uint8_t shotTimer;
const uint8_t shotTimerMaxTime = 2;

const uint8_t maxInvaderBulletCount = 3;
struct EnemyBullet invaderBullet[3];    //The enemy can have n bullets on screen at the same time
const uint8_t initialInvaderBulletSpriteId = 4;
uint8_t freeBulletIdx;
const uint8_t invaderBulletSpeed = 2;

//General helper Functions
void PerformantDelay(uint8_t numloops)
{
    for (uint8_t i = 0; i < numloops; i++)
    {
        wait_vbl_done();
    }
}

//Taken from LaroldsJubilantJunkyard
uint8_t RandomNumber(uint8_t min, uint8_t max)
{

    // Use some sprites in getting a random value
    uint8_t v = DIV_REG + shadow_OAM[0].x + shadow_OAM[2].x + shadow_OAM[3].x;

    return min + (v % (max - min));    // get value at memory address
}
void SplitScore()
{
    scoreSplit[4] = (score / 10000) % 10;
    scoreSplit[3] = (score / 1000) % 10;
    scoreSplit[2] = (score / 100) % 10;
    scoreSplit[1] = (score / 10) % 10;
    scoreSplit[0] = score % 10;
}

void InitHUD()
{
    const unsigned char winmap[] =
    {
        //SCORE 00000
        0x1D,0x0D,0x19,0x1C,0x0F, 0x00, 0x01,0x01,0x01,0x01,0x01,
        //2 spaces
        0x00,0x00,
        //lives 3
        0x37,0x38,0x04
    };
    set_win_tiles(0, 0, 16, 1, winmap);
    move_win(7, 124);
}
void UpdateHUDScore()
{
    SplitScore();
    const uint8_t scoreTileIdx = 10;
    for (uint8_t i = 0;i < 5;i++)
    {
        set_win_tile_xy(scoreTileIdx - i, 0, 0x01 + scoreSplit[i]);
    }
    move_win(7, 124);
}
void UpdateHUDLives()
{
    const uint8_t livesTileIdx = 15;
    set_win_tile_xy(livesTileIdx, 0, 0x01 + lives);
    move_win(7, 124);
}
void GameOverScreen()
{
    HIDE_SPRITES;
    init_bkg(0);
    init_win(0);
    //A=0x0B
    const unsigned char gameover_window[] =
    {
        //game over
        0x11, 0x0B, 0x17, 0x0F, 0x00, 0x19, 0x20, 0x0F, 0x1C
    };
    set_win_tiles(0, 0, 9, 1, gameover_window);
    move_win(50, 60);
    //stop the game loop
    GameRunning = false;
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

    set_sprite_tile(1, 1);
    ship.spriteIds[0] = 1;
    set_sprite_tile(2, 2);
    ship.spriteIds[1] = 2;
    MoveShip(&ship, ship.x, ship.y);

    lives = 3;
    score = 0;
    for (uint8_t i = 0;i < 5;i++)
    {
        scoreSplit[i] = 0;
    }
}

//Player Bullet Functions
void InitBullet()
{
    bullet.spriteId = 3;
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
    set_sprite_tile(3, 3);
    move_sprite(bullet.spriteId, bullet.x, bullet.y);
}
void DestroyBullet()
{
    bullet.isActive = false;
    set_sprite_tile(3, NULL);
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
#ifndef _HITBOXDEBUG
    for (uint8_t i = 0;i < 40;i++)
    {
        //Check for invaders
        if (bullet.isActive && invaders[i].isActive)
        {
            //CollisionCheck method

            //Get actual coords of invader
            // uint8_t invX = invaders[i].x * 8 - invaders[i].slide;
            // uint8_t invY = invaders[i].y * 8;

            const uint8_t invX = invaders[i].x * 8 + 8 + invaders[i].slide - 2 * slideDir;
            const uint8_t invY = invaders[i].y * 8 + 16;

            // bool hit = ((bullet.x >= invX && bullet.x <= invX + 8) &&
            //     (bullet.y >= invY && bullet.y <= invY + 8)) ||
            //     ((invX >= bullet.x && invX <= bullet.x + 1) &&
            //         (invY >= bullet.y && invY <= bullet.y + 8));

            // Tutorial Method

            // int8_t xd = (bullet.x) - (invaders[i].x * 8 + 4 + invaders[i].slide);
            // int8_t yd = (bullet.y) - invaders[i].y * 8 + 4;

            // // Get the absolute value
            // if (xd < 0)xd = -xd;
            // if (yd < 0)yd = -yd;

            // const bool hit = (xd < 5 && yd < 8);

            if ((bullet.x >= invX && bullet.x <= invX + 8) && (bullet.y <= invY + 8 && bullet.y <= invY))
            {
                invaders[i].isActive = false;
                DestroyBullet();

                //Draw the hit invader blank
                set_bkg_tile_xy(invaders[i].x, invaders[i].y, 0);
                set_bkg_tile_xy(invaders[i].x + slideDir, invaders[i].y, 0);

                //Increase score;
                score++;
                UpdateHUDScore();

                return;
            }
        }
    }
#endif
    //Move the bullet
    bullet.y -= bulletSpeed;
    move_sprite(bullet.spriteId, bullet.x, bullet.y);
}

void InitInvaderBullets()
{
    for (uint8_t i = 0;i < maxInvaderBulletCount;i++)
    {
        invaderBullet[i].spriteId = initialInvaderBulletSpriteId + i;
        invaderBullet[i].isActive = false;
        invaderBullet[i].x = 0;
        invaderBullet[i].y = 0;
    }
}

//Invader bullet functions
void TryCreateInvaderBullet(uint8_t x, uint8_t y)
{
    for (uint8_t i = 0;i < maxInvaderBulletCount;i++)
    {
        //Check if there's any unused bullets
        if (invaderBullet[i].isActive)
            continue;
        else
        {
            //Give the index of the first free bullet and go out of the loop
            freeBulletIdx = i;
            break;
        }
        //Return because all bullets are in use
        return;
    }
    //We found a free space, spawn the bullet
    invaderBullet[freeBulletIdx].isActive = true;
    invaderBullet[freeBulletIdx].x = x + freeBulletIdx * 8;
    invaderBullet[freeBulletIdx].y = y;
    set_sprite_tile(invaderBullet[freeBulletIdx].spriteId, 4);
    move_sprite(invaderBullet[freeBulletIdx].spriteId, invaderBullet[freeBulletIdx].x, invaderBullet[freeBulletIdx].y);

}
void DestroyInvaderBullet(uint8_t i)
{
    set_sprite_tile(invaderBullet[i].spriteId, NULL);
    invaderBullet[i].isActive = false;
}
void UpdateInvaderBullets()
{
    for (uint8_t i = 0;i < maxInvaderBulletCount;i++)
    {
        if (!invaderBullet[i].isActive)
            continue;

        //Check if bullet is out of screen
        if (invaderBullet[i].y > 160)
        {
            DestroyInvaderBullet(i);
            return;
        }

        //Move the bullet
        invaderBullet[i].y += invaderBulletSpeed;
        move_sprite(invaderBullet[i].spriteId, invaderBullet[i].x, invaderBullet[i].y);

        //Check if we hit the player
        const bool hit = (invaderBullet[i].x >= ship.x && invaderBullet[i].x <= ship.x + ship.width) &&
            (invaderBullet[i].y >= ship.y && invaderBullet[i].y <= ship.y + ship.height) ||
            (ship.x >= invaderBullet[i].x && ship.x <= invaderBullet[i].x + 8) &&
            (ship.y >= invaderBullet[i].y && ship.y <= invaderBullet[i].y + 8);
        if (hit)
        {
            //hit the player.
            DestroyInvaderBullet(i);
            lives--;
            if (lives == 0)
            {
                GameOverScreen();
                return;
            }
            UpdateHUDLives();
        }
    }
}

//Enemies (invaders) functions
void InitInvaders()
{
    for (uint8_t i = 0;i < 40;i++)
    {
        invaders[i].x = (i % 8) * 2 + 2;
        invaders[i].y = (i / 8) + 2;
        invaders[i].isActive = true;
        invaders[i].spriteId = 38;
        invaders[i].slide = 0;
        set_bkg_tile_xy(invaders[i].x, invaders[i].y, invaders[i].spriteId);
    }

    // for (uint8_t i = 0;i < 24;i++)
    // {
    //     invaders[i].isActive = false;
    // }

    slideDir = -1;
    invaderMoveTimer = 0;
    hasInvaderReachedScreenedge = false;
    shotTimer = shotTimerMaxTime;
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
    bool invMove = true;
    //invader movement timer
    invaderMoveTimer++;
    if (invaderMoveTimer < 16) //16
        return;
    else
        invaderMoveTimer = 0;

    //invader shooting timer
    if (shotTimer != 0)shotTimer--;

    //Shift all invaders down when they reach the edge
    if (hasInvaderReachedScreenedge)
    {
        hasInvaderReachedScreenedge = false;

        for (uint8_t i = 0;i < 40;i++)
        {
            invaders[i].y++;
        }

        slideDir = -slideDir;
        //set the previous top row blank
        fill_bkg_rect(0, invaders[0].y - 1, 20, 1, 0);
    }

    //Invader updateloop
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

            //Try to shoot a bullet
            if (shotTimer == 0 && RandomNumber(0, 100) < 10)
            {
                const uint8_t x = invaders[i].x * 8 + 12 + invaders[i].slide;
                const uint8_t y = invaders[i].y * 8 + 24;
                TryCreateInvaderBullet(x, y);
                shotTimer = shotTimerMaxTime;
            }
        }
        invaders[i].slide += slideDir * 2;

        //If we shifted 7 times, reset.
        if ((invaders[i].slide > 7) || (invaders[i].slide < -7))
        {
            invaders[i].slide = 0;
            invaders[i].x += slideDir;
        }

        //If we are on the edge and fully on 1 tile
        if (((invaders[i].x == 0) || (invaders[i].x == 19)) && (invaders[i].slide == 0))
        {
            hasInvaderReachedScreenedge = true;
        }
    }
}

//Main function
void main()
{
    //initialise the font
    font_t min_font;
    font_init();
    min_font = font_load(font_min); //36 tiles
    font_set(min_font);

    //initialise the sprite and bkg data
    set_sprite_data(0, 6, GameSprites);
    //18 background tiles for invaders + 2 for player on HUD
    set_bkg_data(37, 20, BkgTiles);
    //set the background and window to empty by default
    init_bkg(0);
    init_win(0);

    //Start screen
    const unsigned char gamestart_window[] =
    {
        //press start
        0x1A,0x1C,0x0F,0x1D,0x1D,0x00,0x1D,0x1E,0x0B,0x1C,0x1E
    };
    set_win_tiles(0, 0, 11, 1, gamestart_window);
    move_win(44, 60);
    SHOW_WIN;
    while (1)
    {
        if (joypad() & J_START)
        {
            break;
        }
    }

    //Initialise everything in the game
    InitHUD();
    InitShip();
    InitInvaders();
    InitBullet();
    InitInvaderBullets();
#ifdef _HITBOXDEBUG
    //init debugsprite
    dbgSpr.spriteId = 15; //one that is certainly empty
    dbgSpr.x = 0;
    dbgSpr.y = 0;
    dbgSpr.val = 0;
    set_sprite_tile(dbgSpr.spriteId, 5);

#endif

    DISPLAY_ON;
    SHOW_SPRITES;
    SHOW_BKG;
    SHOW_WIN;
    SPRITES_8x8;

    GameRunning = true; //Initialise the gameloop

    while (GameRunning)
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
            TryCreateInvaderBullet(80, 20);
            PerformantDelay(5);
            break;
        }

        //Updating
        UpdateBullet();
        UpdateInvaders();
        UpdateInvaderBullets();

#ifdef _HITBOXDEBUG
        //Debug stuff

        //Set the debug sprite to the converted X Y coords of the first invader active
        for (uint8_t i = 0;i < 40;i++)
        {
            if (invaders[i].isActive)
            {
                const uint8_t invX = invaders[i].x * 8 + 8 + invaders[i].slide - 2 * slideDir;
                const uint8_t invY = invaders[i].y * 8 + 16;

                dbgSpr.x = invX;
                dbgSpr.y = invY;
                dbgSpr.val = i;

                break;
            }

        }
        move_sprite(dbgSpr.spriteId, dbgSpr.x, dbgSpr.y);

        //Check collision only with the dbgsprt
        if ((bullet.x >= dbgSpr.x && bullet.x <= dbgSpr.x + 8) && (bullet.y <= dbgSpr.y + 8 && bullet.y <= dbgSpr.y))
        {
            const uint8_t it = dbgSpr.val;
            invaders[it].isActive = false;
            DestroyBullet();
            // Just draw blank
            set_bkg_tile_xy(invaders[it].x, invaders[it].y, 0);
            set_bkg_tile_xy(invaders[it].x + slideDir, invaders[it].y, 0);
            GameRunning = false;
        }
#endif

        PerformantDelay(2);
    }
}