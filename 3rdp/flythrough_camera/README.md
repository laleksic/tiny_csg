# flythrough_camera

![demonstration](https://j.gifs.com/gJMrk9.gif)

Single-header single-function C/C++ immediate-mode camera for your graphics demos

Just call `flythrough_camera_update` once per frame.

# Simple Example

Below is a fully-functional example program that works under command prompt.

Create a new Visual Studio project, drop this file in it, and it should just work.

```
#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera.h"

#include <Windows.h>
#include <stdio.h>

int main()
{
    printf("flythrough_camera test:\n");
    printf("hold right click, then move the mouse and press WASD/space/left ctrl\n");

    float pos[3] = { 0.0f, 0.0f, 0.0f };
    float look[3] = { 0.0f, 0.0f, 1.0f };
    const float up[3] = { 0.0f, 1.0f, 0.0f };

    LARGE_INTEGER then, now, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&then);

    POINT oldcursor;
    GetCursorPos(&oldcursor);

    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        QueryPerformanceCounter(&now);
        float delta_time_sec = (float)(now.QuadPart - then.QuadPart) / freq.QuadPart;

        POINT cursor;
        GetCursorPos(&cursor);

        // only move and rotate camera when right mouse button is pressed
        float activated = GetAsyncKeyState(VK_RBUTTON) ? 1.0f : 0.0f;

        float view[16];
        flythrough_camera_update(
            pos, look, up, view,
            delta_time_sec,
            100.0f * (GetAsyncKeyState(VK_LSHIFT) ? 2.0f : 1.0f) * activated,
            0.5f * activated,
            80.0f,
            cursor.x - oldcursor.x, cursor.y - oldcursor.y,
            GetAsyncKeyState('W'), GetAsyncKeyState('A'), GetAsyncKeyState('S'), GetAsyncKeyState('D'),
            GetAsyncKeyState(VK_SPACE), GetAsyncKeyState(VK_LCONTROL),
            FLYTHROUGH_CAMERA_LEFT_HANDED_BIT);

        if (activated)
        {
            printf("\n");
            printf("pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
            printf("look: %f, %f, %f\n", look[0], look[1], look[2]);
            printf("view: %f %f %f %f\n"
                   "      %f %f %f %f\n"
                   "      %f %f %f %f\n"
                   "      %f %f %f %f\n",
                 view[0],  view[1],  view[2],  view[3],
                 view[4],  view[5],  view[6],  view[7],
                 view[8],  view[9], view[10], view[11],
                view[12], view[13], view[14], view[15]);
            
            Sleep(100);
        }

        then = now;
        oldcursor = cursor;
    }

    return 0;
}
```

# OpenGL Example

![openglexample](https://j.gifs.com/gJMA3D.gif)

Below is a fully-functional example program that works using OpenGL.

Create a new Visual Studio project, drop this file in it, and it should just work.

```
#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera.h"

#include <Windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glu32.lib")

LRESULT CALLBACK MyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        ExitProcess(0);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    printf("flythrough_camera test:\n");
    printf("hold right click, then move the mouse and press WASD/space/left ctrl\n");
    
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = MyWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    wc.lpszClassName = TEXT("WindowClass");
    RegisterClassEx(&wc);

    RECT wr = { 0, 0, 640, 480 };
    AdjustWindowRect(&wr, 0, FALSE);
    HWND hWnd = CreateWindowEx(
        0, TEXT("WindowClass"),
        TEXT("BasicGL"),
        WS_OVERLAPPEDWINDOW,
        0, 0, wr.right - wr.left, wr.bottom - wr.top,
        0, 0, hInstance, 0);

    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    HDC hDC = GetDC(hWnd);

    int chosenPixelFormat = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, chosenPixelFormat, &pfd);

    HGLRC hGLRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hGLRC);

    ShowWindow(hWnd, SW_SHOWNORMAL);

    float pos[3] = { 0.0f, 0.0f, 0.0f };
    float look[3] = { 0.0f, 0.0f, 1.0f };
    const float up[3] = { 0.0f, 1.0f, 0.0f };

    LARGE_INTEGER then, now, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&then);

    POINT oldcursor;
    GetCursorPos(&oldcursor);

    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        QueryPerformanceCounter(&now);
        float delta_time_sec = (float)(now.QuadPart - then.QuadPart) / freq.QuadPart;

        POINT cursor;
        GetCursorPos(&cursor);

        // only move and rotate camera when right mouse button is pressed
        float activated = GetAsyncKeyState(VK_RBUTTON) ? 1.0f : 0.0f;

        float view[16];
        flythrough_camera_update(
            pos, look, up, view,
            delta_time_sec,
            10.0f * (GetAsyncKeyState(VK_LSHIFT) ? 2.0f : 1.0f) * activated,
            0.5f * activated,
            80.0f,
            cursor.x - oldcursor.x, cursor.y - oldcursor.y,
            GetAsyncKeyState('W'), GetAsyncKeyState('A'), GetAsyncKeyState('S'), GetAsyncKeyState('D'),
            GetAsyncKeyState(VK_SPACE), GetAsyncKeyState(VK_LCONTROL),
            0);

        if (activated)
        {
            printf("\n");
            printf("pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
            printf("look: %f, %f, %f\n", look[0], look[1], look[2]);
            printf("view: %f %f %f %f\n"
                   "      %f %f %f %f\n"
                   "      %f %f %f %f\n"
                   "      %f %f %f %f\n",
                 view[0],  view[1],  view[2],  view[3],
                 view[4],  view[5],  view[6],  view[7],
                 view[8],  view[9], view[10], view[11],
                view[12], view[13], view[14], view[15]);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(70.0, (double)(wr.right - wr.left) / (wr.bottom - wr.top), 0.001, 100.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(view);

        glBegin(GL_TRIANGLES);
        glColor3ub(255, 0, 0);
        glVertex3f(-1.0f, -1.0f, 10.0f);
        glColor3ub(0, 255, 0);
        glVertex3f(1.0f, -1.0f, 10.0f);
        glColor3ub(0, 0, 255);
        glVertex3f(0.0f, 1.0f, 10.0f);
        glEnd();

        SwapBuffers(hDC);

        then = now;
        oldcursor = cursor;
    }

    return 0;
}
```
