@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo.
  echo ERROR: MSVC compiler not found.
  echo Run this file from "x64 Native Tools Command Prompt for VS".
  echo.
  pause
  exit /b 1
)
if not exist assets mkdir assets
cl /nologo /std:c++17 /EHsc /O2 /DUNICODE /D_UNICODE main.cpp /link /SUBSYSTEM:WINDOWS gdiplus.lib comdlg32.lib shell32.lib user32.lib gdi32.lib
if errorlevel 1 (
  echo Build failed.
  pause
  exit /b 1
)
if not exist release mkdir release
copy /Y main.exe release\HOUP.exe >nul
xcopy /E /I /Y assets release\assets >nul
echo.
echo SUCCESS: release\HOUP.exe
echo Keep the assets folder next to HOUP.exe.
pause
