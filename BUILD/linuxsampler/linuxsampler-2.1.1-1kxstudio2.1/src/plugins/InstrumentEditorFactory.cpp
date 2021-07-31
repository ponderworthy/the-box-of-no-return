/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007 - 2016 Christian Schoenebeck                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "InstrumentEditorFactory.h"

#include "../common/global_private.h"
#include "../Sampler.h"

#if defined(WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <string.h>

#ifndef CONFIG_PLUGIN_DIR
# error "Configuration macro CONFIG_PLUGIN_DIR not defined!"
#endif // CONFIG_PLUGIN_DIR

namespace LinuxSampler {

    std::map<String, InstrumentEditorFactory::InnerFactory*> InstrumentEditorFactory::InnerFactories;

    bool InstrumentEditorFactory::bPluginsLoaded = false;

    std::list<void*> InstrumentEditorFactory::LoadedDLLs;

    std::vector<String> InstrumentEditorFactory::AvailableEditors() {
        // make sure plugins were loaded already
        LoadPlugins();
        // render result
        std::vector<String> result;
        std::map<String, InnerFactory*>::iterator iter = InnerFactories.begin();
        for (; iter != InnerFactories.end(); iter++)
            result.push_back(iter->first);
        return result;
    }

    std::vector<String> InstrumentEditorFactory::MatchingEditors(String sTypeName, String sTypeVersion) {
        // make sure plugins were loaded already
        LoadPlugins();
        // render result
        std::vector<String> result;
        std::map<String, InnerFactory*>::iterator iter = InnerFactories.begin();
        for (; iter != InnerFactories.end(); iter++) {
            InstrumentEditor* pEditor = iter->second->Create();
            if (pEditor->IsTypeSupported(sTypeName, sTypeVersion))
                result.push_back(iter->first);
            iter->second->Destroy(pEditor);
        }
        return result;
    }

    String InstrumentEditorFactory::AvailableEditorsAsString() {
        std::vector<String> drivers = AvailableEditors();
        String result;
        std::vector<String>::iterator iter = drivers.begin();
        for (; iter != drivers.end(); iter++) {
            if (result != "") result += ",";
            result += "'" + *iter + "'";
        }
        return result;
    }

    InstrumentEditor* InstrumentEditorFactory::Create(String InstrumentEditorName) throw (Exception) {
        if (InnerFactories.count(InstrumentEditorName)) {
            InnerFactory* pInnerFactory = InnerFactories[InstrumentEditorName];
            return pInnerFactory->Create();
        } else throw Exception("unknown instrument editor");
    }

    void InstrumentEditorFactory::Destroy(InstrumentEditor* pInstrumentEditor) throw (Exception) {
        if (InnerFactories.count(pInstrumentEditor->Name())) {
            InnerFactory* pInnerFactory = InnerFactories[pInstrumentEditor->Name()];
            return pInnerFactory->Destroy(pInstrumentEditor);
        } else throw Exception("unknown instrument editor");
    }

    void InstrumentEditorFactory::LoadPlugins() {
        if (!bPluginsLoaded) {
            dmsg(1,("Loading instrument editor plugins..."));
            // getenv() is available on Posix and Windows
            char* pcPluginDir = getenv("LINUXSAMPLER_PLUGIN_DIR");
            #if defined(WIN32)
            String installDir = Sampler::GetInstallDir();
            String pluginDir;
            if (pcPluginDir)
                pluginDir = pcPluginDir;
            if (pluginDir.empty())
                pluginDir = installDir + "\\plugins";

            // Put the LS installation directory first in DLL search
            // path, so the plugin finds for example the bundled GTK
            // libraries before any other installed versions
            if (!installDir.empty()) {
                // The SetDllDirectory function is only available on
                // XP and higher, so we call it dynamically
                HMODULE k32 = GetModuleHandleA("kernel32.dll");
                if (k32) {
                    BOOL WINAPI (*setDllDirectory)(LPCSTR) =
                        (BOOL WINAPI (*)(LPCSTR))GetProcAddress(k32, "SetDllDirectoryA");
                    if (setDllDirectory) {
                        setDllDirectory(installDir.c_str());
                    }
                }
            }

            if (pluginDir.empty() || !LoadPlugins(pluginDir)) {
                if (!LoadPlugins(CONFIG_PLUGIN_DIR)) {
                    std::cerr << "Could not open instrument editor plugins "
                              << "directory ('" << pluginDir << "' or '"
                              << CONFIG_PLUGIN_DIR << "'), Error: "
                              << GetLastError() << std::endl;
                    return;
                }
            }
            #else
            String dir;
            if (pcPluginDir)
                dir = pcPluginDir;
            if (dir.empty())
                dir = CONFIG_PLUGIN_DIR;
            if (!LoadPlugins(dir)) {
                std::cerr << "Could not open instrument editor plugins "
                          << "directory ('" << dir << "'): "
                          << strerror(errno) << std::endl;
                return;
            }
            #endif
            bPluginsLoaded = true;
            dmsg(1,("OK\n"));
        }
    }

    bool InstrumentEditorFactory::LoadPlugins(String plugindir) {
        #if defined(WIN32)
        WIN32_FIND_DATA win32FindData;
        const String pluginpattern = plugindir + "\\*.dll";
        HANDLE hDir = FindFirstFile(pluginpattern.c_str(), &win32FindData);
        if (hDir == INVALID_HANDLE_VALUE) {
            if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                return false;
            } else {
                dmsg(1,("None"));
                return true;
            }
        }

        do {
            // skip directory entries
            if (win32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            // dir entry name as full qualified path
            const String sPath = plugindir + "\\" + win32FindData.cFileName;
            // load the DLL
            HINSTANCE hinstLib;
            void* pDLL = hinstLib = LoadLibrary(sPath.c_str());
            if (!pDLL) {
                std::cerr << "Failed to load instrument editor plugin: "
                          << sPath << std::endl;
                continue;
            }

            InnerFactory* (*fn)() = (InnerFactory* (*)())
                GetProcAddress(hinstLib, "createInstrumentEditorInnerFactory");
            if (fn == NULL) {
                std::cerr << "ERROR: unable to find "
                             "createInstrumentEditorInnerFactory() "
                             "in DLL\n" << std::flush;
                FreeLibrary(hinstLib);
                continue;
            }

            // get the plugin instance and register it to the factory

            InnerFactory* pInnerFactory = fn();
            if (!pInnerFactory) {
                std::cerr << "ERROR: !pInnerFactory\n" << std::flush;
                FreeLibrary(hinstLib);
                continue;
            }
            InstrumentEditor* pEditor = pInnerFactory->Create();
            if (InnerFactories.count(pEditor->Name())) {
                std::cerr << "ERROR: a plugin with name '"
                          << pEditor->Name()
                          << "' already loaded (skipping)\n"
                          << std::flush;
                pInnerFactory->Destroy(pEditor);
                FreeLibrary(hinstLib);
                continue;
            }
            InnerFactories[pEditor->Name()] = pInnerFactory;
            pInnerFactory->Destroy(pEditor);

            LoadedDLLs.push_back(pDLL);
        } while (FindNextFile(hDir, &win32FindData));

        if (hDir != INVALID_HANDLE_VALUE) FindClose(hDir);

        #else // POSIX

        #if defined(__APPLE__)  /*  20071224 Toshi Nagata  */
        if (plugindir.find("~") == 0)
            plugindir.replace(0, 1, getenv("HOME"));
        #endif
        DIR* hDir = opendir(plugindir.c_str());
        if (!hDir) {
            return false;
        }
        for (dirent* pEntry = readdir(hDir); pEntry; pEntry = readdir(hDir)) {
            // dir entry name as full qualified path
            const String sPath = plugindir + "/" + pEntry->d_name;
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
            // load the DLL (the plugins should register themselfes automatically)
            void* pDLL = dlopen(sPath.c_str(), RTLD_NOW);
            if (pDLL) LoadedDLLs.push_back(pDLL);
            else {
                std::cerr << "Failed to load instrument editor plugin: '"
                          << sPath << "', cause: " << dlerror() << std::endl;
            }
        }
        closedir(hDir);
        #endif
        return true;
    }

    void InstrumentEditorFactory::ClosePlugins() {
        if (LoadedDLLs.size()) {
            dmsg(1,("Unloading instrument editor plugins..."));
            // free all inner factories
            {
                std::map<String, InnerFactory*>::iterator iter = InnerFactories.begin();
                for (; iter != InnerFactories.end(); iter++) delete iter->second;
                InnerFactories.clear();
            }
            // free the DLLs
            {
                std::list<void*>::iterator iter = LoadedDLLs.begin();
                for (; iter != LoadedDLLs.end(); iter++) {
                    #if defined(WIN32)
                    FreeLibrary((HINSTANCE)*iter);
                    #else
                    dlclose(*iter);
                    #endif
                }
                LoadedDLLs.clear();
                dmsg(1,("OK\n"));
            }
        }
        bPluginsLoaded = false;
    }

} // namespace LinuxSampler
