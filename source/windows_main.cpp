// Following Handmade Hero, The Molly Rocket series
//

#include <cstdint>
#include <iostream>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

#define local_persist static
#define global_variable static
#define internal static


struct WindowsOffscreenBuffer {
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int bytesPerPixel = 4;
};

struct WindowsWindowDimension {
    int height;
    int width;
};


// Global variables
global_variable bool running;
global_variable WindowsOffscreenBuffer globalBackBuffer;


// Functions:
//

// Required since lldb has no support for listening to OutputDebugString()
void DebugString(std::string dbg)
{
    std::cout << dbg;
}

WindowsWindowDimension WindowsGetWindowDimenstion(HWND window)
{
    RECT clientRect;
    GetClientRect(window, &clientRect);

    WindowsWindowDimension windowDimension;
    windowDimension.height = clientRect.bottom - clientRect.top;
    windowDimension.width = clientRect.right - clientRect.left;

    return windowDimension;
}

internal void RenderGradient(WindowsOffscreenBuffer *buffer, int xOffset, int yOffset)
{
    int width = buffer->width;
    int height = buffer->height;

    int pitch = width * buffer->bytesPerPixel;
    uint8_t *row = (uint8_t *)buffer->memory;
    for(int y = 0; y < height; y++)
    {
        uint32_t *pixel = (uint32_t *) row;
        for(int x = 0; x < width; x++)
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
internal void WindowsResizeDibSection(WindowsOffscreenBuffer *buffer, int width, int height)
{
    //TODO: Bulletproof this
    // Maybe free after

    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    BITMAPINFOHEADER bmiHeader;

    bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    bmiHeader.biWidth = width;
    bmiHeader.biHeight = -height;
    bmiHeader.biPlanes = 1;
    bmiHeader.biBitCount = 32; // For aligning memory
    bmiHeader.biCompression = BI_RGB;

    buffer->info.bmiHeader = bmiHeader;

    int bitmapMemorySize = (width * height) * buffer->bytesPerPixel;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

// Used to update the image on the window
internal void WindowsCopyBufToWin(HDC deviceContext, int width, int height, WindowsOffscreenBuffer buffer, int x, int y)
{
    // TODO: Correct aspect ratio
    StretchDIBits(deviceContext,
                  0, 0, width, height,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory,
                  &buffer.info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
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
            //
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

            WindowsWindowDimension dimensions = WindowsGetWindowDimenstion(window);

            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            WindowsCopyBufToWin(deviceContext, dimensions.width, dimensions.height, globalBackBuffer, x, y);

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
int CALLBACK WinMain(HINSTANCE instance,     // The Window Instance
                     HINSTANCE prevInstance, // The Previous Instance
                     LPSTR cmdLine,          // The Command Line
                     int nCmdShow) {         // Show Code??


    WNDCLASS windowClass = {};

    WindowsResizeDibSection(&globalBackBuffer, 1440, 720);

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

                RenderGradient(&globalBackBuffer, xOffset, yOffset);

                {
                    HDC deviceContext = GetDC(window);
                    WindowsWindowDimension dimension = WindowsGetWindowDimenstion(window);

                    WindowsCopyBufToWin(deviceContext, dimension.width, dimension.height, globalBackBuffer, 0, 0);
                    xOffset++;
                    yOffset++;
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
// vim: ts=4 sts=4 sw=4 et
