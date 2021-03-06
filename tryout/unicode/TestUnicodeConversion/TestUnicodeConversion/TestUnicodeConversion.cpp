/*++

Module Name:

    TestUnicodeConversion.c

Abstract:

    This modules tryout functions related to converting WCHAR to multibyte
    char and vice versa.

Author:

    Chandra Kumar [ChandKV]     01-14-2017

Notes:
  Calls UTF-16 little endian as Unicode.

++*/

#include <Windows.h>
#include <stdio.h>

#define CHK(_expr, ...) \
    if (!(_expr)) { \
        wprintf(__VA_ARGS__); \
        error = GetLastError(); \
        wprintf(L" - failed: %d ( %08x )\n", error, error); \
        __leave; \
    }

#define PRINT_FAILURE(_expr, ...) { \
    wprintf(__VA_ARGS__); \
    error = GetLastError(); \
    wprintf(L" - (%s): 0x%08x - failed: %d ( %08x )\n", \
        L#_expr, (_expr), error, error); \
}

CONST UCHAR UnicodeBom[] = {0xff, 0xfe};
CONST UCHAR Utf8Bom[] = {0xef, 0xbb, 0xbf};

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

PSTR UnicodeToUtf8(PWSTR unicodeString)
{
    PSTR utf8String;
    int sizeOfUtf8StringInChars;
    int result;
    DWORD error;

    result = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, unicodeString, -1, NULL, 0, NULL, NULL);
    if ((result == 0) || (result == 0xfffd)) {

        PRINT_FAILURE(result, L"WideCharToMultiByte");
        return NULL;
    }

    sizeOfUtf8StringInChars = result;
    utf8String = (PSTR)malloc(sizeOfUtf8StringInChars);
    if (utf8String == NULL) {

        return NULL;
    }

    result = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, unicodeString, -1, utf8String, sizeOfUtf8StringInChars, NULL, NULL);
    if ((result == 0) || (result == 0xfffd)) {

        PRINT_FAILURE(result, L"WideCharToMultiByte");
        free(utf8String);
        return NULL;
    }

    return utf8String;
}

PWSTR Utf8ToUnicode(PSTR utf8String)
{
    PWSTR unicodeString;
    int sizeOfStringInWChars;
    int result;
    DWORD error;

    result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8String, -1, NULL, 0);
    if ((result == 0) || (result == 0xfffd)) {

        PRINT_FAILURE(result, L"MultiByteToWideChar");
        return NULL;
    }

    sizeOfStringInWChars = result;
    unicodeString = (PWSTR)malloc(sizeOfStringInWChars * sizeof(WCHAR));
    if (unicodeString == NULL) {

        return NULL;
    }

    result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8String, -1, unicodeString, sizeOfStringInWChars);
    if ((result == 0) || (result == 0xfffd)) {

        PRINT_FAILURE(result, L"MultiByteToWideChar");
        free(unicodeString);
        return NULL;
    }

    return unicodeString;
}

DWORD WriteToFile(PWSTR filePath, PVOID string, bool isUnicode)
{
    DWORD error;
    HANDLE file = INVALID_HANDLE_VALUE;
    DWORD bytesWritten;

    __try {

        file = CreateFile(filePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        CHK(file != INVALID_HANDLE_VALUE, L"CreateFile('%s')", filePath);
        if (isUnicode) {
            CHK(WriteFile(file, UnicodeBom, sizeof(UnicodeBom), &bytesWritten, NULL), L"Writing BOM");
            CHK(WriteFile(file, string, (DWORD)wcslen((PWSTR)string) * sizeof(WCHAR), &bytesWritten, NULL), L"Writing string");
        } else {
            CHK(WriteFile(file, Utf8Bom, sizeof(Utf8Bom), &bytesWritten, NULL), L"Writing BOM");
            CHK(WriteFile(file, string, (DWORD)strlen((PSTR)string), &bytesWritten, NULL), L"Writing string");
        }
    } __finally {

        if (file != INVALID_HANDLE_VALUE) {

            CloseHandle(file);
        }
    }

    return ERROR_SUCCESS;
}

bool IsUnicodeEncoded(PUCHAR buffer, SIZE_T size) {

    if ((size >= sizeof(UnicodeBom)) &&
        (memcmp(buffer, UnicodeBom, sizeof(UnicodeBom))) == 0) {

        return true;
    }

    return false;
}

bool IsUtf8Encoded(PUCHAR buffer, SIZE_T size) {

    if ((size >= sizeof(Utf8Bom)) &&
        (memcmp(buffer, Utf8Bom, sizeof(Utf8Bom))) == 0) {

        return true;
    }

    return false;
}

DWORD wmain(int argc, wchar_t *argv[])
{
    DWORD error;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PWSTR fileName;
    UCHAR buffer[1024];
    PWSTR unicodeText;
    PSTR utf8Text;
    DWORD bytesRead;
    CONST SIZE_T buffSize = sizeof(buffer) - 4;

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
        CHK(ReadFile(hFile, buffer, buffSize, &bytesRead, NULL), L"ReadFile");
        PrintHex(buffer, bytesRead, L"Raw bytes");
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        #if READ_AS_TEXT
        //
        //  Read as text, is just differing in how it reads the new line.
        //

        FILE *pFile = _wfopen(fileName, L"r");
        bytesRead = (DWORD)fread(buffer, 1, buffSize, pFile);
        if (bytesRead == 0) {

            wprintf(L"fread - failed %d\n", ferror(pFile));
            __leave;
        }
        PrintHex(buffer, bytesRead, L"Read as text file");
        #endif // READ_AS_TEXT

        //
        //  Let's NULL terminate both for char, WCHAR and even unknown scenarios :)
        //

        *(PWSTR)(buffer + bytesRead) = L'\0';
        *(PWSTR)(buffer + bytesRead + 2) = L'\0';

        //
        //  Print strings assuming various encodings, with and without skipping
        //  BOM bytes.
        //    - 2 BOM bytes for UNICODE
        //    - 3 BOM bytes for UTF8
        //

        if (IsUnicodeEncoded(buffer, bytesRead)) {

            wprintf(L"\nCast as wchar_t *   : '%s'\n", (PWSTR)buffer);
            unicodeText = (PWSTR)(buffer + sizeof(UnicodeBom));
            wprintf(L"\nCast as wchar_t * +%d: '%s'\n", (int)sizeof(UnicodeBom), unicodeText);
            utf8Text = UnicodeToUtf8(unicodeText);
            CHK(utf8Text != NULL, L"UnicodeToUtf8");
            wprintf(L"\nConverted to UTF8   : '%S'\n", utf8Text);
            PrintHex((PUCHAR)utf8Text, strlen(utf8Text), L"\nConverted UTF8");
            error = WriteToFile(L"Converted-Utf8.txt", utf8Text, false);
            unicodeText = Utf8ToUnicode(utf8Text);
            CHK(unicodeText != NULL, L"Utf8ToUnicode");
            wprintf(L"\nBack to unicode     : '%s'\n", unicodeText);
            PrintHex((PUCHAR)unicodeText, wcslen(unicodeText) * sizeof(WCHAR), L"\nBack to Unicode");
            error = WriteToFile(L"BackTo-Unicode.txt", unicodeText, true);

        } else if (IsUtf8Encoded(buffer, bytesRead)) {

            wprintf(L"\nCast as char *      : '%S'\n", buffer);
            utf8Text = (PSTR)(buffer + sizeof(Utf8Bom));
            wprintf(L"\nCast as char * +%d   : '%S'\n", (int)sizeof(Utf8Bom), utf8Text);
            unicodeText = Utf8ToUnicode(utf8Text);
            CHK(unicodeText != NULL, L"Utf8ToUnicode");
            wprintf(L"\nConverted to unicode: '%s'\n", unicodeText);
            PrintHex((PUCHAR)unicodeText, wcslen(unicodeText) * sizeof(WCHAR), L"\nConverted Unicode");
            error = WriteToFile(L"Converted-Unicode.txt", unicodeText, true);
            utf8Text = UnicodeToUtf8(unicodeText);
            CHK(utf8Text != NULL, L"UnicodeToUtf8");
            wprintf(L"\nBack to UTF8        : '%S'\n", utf8Text);
            PrintHex((PUCHAR)utf8Text, strlen(utf8Text), L"\nBack to UTF8");
            error = WriteToFile(L"BackTo-Utf8.txt", utf8Text, false);
        }

        #if PrintDefaultChar
        CPINFO cpInfo;
        CHK(GetCPInfo(CP_UTF8, &cpInfo), L"GetCPInfo(CP_UTF8)");
        wprintf(L"UTF8:\n"
                L"  MaxCharSize: %d\n"
                L"  DefaultChar: ",
                cpInfo.MaxCharSize);
        for (int i = 0; i < MAX_DEFAULTCHAR; i++) {
            wprintf(L"%02x", cpInfo.DefaultChar[i]);
        }
        #endif // PrintDefaultChar

        error = ERROR_SUCCESS;

    } __finally {

        if (hFile != INVALID_HANDLE_VALUE) {

            CloseHandle(hFile);
        }
    }

    return error;
}
