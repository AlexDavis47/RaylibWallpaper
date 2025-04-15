@echo off
echo Building desktop window test programs...

gcc -Wall -Wextra -o desktop_access.exe desktop_access.c -lgdi32 -luser32
if %ERRORLEVEL% neq 0 (
    echo Failed to build desktop_access.exe
    pause
    exit /b 1
)

gcc -Wall -Wextra -o desktop_window_test.exe desktop_window_test.c -lgdi32 -luser32 -mwindows
if %ERRORLEVEL% neq 0 (
    echo Failed to build desktop_window_test.exe
    pause
    exit /b 1
)

gcc -Wall -Wextra -o desktop_window_test2.exe desktop_window_test2.c -lgdi32 -luser32 -lgdiplus -lmsimg32 -mwindows
if %ERRORLEVEL% neq 0 (
    echo Failed to build desktop_window_test2.exe
    pause
    exit /b 1
)

gcc -Wall -Wextra -o wallpaper_alternative.exe wallpaper_alternative.c -lgdi32 -luser32 -lole32 -luuid -mwindows
if %ERRORLEVEL% neq 0 (
    echo Failed to build wallpaper_alternative.exe
    pause
    exit /b 1
)

gcc -Wall -Wextra -o background_window.exe background_window.c -lgdi32 -luser32 -mwindows
if %ERRORLEVEL% neq 0 (
    echo Failed to build background_window.exe
    pause
    exit /b 1
)

gcc -Wall -Wextra -o wallpaper_final.exe wallpaper_final.c -lgdi32 -luser32 -mwindows
if %ERRORLEVEL% neq 0 (
    echo Failed to build wallpaper_final.exe
    pause
    exit /b 1
)

gcc -Wall -Wextra -o visible_background.exe visible_background.c -lgdi32 -luser32 -mwindows
if %ERRORLEVEL% neq 0 (
    echo Failed to build visible_background.exe
    pause
    exit /b 1
)

echo Build completed successfully.
echo.
echo You can now run:
echo   desktop_access.exe - To explore the desktop window handles
echo   desktop_window_test.exe - The basic desktop window test
echo   desktop_window_test2.exe - Enhanced desktop window test with timing display
echo   wallpaper_alternative.exe - Alternative approach using Active Desktop
echo   background_window.exe - Click-through background window approach
echo   wallpaper_final.exe - Final attempt with multiple techniques
echo   visible_background.exe - High visibility test with window debugging
echo.
pause 