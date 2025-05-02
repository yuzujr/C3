@echo off

if not exist image (
    md image
)

ssh server "cd /www/wwwroot/47.94.142.31/server/uploads && tar czf - ./*" | tar xzf - -C image/