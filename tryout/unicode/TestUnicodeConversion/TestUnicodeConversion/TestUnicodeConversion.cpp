#include <Windows.h>
#include <stdio.h>

#define CHK(_expr, ...) \
    if (!(_expr)) { \
        wprintf(__VA_ARGS__); \
        error = GetLastError(); \
        wprintf(L" - failed: %d ( %08x )\n", error, error); \
        __leave; \
    }

void PrintHex(PUCHAR buffer, SIZE_T length, PWSTR header)
{
    wprintf(L"%s:", header);
    for (SIZE_T i = 0; i < length; i++) {

        if (i % 16 == 0) {

            wprintf(L"\n");
        }

        wprintf(L"%02x ", buffer[i]);

        if ((i % 8 == 0) && (i % 16 != 0)) {

            wprintf(L": ");
        }
    }
    wprintf(L"\n");
}

DWORD wmain(int argc, wchar_t *argv[])
{
    DWORD error;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PWSTR fileName;
    UCHAR buffer[1024];
    DWORD bytesRead;
    FILE *pFile;

    __try {

        if (argc != 2) {

            wprintf(L"Usage:> %s filePath\n", argv[0]);
            error = ERROR_INVALID_COMMAND_LINE;
            __leave;
        }

        fileName = argv[1];

        //
        //  Read bytes
        //

        hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        CHK(hFile != INVALID_HANDLE_VALUE, L"CreateFile( '%s' )", fileName);
        CHK(ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL), L"ReadFile");
        PrintHex(buffer, bytesRead, L"Raw bytes");
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        //
        //  Read as text
        //

        pFile = _wfopen(fileName, L"r");
        bytesRead = (DWORD)fread(buffer, 1, sizeof(buffer), pFile);
        if (bytesRead == 0) {

            wprintf(L"fread - failed %d\n", ferror(pFile));
            __leave;
        }
        PrintHex(buffer, bytesRead, L"Read as text file");

        error = ERROR_SUCCESS;
        getchar();

    } __finally {

        if (hFile != INVALID_HANDLE_VALUE) {

            CloseHandle(hFile);
        }
    }

    return error;
}