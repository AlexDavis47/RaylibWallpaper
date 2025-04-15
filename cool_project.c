/*******************************************************************************************
 *
 *   Wallpaper 3D Demo - Using raylib to render to the desktop wallpaper
 *
 *   This example has been created using raylib 1.0 (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
 *
 *   Copyright (c) 2013-2024 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include <stddef.h> // For NULL
#include "raylib.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
Camera camera = {0};
Vector3 cubePosition = {0};

// Wallpaper variables (Windows only)
#if defined(_WIN32)
typedef void *WallpaperHandle;
WallpaperHandle g_wallpaperHandle = NULL;
bool g_renderToWallpaper = true;

// Forward declarations for our wallpaper functions
// Implementation will be in a separate source file
bool SetupWallpaperWin32(void);
bool ToggleWallpaperWin32(bool enableWallpaper);
void *GetNativeWindowHandle(void);
bool NextMonitor(void);
bool PreviousMonitor(void);
int GetCurrentMonitorIndex(void);
int GetTotalMonitors(void);
void CleanupWallpaperWin32(void);
#endif

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void); // Update and draw one frame

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib - Wallpaper 3D Demo");

    camera.position = (Vector3){3.0f, 3.0f, 2.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

// Set up as wallpaper (Windows only)
#if defined(_WIN32)
    SetupWallpaperWin32();
#endif

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60); // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
#if defined(_WIN32)
        // Check for key press to toggle wallpaper mode
        if (IsKeyPressed(KEY_W))
        {
            g_renderToWallpaper = !g_renderToWallpaper;
            ToggleWallpaperWin32(g_renderToWallpaper);
        }

        // Monitor switching controls - arrow keys to switch monitors
        if (g_renderToWallpaper)
        {
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
            {
                NextMonitor();
            }
            else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
            {
                PreviousMonitor();
            }
        }
#endif

        UpdateDrawFrame();
    }
#endif

// De-Initialization
//--------------------------------------------------------------------------------------
#if defined(_WIN32)
    // Clean up wallpaper resources
    CleanupWallpaperWin32();
#endif

    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    UpdateCamera(&camera, CAMERA_ORBITAL);
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

    DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);
    DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);
    DrawGrid(10, 1.0f);

    EndMode3D();

#if defined(_WIN32)
    if (g_renderToWallpaper)
    {
        int currentMonitor = GetCurrentMonitorIndex() + 1;
        int totalMonitors = GetTotalMonitors();

        DrawText(TextFormat("Wallpaper Mode - Press [W] to toggle - Monitor %d/%d",
                            currentMonitor, totalMonitors),
                 10, 40, 20, DARKGRAY);
        DrawText("Use Left/Right arrows to change monitor", 10, 70, 16, DARKGRAY);
    }
    else
        DrawText("Window Mode - Press [W] to toggle", 10, 40, 20, DARKGRAY);
#endif

    DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
