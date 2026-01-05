#include <windows.h>
#include <restartmanager.h>
#include <cstdio>

#pragma comment(lib, "Rstrtmgr.lib")

namespace IDontLikeFileLocks {

    static const DWORD error_success = 0;
    static const DWORD error_more_data = 234;

    DWORD unlockfile(const wchar_t* filepath) {
        DWORD sessionhandle = 0;
        wchar_t sessionkey[CCH_RM_SESSION_KEY + 1]{};

        DWORD ret = RmStartSession(&sessionhandle, 0, sessionkey);
        if (ret != error_success)
            return ret;

        __try {
            LPCWSTR resources[1] = { filepath };

            ret = RmRegisterResources(
                sessionhandle,
                1,
                resources,
                0,
                nullptr,
                0,
                nullptr
            );

            if (ret != error_success)
                return ret;

            UINT needed = 0, count = 0;
            DWORD reboot = 0;

            ret = RmGetList(
                sessionhandle,
                &needed,
                &count,
                nullptr,
                &reboot
            );

            if (ret != error_success && ret != error_more_data)
                return ret;

            if (needed > 0)
                ret = RmShutdown(sessionhandle, RmForceShutdown, nullptr);
        }
        __finally {
            RmEndSession(sessionhandle);
        }

        return ret;
    }

    bool readanddump(const wchar_t* filepath, DWORD* outbytes) {

        CreateDirectoryW(L"dump", nullptr);

        HANDLE hfile = CreateFileW(
            filepath,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hfile == INVALID_HANDLE_VALUE)
            return false;

        DWORD filesize = GetFileSize(hfile, nullptr);
        if (filesize == INVALID_FILE_SIZE) {
            CloseHandle(hfile);
            return false;
        }

        BYTE* buffer = new BYTE[filesize];
        DWORD bytesread = 0;

        if (!ReadFile(hfile, buffer, filesize, &bytesread, nullptr)) {
            delete[] buffer;
            CloseHandle(hfile);
            return false;
        }

        CloseHandle(hfile);

        const wchar_t* name = wcsrchr(filepath, L'\\');
        name = name ? name + 1 : filepath;

        wchar_t outpath[MAX_PATH]{};
        swprintf_s(outpath, L"dump\\%ls", name);

        HANDLE hout = CreateFileW(
            outpath,
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hout == INVALID_HANDLE_VALUE) {
            delete[] buffer;
            return false;
        }

        DWORD written = 0;
        WriteFile(hout, buffer, bytesread, &written, nullptr);
        CloseHandle(hout);

        delete[] buffer;

        *outbytes = bytesread;
        wprintf(L"[+] dumped %lu bytes -> %ls\n", bytesread, outpath);
        return true;
    }

    bool fetchfile(const wchar_t* filepath, DWORD* outbytes) {
        for (int i = 0; i < 5; i++) {
            if (unlockfile(filepath) == error_success) {
                if (readanddump(filepath, outbytes))
                    return true;
            }
        }
        return false;
    }

}

int wmain(int argc, wchar_t** argv) {

    if (argc < 2) {
        printf("usage: IDontLikeFileLocks.exe <file>\n");
        return 0;
    }

    DWORD bytes = 0;

    if (!IDontLikeFileLocks::fetchfile(argv[1], &bytes)) {
        printf("[!] failed to dump file\n");
        return 0;
    }

    printf("[+] success\n");
    return 0;
}
