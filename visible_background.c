#include <windows.h>
#include <stdio.h>
#include <math.h>

// Global variables
HWND g_hwnd = NULL;
BOOL g_isRunning = TRUE;
int g_screenWidth, g_screenHeight;
HDC g_memDC = NULL;
HBITMAP g_memBitmap = NULL;
HINSTANCE g_hInstance = NULL;
int g_animationOffset = 0;

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void RenderFrame();
void Cleanup();
BOOL SetupWallpaper();
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

// Struct for passing data to EnumWindowsProc
typedef struct
{
    HWND WorkerW;
} FindWorkerWData;

// Callback for EnumWindows to find the correct WorkerW window
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    FindWorkerWData *data = (FindWorkerWData *)lParam;

    // Find the WorkerW window with SHELLDLL_DefView as a child
    HWND defView = FindWindowEx(hwnd, NULL, "SHELLDLL_DefView", NULL);
    if (defView != NULL)
    {
        // We've found the WorkerW window that has the desktop icons
        // The next WorkerW window after this one should be our target
        data->WorkerW = FindWindowEx(NULL, hwnd, "WorkerW", NULL);
        return FALSE; // Stop enumeration
    }

    return TRUE; // Continue enumeration
}

// Set up our window as a desktop wallpaper
BOOL SetupWallpaper()
{
    // Create console window for debugging
    AllocConsole();
    FILE *pFile = NULL;
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    printf("Attempting to set up wallpaper...\n");

    // Step 1: Find the Program Manager window
    HWND progman = FindWindow("Progman", NULL);
    if (!progman)
    {
        printf("Failed to find Program Manager window!\n");
        return FALSE;
    }
    printf("Found Program Manager: %p\n", progman);

    // Step 2: Send the special message to create the WorkerW window
    // The magic message is 0x052C with parameters 0xD and 0x1
    DWORD_PTR resultPtr;
    SendMessageTimeout(progman, 0x052C, 0xD, 0x1, SMTO_NORMAL, 1000, &resultPtr);

    // Give Windows a moment to create the WorkerW window
    Sleep(100);

    // Step 3: Find the newly created WorkerW window
    FindWorkerWData data = {NULL};
    EnumWindows(EnumWindowsProc, (LPARAM)&data);

    if (!data.WorkerW)
    {
        printf("Failed to find WorkerW window!\n");
        return FALSE;
    }

    printf("Found WorkerW window: %p\n", data.WorkerW);

    // Step 4: Set our window as a child of the WorkerW window
    SetWindowLongPtr(g_hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP | WS_CHILD);
    HWND oldParent = SetParent(g_hwnd, data.WorkerW);
    printf("Set parent result: Old parent %p, Error %d\n", oldParent, GetLastError());

    // Step 5: Position our window to fill the entire WorkerW area
    BOOL posResult = SetWindowPos(g_hwnd, HWND_BOTTOM,
                                  0, 0, g_screenWidth, g_screenHeight,
                                  SWP_SHOWWINDOW);

    printf("SetWindowPos result: %d, Error: %d\n", posResult, GetLastError());

    // Step 6: Refresh the window
    InvalidateRect(g_hwnd, NULL, TRUE);

    return TRUE;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Initialize rendering resources
        HDC hdc = GetDC(hwnd);
        g_memDC = CreateCompatibleDC(hdc);
        g_memBitmap = CreateCompatibleBitmap(hdc, g_screenWidth, g_screenHeight);
        SelectObject(g_memDC, g_memBitmap);
        ReleaseDC(hwnd, hdc);

        // Set a timer for animation
        SetTimer(hwnd, 1, 50, NULL); // 20 FPS animation
        return 0;
    }

    case WM_DESTROY:
        g_isRunning = FALSE;
        Cleanup();
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Render to memory DC
        RenderFrame();

        // Copy from memory DC to window DC
        BitBlt(hdc, 0, 0, g_screenWidth, g_screenHeight, g_memDC, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER:
        // For animation
        g_animationOffset = (g_animationOffset + 1) % 100;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    // Keep this for debugging so we know if the window is receiving mouse events
    case WM_LBUTTONDOWN:
        printf("Mouse clicked at %d, %d\n", LOWORD(lParam), HIWORD(lParam));
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Render a frame
void RenderFrame()
{
    RECT rect = {0, 0, g_screenWidth, g_screenHeight};

    // Fill with dark blue background
    FillRect(g_memDC, &rect, CreateSolidBrush(RGB(0, 40, 80)));

    // Draw some animated circles
    for (int i = 0; i < 20; i++)
    {
        int radius = 30 + i * 5;
        int x = g_screenWidth / 2 + cos((g_animationOffset + i * 18) * 0.03) * (g_screenWidth / 3);
        int y = g_screenHeight / 2 + sin((g_animationOffset + i * 18) * 0.03) * (g_screenHeight / 3);

        HBRUSH brush = CreateSolidBrush(RGB(i * 12, 255 - i * 10, 128 + i * 6));
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));

        HBRUSH oldBrush = SelectObject(g_memDC, brush);
        HPEN oldPen = SelectObject(g_memDC, pen);

        Ellipse(g_memDC, x - radius, y - radius, x + radius, y + radius);

        SelectObject(g_memDC, oldBrush);
        SelectObject(g_memDC, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    // Draw text
    SetBkMode(g_memDC, TRANSPARENT);
    SetTextColor(g_memDC, RGB(255, 255, 255));

    // Current time
    SYSTEMTIME st;
    GetLocalTime(&st);

    char buffer[256];
    sprintf(buffer, "LIVELY WALLPAPER DEMO\n%02d:%02d:%02d",
            st.wHour, st.wMinute, st.wSecond);

    // Use a large font
    HFONT hFont = CreateFont(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT hOldFont = SelectObject(g_memDC, hFont);

    // Draw text with shadow
    RECT textRect = rect;
    OffsetRect(&textRect, 3, 3);
    SetTextColor(g_memDC, RGB(0, 0, 0));
    DrawText(g_memDC, buffer, -1, &textRect, DT_CENTER | DT_VCENTER);

    // Draw main text
    OffsetRect(&textRect, -3, -3);
    SetTextColor(g_memDC, RGB(255, 255, 255));
    DrawText(g_memDC, buffer, -1, &textRect, DT_CENTER | DT_VCENTER);

    // Cleanup
    SelectObject(g_memDC, hOldFont);
    DeleteObject(hFont);
}

// Clean up resources
void Cleanup()
{
    if (g_memBitmap)
        DeleteObject(g_memBitmap);
    if (g_memDC)
        DeleteDC(g_memDC);
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;

    // Get screen dimensions
    g_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    g_screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Register window class
    const char CLASS_NAME[] = "LivelyWallpaperClass";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClass(&wc);

    // Create the window - initially we create it as a normal popup window
    g_hwnd = CreateWindowEx(
        0,                             // Extended window style (no special style)
        CLASS_NAME,                    // Class name
        "Lively Wallpaper Demo",       // Window title
        WS_POPUP,                      // Window style
        0, 0,                          // Position
        g_screenWidth, g_screenHeight, // Size
        NULL,                          // Parent window
        NULL,                          // Menu
        hInstance,                     // Instance handle
        NULL                           // Additional data
    );

    if (!g_hwnd)
    {
        MessageBox(NULL, "Failed to create window!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Show the window
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    // Set up as wallpaper
    if (!SetupWallpaper())
    {
        MessageBox(NULL, "Failed to set up as wallpaper!", "Error", MB_OK | MB_ICONERROR);
    }

    // Message loop
    MSG msg = {0};
    while (g_isRunning)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Just wait a bit to avoid hogging the CPU
            Sleep(10);
        }
    }

    return 0;
}