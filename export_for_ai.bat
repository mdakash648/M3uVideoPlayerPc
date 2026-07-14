@echo off
echo ========================================================
echo       Exporting Project Source Code for AI
echo ========================================================
echo.

set EXPORT_FILE=M3uVideoPlayerPc_Source.zip

:: Delete old zip if it exists
if exist "%EXPORT_FILE%" del "%EXPORT_FILE%"

:: Use PowerShell to zip only the important source files and config
:: This deliberately ignores the build folder, .user files, etc.
powershell -Command "Compress-Archive -Path 'src', 'CMakeLists.txt', 'implementation_plan.md' -DestinationPath '%EXPORT_FILE%' -Force"

echo.
echo [Success] Export complete! 
echo Your zip file has been created: %EXPORT_FILE%
echo.
pause
