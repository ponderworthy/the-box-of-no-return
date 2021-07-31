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

#ifndef LS_FILE_H
#define LS_FILE_H

#include <memory>
#include <string>
#include <vector>

#include "Mutex.h"
#include <sys/stat.h>

namespace LinuxSampler {

    class Path;

#if __cplusplus >= 201103L && !CONFIG_NO_CPP11STL
    typedef std::unique_ptr<std::vector<std::string>> FileListPtr;
#else
    typedef std::auto_ptr<std::vector<std::string> > FileListPtr;
#endif

    class File {
        public:
            class DirectoryWalker {
                public:
                    virtual void DirectoryEntry(std::string Path) = 0;
                    virtual void FileEntry(std::string Path) = 0;
            };
            
            File(std::string FileName);

            File(const Path& path);

            /**
             * Tests whether the file exists.
             */
            bool Exist();

            /**
             * Provides appropriate error message if failed to retrieve
             * information about the specified file, in which case Exist() returns false.
             */
            std::string GetErrorMsg();

            /**
             * Tests whether it's a regular file.
             */
            bool IsFile();

            /**
             * Tests whether it's a directory.
             */
            bool IsDirectory();
            
            /**
             * Returns the size of the file in bytes.
             */
            unsigned long GetSize();

            /**
             * Returns the names of the regular files in the specified directory.
             * @throws Exception If failed to list the directory content.
             */
            static FileListPtr GetFiles(std::string Dir);
            
            /**
             * Walks through the directory tree that is located under the
             * directory <b>Dir</b>, and calls <b>pWalker->DirectoryEntry()</b>
             * once for each directory in the tree and <b>pWalker->FileEntry()</b>
             * once for each file in the tree. Note that this method can be 
             * recursively invoked from within the callback functions
             * <b>pWalker->DirectoryEntry()</b> and <b>pWalker->FileEntry()</b>.
             * @throws Exception If the specified path is missing or is not a directory.
             * Exception is also thrown, and the directory tree walk is stopped,
             * if <b>pWalker->DirectoryEntry()</b> or <b>pWalker->FileEntry()</b>
             * throws Exception.
             */
            static void WalkDirectoryTree(std::string Dir, DirectoryWalker* pWalker);

            static const char DirSeparator;
            static const char PathSeparator;

        private:
            bool bExist;
            std::string ErrorMsg;
            static Mutex DirectoryWalkerMutex;
            static std::vector<DirectoryWalker*> DirectoryWalkers;
            static std::string DWErrorMsg;

            struct stat Status;
#ifdef WIN32
            static void WalkDirectoryTreeSub(std::string Dir, DirectoryWalker* pWalker);
#else
            static int FtwCallback(const char* fpath, const struct stat* sb, int typeflag);
#endif
    };
}

#endif
