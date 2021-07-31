/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2012 Grigor Iliev, Benno Senoner                 *
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
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,                *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include "File.h"

#include <cstring>
#include <errno.h>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>

#include "Exception.h"
#include "Path.h"
#include "global_private.h"

#if WIN32
#include <windows.h>
#else
#include <ftw.h>
#endif

namespace LinuxSampler {
#ifdef WIN32
    const char File::DirSeparator = '\\';
    const char File::PathSeparator = ';';
#else
    const char File::DirSeparator = '/';
    const char File::PathSeparator = ':';
#endif
    Mutex File::DirectoryWalkerMutex;
    std::vector<File::DirectoryWalker*> File::DirectoryWalkers;
    std::string File::DWErrorMsg;

    File::File(const Path& path) {
        bExist = !stat(path.toNativeFSPath().c_str(), &Status);
        if (!bExist) ErrorMsg = strerror(errno);
    }

    File::File(std::string Path) {
            bExist = !stat(Path.c_str(), &Status);
            if(!bExist) ErrorMsg = strerror(errno);
    }
    
    bool File::Exist() {
        return bExist;
    }
    
    std::string File::GetErrorMsg() {
        return ErrorMsg;
    }
    
    bool File::IsFile() {
        if(!Exist()) return false;
        
            return S_ISREG(Status.st_mode);
    }
    
    bool File::IsDirectory() {
        if(!Exist()) return false;
        
            return S_ISDIR(Status.st_mode);
    }
    
    unsigned long File::GetSize() {
        if(!Exist()) return 0;
        
            return Status.st_size;      
    }
    
    FileListPtr File::GetFiles(std::string Dir) {
            DIR* pDir = opendir(Dir.c_str());
            if (pDir == NULL) {
                std::stringstream ss;
                ss << "Failed to list the directory content of `";
                ss << Dir << "`: " << strerror(errno);
                throw Exception(ss.str());
            }
            
            FileListPtr fileList(new std::vector<std::string>);
            
            struct dirent* pEnt = readdir(pDir);
            while (pEnt != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
            if (pEnt->d_type == DT_REG) {
                fileList->push_back(std::string(pEnt->d_name));
            }
#else
            struct stat s;
            if (stat((Dir + DirSeparator + pEnt->d_name).c_str(), &s) == 0 && S_ISREG(s.st_mode)) {
                fileList->push_back(std::string(pEnt->d_name));
            }
#endif
                pEnt = readdir(pDir);
            }
            
            if (closedir(pDir)) {
                std::stringstream ss;
                ss << "Failed to close directory `" << Dir << "`: ";
                ss << strerror(errno);
                throw Exception(ss.str());
            }
            
            return fileList;
    }
    
    void File::WalkDirectoryTree(std::string Dir, DirectoryWalker* pWalker) {
        dmsg(2,("File: WalkDirectoryTree(Dir=%s)\n", Dir.c_str()));
        File f = File(Dir);
        if(!f.Exist()) throw Exception("Fail to stat `" + Dir + "`: " + f.GetErrorMsg());
        if(!f.IsDirectory()) throw Exception("The specified path is not a directory: " + Dir);
#ifdef WIN32
        WalkDirectoryTreeSub(Dir, pWalker);
#else
        DirectoryWalkerMutex.Lock();
        DirectoryWalkers.push_back(pWalker);
        DWErrorMsg = "Failed to process directory tree: " + Dir;
        
            if (ftw(Dir.c_str(), FtwCallback, 10)) {
                DirectoryWalkers.pop_back();
                if (DirectoryWalkers.size() == 0) DirectoryWalkerMutex.Unlock();
                throw Exception(DWErrorMsg);
            }
        DirectoryWalkers.pop_back();
        if (DirectoryWalkers.size() == 0) DirectoryWalkerMutex.Unlock();
#endif
    }

#ifdef WIN32
    void File::WalkDirectoryTreeSub(String Dir, DirectoryWalker* pWalker) {
        dmsg(2,("File: WalkDirectoryTreeSub(Dir=%s)\n", Dir.c_str()));
        DWORD attrs = GetFileAttributes(Dir.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) return;

        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            pWalker->DirectoryEntry(Dir);

            std::string::size_type l = Dir.length() - 1;
            if (Dir[l] == '/') Dir[l] = '\\';
            else if (Dir[l] != '\\') Dir += '\\';

            WIN32_FIND_DATA fd;
            HANDLE h = FindFirstFile((Dir + "*").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) return;
            do {
                if (strcmp(fd.cFileName, ".") != 0 &&
                    strcmp(fd.cFileName, "..") != 0) {
                    WalkDirectoryTreeSub(Dir + fd.cFileName, pWalker);
                }
            } while (FindNextFile(h, &fd));
            FindClose(h);
        } else {
            pWalker->FileEntry(Dir);
        }
    }
#else
    int File::FtwCallback(const char* fpath, const struct stat* sb, int typeflag) {
        dmsg(2,("File: FtwCallback(fpath=%s)\n", fpath));
        try {
            if (typeflag == FTW_D) DirectoryWalkers.back()->DirectoryEntry(std::string(fpath));
            else if (typeflag == FTW_F) DirectoryWalkers.back()->FileEntry(std::string(fpath));
        } catch(Exception e) {
            e.PrintMessage();
            DWErrorMsg = e.Message();
            return -1;
        }
        
        return 0;
    }
#endif
}
