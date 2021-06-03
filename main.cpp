#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <iostream>
#include "resources.h"
#include "rasterizer.h"

#define MAX_LOADSTRING 100

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

static HINSTANCE hInst;
static WCHAR szTitle[MAX_LOADSTRING];
static WCHAR szWindowClass[MAX_LOADSTRING];
static int frameCount = 0;
static COLORREF* final = (COLORREF*)calloc(400 * 400, sizeof(COLORREF));
static std::vector<float> depth(400 * 400);
static int mxVel = 0;
static int myVel = 0;
static int mwVel = 0;
static bool mouse1 = false;
static bool mouse2 = false;
static bool frameRendered = false;
static float camera[5]{0, 0, -5, 0, 0};
static float shadowCamera[5]{-5, 3, -5, 0, 0.78539816339};
static COLORREF* shadowMap = (COLORREF*)calloc(400 * 400, sizeof(COLORREF));

static Rasterizer everything(6, 4);
static float v1[4];
float* everythingVertex(std::vector<float> &attributes, std::vector<float> &varyings) {
    float x = attributes[0] - everything.uniforms[7], y = attributes[1] - everything.uniforms[8], z = attributes[2] - everything.uniforms[9];
    float c = everything.uniforms[10], s = everything.uniforms[11];
    float t = x * c + z * s;
    z = z * c - x * s;
    x = t;
    c = everything.uniforms[12], s = everything.uniforms[13];
    t = y * c - z * s;
    z = z * c + y * s;
    y = t;
    varyings[1] = x * 25 + 200;
    varyings[2] = y * 25 + 200;
    varyings[3] = z * 10 + 255;

    c = everything.uniforms[0], s = everything.uniforms[1];
    x = attributes[0] - everything.uniforms[4], y = attributes[1] - everything.uniforms[5], z = attributes[2] - everything.uniforms[6];
    float nx = attributes[3], ny = attributes[4], nz = attributes[5];
    if(nx * x + ny * y + nz * z > 0) { return 0; }
    attributes[0] = x * c + z * s;
    z = z * c - x * s;
    x = attributes[0];
    c = everything.uniforms[2], s = everything.uniforms[3];
    attributes[1] = y * c - z * s;
    z = z * c + y * s;
    y = attributes[1];
    varyings[0] = max(0.2f, min(1.f, nx * -everything.uniforms[14] + ny * -everything.uniforms[15] + nz * -everything.uniforms[16]));
    v1[0] = x;
    v1[1] = y;
    v1[2] = z;
    v1[3] = z * 0.004;
    return v1;
}
static float c1[4]{0, 0, 0, 255};
float* everythingFragment(std::vector<float> &varyings, int X, int Y) {
    float shade = varyings[0];
    int x = varyings[1], y = varyings[2], z = varyings[3];
    if(shade > 0.2 && x < 400 && x > 0 && y < 400 && y > 0 && z < 250 && ((shadowMap[x + y * 400] >> 16) & 0xFF) < z) { shade = 0.2; }
    c1[0] = shade * 255;
    c1[1] = shade * 255;
    c1[2] = shade * 255;
    return c1;
}

Rasterizer shadow = Rasterizer(6, 4);
static float v2[4]{0, 0, 0, 0.04};
float* shadowVertex(std::vector<float> &attributes, std::vector<float> &varyings) {
    float c = shadow.uniforms[0], s = shadow.uniforms[1];
    float x = attributes[0] - shadow.uniforms[4], y = attributes[1] - shadow.uniforms[5], z = attributes[2] - shadow.uniforms[6];
    attributes[0] = x * c + z * s;
    z = z * c - x * s;
    x = attributes[0];
    c = shadow.uniforms[2], s = shadow.uniforms[3];
    attributes[1] = y * c - z * s;
    z = z * c + y * s;
    y = attributes[1];
    varyings[0] = z;
    v2[0] = x;
    v2[1] = y;
    v2[2] = z;
    return v2;
};
static float c2[4]{0, 0, 0, 255};
float* shadowFragment(std::vector<float> &varyings, int x, int y) {
    c2[0] = varyings[0] * 10.f;
    return c2;
};

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_SOFTWARERASTERIZER, szWindowClass, MAX_LOADSTRING);
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOFTWARERASTERIZER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SOFTWARERASTERIZER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    RegisterClassEx(&wcex);
    hInst = hInstance;
    
    HWND hWnd = CreateWindowEx(0, szWindowClass, szTitle, (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX), CW_USEDEFAULT, CW_USEDEFAULT, 416, 459, nullptr, nullptr, hInstance, nullptr);
    if(!hWnd) {
        return FALSE;
    }

    #ifndef HID_USAGE_PAGE_GENERIC
    #define HID_USAGE_PAGE_GENERIC ((unsigned short) 0x01)
    #endif
    #ifndef HID_USAGE_GENERIC_MOUSE
    #define HID_USAGE_GENERIC_MOUSE ((unsigned short) 0x02)
    #endif
    #ifndef HID_USAGE_GENERIC_KEYBOARD
    #define HID_USAGE_GENERIC_KEYBOARD ((unsigned short) 0x06)
    #endif
    RAWINPUTDEVICE rid[2];
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hWnd;
    rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid[1].dwFlags = RIDEV_INPUTSINK;
    rid[1].hwndTarget = hWnd;
    RegisterRawInputDevices(rid, 2, sizeof(rid[0]));

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SOFTWARERASTERIZER));
    MSG msg;
    while(GetMessage(&msg, nullptr, 0, 0)) {
        if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch(message) {
        case WM_CREATE:
            SetTimer(hWnd, 1, 16, NULL);
            everything.vertices.insert(everything.vertices.end(), {-1, -1, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 1, -1, -1, 0, 0, -1, 1, 1, -1, 0, 0});
            everything.indices.insert(everything.indices.end(), {0, 1, 2, 0, 2, 3});
            everything.vertices.insert(everything.vertices.end(), {1, -1, 1, 1, 0, 0, 1, -1, -1, 1, 0, 0, 1, 1, -1, 1, 0, 0, 1, 1, 1, 1, 0, 0});
            everything.indices.insert(everything.indices.end(), {4, 5, 6, 4, 6, 7});
            everything.vertices.insert(everything.vertices.end(), {-1, -1, -1, 0, 0, -1, 1, -1, -1, 0, 0, -1, 1, 1, -1, 0, 0, -1, -1, 1, -1, 0, 0, -1});
            everything.indices.insert(everything.indices.end(), {8, 9, 10, 8, 10, 11});
            everything.vertices.insert(everything.vertices.end(), {-1, -1, 1, 0, 0, 1, 1, -1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, -1, 1, 1, 0, 0, 1});
            everything.indices.insert(everything.indices.end(), {12, 13, 14, 12, 14, 15});
            everything.vertices.insert(everything.vertices.end(), {-1, -1, -1, 0, -1, 0, 1, -1, -1, 0, -1, 0, 1, -1, 1, 0, -1, 0, -1, -1, 1, 0, -1, 0});
            everything.indices.insert(everything.indices.end(), {16, 17, 18, 16, 18, 19});
            everything.vertices.insert(everything.vertices.end(), {-1, 1, -1, 0, 1, 0, 1, 1, -1, 0, 1, 0, 1, 1, 1, 0, 1, 0, -1, 1, 1, 0, 1, 0});
            everything.indices.insert(everything.indices.end(), {20, 21, 22, 20, 22, 23});
            everything.vertices.insert(everything.vertices.end(), {-5, 1, -5, 0, -1, 0, 5, 1, -5, 0, -1, 0, 5, 1, 5, 0, -1, 0, -5, 1, 5, 0, -1, 0});
            everything.indices.insert(everything.indices.end(), {24, 25, 26, 24, 26, 27});
            everything.uniforms.insert(everything.uniforms.end(), {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

            shadow.vertices.insert(shadow.vertices.end(), {-1, -1, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 1, -1, -1, 0, 0, -1, 1, 1, -1, 0, 0});
            shadow.indices.insert(shadow.indices.end(), {0, 1, 2, 0, 2, 3});
            shadow.vertices.insert(shadow.vertices.end(), {1, -1, 1, 1, 0, 0, 1, -1, -1, 1, 0, 0, 1, 1, -1, 1, 0, 0, 1, 1, 1, 1, 0, 0});
            shadow.indices.insert(shadow.indices.end(), {4, 5, 6, 4, 6, 7});
            shadow.vertices.insert(shadow.vertices.end(), {-1, -1, -1, 0, 0, -1, 1, -1, -1, 0, 0, -1, 1, 1, -1, 0, 0, -1, -1, 1, -1, 0, 0, -1});
            shadow.indices.insert(shadow.indices.end(), {8, 9, 10, 8, 10, 11});
            shadow.vertices.insert(shadow.vertices.end(), {-1, -1, 1, 0, 0, 1, 1, -1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, -1, 1, 1, 0, 0, 1});
            shadow.indices.insert(shadow.indices.end(), {12, 13, 14, 12, 14, 15});
            shadow.vertices.insert(shadow.vertices.end(), {-1, -1, -1, 0, -1, 0, 1, -1, -1, 0, -1, 0, 1, -1, 1, 0, -1, 0, -1, -1, 1, 0, -1, 0});
            shadow.indices.insert(shadow.indices.end(), {16, 17, 18, 16, 18, 19});
            shadow.vertices.insert(shadow.vertices.end(), {-1, 1, -1, 0, 1, 0, 1, 1, -1, 0, 1, 0, 1, 1, 1, 0, 1, 0, -1, 1, 1, 0, 1, 0});
            shadow.indices.insert(shadow.indices.end(), {20, 21, 22, 20, 22, 23});
            shadow.vertices.insert(shadow.vertices.end(), {-5, 1, -5, 0, -1, 0, 5, 1, -5, 0, -1, 0, 5, 1, 5, 0, -1, 0, -5, 1, 5, 0, -1, 0});
            shadow.indices.insert(shadow.indices.end(), {24, 25, 26, 24, 26, 27});
            shadow.uniforms.insert(shadow.uniforms.end(), {0, 0, 0, 0, 0, 0, 0});
            break;
        case WM_TIMER:
            InvalidateRect(hWnd, NULL, FALSE);
            frameRendered = false;
            break;
        case WM_COMMAND:
            {
            int wmId = LOWORD(wParam);
            switch (wmId) {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
            }
            break;
        case WM_PAINT:
            if(!frameRendered) {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                frameCount += 1;
                if(GetKeyState(VK_SHIFT) & 0x8000) {
                    camera[1] += 0.1;
                }
                if(GetKeyState(VK_SPACE) & 0x8000) {
                    camera[1] -= 0.1;
                }
                if(GetKeyState('D') & 0x8000) {
                    camera[0] += cos(camera[3]) * 0.1;
                    camera[2] += sin(camera[3]) * 0.1;
                }
                if(GetKeyState('A') & 0x8000) {
                    camera[0] -= cos(camera[3]) * 0.1;
                    camera[2] -= sin(camera[3]) * 0.1;
                }
                if(GetKeyState('W') & 0x8000) {
                    camera[0] -= sin(camera[3]) * 0.1;
                    camera[2] += cos(camera[3]) * 0.1;
                }
                if(GetKeyState('S') & 0x8000) {
                    camera[0] += sin(camera[3]) * 0.1;
                    camera[2] -= cos(camera[3]) * 0.1;
                }
                shadowCamera[3] += 0.01;

                shadow.uniforms[0] = cos(shadowCamera[3]);
                shadow.uniforms[1] = sin(shadowCamera[3]);
                shadow.uniforms[2] = cos(shadowCamera[4]);
                shadow.uniforms[3] = sin(shadowCamera[4]);
                shadow.uniforms[4] = shadowCamera[0];
                shadow.uniforms[5] = shadowCamera[1];
                shadow.uniforms[6] = shadowCamera[2];
                for(int i = 0; i < 400 * 400; i += 1) {
                    final[i] = 0, shadowMap[i] = 0, depth[i] = 0;
                }
                shadow.renderTo(shadowMap, 400, 400, shadowVertex, shadowFragment, depth);
                //shadow.renderTo(final, 400, 400, shadowVertex, shadowFragment, depth);

                everything.uniforms[0] = cos(camera[3]);
                everything.uniforms[1] = sin(camera[3]);
                everything.uniforms[2] = cos(camera[4]);
                everything.uniforms[3] = sin(camera[4]);
                everything.uniforms[4] = camera[0];
                everything.uniforms[5] = camera[1];
                everything.uniforms[6] = camera[2];
                everything.uniforms[7] = shadowCamera[0];
                everything.uniforms[8] = shadowCamera[1];
                everything.uniforms[9] = shadowCamera[2];
                float x = 0, y = 0, z = 1;
                float c = cos(-shadowCamera[4]), s = sin(-shadowCamera[4]);
                float t = y * c - z * s;
                z = z * c + y * s;
                y = t;
                c = cos(-shadowCamera[3]), s = sin(-shadowCamera[3]);
                t = x * c + z * s;
                z = z * c - x * s;
                x = t;
                shadowCamera[0] = x * 5;
                shadowCamera[2] = z * 5;
                everything.uniforms[10] = cos(shadowCamera[3]);
                everything.uniforms[11] = sin(shadowCamera[3]);
                everything.uniforms[12] = cos(shadowCamera[4]);
                everything.uniforms[13] = sin(shadowCamera[4]);
                everything.uniforms[14] = x;
                everything.uniforms[15] = y;
                everything.uniforms[16] = z;
                for(int i = 0; i < 400*400; i += 1) {
                    depth[i] = 0;
                }
                everything.renderTo(final, 400, 400, everythingVertex, everythingFragment, depth);
                HBITMAP map = CreateBitmap(400, 400, 1, 8 * 4, (void*)final);
                HDC src = CreateCompatibleDC(hdc);
                SelectObject(src, map);
                BitBlt(hdc, 0, 0, 400, 400, src, 0, 0, SRCCOPY);
                DeleteDC(src);
                EndPaint(hWnd, &ps);
                frameRendered = true;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_INPUT:
            {
            unsigned size = sizeof(RAWINPUT);
            static RAWINPUT raw[sizeof(RAWINPUT)];
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));
            if(raw->header.dwType == RIM_TYPEMOUSE) {
                mxVel = raw->data.mouse.lLastX;
                myVel = raw->data.mouse.lLastY;
                if(mouse1 || mouse2) {
                    camera[3] -= mxVel * 0.005;
                    camera[4] += myVel * 0.005;
                    camera[4] = max(-1.57079632679, min(1.57079632679, camera[4]));
                }
                if(raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                    mwVel = (*(short*)&raw->data.mouse.usButtonData) / WHEEL_DELTA;
                }
                if(raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) {
                    mouse1 = true;
                }
                if(raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) {
                    mouse1 = false;
                }
                if(raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) {
                    mouse2 = true;
                }
                if(raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) {
                    mouse2 = false;
                }
            }
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch(message) {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;
            break;
        case WM_COMMAND:
            if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}
