cd build\Release
7z a ScreenUploader-x86_64-pc-windows-msvc.zip .
sha256sum ScreenUploader-x86_64-pc-windows-msvc.zip > ScreenUploader-x86_64-pc-windows-msvc.sha256
scp ScreenUploader-x86_64-pc-windows-msvc.zip ScreenUploader-x86_64-pc-windows-msvc.sha256 server:/www/wwwroot/downloads/ScreenUploader/
del ScreenUploader-x86_64-pc-windows-msvc.zip ScreenUploader-x86_64-pc-windows-msvc.sha256