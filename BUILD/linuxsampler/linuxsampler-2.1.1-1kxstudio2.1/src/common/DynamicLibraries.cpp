/*
    Copyright (C) 2010-2016 Christian Schoenebeck
*/

#include "DynamicLibraries.h"
#include "global_private.h"
#if defined(WIN32)
# include <windows.h>
#else
# include <dlfcn.h>
# include <errno.h>
# include <dirent.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
#endif
#include <string.h>
#include <iostream>

namespace LinuxSampler {

/* 
   NOTE: it is expected by the sampler that a reference count is maintained for
   DLLs, so DLLs can be opened n times, and a DLL shall only be freed from
   memory after DynamicLibraryClose() was also called n times. Usually this
   behavior is already implemented on Operating System level. If not, it has to
   be implemented here for the respective OS !
*/

int DynamicLibrariesSearch(String dir, String funct, DynamicLibrariesSearchCallbackFunction* pCallback, void* pCustom) throw (Exception) {
    int iLibsLoadedCount = 0;

    #if defined(WIN32)
    WIN32_FIND_DATA win32FindData;
    const String dllpattern = dir + "\\*.dll";
    HANDLE hDir = FindFirstFile(dllpattern.c_str(), &win32FindData);
    if (hDir == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            throw Exception("library path '" + dir + "' doesn't exist");
        } else {
            return 0; // no file found
        }
    }

    do {
        // skip directory entries
        if (win32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        // dir entry name as full qualified path
        const String sPath = dir + "\\" + win32FindData.cFileName;
        // load the DLL
        HINSTANCE hinstLib;
        void* pDLL = hinstLib = LoadLibrary(sPath.c_str());
        if (!pDLL) {
            std::cerr << "Failed to load DLL: " << sPath << std::endl;
            continue;
        }

        void* pFunct = (void*)GetProcAddress(hinstLib, funct.c_str());
        if (pFunct == NULL) {
            std::cerr << "ERROR: unable to find " << funct << "() in " << sPath
                      << std::endl << std::flush;
            FreeLibrary(hinstLib);
            continue;
        }

        // call the the supplied callback function to report the found DLL
        pCallback(sPath, pDLL, pFunct, pCustom);

        iLibsLoadedCount++;

    } while (FindNextFile(hDir, &win32FindData));

    if (hDir != INVALID_HANDLE_VALUE) FindClose(hDir);

    #else // POSIX

    #if defined(__APPLE__)  /*  20071224 Toshi Nagata  */
    if (dir.find("~") == 0)
        dir.replace(0, 1, getenv("HOME"));
    #endif
    DIR* hDir = opendir(dir.c_str());
    if (!hDir) {
        throw Exception("library path '" + dir + "' doesn't exist");
    }
    for (dirent* pEntry = readdir(hDir); pEntry; pEntry = readdir(hDir)) {
        // dir entry name as full qualified path
        const String sPath = dir + "/" + pEntry->d_name;
        // skip entries that are not regular files
        struct stat entry_stat;
        if (lstat(sPath.c_str(), &entry_stat) != 0 ||
            (entry_stat.st_mode & S_IFMT) != S_IFREG)
            continue;
        // skip files that are not .so files
        if (sPath.length() < 3 ||
            (sPath.substr(sPath.length() - 3) != ".so" &&
             sPath.find(".so.") == String::npos) )
            continue;
        // load the DLL
        void* pDLL = dlopen(sPath.c_str(), RTLD_NOW);
        if (!pDLL) {
            std::cerr << "failed to load DLL: '" << sPath << "', cause: "
                      << dlerror() << std::endl;
            continue;
        }
        // load the requested function
        void* pFunct = dlsym(pDLL, funct.c_str());
        char* pcErr = dlerror();
        if (pcErr || !pFunct) {
            std::cerr << "ERROR: unable to find " << funct << "() in '" << sPath
                      << "'" << std::endl << std::flush;
            dlclose(pDLL);
            continue;
        }

        // call the the supplied callback function to report the found and
        // successfully opened DLL
        pCallback(sPath, pDLL, pFunct, pCustom);

        iLibsLoadedCount++;
    }
    closedir(hDir);
    #endif

    return iLibsLoadedCount;
}

void* DynamicLibraryOpen(String filename) {
    #if defined(WIN32)
    return LoadLibrary(filename.c_str());
    #else
    return dlopen(filename.c_str(), RTLD_NOW);
    #endif
}

void* DynamicLibraryGetSymbol(void* hDLL, String symbol) {
    #if defined(WIN32)
    return (void*)GetProcAddress((HMODULE)hDLL, symbol.c_str());
    #else
    return dlsym(hDLL, symbol.c_str());
    #endif
}

void DynamicLibraryClose(void* hDLL) {
    #if defined(WIN32)
    FreeLibrary((HMODULE)hDLL);
    #else
    dlclose(hDLL);
    #endif
}

} // namespace LinuxSampler
