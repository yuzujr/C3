cd build
tar czvf ScreenUploader-x86_64-unknown-linux-gnu.tar.gz ./ScreenUploader ./config.json
sha256sum ScreenUploader-x86_64-unknown-linux-gnu.tar.gz > ScreenUploader-x86_64-unknown-linux-gnu.sha256
scp ScreenUploader-x86_64-unknown-linux-gnu.tar.gz ScreenUploader-x86_64-unknown-linux-gnu.sha256 server:/www/wwwroot/downloads/ScreenUploader/
rm ScreenUploader-x86_64-unknown-linux-gnu.tar.gz ScreenUploader-x86_64-unknown-linux-gnu.sha256