@cd /d "%~dp0"
@echo.

msdf-atlas-gen.exe ^
    -font Inconsolata-Regular.ttf -charset charset.txt ^
    -type msdf ^
    -format png -imageout font.png ^
    -json font.json ^
    -size 32 -pxrange 3
@if errorlevel 1 goto end

@echo.

convert_font.py
@if errorlevel 1 goto end

:end
@echo.
@pause
