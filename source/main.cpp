// Following Handmade Hero, The Molly Rocket series
//

#include <cstdint>
#include <iostream>
#include <windows.h>
#include <winuser.h>

#define local_persist static
#define global_variable static
#define internal static


// Global variables
global_variable bool running;

global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable int bitmapWidth;
global_variable int bitmapHeight;
global_variable int bytesPerPixel = 4;


// Functions:
//

// Required since lldb has no support for listening to OutputDebugString()
void DebugString(std::string dbg)
{
  std::cout << dbg;
}

internal void RenderGradient(int xOffset, int yOffset)
{
  int width = bitmapWidth;
  int height = bitmapHeight;

  int pitch = width * bytesPerPixel;
  uint8_t *row = (uint8_t *)bitmapMemory;
  for(int y = 0; y < bitmapHeight; y++)
  {
    uint32_t *pixel = (uint32_t *) row;
    for(int x = 0; x < bitmapWidth; x++)
    {
      /* 
       * BBGGRRxx
       */
      uint8_t red = 0;
      uint8_t blue = x + xOffset;
      uint8_t green = y + yOffset;

      *pixel++ = (uint32_t)((red << 16) | (green << 8) | blue);
    }
    row += pitch;
  }
}

// Used to resize the window
internal void WindowsResizeDibSection(int width, int height)
{
  //TODO: Bulletproof this
  // Maybe free after

  if(bitmapMemory)
  {
    VirtualFree(bitmapMemory, 0, MEM_RELEASE);
  }

  bitmapWidth = width;
  bitmapHeight = height;

  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = bitmapWidth;
  bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32; // For aligning memory
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = (bitmapWidth * bitmapHeight) * bytesPerPixel;
  bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  RenderGradient(0, 0);
}

// Used to update the image on the window
internal void WindowsUpdateWindow(HDC deviceContext, RECT *clientRect, int x, int y, int width, int height)
{
  int windowWidth = clientRect->right - clientRect->left;
  int windowHeight = clientRect->bottom - clientRect->top;

  StretchDIBits(deviceContext,
                0, 0, bitmapWidth, bitmapHeight,     // Old dimensions
                0, 0, windowWidth, windowHeight,     // New dimensions
                bitmapMemory,                        // The Bitmap
                &bitmapInfo,                         // The bitmap info
                DIB_RGB_COLORS,                      // Type of colours
                SRCCOPY);                            // What type of bitwise operations do I want to use
}

// Handles the messages from windows
LRESULT CALLBACK MainWindowCallBack(HWND window,   // Window handle
                                    UINT msg,      // The message
                                    WPARAM wParam, //
                                    LPARAM lParam) //
{
  LRESULT result = 0;

  switch (msg) {
    case WM_SIZE: {
      RECT clientRect;
      GetClientRect(window, &clientRect);
      int height = clientRect.bottom - clientRect.top;
      int width = clientRect.right - clientRect.left;

      WindowsResizeDibSection(width, height);

      DebugString("WM_SIZE\n");
    } break;
    case WM_DESTROY: {
      // TODO: Handle this as an error
      running = false;
    } break;
    case WM_CLOSE: {
      // TODO: Handle with message to user
      running = false;
    } break;
    case WM_ACTIVATEAPP: {
      DebugString("WM_ACTIVATEAPP\n");
    } break;
    case WM_PAINT: {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(window, &paint);

      RECT clientRect;
      GetClientRect(window, &clientRect);

      int x = paint.rcPaint.left;
      int y = paint.rcPaint.top;
      int height = paint.rcPaint.bottom - paint.rcPaint.top;
      int width = paint.rcPaint.right - paint.rcPaint.left;
      WindowsUpdateWindow(deviceContext, &clientRect, x, y, width, height);

      EndPaint(window, &paint);
    }
    default: {
      // DebugStringA("default\n");
      result = DefWindowProc(window, msg, wParam, lParam);
    } break;
  }

  return result;
}

// The entrypoint to the app
int CALLBACK WinMain(_In_ HINSTANCE instance,     // The Window Instance
                     _In_ HINSTANCE prevInstance, // The Previous Instance
                     _In_ LPSTR cmdLine,          // The Command Line
                     _In_ int nCmdShow) {         // Show Code??


  WNDCLASS windowClass = {};

  // TODO: Check thesee things still matter
  windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = MainWindowCallBack;
  // WindowClass.cbClsExtra = ;
  // WindowClass.cbWndExtra = ;
  windowClass.hInstance = instance;
  // WindowClass.hIcon = ;
  // WindowClass.hCursor = ;
  // WindowClass.hbrBackground = ;
  // WindowClass.lpszMenuName = ;
  windowClass.lpszClassName = "HandmadeHeroWindowClass";


  if (RegisterClass(&windowClass)) {
    HWND window = CreateWindowEx(0,                                // Window style
                                       windowClass.lpszClassName,        // Window class name
                                       "Handmade Hero",                  // Window name
                                       WS_OVERLAPPEDWINDOW | WS_VISIBLE, // The style of window created
                                       CW_USEDEFAULT,                    // initial x
                                       CW_USEDEFAULT,                    // initial y
                                       CW_USEDEFAULT,                    // width
                                       CW_USEDEFAULT,                    // height
                                       0,                                // Handle to the parent or owner window
                                       0,                                // Handle to a menu
                                       instance,                         // handle of the instance
                                       0);                               // Pointer to value to be passed to created window
    if (window != NULL)
    {
        int xOffset = 0;
        int yOffset = 0;
      // Calls message loop from windows
      running = true;
      while(running)
      {
        MSG message;
        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
          if(message.message == WM_QUIT)
          {
            running = false;
          }

          TranslateMessage(&message);
          DispatchMessage(&message);
        }

        RenderGradient(xOffset, yOffset);

        {
        HDC deviceContext = GetDC(window);
        RECT clientRect;
        GetClientRect(window, &clientRect);
        int windowHeight = clientRect.bottom - clientRect.top;
        int windowWidth = clientRect.right - clientRect.left;

        WindowsUpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);
        xOffset++;
        }
      }
    } else {
      // TODO: Logging
    DebugString("Uh oh, Failed to create window!!\n");
    }
  } else {
    // TODO: Implement logging
    DebugString("Uh oh, Faled to register class!!\n");
  }

  return (0);
}

//
// vim: ts=2 sts=2 sw=2 et
