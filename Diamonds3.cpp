/*
 * DIAMONDS3.CPP
 * created by Matthew Blaine, November 2005
 * updated by Matthew Blaine, August 2006
 *
 * Diamonds that runs on DirectX and contains a total of 70 levels.
 *
 * This WIN32 application uses classes 'Ball' and 'Field', and enum 'Block'
 * to orchestrate a full-blown, 70-level game of 'Diamonds'. To play
 * Diamonds, the player uses the left and right arrow keys to direct a
 * constantly bouncing up-and-down ball through the playing field. To beat
 * a level, destroy all the Diamond blocks, defined in the enumeration
 * 'Block'. To desroy a bock, simply cause the ball to make contact with it.
 * Diamond blocks cannot be destroyed until all color-blocks are destroyed.
 * These can only be destroyed if the ball matches the given block's color.
 * To change the ball's color, run into a brush block. There is no block
 * to make the ball light blue, so remove those blocks first. If you hit a
 * brush block, changing the ball's color, before all light blue blocks are
 * gone, you must run the ball into a 'skull' block, losing a life and
 * reseting the ball.
 *
 */

#define _WIN32_WINNT 0x0500
#define WINVER       0x0500

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <dsound.h>

#include "Constants.h"
#include "Field.h"
#include "Ball.h"
#include "Resource_Diamonds.h"
#include "HiScore.h"
#include "D3Globals.h"
#include "Surface.h"

#define CLASSNAME "MJB_Diamonds"

//global handles
HWND myHandle = NULL;
HINSTANCE hinst1 = NULL;
HACCEL hmyaccel = NULL;
HMENU hmenu = NULL;
HANDLE fontResource = NULL;

//handles to bitmaps
HBITMAP readout_bmp = NULL;
HBITMAP logo_bmp = NULL;


//handles to bitmap surfaces
Surface* ball_surf = NULL;
Surface* blocks_surf = NULL;
Surface* readout_surf = NULL;
Surface* bkgrnd_surf = NULL;

//handles to wave buffers
SoundBuffer* bounce_snd = NULL;
SoundBuffer* colorblk_snd = NULL;
SoundBuffer* colorbrush_snd = NULL;
SoundBuffer* diamond_snd = NULL;
SoundBuffer* die_snd = NULL;
SoundBuffer* laughing_snd = NULL;
SoundBuffer* levelwon_snd = NULL;
SoundBuffer* lock_snd = NULL;
SoundBuffer* key_snd = NULL;
SoundBuffer* reverse_snd = NULL;
SoundBuffer* timebonus_snd = NULL;
SoundBuffer* oneup_snd = NULL;
SoundBuffer* bounce2_snd = NULL;


//handle to brushes for drawing screen
HBRUSH gray = NULL;
HBRUSH borderBrushes[7];

//handle to Diamonds font
HFONT dfont = NULL;

//if a game is running
bool gameOn = false;

//if sound is on or off
// -turned on under WM_CREATE
bool sound = false;

//if initializing DirectSound failed
bool dirSndFailed = false;

//high scores
HiScore *scores = NULL;

//for actual game
Field *field = NULL;
bool isPaused = false;

//for DirectDraw
LPDIRECTDRAW        DirDrawObj = NULL;  // DirectDraw object
LPDIRECTDRAWSURFACE SurfaceFront = NULL;// fronbuffer surface
LPDIRECTDRAWSURFACE SurfaceBack = NULL; // backbuffer surface
LPDIRECTDRAWCLIPPER clipper = NULL;

//for DirectSound
LPDIRECTSOUND DirSoundObj = NULL;//DirectSound object

//prototypes
LRESULT  CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);
BOOL    CALLBACK NameDlgProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);
void DrawScreen(Field *field, Ball *ball);
void DrawMainMenu(HiScore *score);
void RackUpTimeBonus();

int InitDirectDraw();
void CleanUpDirDraw();
bool Flip();

int InitDirectSound();
void CleanUpDirSound();

//begin WinMain//////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance,
                                        LPSTR lpcmdline, int ncmdshow)
{
  InitCommonControls();

  WNDCLASSEX winclass;
  MSG msg;
  ATOM atom;
  hinst1 = hinstance;
  
  winclass.cbSize = sizeof(WNDCLASSEX);
  winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  winclass.lpfnWndProc = WindowProc;
  winclass.cbClsExtra = 0;
  winclass.cbWndExtra = 0;
  winclass.hInstance = hinstance;
  winclass.hIcon = LoadIcon(hinstance,MAKEINTRESOURCE(PROG_ICON));
  winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
  winclass.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
  winclass.lpszMenuName = "MyMenu";
  winclass.lpszClassName = CLASSNAME;
  winclass.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL),
                         MAKEINTRESOURCE(PROG_ICON), IMAGE_ICON, 16, 16, 0);

  atom=
  
   RegisterClassEx(&winclass);

  if(!atom)
  {return 0;}
  
  myHandle=
  
   CreateWindow(CLASSNAME,"Diamonds",
                                     WS_OVERLAPPED|WS_MINIMIZEBOX|
                                     WS_CAPTION | WS_SYSMENU,
                                     //WS_POPUP | WS_VISIBLE,
      CW_USEDEFAULT,CW_USEDEFAULT,WINWIDTH,
      WINHEIGHT,NULL,NULL,hinstance,NULL);
             
  if(!myHandle)
  {return 0;}

  if(InitDirectDraw() < 0)
  {
    CleanUpDirDraw();
    MessageBox(myHandle, 
               "Couldn't start DirectDraw engine on your computer. " 
               "Make sure you have at least version 8 of "
               "DirectX installed.", 
               "Error", MB_OK | MB_ICONEXCLAMATION);
    return 0;
  }
  
  if(InitDirectSound() < 0)
  {
    CleanUpDirSound();
    MessageBox(myHandle, 
               "Couldn't start DirectSound engine on your computer. " 
               "Make sure you have at least version 8 of "
               "DirectX installed.\nThis program will continue, however "
			   "sound will be disabled.", 
               "Error", MB_OK | MB_ICONEXCLAMATION);
   	dirSndFailed = true;
	PostMessage(myHandle,WM_COMMAND,GAME_SOUNDONOFF,0);
  }
  
  

  ShowWindow( myHandle, ncmdshow);

  //load keyboard accelerator table
  hmyaccel = LoadAccelerators(hinstance,"Accel");
   
  while( GetMessage(&msg,NULL,0,0) > 0 )
  {
  if (!TranslateAccelerator( myHandle, hmyaccel, &msg))//catch accel. keys
     {
     TranslateMessage(&msg);
     DispatchMessage(&msg);
     }
  }
  
  return msg.wParam;
  
}//end WinMain///////////////////////////////////////////////////////////////