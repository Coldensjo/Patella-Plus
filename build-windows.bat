@echo off
rem Compiles the Windows client (Release) into build\windows-x64\Release\Patella.exe,
rem then places it into the play folder build\play with the files it needs to run.
rem Patella.ini and the automap .map files in the play folder are the player's own and are never touched.
setlocal
cd /d "%~dp0"
python -I tools\build.py windows-x64
set BuildResult=%ERRORLEVEL%
echo.
if %BuildResult% neq 0 (
	echo The build failed with exit code %BuildResult%.
	pause
	exit /b %BuildResult%
)
echo Done: build\windows-x64\Release\Patella.exe

set ReleaseFolder=build\windows-x64\Release
set PlayFolder=build\play
if not exist "%PlayFolder%" mkdir "%PlayFolder%"
set CopyResult=0
for %%F in (Patella.exe vulkan_lvp.dll lvp_icd.x86_64.json) do (
	if exist "%ReleaseFolder%\%%F" (
		copy /y "%ReleaseFolder%\%%F" "%PlayFolder%\%%F" >nul || set CopyResult=1
	)
)
for %%F in (Tibia.dat Tibia.spr Tibia.pic) do (
	if exist "%%F" (
		copy /y "%%F" "%PlayFolder%\%%F" >nul || set CopyResult=1
	)
)
if %CopyResult% neq 0 (
	echo Copying into %PlayFolder% failed. Is the client still running?
	pause
	exit /b 1
)
echo Placed into %PlayFolder%\Patella.exe
pause
exit /b 0
