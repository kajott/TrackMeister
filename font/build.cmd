@cd /d "%~dp0"
@echo.

@set font1file=Inconsolata-Regular.ttf
@set font2file=IosevkaFixed-Regular.ttf

@if exist msdf-atlas-gen.exe goto gen_ok
@set msdfgenver=1.2.2
@set msdfgendir=msdf-atlas-gen-%msdfgenver%-win64
@if exist %msdfgendir%.zip goto gen_zip_ok
curl -LO https://github.com/Chlumsky/msdf-atlas-gen/releases/download/v1.2.2/%msdfgendir%.zip
@if errorlevel 1 goto end
:gen_zip_ok
powershell Expand-Archive %msdfgendir%.zip
@if errorlevel 1 goto end
move %msdfgendir%\msdf-atlas-gen\msdf-atlas-gen.exe .
@if errorlevel 1 goto end
rmdir /s /q %msdfgendir%
:gen_ok

@if exist %font1file% goto font1_ok
curl -LO https://github.com/google/fonts/raw/main/ofl/inconsolata/static/%font1file%
@if errorlevel 1 goto end
:font1_ok

@if exist %font2file% goto font2_ok
@set font2ver=29.2.1
@set font2dir=Iosevka-TTF
@if exist %font2dir%.zip goto font2_zip_ok
curl -L -o %font2dir%.zip https://github.com/be5invis/Iosevka/releases/download/v%font2ver%/PkgTTF-IosevkaFixed-%font2ver%.zip
@if errorlevel 1 goto end
:font2_zip_ok
powershell Expand-Archive %font2dir%.zip
@if errorlevel 1 goto end
move %font2dir%\%font2file% .
@if errorlevel 1 goto end
rmdir /s /q %font2dir%
:font2_ok

msdf-atlas-gen.exe ^
    -font %font1file% -charset charset.txt ^
    -type msdf ^
    -format png -imageout Inconsolata.png ^
    -json Inconsolata.json ^
    -size 32 -pxrange 2 ^
    -potr
@if errorlevel 1 goto end

@echo.

msdf-atlas-gen.exe ^
    -font %font2file% -charset charset.txt ^
    -type msdf ^
    -format png -imageout Iosevka.png ^
    -json Iosevka.json ^
    -size 32 -pxrange 2 ^
    -potr
@if errorlevel 1 goto end

@echo.

convert_font.py
@if errorlevel 1 goto end

:end
@echo.
@pause
