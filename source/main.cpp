// Following Handmade Hero, The Molly Rocket series
//
#include <iostream>

#include <windows.h>

void DebugString(std::string dbg)
{
  std::cout << dbg;
}

LRESULT CALLBACK MainWindowCallBack(HWND window,   // Window handle
                                    UINT msg,      // The message
                                    WPARAM wParam, //
                                    LPARAM lParam) //
{
  LRESULT result = 0;

  switch (msg) {
    case WM_SIZE: {
      DebugString("WM_SIZE\n");
    } break;
    case WM_DESTROY: {
      DebugString("WM_DESTROY\n");
    } break;
    case WM_CLOSE: {
      DebugString("WM_CLOSE\n");
    } break;
    case WM_ACTIVATEAPP: {
      DebugString("WM_ACTIVATEAPP\n");
    } break;
    case WM_PAINT: {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(window, &paint);

      LONG height = paint.rcPaint.bottom - paint.rcPaint.top;
      LONG width = paint.rcPaint.right - paint.rcPaint.left;
      PatBlt(deviceContext, paint.rcPaint.left, paint.rcPaint.top, width, height, WHITENESS);

      EndPaint(window, &paint);
    }
    default: {
      // DebugStringA("default\n");
      result = DefWindowProc(window, msg, wParam, lParam);
    } break;
  }

  return result;
}

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
      // Message Loop from windows
      MSG message;
      for(;;)
      {
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
