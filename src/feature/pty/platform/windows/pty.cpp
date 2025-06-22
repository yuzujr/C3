#include <Windows.h>
#include <process.h>

#include <cstdio>
#include <cstring>

// Forward declarations
HRESULT CreatePseudoConsoleAndPipes(HPCON*, HANDLE*, HANDLE*);
HRESULT InitializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEXW*, HPCON);
void TestDirectPowerShell();          // 测试直接管道PowerShell
void __cdecl OutputListener(LPVOID);  // 输出监听线程
void __cdecl InputHandler(LPVOID);    // 输入处理线程

int main() {
    // 启动交互式shell而不是固定命令
    wchar_t szCommand[]{L"cmd.exe"};  // 启动命令提示符

    // PowerShell - 让它自动检测ConPTY环境
    // wchar_t szCommand[]{L"powershell.exe -NoLogo"};

    // wchar_t szCommand[]{L"wsl.exe"};             // 启动WSL
    // wchar_t szCommand[]{L"ping localhost"};      // 旧版本：直接执行ping命令
    HRESULT hr{E_UNEXPECTED};
    HANDLE hConsole = {GetStdHandle(STD_OUTPUT_HANDLE)};

    // Enable Console VT Processing
    DWORD consoleMode{};
    GetConsoleMode(hConsole, &consoleMode);
    hr = SetConsoleMode(hConsole,
                        consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
             ? S_OK
             : GetLastError();
    if (S_OK == hr) {
        HPCON hPC{INVALID_HANDLE_VALUE};

        //  Create the Pseudo Console and pipes to it
        HANDLE hPipeIn{INVALID_HANDLE_VALUE};
        HANDLE hPipeOut{INVALID_HANDLE_VALUE};
        hr = CreatePseudoConsoleAndPipes(&hPC, &hPipeIn, &hPipeOut);
        if (S_OK == hr) {
            // Create & start thread to listen to the incoming pipe
            // Note: Using CRT-safe _beginthread() rather than CreateThread()
            HANDLE hOutputListenerThread{reinterpret_cast<HANDLE>(
                _beginthread(OutputListener, 0, hPipeIn))};

            // Create & start thread to handle user input
            HANDLE hInputHandlerThread{reinterpret_cast<HANDLE>(_beginthread(
                InputHandler, 0,
                hPipeOut))};  // Initialize the necessary startup info struct
            STARTUPINFOEXW startupInfo{};
            if (S_OK == InitializeStartupInfoAttachedToPseudoConsole(
                            &startupInfo, hPC)) {
                // Launch PowerShell - it should auto-detect ConPTY and enable
                // colors
                PROCESS_INFORMATION piClient{};
                hr = CreateProcessW(
                         NULL,       // No module name - use Command Line
                         szCommand,  // Command Line
                         NULL,       // Process handle not inheritable
                         NULL,       // Thread handle not inheritable
                         FALSE,      // Inherit handles
                         EXTENDED_STARTUPINFO_PRESENT,  // Creation flags
                         NULL,  // Use parent's environment
                         NULL,  // Use parent's starting directory
                         &startupInfo.StartupInfo,  // Pointer to STARTUPINFO
                         &piClient)  // Pointer to PROCESS_INFORMATION
                         ? S_OK
                         : GetLastError();
                if (S_OK == hr) {
                    // 等待shell进程运行（而不是固定时间）
                    // 用户可以通过在shell中输入 "exit" 来结束
                    printf("交互式终端已启动，输入 'exit' 退出\n");
                    WaitForSingleObject(piClient.hProcess, INFINITE);

                    // Allow listening thread to catch-up with final output!
                    Sleep(500);
                }

                // --- CLOSEDOWN ---

                // Now safe to clean-up client app's process-info & thread
                CloseHandle(piClient.hThread);
                CloseHandle(piClient.hProcess);

                // Cleanup attribute list
                DeleteProcThreadAttributeList(startupInfo.lpAttributeList);
                free(startupInfo.lpAttributeList);
            }

            // Close ConPTY - this will terminate client process if running
            ClosePseudoConsole(hPC);

            // Clean-up the pipes
            if (INVALID_HANDLE_VALUE != hPipeOut) CloseHandle(hPipeOut);
            if (INVALID_HANDLE_VALUE != hPipeIn) CloseHandle(hPipeIn);
        }
    }

    return S_OK == hr ? EXIT_SUCCESS : EXIT_FAILURE;
}

HRESULT CreatePseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn,
                                    HANDLE* phPipeOut) {
    HRESULT hr{E_UNEXPECTED};
    HANDLE hPipePTYIn{INVALID_HANDLE_VALUE};
    HANDLE hPipePTYOut{INVALID_HANDLE_VALUE};

    // Create the pipes to which the ConPTY will connect
    if (CreatePipe(&hPipePTYIn, phPipeOut, NULL, 0) &&
        CreatePipe(phPipeIn, &hPipePTYOut, NULL, 0)) {
        // Determine required size of Pseudo Console
        COORD consoleSize{};
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        HANDLE hConsole{GetStdHandle(STD_OUTPUT_HANDLE)};
        if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
            consoleSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            consoleSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }  // Create the Pseudo Console of the required size, attached to the
        // PTY-end of the pipes with VT processing enabled
        hr = CreatePseudoConsole(consoleSize, hPipePTYIn, hPipePTYOut, 0, phPC);

        // Note: We can close the handles to the PTY-end of the pipes here
        // because the handles are dup'ed into the ConHost and will be released
        // when the ConPTY is destroyed.
        if (INVALID_HANDLE_VALUE != hPipePTYOut) CloseHandle(hPipePTYOut);
        if (INVALID_HANDLE_VALUE != hPipePTYIn) CloseHandle(hPipePTYIn);
    }

    return hr;
}

// Initializes the specified startup info struct with the required properties
// and updates its thread attribute list with the specified ConPTY handle
HRESULT InitializeStartupInfoAttachedToPseudoConsole(
    STARTUPINFOEXW* pStartupInfo, HPCON hPC) {
    HRESULT hr{E_UNEXPECTED};

    if (pStartupInfo) {
        size_t attrListSize{};

        pStartupInfo->StartupInfo.cb = sizeof(STARTUPINFOEXW);

        // Get the size of the thread attribute list.
        InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);

        // Allocate a thread attribute list of the correct size
        pStartupInfo->lpAttributeList =
            reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
                malloc(attrListSize));

        // Initialize thread attribute list
        if (pStartupInfo->lpAttributeList &&
            InitializeProcThreadAttributeList(pStartupInfo->lpAttributeList, 1,
                                              0, &attrListSize)) {
            // Set Pseudo Console attribute
            hr = UpdateProcThreadAttribute(pStartupInfo->lpAttributeList, 0,
                                           PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                           hPC, sizeof(HPCON), NULL, NULL)
                     ? S_OK
                     : HRESULT_FROM_WIN32(GetLastError());
        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

// 输出监听线程：从ConPTY读取子进程输出并显示到控制台
void __cdecl OutputListener(LPVOID pipe) {
    HANDLE hPipe{pipe};
    HANDLE hConsole{GetStdHandle(STD_OUTPUT_HANDLE)};

    const DWORD BUFF_SIZE{512};
    char szBuffer[BUFF_SIZE]{};

    DWORD dwBytesWritten{};
    DWORD dwBytesRead{};
    BOOL fRead{FALSE};
    do {
        // Read from the pipe
        fRead = ReadFile(hPipe, szBuffer, BUFF_SIZE, &dwBytesRead, NULL);

        // Write received text to the Console
        // Note: Write to the Console using WriteFile(hConsole...), not
        // printf()/puts() to prevent partially-read VT sequences from
        // corrupting output
        WriteFile(hConsole, szBuffer, dwBytesRead, &dwBytesWritten, NULL);

    } while (fRead && dwBytesRead >= 0);
}

// 输入处理线程：从控制台读取用户输入并发送给ConPTY
void __cdecl InputHandler(LPVOID pipeOut) {
    HANDLE hPipeOut = static_cast<HANDLE>(pipeOut);
    HANDLE hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);

    DWORD bytesRead{};
    DWORD bytesWritten{};

    // 设置控制台输入模式 - 使用字符模式而不是原始模式
    DWORD inputMode{};
    GetConsoleMode(hConsoleInput, &inputMode);

    // 保持字符处理但允许特殊键正确处理
    inputMode &= ~ENABLE_VIRTUAL_TERMINAL_INPUT;  // 禁用VT输入
    inputMode &= ~ENABLE_ECHO_INPUT;      // 禁用本地回显（让ConPTY处理）
    inputMode &= ~ENABLE_LINE_INPUT;      // 禁用行输入（逐字符传输）
    inputMode |= ENABLE_PROCESSED_INPUT;  // 启用处理输入（处理Ctrl+C等）

    SetConsoleMode(hConsoleInput, inputMode);

    while (true) {
        INPUT_RECORD inputRecord;
        DWORD eventsRead;

        // 使用ReadConsoleInput来正确处理键盘事件
        if (ReadConsoleInput(hConsoleInput, &inputRecord, 1, &eventsRead) &&
            eventsRead > 0) {
            if (inputRecord.EventType == KEY_EVENT &&
                inputRecord.Event.KeyEvent.bKeyDown) {
                // 只处理按键按下事件
                WCHAR keyChar = inputRecord.Event.KeyEvent.uChar.UnicodeChar;
                WORD vkCode = inputRecord.Event.KeyEvent.wVirtualKeyCode;
                DWORD controlKeys =
                    inputRecord.Event.KeyEvent.dwControlKeyState;

                char outputBuffer[8] = {0};
                int outputLen = 0;

                // 首先处理特殊功能键（无字符输出的键）
                switch (vkCode) {
                    case VK_BACK:
                        if (controlKeys &
                            (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
                            // Ctrl+Backspace：删除整个单词
                            outputBuffer[0] = 0x17;  // Ctrl+W
                            outputLen = 1;
                        } else {
                            // 普通退格：删除一个字符
                            outputBuffer[0] = 0x08;  // BS
                            outputLen = 1;
                        }
                        break;

                    case VK_DELETE:
                        strcpy(outputBuffer, "\x1b[3~");
                        outputLen = 4;
                        break;

                    case VK_UP:
                        strcpy(outputBuffer, "\x1b[A");
                        outputLen = 3;
                        break;

                    case VK_DOWN:
                        strcpy(outputBuffer, "\x1b[B");
                        outputLen = 3;
                        break;

                    case VK_LEFT:
                        strcpy(outputBuffer, "\x1b[D");
                        outputLen = 3;
                        break;

                    case VK_RIGHT:
                        strcpy(outputBuffer, "\x1b[C");
                        outputLen = 3;
                        break;

                    default:
                        // 处理普通字符输入
                        if (keyChar != 0) {
                            // 将Unicode字符转换为UTF-8
                            outputLen = WideCharToMultiByte(
                                CP_UTF8, 0, &keyChar, 1, outputBuffer,
                                sizeof(outputBuffer), NULL, NULL);
                        }
                        break;
                }

                // 发送处理后的输入到ConPTY
                if (outputLen > 0) {
                    if (!WriteFile(hPipeOut, outputBuffer, outputLen,
                                   &bytesWritten, NULL)) {
                        break;  // 管道关闭，退出循环
                    }
                }
            }
        } else {
            break;  // 读取失败，退出循环
        }
    }
}