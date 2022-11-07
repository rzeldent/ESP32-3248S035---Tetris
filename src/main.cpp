//=====================================================================
// TETRIS with ESP32_3248S035 : 2022.09.05 : macsbug
// https://macsbug.wordpress.com/2022/09/16/esp32-3248s035/
//=====================================================================
// HARD              : ESP32_3248S035
//  Display          : 3.5" 320x480 SPI ST7796 LCD Touch XPT2046
// Dev environment   : Arduino IDE 1.8.19
//  Board Manager    : arduino-esp32 2.0.3-RC1
//  Board            : "ESP32 Dev Module"
//  Upload Speed     : "921600"
//  CPU Frequency    : "240MHz (WiFi/BT)"
//  Flash Frequency  : "80MHz"
//  Flash Mode       : "DIO"
//  Flash Size       : "4MB (32Mb)"
//  Partition Scheme : "No OTA (2MB APP/2MB SPISSF)"
//  Core Degug Level : "None"
//  PSRAM            : "Disabled"
//  Arduino Runs On  : "Core 1"
//  Events Run On    : "Core 1"
//  Pord             : "dev/cu.wchusbserial14240"
// Library           : lovyanGFX
//                   : https://github.com/lovyan03/LovyanGFX
//=====================================================================

#pragma GCC optimize("Ofast")
#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include "lgfx_ESP32_3248S035.h"
#include "tetris_image.c"

uint16_t BlockImage[8][12][12];  // Block
uint16_t backBuffer[240][120];   // GAME AREA
uint16_t nextBlockBuf[60][48];   // NEXT BLOCK AREA
const int Length = 12;           // the number of pixels for a side of a block
const int Width = 10;            // the number of horizontal blocks
const int Height = 20;           // the number of vertical blocks
int screen[Width][Height] = {0}; // it shows color-numbers of all positions

struct Point
{
  int X, Y;
};

struct Block
{
  Point square[4][4];
  int numRotate, color;
};

void make_block(int n, uint16_t color);
void PutStartPos();
void Draw();
void Touch_name();
void DrawNextBlock();
void KeyPadLoop();
void ReviseScreen(Point next_pos, int next_rot);
void GetNextPosRot(Point *pnext_pos, int *pnext_rot);

Point pos;
Block block;
int nextBlockType = -1;
long score = 0;
Block nextBlock;
int rot, fall_cnt = 0;
bool started = false, gameover = false;
bool STROT = false, LEFT = false, RIGHT = false, DOWN = false, PAUSE = false;
uint16_t tX, tY;
int speed_ = 1800;

Block blocks[7] = {
    {{{{-1, 0}, {0, 0}, {1, 0}, {2, 0}}, {{0, -1}, {0, 0}, {0, 1}, {0, 2}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 1},
    {{{{0, -1}, {1, -1}, {0, 0}, {1, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 1, 2},
    {{{{-1, -1}, {-1, 0}, {0, 0}, {1, 0}}, {{-1, 1}, {0, 1}, {0, 0}, {0, -1}}, {{-1, 0}, {0, 0}, {1, 0}, {1, 1}}, {{1, -1}, {0, -1}, {0, 0}, {0, 1}}}, 4, 3},
    {{{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}, {{0, -1}, {0, 0}, {-1, 0}, {-1, 1}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 4},
    {{{{-1, 0}, {0, 0}, {1, 0}, {1, -1}}, {{-1, -1}, {0, -1}, {0, 0}, {0, 1}}, {{-1, 1}, {-1, 0}, {0, 0}, {1, 0}}, {{0, -1}, {0, 0}, {0, 1}, {1, 1}}}, 4, 5},
    {{{{-1, 1}, {0, 1}, {0, 0}, {1, 0}}, {{0, -1}, {0, 0}, {1, 0}, {1, 1}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 6},
    {{{{-1, 0}, {0, 0}, {1, 0}, {0, -1}}, {{0, -1}, {0, 0}, {0, 1}, {-1, 0}}, {{-1, 0}, {0, 0}, {1, 0}, {0, 1}}, {{0, -1}, {0, 0}, {0, 1}, {1, 0}}}, 4, 7}};
//========================================================================

void setup(void)
{
  Serial.begin(115200);
  tft.init();

  tft.setBrightness(200);
  tft.setRotation(1); // USB Right
                      // tft.setRotation(3);           // USB Left
  tft.setColorDepth(24);
  tft.fillScreen(TFT_BLACK);
  tft.startWrite();


  //----------------------------// Make Block ----------------------------
  make_block(0, TFT_BLACK); // Type No, Color
  make_block(1, 0x00F0);    // DDDD     RED
  make_block(2, 0xFBE4);    // DD,DD    PUPLE
  make_block(3, 0xFF00);    // D__,DDD  BLUE
  make_block(4, 0xFF87);    // DD_,_DD  GREEN
  make_block(5, 0x87FF);    // __D,DDD  YELLO
  make_block(6, 0xF00F);    // _DD,DD_  LIGHT GREEN
  make_block(7, 0xF8FC);    // _D_,DDD  PINK
  //----------------------------------------------------------------------
  tft.pushImage(80, 40, 320, 240, (uint16_t *)tetris_image); // background
  tft.drawLine(80, 39, 399, 39, 0x561F);
  tft.drawLine(80, 280, 399, 280, 0x561F);
  PutStartPos(); // Start Position
  for (int i = 0; i < 4; ++i)
    screen[pos.X +
           block.square[rot][i].X][pos.Y + block.square[rot][i].Y] = block.color;
  Draw();          // draw a block
  Touch_name();    // Touch sens name
  DrawNextBlock(); // next block
}
//========================================================================
void loop()
{
  if (gameover)
  {
    KeyPadLoop();
    if (STROT)
    {
      ESP.restart();
    }
    return;
  }
  Point next_pos;
  int next_rot = rot;
  // KeyPadLoop();
  GetNextPosRot(&next_pos, &next_rot);
  ReviseScreen(next_pos, next_rot);
  for (int i = 0; i < speed_; i++)
  {
    KeyPadLoop();
    if (DOWN)
    {
      i = i + 100;
    }
  }
}
//========================================================================
void Draw()
{ // Draw 120x240 in the center
  for (int i = 0; i < Width; ++i)
    for (int j = 0; j < Height; ++j)
      for (int k = 0; k < Length; ++k)
        for (int l = 0; l < Length; ++l)
          backBuffer[j * Length + l][i * Length + k] = BlockImage[screen[i][j]][k][l];
  tft.pushImage(180, 40, 120, 240, (uint16_t *)backBuffer); // draw a block
}
//========================================================================
void DrawNextBlock()
{
  for (int x = 0; x < 48; x++)
  {
    for (int y = 0; y < 60; y++)
    {
      nextBlockBuf[y][x] = 0;
    }
  }
  nextBlock = blocks[nextBlockType];
  int offset = 6 + 12;
  for (int i = 0; i < 4; ++i)
  {
    for (int k = 0; k < Length; ++k)
      for (int l = 0; l < Length; ++l)
      {
        nextBlockBuf[60 - (nextBlock.square[0][i].X * Length + l +
                           offset)][nextBlock.square[0][i].Y * Length + k + offset] =
            BlockImage[nextBlockType + 1][k][l];
      }
  }
  tft.pushImage(106, 140, 48, 60, (uint16_t *)nextBlockBuf);
  tft.fillRect(85, 116, 92, 19, TFT_BLACK);
  tft.setCursor(92, 122);
  tft.printf("%7d", score); // score
}
//========================================================================
void PutStartPos()
{
  pos.X = 4;
  pos.Y = 1;
  if (nextBlockType == -1)
  {
    block = blocks[random(7)];
  }
  else
  {
    block = blocks[nextBlockType];
  }
  nextBlockType = random(7);
  rot = random(block.numRotate);
}
//========================================================================
bool GetSquares(Block block, Point pos, int rot, Point *squares)
{
  bool overlap = false;
  for (int i = 0; i < 4; ++i)
  {
    Point p;
    p.X = pos.X + block.square[rot][i].X;
    p.Y = pos.Y + block.square[rot][i].Y;
    overlap |= p.X < 0 || p.X >= Width || p.Y < 0 || p.Y >= Height || screen[p.X][p.Y] != 0;
    squares[i] = p;
  }
  return !overlap;
}
//========================================================================
void GameOver()
{
  for (int i = 0; i < Width; ++i)
    for (int j = 0; j < Height; ++j)
      if (screen[i][j] != 0)
      {
        screen[i][j] = 4;
        gameover = true;
      }
}
//========================================================================
void ClearKeys()
{
  STROT = false;
  LEFT = false;
  RIGHT = false;
  DOWN = false;
  PAUSE = false;
}
//========================================================================
void KeyPadLoop()
{
T:
  bool touched = tft.getTouch(&tX, &tY);
  if (touched && tX < 480 && tY < 320)
  {
    Serial.printf("%d,%d\n", tX, tY);
    if (tX < 80 && tY < 160)
    {
      ClearKeys();
      DOWN = true;
      return;
    } // DOWN
    if (tX < 80 && tY > 160)
    {
      ClearKeys();
      LEFT = true;
      return;
    } // LEFT
    if (tX > 360 && tY > 160)
    {
      ClearKeys();
      RIGHT = true;
      return;
    } // RIGHT
    if (tX > 360 && tY < 160)
    {
      ClearKeys();
      STROT = true;
      return;
    } // STROT
    // PAUSE -------------------------------------------------------------
    if (tX > 150 && tX < 270)
    { // PAUSE
      if (PAUSE)
      {
        PAUSE = false;
        return;
      }
      ClearKeys();
      PAUSE = true;
      return;
    }
    //--------------------------------------------------------------------
  }
  if (PAUSE)
  {
    tft.setCursor(226, 64);
    tft.println("PAUSE");
    goto T;
  }
}
//========================================================================
void GetNextPosRot(Point *pnext_pos, int *pnext_rot)
{
  if (STROT)
    started = true;
  if (!started)
    return;
  pnext_pos->X = pos.X;
  pnext_pos->Y = pos.Y;
  if ((fall_cnt = (fall_cnt + 1) % 10) == 0)
    pnext_pos->Y += 1;
  else
  {
    if (LEFT)
    {
      LEFT = false;
      pnext_pos->X -= 1;
    }
    else if (RIGHT)
    {
      RIGHT = false;
      pnext_pos->X += 1;
    }
    else if (DOWN)
    {
      DOWN = false;
      pnext_pos->Y += 1;
    }
    else if (STROT)
    {
      STROT = false;
      *pnext_rot = (*pnext_rot + block.numRotate - 1) % block.numRotate;
    }
  }
}
//========================================================================
void DeleteLine()
{
  int deleteCount = 0;
  for (int j = 0; j < Height; ++j)
  {
    bool Delete = true;
    for (int i = 0; i < Width; ++i)
      if (screen[i][j] == 0)
        Delete = false;
    if (Delete)
    {
      for (int k = j; k >= 1; --k)
      {
        for (int i = 0; i < Width; ++i)
        {
          screen[i][k] = screen[i][k - 1];
        }
      }
      deleteCount++;
    }
  }
  switch (deleteCount)
  {
  case 1:
    score = score + 40;
    break;
  case 2:
    score = score + 100;
    break;
  case 3:
    score = score + 300;
    break;
  case 4:
    score = score + 1200;
    break;
  }
  if (score > 9999999)
  {
    score = 9999999;
  }
}
//========================================================================
void ReviseScreen(Point next_pos, int next_rot)
{
  if (!started)
    return;
  Point next_squares[4];
  for (int i = 0; i < 4; ++i)
    screen[pos.X +
           block.square[rot][i].X][pos.Y + block.square[rot][i].Y] = 0;
  if (GetSquares(block, next_pos, next_rot, next_squares))
  {
    for (int i = 0; i < 4; ++i)
    {
      screen[next_squares[i].X][next_squares[i].Y] = block.color;
    }
    pos = next_pos;
    rot = next_rot;
  }
  else
  {
    for (int i = 0; i < 4; ++i)
      screen[pos.X +
             block.square[rot][i].X][pos.Y + block.square[rot][i].Y] = block.color;
    if (next_pos.Y == pos.Y + 1)
    {
      DeleteLine();
      PutStartPos();
      DrawNextBlock();
      if (!GetSquares(block, pos, rot, next_squares))
      {
        for (int i = 0; i < 4; ++i)
          screen[pos.X +
                 block.square[rot][i].X][pos.Y + block.square[rot][i].Y] = block.color;
        GameOver();
      }
    }
  }
  Draw();
}
//========================================================================
void make_block(int n, uint16_t color)
{ // Make Block color
  for (int i = 0; i < 12; i++)
    for (int j = 0; j < 12; j++)
    {
      BlockImage[n][i][j] = color; // Block color
      if (i == 0 || j == 0)
        BlockImage[n][i][j] = 0; // BLACK Line
    }
}
//========================================================================
void Touch_name()
{ // controll name
  tft.setCursor(30, 78);
  tft.println("DOWN");
  tft.setCursor(30, 240);
  tft.println("LEFT");
  tft.setCursor(425, 70);
  tft.println("START");
  tft.setCursor(422, 88);
  tft.println("ROTATE");
  tft.setCursor(428, 240);
  tft.println("RIGHT");
}
//========================================================================
