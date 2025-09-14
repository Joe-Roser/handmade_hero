// Following Handmade Hero, The Molly Rocket series
//

#include <cstdint>
#include <iostream>
#include <windows.h>
#include <Xinput.h>

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
// 

// Loading from OS
// 

// Load the windows functions into the program rather than referencing them
// XInputGetState
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(xinput_get_state);
XINPUT_GET_STATE(XInputGetStateStub) { return 0; }
global_variable xinput_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVIBRATION)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub) { return 0; }
global_variable xinput_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// Required since lldb has no support for listening to OutputDebugString()
internal void debug_string(std::string dbg)
{
    std::cout << dbg;
}


// Bring in the functions from xinput
internal void windows_load_xinput()
{
    HMODULE xInputLibrary = LoadLibrary("xinput1_4.dll");

    if(xInputLibrary)
    {
        XInputGetState = (xinput_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (xinput_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
    }
}

// Calculate the window dimensions
internal WindowsWindowDimension windows_get_window_dimenstion(HWND window)
{
    RECT clientRect;
    GetClientRect(window, &clientRect);

    WindowsWindowDimension windowDimension;
    windowDimension.height = clientRect.bottom - clientRect.top;
    windowDimension.width = clientRect.right - clientRect.left;

    return windowDimension;
}

// Render the weird gradient
internal void render_gradient(WindowsOffscreenBuffer *buffer, int xOffset, int yOffset)
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

// Resize window
internal void windows_resize_dib_section(WindowsOffscreenBuffer *buffer, int width, int height)
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

// Overwrite the current buffer with a new buffer
internal void windows_copy_buf_to_win(HDC deviceContext, int width, int height, WindowsOffscreenBuffer *buffer, int x, int y)
{
    // TODO: Correct aspect ratio
    StretchDIBits(deviceContext,
                  0, 0, width, height,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->info,
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
            debug_string("WM_ACTIVATEAPP\n");
        } break;
        case WM_SYSKEYDOWN: {
        } break;
        case WM_SYSKEYUP: {
        } break;
        case WM_KEYDOWN: {
            uint32_t vKeyCode = wParam;
            bool wasDown = ((lParam & (1 << 30)) != 0);
            bool isDown = ((lParam & (1 << 31)) == 0);

            if(wasDown != isDown){
                if (vKeyCode == 'W') {
                    debug_string("w\n");
                } else if(vKeyCode == 'A') {
                    debug_string("a\n");
                } else if (vKeyCode == 'S') {
                    debug_string("s\n");
                } else if (vKeyCode == 'D') {
                    debug_string("d\n");
                } else if (vKeyCode == 'Q') {
                    debug_string("q\n");
                } else if (vKeyCode == 'E') {
                    debug_string("e\n");
                } else if (vKeyCode == VK_UP) {
                    debug_string("up\n");
                } else if (vKeyCode == VK_DOWN){
                    debug_string("down\n");
                } else if (vKeyCode == VK_LEFT){
                    debug_string("left\n");
                } else if (vKeyCode == VK_RIGHT){
                    debug_string("right\n");
                } else if (vKeyCode == VK_ESCAPE){
                    debug_string("escape\n");
                } else if (vKeyCode == VK_SPACE){
                    debug_string("space\n");
                }
            }

        } break;
        case WM_KEYUP: {
            uint32_t vKeyCode = wParam;
        } break;
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);

            WindowsWindowDimension dimensions = windows_get_window_dimenstion(window);

            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            windows_copy_buf_to_win(deviceContext, dimensions.width, dimensions.height, &globalBackBuffer, x, y);

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


    windows_load_xinput();

    WNDCLASS windowClass = {};

    windows_resize_dib_section(&globalBackBuffer, 1440, 720);

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
                // Message loop
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

                // Controller input
                for(DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++){
                    XINPUT_STATE controllerState;
                    if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // Controller available
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
                        bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
                        bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool lShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool rShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
                        bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
                        bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
                        bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16_t stickX = pad->sThumbLX;
                        int16_t stickY = pad->sThumbLY;

                        if(aButton)
                        {
                            yOffset += 2;
                            PXINPUT_VIBRATION vibration;
                            vibration->wLeftMotorSpeed = 60000;
                            vibration->wRightMotorSpeed = 60000;
                            XInputSetState(controllerIndex, vibration);
                        }
                    }
                    else
                    {

                    }
                }

                render_gradient(&globalBackBuffer, xOffset, yOffset);

                {
                    HDC deviceContext = GetDC(window);
                    WindowsWindowDimension dimension = windows_get_window_dimenstion(window);
                    windows_copy_buf_to_win(deviceContext, dimension.width, dimension.height, &globalBackBuffer, 0, 0);

                    xOffset++;
                    yOffset++;
                }
            }
        } else {
            // TODO: Logging
            debug_string("Uh oh, Failed to create window!!\n");
        }
    } else {
        // TODO: Implement logging
        debug_string("Uh oh, Faled to register class!!\n");
    }

    return (0);
}

//
// vim: ts=4 sts=4 sw=4 et
