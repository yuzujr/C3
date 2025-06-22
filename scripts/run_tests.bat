@echo off
REM Test Runner Script

echo === Building Project ===
cmake --build build --config Debug

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo === Running All Tests ===
cd build
ctest -C Debug --output-on-failure --verbose

echo.
echo === Test Results Summary ===
ctest -C Debug --output-on-failure
