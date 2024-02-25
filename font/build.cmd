@cd /d "%~dp0"
@echo.

@set fontfile=Inconsolata-Regular.ttf

@if exist msdf-atlas-gen.exe goto gen_ok
@set msdfgenver=1.2.2
@set msdfgendir=msdf-atlas-gen-%msdfgenver%-win64
@if exist %msdfgendir%.zip goto zip_ok
curl -LO https://github.com/Chlumsky/msdf-atlas-gen/releases/download/v1.2.2/%msdfgendir%.zip
@if errorlevel 1 goto end
:zip_ok
powershell Expand-Archive %msdfgendir%.zip
@if errorlevel 1 goto end
move %msdfgendir%\msdf-atlas-gen\msdf-atlas-gen.exe .
@if errorlevel 1 goto end
rmdir /s /q %msdfgendir%
@if errorlevel 1 goto end
:gen_ok

@if exist %fontfile% goto font_ok
curl -LO https://github.com/google/fonts/raw/main/ofl/inconsolata/static/%fontfile%
@if errorlevel 1 goto end
:font_ok

msdf-atlas-gen.exe ^
    -font %fontfile% -charset charset.txt ^
    -type msdf ^
    -format png -imageout Inconsolata.png ^
    -json Inconsolata.json ^
    -size 32 -pxrange 2 ^
    -potr
@if errorlevel 1 goto end

@echo.

convert_font.py
@if errorlevel 1 goto end

:end
@echo.
@pause
