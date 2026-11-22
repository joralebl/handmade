/* =====================================================================
   $File: win32_handmade.cpp $
   $Author: joralebl $
   ===================================================================== */

#include <cstdint>
#include <windows.h>
#include <stdint.h>

#if !defined(internal)
#define internal static
#endif
#define local_persist static
#define global static

// unsigned integers
typedef uint8_t u8;     // 1-byte long unsigned integer
typedef uint16_t u16;   // 2-byte long unsigned integer
typedef uint32_t u32;   // 4-byte long unsigned integer
typedef uint64_t u64;   // 8-byte long unsigned integer
// signed integers
typedef int8_t s8;      // 1-byte long signed integer
typedef int16_t s16;    // 2-byte long signed integer
typedef int32_t s32;    // 4-byte long signed integer
typedef int64_t s64;    // 8-byte long signed integer


// TODO: This is a global for now...
// These are for early debugging. REWORK THIS LATER!!!!!!!
global bool running;
global BITMAPINFO bitmapInfo;
global void *bitmapMem;  
global int bitmapWidth;
global int bitmapHeight;

global int bytesPerPixel = 4; // 3 bytes rgb, 1 byte for padding (multiple of 4)

internal void RenderWeirdGradient(int xOffset, int yOffset)
{
    int width = bitmapWidth;
    int height = bitmapHeight;
    int pitch = width*bytesPerPixel;
    u8 *row = (u8 *)bitmapMem;
    for(int y = 0; y < bitmapHeight; ++y)
    {
        
        u32 *pixel = (u32 *)row; 
        for(int x = 0; x < bitmapWidth; ++x)
        {
            u8 b = (x + xOffset);
            u8 g = (y + yOffset);

            *pixel++ = ((g << 8) | b);

        }
        row += pitch;
    }
    
}

internal void
Win32ResizeDIBSection(int width, int height)
{

    if(bitmapMem)
    {
        VirtualFree(bitmapMem, 0, MEM_RELEASE);
    }
    
    bitmapWidth = width;
    bitmapHeight = height;

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    // bitmapInfo.bmiHeader.biSizeImage = 0;
    // bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    // bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    // bitmapInfo.bmiHeader.biClrUsed = 0;
    // bitmapInfo.bmiHeader.biClrImportant = 0;

    int bitmapMemSize = (bitmapWidth*bitmapHeight)*bytesPerPixel;
    bitmapMem = VirtualAlloc(0, bitmapMemSize,MEM_COMMIT,PAGE_READWRITE);

    //TODO: Probably clear this to black.
}

internal void
Win32UpdateWindow(HDC deviceContext, RECT *windowRect, int x, int y, int width, int height)
{
    int windowWidth = windowRect->right - windowRect->left;
    int windowHeight = windowRect->bottom - windowRect->top;
    StretchDIBits(
          deviceContext,
          // x, y, width, height,
          // x, y, width, height,
          0, 0, bitmapWidth, bitmapHeight,
          0, 0, windowWidth, windowHeight,
          bitmapMem,
          &bitmapInfo,
          DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
{
    LRESULT result = {};
    switch(message)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(window, &clientRect);
            int height = clientRect.bottom - clientRect.top;
            int width = clientRect.right - clientRect.left;
            Win32ResizeDIBSection(width, height);
        } break;

        case WM_DESTROY:
        {
            // TODO: Handle this with an error?
            running = false;
        } break;

        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user?
            running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;

            
            RECT clientRect;
            GetClientRect(window, &clientRect);
            
            Win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);
            EndPaint(window, &paint);
        } break;

        default:
        {
            
            OutputDebugStringA("default\n");
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }

    return(result);
  
}

int CALLBACK
WinMain(
        HINSTANCE instance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nShowCmd)
{
    WNDCLASSA WindowClass = {};

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        HWND windowHandle =
            CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0);

        if(windowHandle != NULL)
        {

            int xOffset = 0;
            int yOffset = 0;
            running = true;
            while(running)
            {
                MSG message;
                while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
                {
                    if(message.message == WM_QUIT)
                    {
                        running = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                    
                }

                RenderWeirdGradient(xOffset, yOffset);
                HDC deviceContext = GetDC(windowHandle);
                RECT clientRect;
                GetClientRect(windowHandle, &clientRect);
                int height = clientRect.bottom - clientRect.top;
                int width = clientRect.right - clientRect.left;
                Win32UpdateWindow(deviceContext, &clientRect, 0, 0, width, height);
                ReleaseDC(windowHandle, deviceContext);

                ++xOffset;
                
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        // TODO: Logging
    }
    return (0);
}
