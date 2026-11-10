@echo off
rem Compiles the Windows client (Release) into build\windows-x64\Release\Patella.exe.
setlocal
cd /d "%~dp0"
python -I tools\build.py windows-x64
set BuildResult=%ERRORLEVEL%
echo.
if %BuildResult% equ 0 (
	echo Done: build\windows-x64\Release\Patella.exe
) else (
	echo The build failed with exit code %BuildResult%.
)
pause
exit /b %BuildResult%
