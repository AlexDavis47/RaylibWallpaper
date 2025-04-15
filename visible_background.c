#include <windows.h>

// Global variables
HWND g_hwnd = NULL;
BOOL g_isRunning = TRUE;
int g_screenWidth, g_screenHeight;
HDC g_memDC = NULL;
HBITMAP g_memBitmap = NULL;
HINSTANCE g_hInstance = NULL;

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
    // Step 1: Find the Program Manager window
    HWND progman = FindWindow("Progman", NULL);
    if (!progman)
    {
        return FALSE;
    }

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
        return FALSE;
    }

    // Step 4: Set our window as a child of the WorkerW window
    SetWindowLongPtr(g_hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP | WS_CHILD);
    SetParent(g_hwnd, data.WorkerW);

    // Step 5: Position our window to fill the entire WorkerW area
    SetWindowPos(g_hwnd, HWND_BOTTOM, 0, 0, g_screenWidth, g_screenHeight, SWP_SHOWWINDOW);

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
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Render a frame
void RenderFrame()
{
    RECT rect = {0, 0, g_screenWidth, g_screenHeight};

    // Fill with a solid color background
    FillRect(g_memDC, &rect, CreateSolidBrush(RGB(0, 100, 200)));

    // Draw a simple rectangle to show it's working
    HBRUSH brush = CreateSolidBrush(RGB(255, 100, 100));
    SelectObject(g_memDC, brush);
    Rectangle(g_memDC, 100, 100, g_screenWidth - 100, g_screenHeight - 100);
    DeleteObject(brush);
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
    const char CLASS_NAME[] = "SimpleWallpaperClass";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClass(&wc);

    // Create the window - initially we create it as a normal popup window
    g_hwnd = CreateWindowEx(
        0,                             // Extended window style
        CLASS_NAME,                    // Class name
        "Simple Wallpaper",            // Window title
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
        return 1;
    }

    // Show the window
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    // Set up as wallpaper
    SetupWallpaper();

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