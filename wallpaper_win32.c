#if defined(_WIN32)
#include <windows.h>
#include <stdbool.h>
#include <stdlib.h>

// Get the raylib window handle
extern void *GetWindowHandle(void);

// Windows-specific wallpaper variables
static HWND g_workerW = NULL;
static int g_currentMonitorIndex = 0;
static int g_monitorCount = 0;
static RECT *g_monitorRects = NULL;

// Forward declaration for monitor enumeration callback
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

// Find the center-most monitor
int FindCenterMonitor(void)
{
    // Find center point of virtual screen
    int centerX = GetSystemMetrics(SM_CXVIRTUALSCREEN) / 2;
    int centerY = GetSystemMetrics(SM_CYVIRTUALSCREEN) / 2;

    // Find which monitor contains this center point
    for (int i = 0; i < g_monitorCount; i++)
    {
        if (centerX >= g_monitorRects[i].left && centerX < g_monitorRects[i].right &&
            centerY >= g_monitorRects[i].top && centerY < g_monitorRects[i].bottom)
        {
            return i;
        }
    }

    // If no monitor contains the center point, default to first monitor
    return 0;
}

// Initialize monitor information
void InitMonitorInfo(void)
{
    // First, count the monitors
    g_monitorCount = GetSystemMetrics(SM_CMONITORS);

    // Allocate memory for monitor rects
    if (g_monitorRects != NULL)
    {
        free(g_monitorRects);
    }
    g_monitorRects = (RECT *)malloc(sizeof(RECT) * g_monitorCount);

    // Enumerate monitors to get their positions and sizes
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    // Set to center monitor by default
    g_currentMonitorIndex = FindCenterMonitor();
}

// Callback for EnumDisplayMonitors
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    static int index = 0;

    // Get monitor info
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hMonitor, &monitorInfo);

    // Store work area (excludes taskbar)
    g_monitorRects[index] = monitorInfo.rcMonitor;

    index++;
    if (index >= g_monitorCount)
    {
        index = 0; // Reset for next enumeration
        return FALSE;
    }

    return TRUE;
}

// Change to a different monitor
bool ChangeMonitor(int monitorIndex)
{
    if (monitorIndex < 0 || monitorIndex >= g_monitorCount)
    {
        return false;
    }

    g_currentMonitorIndex = monitorIndex;

    // If we're already in wallpaper mode, update the window position
    HWND hwnd = (HWND)GetWindowHandle();
    if (GetParent(hwnd) == g_workerW)
    {
        // Update window position to the selected monitor
        RECT monitorRect = g_monitorRects[g_currentMonitorIndex];
        int width = monitorRect.right - monitorRect.left;
        int height = monitorRect.bottom - monitorRect.top;

        // Position window over the selected monitor
        SetWindowPos(hwnd, HWND_BOTTOM,
                     monitorRect.left, monitorRect.top,
                     width, height,
                     SWP_SHOWWINDOW);
    }

    return true;
}

// Switch to next monitor
bool NextMonitor(void)
{
    return ChangeMonitor((g_currentMonitorIndex + 1) % g_monitorCount);
}

// Switch to previous monitor
bool PreviousMonitor(void)
{
    int prevIndex = (g_currentMonitorIndex - 1);
    if (prevIndex < 0)
        prevIndex = g_monitorCount - 1;
    return ChangeMonitor(prevIndex);
}

// Get current monitor info
void GetCurrentMonitorInfo(int *monitorIndex, int *totalMonitors, int *x, int *y, int *width, int *height)
{
    if (monitorIndex)
        *monitorIndex = g_currentMonitorIndex;
    if (totalMonitors)
        *totalMonitors = g_monitorCount;

    RECT rect = g_monitorRects[g_currentMonitorIndex];
    if (x)
        *x = rect.left;
    if (y)
        *y = rect.top;
    if (width)
        *width = rect.right - rect.left;
    if (height)
        *height = rect.bottom - rect.top;
}

// Callback for EnumWindows to find the WorkerW window
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    HWND *result = (HWND *)lParam;
    HWND defView = FindWindowEx(hwnd, NULL, "SHELLDLL_DefView", NULL);

    if (defView != NULL)
    {
        // We've found the WorkerW window that has the desktop icons
        // The next WorkerW window after this one should be our target
        *result = FindWindowEx(NULL, hwnd, "WorkerW", NULL);
        return FALSE; // Stop enumeration
    }

    return TRUE; // Continue enumeration
}

// Set up window as wallpaper
bool SetupWallpaperWin32(void)
{
    // Initialize monitor information if needed
    if (g_monitorRects == NULL)
    {
        InitMonitorInfo();
    }

    // Step 1: Find the Program Manager window
    HWND progman = FindWindow("Progman", NULL);
    if (!progman)
    {
        return false;
    }

    // Step 2: Send the special message to create the WorkerW window
    // The magic message is 0x052C with parameters 0xD and 0x1
    DWORD_PTR resultPtr;
    SendMessageTimeout(progman, 0x052C, 0xD, 0x1, SMTO_NORMAL, 1000, &resultPtr);

    // Give Windows a moment to create the WorkerW window
    Sleep(100);

    // Step 3: Find the newly created WorkerW window
    g_workerW = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&g_workerW);

    if (!g_workerW)
    {
        return false;
    }

    // Step 4: Set our window as a child of the WorkerW window (IMPORTANT!)
    // This is the key to making it work as a wallpaper
    HWND hwnd = (HWND)GetWindowHandle();

    // Change window style to be a child window
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP | WS_CHILD);

    // Set the WorkerW as our parent
    SetParent(hwnd, g_workerW);

    // Position the window on the selected monitor
    RECT monitorRect = g_monitorRects[g_currentMonitorIndex];
    int width = monitorRect.right - monitorRect.left;
    int height = monitorRect.bottom - monitorRect.top;

    SetWindowPos(hwnd, HWND_BOTTOM,
                 monitorRect.left, monitorRect.top,
                 width, height,
                 SWP_SHOWWINDOW);

    return true;
}

// Toggle wallpaper mode
bool ToggleWallpaperWin32(bool enableWallpaper)
{
    if (enableWallpaper)
    {
        return SetupWallpaperWin32();
    }
    else
    {
        // Restore window to normal
        HWND hwnd = (HWND)GetWindowHandle();

        // Change back to a normal popup window
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);

        // Remove the parent relationship
        SetParent(hwnd, NULL);

        // Move to top and show
        SetWindowPos(hwnd, HWND_TOP, 100, 100, 800, 450, SWP_SHOWWINDOW);
        return true;
    }
}

// Free resources when app is closing
void CleanupWallpaperWin32(void)
{
    if (g_monitorRects != NULL)
    {
        free(g_monitorRects);
        g_monitorRects = NULL;
    }
}

// Forward the GetWindowHandle from raylib
void *GetNativeWindowHandle(void)
{
    return GetWindowHandle();
}

// Get the current monitor index
int GetCurrentMonitorIndex(void)
{
    return g_currentMonitorIndex;
}

// Get the total number of monitors
int GetTotalMonitors(void)
{
    return g_monitorCount;
}
#endif