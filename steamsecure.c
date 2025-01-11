#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <process.h>


#define ID_CHECK 1
#define ID_FIX 2


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_CHECK)
            {

                RunPowerShellCommand(hwnd, "Get-ExecutionPolicy");
            }
            else if (LOWORD(wParam) == ID_FIX)
            {
                RunPowerShellCommand(hwnd, "Set-ExecutionPolicy -Scope CurrentUser Restricted -Force");
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void RunPowerShellCommand(HWND hwnd, const char* command)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE hRead, hWrite;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    CHAR buffer[4096];
    DWORD bytesRead;
    BOOL bSuccess = FALSE;
    char output[8192] = ""; 

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
    {
        MessageBox(hwnd, "CreatePipe failed", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    char cmd[1024];
    sprintf(cmd, "powershell -Command \"%s\"", command);

    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        MessageBox(hwnd, "CreateProcess failed", "Error", MB_OK | MB_ICONERROR);
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return;
    }

    CloseHandle(hWrite);

    while (bSuccess = ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL), bSuccess && bytesRead)
    {
        buffer[bytesRead] = '\0';
        strcat(output, buffer); 
    }

    CloseHandle(hRead);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    MessageBox(hwnd, output, "PowerShell Output", MB_OK | MB_ICONINFORMATION);
    
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MyWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("MyWindowClass", "监测Steam是否安全与改正", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 300, 150, NULL, NULL, hInstance, NULL);

    CreateWindow("BUTTON", "检查", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 50, 50, 100, 30, hwnd, (HMENU)ID_CHECK, hInstance, NULL);
    CreateWindow("BUTTON", "改正", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 160, 50, 100, 30, hwnd, (HMENU)ID_FIX, hInstance, NULL);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
