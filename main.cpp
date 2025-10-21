#include <windows.h>
#include <iostream>
#include <sstream>
#include <shobjidl.h>
#include <objbase.h>
#include <fstream>
#include <string>

#define DEBUG 0

#define ID_EDIT 1
#define ID_BROWSING 2
#define ID_SAVEB 3

// global handles
HWND hEdit;
HWND browsingB;
HWND saveB;
char path[MAX_PATH];

// puts text into the input character by character
void puttext(char path[MAX_PATH], HWND hwnd){
    std::ifstream fileobj(path);
    if (!fileobj.is_open()){
        MessageBoxA(hwnd, "ERROR", "FAILED TO FIND THE SPECIFIED FILE", MB_OK);
        return;
    }

    std::stringstream buffer;
    buffer << fileobj.rdbuf();  // read file into buffer
    std::string content = buffer.str();  // get string from buffer

    std::string FixedContent;
    FixedContent.reserve(content.size() * 2); // " *2" is there so if the entire file was \n there will be still some space for the \r to each \n without reallocating
    
    // loop through each character
    for (int i = 0; i <= content.size();i++){
        if (content[i] == '\n'){
            if (i == 0 || content[i-1] != '\r'){
                FixedContent += "\r\n";
            }else{
                FixedContent += '\n';
            }
        }else{
            FixedContent += content[i];
        }
    }

    SetWindowTextA(hEdit, FixedContent.c_str());
    if (DEBUG == 1){
        MessageBoxA(hwnd, path, "debug", MB_OK);
    }
};



void Save(HWND hwnd, char path[MAX_PATH]) {
    // create the ofstream object
    std::ofstream obj(path);

    //check for errors
    if (!obj.is_open()) {
        if (DEBUG == 1){
            MessageBoxA(hwnd, path, "debug", MB_OK);
        }
  
        MessageBoxA(hwnd,"FAILED TO SAVE TO THE SPECIFIED FILE", "ERROR", MB_OK);
        return;
    }

    //get the content length
    int length = GetWindowTextLengthA(hEdit);

    // create a string to save the input to
    std::string text;
    text.resize(length + 1);  // +1 for null terminator

    // saving the input to the created string
    GetWindowTextA(hEdit, &text[0], length + 1);


    // overwriting to the obj
    obj << text;

    // send a messagebox to tell the user his files got saved
    MessageBoxA(hwnd, "Saved!", "Success", MB_OK);
}


void Browser(HWND hwnd, char lpPath[MAX_PATH]) {
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog *pFileOpen = nullptr;

        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog,reinterpret_cast<void**>(&pFileOpen));
        if (SUCCEEDED(hr)) {
            DWORD options;
            pFileOpen->GetOptions(&options);
            pFileOpen->SetOptions(options | FOS_CREATEPROMPT | FOS_FORCESHOWHIDDEN);

            hr = pFileOpen->Show(hwnd);
            if (SUCCEEDED(hr)) {
                IShellItem *item = nullptr;
                hr = pFileOpen->GetResult(&item);
                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath = nullptr;
                    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    if (SUCCEEDED(hr)) {
                        char path[MAX_PATH];
                        WideCharToMultiByte(CP_ACP, 0, pszFilePath, -1, path, MAX_PATH, NULL, NULL);
                        puttext(path, hwnd);
                        memcpy(lpPath, path, MAX_PATH);
                        CoTaskMemFree(pszFilePath);
                    }
                    item->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
}



// window message handler
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam) - 40; // -40 to leave some space at the top for the buttons
        MoveWindow(hEdit, 0, 40, width, height, TRUE);

        MoveWindow(browsingB, 10, 10, 100, 30, TRUE);

        MoveWindow(saveB, 120, 10, 100, 30, TRUE);
        return 0;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == ID_BROWSING) {
            Browser(hwnd, path);
        }

        if (LOWORD(wParam) == ID_SAVEB) {
            Save(hwnd, path);
        }

        return 0;
    }

    case WM_CREATE:
    // the EDIT input
        hEdit = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER, 0, 40, 100, 100, hwnd, (HMENU)ID_EDIT, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
    // the browsing button
        browsingB= CreateWindowA("Button", "Browsing", WS_CHILD | WS_VISIBLE, 10, 10, 100, 30, hwnd, (HMENU)ID_BROWSING, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        

    // the save button
        saveB = CreateWindowA("Button", "Save", WS_CHILD | WS_VISIBLE, 120, 10, 100, 30, hwnd, (HMENU)ID_SAVEB, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
    
        return 0;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "test";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, CLASS_NAME, "TheNoter", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
                               NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}