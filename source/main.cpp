// Following Handmade Hero, The Molly Rocket series
//

#include <iostream>
#include <windef.h>
#include <windows.h>
#include <wingdi.h>

#define local_persist static
#define global_variable static
#define internal static

// Global variables
global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable HBITMAP bitmapHandle;


// Functions:
//

// Required since lldb has no support for listening to OutputDebugString()
void DebugString(std::string dbg)
{
  std::cout << dbg;
}

// Used to resize the window
internal void WindowsResizeDibSection(int width, int height)
{
  //TODO: Bulletproof this
  // Maybe free after

  HDC bitmapDeviceContext;

  if (bitmapHandle) {
    DeleteObject(bitmapHandle);
  }

  if(!bitmapDeviceContext)
  {
    // TODO: Should these ever be recreated
    bitmapDeviceContext = CreateCompatibleDC(0);
  }

  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = width;
  bitmapInfo.bmiHeader.biHeight = height;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  bitmapHandle = CreateDIBSection(bitmapDeviceContext,
                                          &bitmapInfo,
                                          DIB_RGB_COLORS,
                                          &bitmapMemory,
                                          0, 0);
}

// Used to update the image on the window
internal void WindowsUpdateWindow(HDC deviceContext, int x, int y, int width, int height)
{
  StretchDIBits(deviceContext,           // Device context
                x, y, width, height,     // Old dimensions
                x, y, width, height,     // New dimensions
                bitmapMemory,      // 
                &bitmapInfo, // 
                DIB_RGB_COLORS,          // Type of colours
                SRCCOPY);                // What type of bitwise operations do I want to use
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

      int x = paint.rcPaint.left;
      int y = paint.rcPaint.top;
      int height = paint.rcPaint.bottom - paint.rcPaint.top;
      int width = paint.rcPaint.right - paint.rcPaint.left;
      WindowsUpdateWindow(deviceContext, x, y, width, height);

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
    HWND windowHandle = CreateWindowEx(0,                                // Window style
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
    if (windowHandle != NULL)
    {
      // Calls message loop from windows
      running = true;
      while(running)
      {
        MSG message;
        BOOL messageResult = GetMessage(&message, 0, 0, 0);
        if (messageResult > 0)
        {
          TranslateMessage(&message);
          DispatchMessage(&message);
        }
        else
        {
          break;
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
