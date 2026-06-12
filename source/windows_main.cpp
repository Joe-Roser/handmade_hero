// Following Handmade Hero, The Molly Rocket series
//

#include <cstdint>
#include <iostream>
#include <minwindef.h>
#include <windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <winerror.h>
#include <wingdi.h>
#include <winnt.h>

#define local_persist static
#define global_variable static
#define internal static


struct Win32OffscreenBuffer {
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int bytesPerPixel = 4;
};

struct Win32WindowDimension {
    int height;
    int width;
};


// Global variables
global_variable bool running;
global_variable Win32OffscreenBuffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER secondary_buffer;


// Functions:
// 
// 

// Loading from OS
// 

// Load the windows functions into the program rather than referencing them
// XInputGetState
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(xinput_get_state);
XINPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_variable xinput_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVIBRATION)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_variable xinput_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// XSound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// Required since lldb has no support for listening to OutputDebugString() by default
internal void debug_string(std::string dbg) { std::cout << dbg; }


// Bring in the functions from xinput
internal void win32_load_xinput() {
    HMODULE xInputLibrary = LoadLibrary("xinput1_4.dll");

    if(xInputLibrary) {
        XInputGetState = (xinput_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
        if (!XInputGetState) { XInputGetState = XInputGetStateStub; debug_string("Couldn't load XInput get"); }

        XInputSetState = (xinput_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
        if (!XInputSetState) { XInputSetState = XInputSetStateStub; debug_string("Couldn't load XInput set"); }
    }
}

// Setting up the sound buffer
internal void win32_emit_dsound(HWND window, int32_t samples_per_second, int32_t buffer_size) {
    // Load the lib
    HMODULE dSoundLibrary = LoadLibrary("dsound.dll");

    if(dSoundLibrary)
    {
        // Get a DSound object
        direct_sound_create *directSoundCreate = (direct_sound_create *)GetProcAddress(dSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND direct_sound;
        if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &direct_sound, 0)))
        {
            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.cbSize = 0;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = wave_format.nChannels * wave_format.wBitsPerSample / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;

            if(SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                // Create a primary buffer

                DSBUFFERDESC p_buffer_description = {};
                p_buffer_description.dwSize = sizeof(p_buffer_description);
                p_buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primary_buffer;

                if(SUCCEEDED(direct_sound->CreateSoundBuffer(&p_buffer_description, &primary_buffer, 0)))
                {
                    if(SUCCEEDED(primary_buffer->SetFormat(&wave_format)))
                    {
                    }
                    else
                    {
                        debug_string("uh oh, no sound 1\n");
                    }
                    }

                // Create a secondary buffer
                DSBUFFERDESC s_buffer_description = {};
                s_buffer_description.dwSize = sizeof(s_buffer_description);
                s_buffer_description.dwFlags = 0;
                s_buffer_description.dwBufferBytes = buffer_size;
                s_buffer_description.lpwfxFormat = &wave_format;

                HRESULT error = direct_sound->CreateSoundBuffer(&s_buffer_description, &secondary_buffer, 0);
                if(SUCCEEDED(error))
                {
                    debug_string("Sound Buffer Created Successfully\n");
                }
                else
                {
                    debug_string("uh oh, no sound 2\n");
                }
            }
            else {

            }
            // Start playing
        }
    }
}

// Calculate the window dimensions
internal Win32WindowDimension win32_get_window_dimenstion(HWND window) {
    RECT clientRect;
    GetClientRect(window, &clientRect);

    Win32WindowDimension windowDimension;
    windowDimension.height = clientRect.bottom - clientRect.top;
    windowDimension.width = clientRect.right - clientRect.left;

    return windowDimension;
}

// Render the weird gradient
internal void render_gradient(Win32OffscreenBuffer *buffer, int xOffset, int yOffset) {
    int width = buffer->width;
    int height = buffer->height;

    int pitch = width * buffer->bytesPerPixel;
    uint8_t *row = (uint8_t *)buffer->memory;
    for(int y = 0; y < height; y++) {
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
internal void win32_resize_dib_section(Win32OffscreenBuffer *buffer, int width, int height) {
    //TODO: Bulletproof this
    // Maybe free after

    if(buffer->memory) {
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
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

// Overwrite the current buffer with a new buffer
internal void win32_copy_buf_to_win(HDC deviceContext, int width, int height, Win32OffscreenBuffer *buffer, int x, int y) {
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
LRESULT CALLBACK MainWindowCallBack(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
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
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: break;
        case WM_SYSKEYUP:
        case WM_KEYUP: {
            uint32_t vKeyCode = wParam;
            bool wasDown = ((lParam & (1 << 30)) != 0);
            bool isDown = ((lParam & (1 << 31)) == 0);

            int32_t altWasDown = (lParam & (1 << 29));

            if(wasDown != isDown) {
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
                } else if( ( altWasDown ) && ( vKeyCode == VK_F4 ) ) {
                    running = false;
                } else if (vKeyCode == VK_F4) {
                    debug_string("<F4>\n");
                }
            }

        } break;
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);

            Win32WindowDimension dimensions = win32_get_window_dimenstion(window);

            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            win32_copy_buf_to_win(deviceContext, dimensions.width, dimensions.height, &globalBackBuffer, x, y);

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
int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int nCmdShow) {
    win32_load_xinput();

    win32_resize_dib_section(&globalBackBuffer, 1920, 1080);

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
        if (window != NULL) {
            // Graphics test
            int xOffset = 0;
            int yOffset = 0;

            // Sound system test defs
            int samples_pr_sec = 48000;
            int tone_hz = 261;
            uint32_t running_sample_index = 0;
            int square_wave_period = samples_pr_sec/tone_hz;
            int half_square_wave_period = square_wave_period/2;
            int bytes_per_sample = sizeof(int16_t) * 2;
            int buffer_size = samples_pr_sec * bytes_per_sample;
            int tone_volume = 4000;

            win32_emit_dsound(window, samples_pr_sec, buffer_size);
            secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);

            // Calls message loop from windows
            running = true;
            while(running)
            {
                // Message loop
                MSG message;
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
                    if(message.message == WM_QUIT) {
                        running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                // Controller input
                for(DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++) {
                    XINPUT_STATE controllerState;
                    if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS) {
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

                        if(aButton) {
                            yOffset += 2;
                            PXINPUT_VIBRATION vibration;
                            vibration->wLeftMotorSpeed = 60000;
                            vibration->wRightMotorSpeed = 60000;
                            XInputSetState(controllerIndex, vibration);
                        }
                    }
                    else {}
                }

                render_gradient(&globalBackBuffer, xOffset, yOffset);

                // Direct Sound Output Test
                DWORD play_cursor;
                DWORD write_cursor;
                if(SUCCEEDED(secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor))) {
                    DWORD bytes_to_lock = running_sample_index * bytes_per_sample % buffer_size;
                    DWORD bytes_to_write;
                    if (bytes_to_lock > play_cursor) {
                        bytes_to_write = buffer_size - bytes_to_lock;
                        bytes_to_write += play_cursor;
                    } else {
                        bytes_to_write = play_cursor - bytes_to_lock;
                    }

                    void *region_1;
                    DWORD region_1_size;
                    void *region_2;
                    DWORD region_2_size;

                    HRESULT hr = secondary_buffer->Lock(bytes_to_lock, bytes_to_write, &region_1, &region_1_size, &region_2, &region_2_size, 0);

                    std::cout << hr << '\n';

                    if(SUCCEEDED(hr)) {
                        // Good place for an assert
                        int16_t *sample_out = (int16_t *)region_1;
                        DWORD region_1_sample_count = region_1_size / bytes_per_sample;

                        for (DWORD SampleIndex = 0; SampleIndex < region_1_sample_count; SampleIndex++) {
                            int16_t sample_val = ((running_sample_index++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;

                            *sample_out++ = sample_val;
                            *sample_out++ = sample_val;
                        }
                        sample_out = (int16_t *)region_2;
                        DWORD region_2_sample_count = region_2_size / bytes_per_sample;
                        for (DWORD SampleIndex = 0; SampleIndex < region_2_sample_count; SampleIndex++) {
                            int16_t sample_val = ((running_sample_index++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;

                            *sample_out++ = sample_val;
                            *sample_out++ = sample_val;
                        }

                        // Need to unlock
                        if(SUCCEEDED(secondary_buffer->Unlock(region_1, region_1_size, region_2, region_2_size))) {
                        } else {
                        debug_string("Error: Failed to unlock audio buffern");
                        }
                    } else {
                        debug_string("Error: Failed to lock audio buffer\n");
                    }


                } else {
                    debug_string("Error: Failed to get audio buffer positions\n");
                }

                {
                    HDC deviceContext = GetDC(window);
                    Win32WindowDimension dimension = win32_get_window_dimenstion(window);
                    win32_copy_buf_to_win(deviceContext, dimension.width, dimension.height, &globalBackBuffer, 0, 0);

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
