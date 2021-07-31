/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007 - 2009 Grigor Iliev                                *
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

#include "../common/global.h"

#ifndef __LS_INSTRUMENTSDB_H__
#define __LS_INSTRUMENTSDB_H__

#include <sqlite3.h>
#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/gig.h>
#else
# include <gig.h>
#endif
#include "../common/Mutex.h"
#include "../common/WorkerThread.h"
#include "../EventListeners.h"
#include "InstrumentsDbUtilities.h"

namespace LinuxSampler {
    /**
     * @brief Provides access to the instruments database.
     */
    class InstrumentsDb {
        friend class DirectoryScanner;
        friend class DirectoryFinder;
        friend class InstrumentFinder;
        friend class DirectoryCounter;
        friend class InstrumentCounter;
        friend class DirectoryCopier;
        friend class AddInstrumentsJob;
        friend class ScanProgress;
        
        public:
            /**
             * This class is used as a listener, which is notified when
             * changes to the instruments database are made.
             */
            class Listener {
                public:
                    /**
                     * Invoked when the number of instrument directories
                     * in a specific directory has changed.
                     * @param Dir The absolute pathname of the directory
                     * in which the number of directories is changed.
                     * All slashes in the directory names are replaced with '\0'.
                     */
                    virtual void DirectoryCountChanged(String Dir) = 0;

                    /**
                     * Invoked when the settings of an instrument directory
                     * are changed.
                     * @param Dir The absolute pathname of the directory
                     * whose settings are changed.
                     * All slashes in the directory names are replaced with '\0'.
                     */
                    virtual void DirectoryInfoChanged(String Dir) = 0;

                    /**
                     * Invoked when an instrument directory is renamed.
                     * @param Dir The old absolute pathname of the directory.
                     * All slashes in the directory names are replaced with '\0'.
                     * @param NewName The new name of the directory.
                     * All slashes in the name are replaced with '\0'.
                     */
                    virtual void DirectoryNameChanged(String Dir, String NewName) = 0;

                    /**
                     * Invoked when the number of instruments
                     * in a specific directory has changed.
                     * @param Dir The absolute pathname of the directory
                     * in which the number of instruments is changed.
                     * All slashes in the directory names are replaced with '\0'.
                     */
                    virtual void InstrumentCountChanged(String Dir) = 0;

                    /**
                     * Invoked when the settings of an instrument are changed.
                     * @param Instr The absolute pathname of the
                     * instrument whose settings are changed.
                     * All slashes in the directory/instrument names are replaced with '\0'.
                     */
                    virtual void InstrumentInfoChanged(String Instr) = 0;

                    /**
                     * Invoked when an instrument is renamed.
                     * @param Instr The old absolute pathname of the instrument.
                     * All slashes in the directory/instrument names are replaced with '\0'.
                     * @param NewName The new name of the directory.
                     * All slashes in the name are replaced with '\0'.
                     */
                    virtual void InstrumentNameChanged(String Instr, String NewName) = 0;

                    /**
                     * Invoked when the status of particular job is changed.
                     * @param JobId The ID of the job.
                     */
                    virtual void JobStatusChanged(int JobId) = 0;
            };

            /**
             * Registers the specified listener to be notified
             * when changes to the instruments database are made.
             */
            void AddInstrumentsDbListener(InstrumentsDb::Listener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveInstrumentsDbListener(InstrumentsDb::Listener* l);

            /**
             * Creates an instruments database file.
             * @param FilePath optional pathname of the file to create.
             * @throws Exception If the creation of the database file failed.
             */
            void CreateInstrumentsDb(String FilePath = "");

            /**
             * This method is used to access the instruments database.
             */
            static InstrumentsDb* GetInstrumentsDb();

            /**
             * Sets the absolute path name of the instruments database file to use.
             * The instruments database file location should be set before
             * any attempt to access the database and once set could not be changed.
             * @throws Exception - if an empty string is provided or if
             * the method is called more than once.
             */
            void SetDbFile(String File);

            /**
             * Gets the number of directories in the specified directory.
             * @param Dir The absolute path name of the directory.
             * All slashes in the directory names should be replaced with '\0'.
             * @param Recursive If true, the number of all directories
             * in the specified subtree will be returned.
             * @throws Exception - if database error occurs, or
             * the specified path name is invalid.
             * @returns The number of directories in the specified directory.
             */
            int GetDirectoryCount(String Dir, bool Recursive);

            /**
             * Gets the list of directories in the specified directory.
             * @param Dir The absolute path name of the directory.
             * All slashes in the directory names should be replaced with '\0'.
             * @param Recursive If true, all directories
             * in the specified subtree will be returned.
             * @throws Exception - if database error occurs, or
             * the specified path name is invalid.
             * @returns The list of directories, where the directories are
             * represented in abstract path - all slashes in the directory
             * names are replaced with '\0'.
             */
            StringListPtr GetDirectories(String Dir, bool Recursive);

            /**
             * Adds the specified directory to the database.
             * @param Dir The absolute path name of the directory to add.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - if database error occurs, or the
             * specified path is invalid.
             */
            void AddDirectory(String Dir);

            /**
             * Removes the specified directory from the database.
             * @param Dir The absolute path name of the directory to remove.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - If the specified path is invalid, or 
             * Force is false and the specified directory is
             * not empty, or if database error occurs.
             */
            void RemoveDirectory(String Dir, bool Force = false);

            /**
             * Determines whether the specified directory exists in the database.
             * @param Dir The absolute path name of the directory.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - If database error occurs.
             */
            bool DirectoryExist(String Dir);

            /**
             * Gets information about the specified directory.
             * @param Dir The absolute path name of the directory.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - if database error occurs, or if
             * the specified directory is not found.
             */
            DbDirectory GetDirectoryInfo(String Dir);

            /**
             * Renames the specified directory.
             * @param Dir The absolute path name of the directory to rename.
             * All slashes in the directory names should be replaced with '\0'.
             * @param Name The new name for the directory.
             * @throws Exception - In case the given directory does not exists,
             * or the specified name is not a valid name,
             * or if a directory with name equal to the new name already
             * exists, or if database error occurs.
             */
            void RenameDirectory(String Dir, String Name);

            /**
             * Moves the specified directory into the specified location.
             * @param Dir The absolute path name of the directory to move.
             * All slashes in the directory names should be replaced with '\0'.
             * @param Dst The location where the directory will be moved to.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - In case a given directory does not exists,
             * or if a directory with name equal to the directory name already
             * exists in the specified destination directory, or if database error
             * occurs. Error is also thrown when trying to move a directory to a
             * subdirectory of itself.
             */
            void MoveDirectory(String Dir, String Dst);

            /**
             * Copies the specified directory into the specified location.
             * @param Dir The absolute path name of the directory to copy.
             * All slashes in the directory names should be replaced with '\0'.
             * @param Dst The location where the directory will be copied to.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - In case a given directory does not exists,
             * or if a directory with name equal to the directory name already
             * exists in the specified destination directory, or if database error
             * occurs. Error is also thrown when trying to copy a directory to a
             * subdirectory of itself.
             */
            void CopyDirectory(String Dir, String Dst);

            /**
             * Changes the description of the specified directory.
             * @param Dir The absolute path name of the directory.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - if database error occurs, or if
             * the specified directory is not found.
             */
            void SetDirectoryDescription(String Dir, String Desc);
            
            /**
             * Finds all directories that match the search query.
             * @param Dir The absolute path name of the database
             * directory to search in.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - if database error occurs, or
             * if the specified path name is invalid.
             * @returns The absolute path names of all directories
             * that match the search query.
             */
            StringListPtr FindDirectories(String Dir, SearchQuery* pQuery, bool Recursive);

            /**
             * Adds the instruments in the specified file or
             * directory to the specified instruments database directory.
             * @param DbDir The absolute path name of a directory in the
             * instruments database in which only the new instruments
             * (that are not already in the database) will be added.
             * All slashes in the directory names should be replaced with '\0'.
             * @param FilePath The absolute path name of a file or
             * directory in the file system. In case a directory is
             * supplied, all supported instruments in the specified directory
             * will be added to the instruments database, including the
             * instruments in the subdirectories. The respective subdirectory
             * structure will be recreated in the supplied database directory.
             * @param Index The index of the instrument (in the given
             * instrument file) to add. If -1 is specified, all instruments in
             * the supplied instrument file will be added. Error is thrown if
             * a directory is supplied and Index is not equal to -1.
             * @param bBackground Determines whether
             * the task should be done in the background.
             * @returns If bBackground is true, the ID of the scan job;
             * -1 otherwise.
             * @throws Exception if the operation failed.
             */
            int AddInstruments(String DbDir, String FilePath, int Index, bool bBackground);

            /**
             * Adds the instruments in the specified file
             * to the specified instruments database directory.
             * @param DbDir The absolute path name of a directory in the
             * instruments database in which only the new instruments
             * (that are not already in the database) will be added.
             * All slashes in the directory names should be replaced with '\0'.
             * @param FilePath The absolute path name of the instrument file.
             * @param Index The index of the instrument (in the given
             * instrument file) to add. If -1 is specified, all instruments in
             * the supplied instrument file will be added.
             * @param pProgress The progress used to monitor the scan process.
             * @throws Exception if the operation failed.
             */
            void AddInstruments(String DbDir, bool insDir, String FilePath, int Index = -1, ScanProgress* pProgress = NULL);

            /**
             * Adds all supported instruments in the specified file system
             * direcotry to the specified instruments database directory.
             * @param Mode Determines the scanning mode. If RECURSIVE is
             * specified, all supported instruments in the specified file system
             * direcotry will be added to the specified instruments database
             * directory, including the instruments in subdirectories
             * of the supplied directory. If NON_RECURSIVE is specified,
             * the instruments in the subdirectories will not be processed.
             * If FLAT is specified, all supported instruments in the specified
             * file system direcotry will be added, including the instruments in
             * subdirectories of the supplied directory, but the respective
             * subdirectory structure will not be recreated in the instruments
             * database and all instruments will be added directly in the
             * specified database directory.
             * @param DbDir The absolute path name of a directory in the
             * instruments database in which only the new instruments
             * (that are not already in the database) will be added.
             * All slashes in the directory names should be replaced with '\0'.
             * @param FsDir The absolute path name of an existing
             * directory in the file system.
             * @param bBackground Determines whether
             * the task should be done in the background.
             * @returns If bBackground is true, the ID of the scan job;
			 * @param insDir if true a directory is added for each instrument file
             * -1 otherwise.
             * @throws Exception if the operation failed.
             */
            int AddInstruments(ScanMode Mode, String DbDir, String FsDir, bool bBackground, bool insDir = false);

            /**
             * Gets the number of instruments in the specified directory.
             * @param Dir The absolute path name of the directory.
             * All slashes in the directory names should be replaced with '\0'.
             * @param Recursive If true, the number of all instruments
             * in the specified subtree will be returned.
             * @throws Exception - if database error occurs, or
             * the specified path name is invalid.
             * @returns The number of instruments in the specified directory.
             */
            int GetInstrumentCount(String Dir, bool Recursive);

            /**
             * Gets the list of instruments in the specified directory.
             * @param Dir The absolute path name of the directory.
             * All slashes in the directory names should be replaced with '\0'.
             * @param Recursive If true, all instruments
             * in the specified subtree will be returned.
             * @throws Exception - if database error occurs, or
             * the specified path name is invalid.
             */
            StringListPtr GetInstruments(String Dir, bool Recursive);

            /**
             * Removes the specified instrument from the database.
             * @param Instr The absolute path name of the instrument to remove.
             * All slashes in the directory/instrument names should be replaced with '\0'.
             * @throws Exception - If the specified instrument does not exist,
             * or database error occurs.
             */
            void RemoveInstrument(String Instr);

            /**
             * Gets information about the specified instrument.
             * @param Instr The absolute path name of the instrument.
             * All slashes in the directory/instrument names should be replaced with '\0'.
             * @throws Exception - if database error occurs, or if
             * the specified instrument is not found.
             */
            DbInstrument GetInstrumentInfo(String Instr);

            /**
             * Renames the specified instrument.
             * @param Instr The absolute path name of the instrument to rename.
             * All slashes in the directory/instrument names should be replaced with '\0'.
             * @param Name The new name for the instrument.
             * @throws Exception - In case the given instrument does not exists,
             * or the specified name is not a valid name, or if an instrument
             * with name equal to the new name already exists, or
             * if database error occurs.
             */
            void RenameInstrument(String Instr, String Name);

            /**
             * Moves the specified instrument into the specified directory.
             * @param Instr The absolute path name of the instrument to move.
             * All slashes in the directory/instrument names should be replaced with '\0'.
             * @param Dst The directory where the instrument will be moved to.
             * @throws Exception - In case the given directory or instrument
             * does not exist, or if an instrument with name equal to the name
             * of the specified instrument already exists in the specified
             * destination directory, or if database error occurs.
             */
            void MoveInstrument(String Instr, String Dst);

            /**
             * Copies the specified instrument into the specified directory.
             * @param Instr The absolute path name of the instrument to copy.
             * All slashes in the directory/instrument names should be replaced with '\0'.
             * @param Dst The absolute path name of the directory where the
             * instrument will be copied to. All slashes in the directory names
             * should be replaced with '\0'.
             * @throws Exception - In case the given directory or instrument
             * does not exist, or if an instrument with name equal to the name
             * of the specified instrument already exists in the specified
             * destination directory, or if database error occurs.
             */
            void CopyInstrument(String Instr, String Dst);

            /**
             * Changes the description of the specified instrument.
             * @throws Exception - if database error occurs, or if
             * the specified instrument is not found.
             */
            void SetInstrumentDescription(String Instr, String Desc);
            
            /**
             * Finds all instruments that match the search query.
             * @param Dir The absolute path name of the database
             * directory to search in.
             * All slashes in the directory names should be replaced with '\0'.
             * @throws Exception - if database error occurs, or
             * if the specified path name is invalid.
             * @returns The absolute path names of all instruments
             * that match the search query.
             */
            StringListPtr FindInstruments(String Dir, SearchQuery* pQuery, bool Recursive);
            
            /**
             * Checks all instrument files in the database and returns a list
             * of all files that dosn't exist in the filesystem.
             * @throws Exception - if database error occurs.
             * @returns The absolute path names of all lost instrument files.
             */
            StringListPtr FindLostInstrumentFiles();

            /**
             * Substitutes all occurrences of the instrument file
             * OldPath in the database, with NewPath.
             * @throws Exception - If error occurs.
             */
            void SetInstrumentFilePath(String OldPath, String NewPath);
            
            /**
             * Gets a list of all instruments in the instruments database
             * that are located in the specified instrument file.
             * @param File The absolute path name of the instrument file.
             * @throws Exception If database error occurs.
             */
            StringListPtr GetInstrumentsByFile(String File);

            /**
             * Removes the old instruments datbase and re-creates
             * the instruments database from scratch.
             */
            void Format();

            /**
             * All '\0' chars in the string are replaced with "\x2f";
             * ', ", \ are escaped with backslash and
             * <CR> and <LF> are replaced with \r and \n.
             */
            static String toEscapedPath(String AbstractPath);

            /**
             * The characters ', ", \ are escaped with backslash and
             * <CR> and <LF> are replaced with \r and \n.
             */
            static String toEscapedText(String text);

            static String toNonEscapedText(String text);
            
            JobList Jobs;

        private:
            sqlite3* db;
            String DbFile;
            static InstrumentsDb instance;
            Mutex DbInstrumentsMutex;
            ListenerList<InstrumentsDb::Listener*> llInstrumentsDbListeners;
            bool InTransaction;
            WorkerThread InstrumentsDbThread;
            
            InstrumentsDb();
            ~InstrumentsDb();
            
            /**
             * Gets the instruments database. If the database is not
             * opened, a connection to the database is established first.
             * @returns The instruments database.
             * @throws Exception if fail to open the database.
             */
            sqlite3* GetDb();

            /**
             * Gets the number of directories in the directory
             * with ID DirId.
             * @returns The number of directories in the specified directory
             * or -1 if the directory doesn't exist.
             */
            int GetDirectoryCount(int DirId);

            /**
             * Gets a list containing the IDs of all directories in
             * the specified instrument directory.
             * @returns The IDs of all directories in the specified directory.
             * @throws Exception - if database error occurs.
             */
            IntListPtr GetDirectoryIDs(int DirId);

            /**
             * Gets the list of directories in the specified directory.
             * @param DirId The ID of the directory.
             * @returns The list of directories, where the directories are
             * represented in abstract path - all slashes in the directory
             * names are replaced with '\0'.
             * @throws Exception - if database error occurs, or
             * the specified ID is invalid.
             */
            StringListPtr GetDirectories(int DirId);

            /**
             * Gets the directory ID.
             * @param Dir The absolute path name of the directory.
             * All slashes in the directory names should be replaced with '\0'.
             * @returns The directory ID or -1 if the directory is not found.
             * @throws Exception - if database error occurs.
             */
            int GetDirectoryId(String Dir);

            /**
             * Gets the directory ID.
             * @param ParentDirId The ID of the parent directory.
             * @param DirName The directory name.
             * All slashes in the directory name should be replaced with '\0'.
             * @throws Exception - if database error occurs.
             * @returns The ID of the specified directory
             * or -1 if the directory doesn't exist.
             */
            int GetDirectoryId(int ParentDirId, String DirName);

            /**
             * Gets the ID of the directory, in which the specified instrument is located.
             * @param InstrId The ID of the instrument.
             * @returns The directory ID or -1 if the directory is not found.
             * @throws Exception - if database error occurs.
             */
            int GetDirectoryId(int InstrId);

            /**
             * Gets the name of the specified directory.
             * @throws Exception - if the directory is not found
             * or if database error occurs.
             */
            String GetDirectoryName(int DirId);

            /**
             * Gets the ID of the parent directory.
             * @throws Exception - if the root directory is specified, or if the
             * specified directory is not found, or if database error occurs.
             */
            int GetParentDirectoryId(int DirId);

            /**
             * Gets the absolute path name of the specified directory.
             * @param DirId The ID of the directory, which absolute
             * path name should be returned.
             * @throws Exception If the specified directory is not
             * found or if database error occurs.
             */
            String GetDirectoryPath(int DirId);

            /**
             * Removes the specified directory from the database.
             * @param DirId The ID of the directory to remove.
             * @throws Exception - If the specified directory is not empty.
             */
            void RemoveDirectory(int DirId);

            /**
             * Removes all directories in the specified directory.
             * Do NOT call this method before ensuring that all
             * directories in the specified directory are empty.
             * @param DirId The ID of the directory.
             * @throws Exception - if at least one of the directories in the
             * specified directory is not empty or if database error occurs.
             * @see IsDirectoryEmpty
             */
            void RemoveAllDirectories(int DirId);

            /**
             * Determines whether the specified directory is empty.
             * If the directory doesn't exist the return value is false.
             * @throws Exception - if database error occurs.
             */
            bool IsDirectoryEmpty(int DirId);

            /**
             * Removes the content of the specified directory from the database.
             * @param DirId The ID of the directory.
             * @param Level Used to prevent stack overflow, which may occur
             * due to large or infinite recursive loop.
             * @throws Exception - If database error occurs.
             */
            void RemoveDirectoryContent(int DirId, int Level = 0);

            /**
             * Gets the ID of the specified database instrument.
             * @param Instr The absolute path name of the instrument.
             * All slashes in the directory/instrument names should be replaced with '\0'.
             * @returns The instrument ID or -1 if the instrument is not found.
             * @throws Exception - if database error occurs.
             */
            int GetInstrumentId(String Instr);

            /**
             * Gets the ID of the specified database instrument.
             * @param DirId The ID of the directory containing the instrument.
             * @param InstrName The name of the instrument.
             * All slashes in the instrument name should be replaced with '\0'.
             * @returns The instrument ID or -1 if the instrument is not found.
             * @throws Exception - if database error occurs.
             */
            int GetInstrumentId(int DirId, String InstrName);

            /**
             * Gets the name of the instrument with the specified ID.
             * @param InstrId The ID of the instrument, which name should be obtained.
             * @returns The name of the specified instrument, where all slashes
             * in the name are replaced with '\0'.
             * @throws Exception - if database error occurs.
             */
            String GetInstrumentName(int InstrId);

            /**
             * Removes the specified instrument from the database.
             * @param InstrId The ID of the instrument to remove.
             * @throws Exception - If the specified instrument does not exist.
             */
            void RemoveInstrument(int InstrId);

            /**
             * Removes all instruments in the specified directory.
             * @param DirId The ID of the directory.
             * @throws Exception - if database error occurs.
             */
            void RemoveAllInstruments(int DirId);

            /**
             * Gets the number of instruments in the directory
             * with ID DirId.
             * @returns The number of instruments in the specified directory
             * or -1 if the directory doesn't exist.
             */
            int GetInstrumentCount(int DirId);

            /**
             * Gets a list containing the IDs of all instruments in
             * the specified instrument directory.
             * @returns The IDs of all instruments in the specified directory.
             * @throws Exception - if database error occurs.
             */
            IntListPtr GetInstrumentIDs(int DirId);

            /**
             * Gets information about the specified instrument.
             * @param InstrId The ID of the instrument.
             * @throws Exception - if database error occurs.
             */
            DbInstrument GetInstrumentInfo(int InstrId);

            /**
             * Copies the specified instrument into the specified directory.
             * @param InstrId The ID of the instrument to copy.
             * @param InstrName The name of the instrument to copy.
             * All slashes in the instrument name should be replaced with '\0'.
             * @param DstDirId The ID of the directory where the
             * instrument will be copied to.
             * @param DstDir The name of the directory where the
             * instrument will be copied to.
             * @throws Exception - If database error occurs.
             */
            void CopyInstrument(int InstrId, String InstrName, int DstDirId, String DstDir);

            /**
             * Adds all supported instruments in the specified directory
             * to the specified instruments database directory. The
             * instruments in the subdirectories will not be processed.
             * @param DbDir The absolute path name of a directory in the
             * instruments database in which only the new instruments
             * (that are not already in the database) will be added.
             * All slashes in the directory names should be replaced with '\0'.
             * @param FsDir The absolute path name of a directory in the file
             * system.
			 * @param insDir If true a directory will be create for each gig file
             * @param pProgress The progress used to monitor the scan process.
             * @throws Exception if the operation failed.
             */
            void AddInstrumentsNonrecursive(String DbDir, String FsDir, bool insDir = false, ScanProgress* pProgress = NULL);

            /**
             * Adds all supported instruments in the specified file system
             * direcotry to the specified instruments database directory,
             * including the instruments in the subdirectories of the
             * supplied directory.
             * @param DbDir The absolute path name of a directory in the
             * instruments database in which only the new instruments
             * (that are not already in the database) will be added.
             * All slashes in the directory names should be replaced with '\0'.
             * @param FsDir The absolute path name of an existing
             * directory in the file system.
             * @param Flat If true, the respective subdirectory structure will
             * not be recreated in the instruments database and all instruments
             * will be added directly in the specified database directory.
             * @param pProgress The progress used to monitor the scan process.
             * @throws Exception if the operation failed.
             */
            void AddInstrumentsRecursive(String DbDir, String FsDir, bool Flat = false, bool insDir = false, ScanProgress* pProgress = NULL);

            /**
             * Adds the instruments in the specified file
             * to the specified instruments database directory.
             * @param DbDir The absolute path name of a directory in the
             * instruments database in which only the new instruments
             * (that are not already in the database) will be added.
             * All slashes in the directory names should be replaced with '\0'.
             * @param File The absolute path name of a file in the file system.
             * @param Index The index of the instrument (in the given
             * instrument file) to add. If -1 is specified, all instruments in
             * the supplied instrument file will be added.
             * @param pProgress The progress used to monitor the scan process.
             * Specify NULL if you don't want to monitor the scanning process.
             * @throws Exception if the operation failed.
             */
            void AddInstrumentsFromFile(String DbDir, String File, int Index = -1, ScanProgress* pProgress = NULL);

            /**
             * Adds the specified GIG instrument(s) to the specified location
             * in the instruments database.
             * @param DbDir The instruments database directory
             * in which the instrument will be added.
             * All slashes in the directory names should be replaced with '\0'.
             * @param FilePath The absolute path name of the instrument file.
             * @param Index The index of the instrument (in the given
             * instrument file) to add. If -1 is specified, all instruments in
             * the supplied instrument file will be added.
             * @param pProgress The progress used to monitor the scan process.
             * Specify NULL if you don't want to monitor the scanning process.
             * @throws Exception if the operation failed.
             */
            void AddInstrumentsFromFilePriv(String DbDir, const int dirId, String FilePath, File file, int Index = -1, ScanProgress* pProgress = NULL);

            void DirectoryTreeWalk(String AbstractPath, DirectoryHandler* pHandler);

            void DirectoryTreeWalk(DirectoryHandler* pHandler, String AbstractPath, int DirId, int Level);

            /** Locks the DbInstrumentsMutex and starts a transaction. */
            void BeginTransaction();

            /** Commits the transaction and unocks the DbInstrumentsMutex. */
            void EndTransaction();
            
            /**
             * Used to execute SQL commands which return empty result set.
             */
            void ExecSql(String Sql);

            /**
             * Used to execute SQL commands which return empty result set.
             */
            void ExecSql(String Sql, String Param);

            /**
             * Used to execute SQL commands which return empty result set.
             */
            void ExecSql(String Sql, std::vector<String>& Params);

            /**
             * Used to execute SQL commands which returns integer.
             */
            int ExecSqlInt(String Sql);

            /**
             * Used to execute SQL commands which returns integer.
             */
            int ExecSqlInt(String Sql, String Param);

            /**
             * Used to execute SQL commands which returns string.
             */
            String ExecSqlString(String Sql);

            /**
             * Used to execute SQL commands which returns integer list.
             */
            IntListPtr ExecSqlIntList(String Sql);

            /**
             * Used to execute SQL commands which returns integer list.
             */
            IntListPtr ExecSqlIntList(String Sql, String Param);

            /**
             * Used to execute SQL commands which returns integer list.
             */
            IntListPtr ExecSqlIntList(String Sql, std::vector<String>& Params);

            /**
             * Used to execute SQL commands which returns string list.
             */
            StringListPtr ExecSqlStringList(String Sql);
            
            /**
             * Binds the specified text parameter.
             */
            void BindTextParam(sqlite3_stmt* pStmt, int Index, String Text);
            
            /**
             * Binds the specified integer parameter.
             */
            void BindIntParam(sqlite3_stmt* pStmt, int Index, int Param);

            /**
             * Checks whether an instrument or directory with the specified name
             * already exists in the specified directory and if so a new unique name
             * is returnded. The new name is generated by adding the suffix
             * [<nr>] to the end of the name, where <nr> is a number between
             * 2 and 1000.
             * throws Exception if cannot find an unique name. This is done
             * because it is highly unlikely that this can happen, so it is
             * supposed that this is due to a bug or an infinite loop.
             */
            String GetUniqueName(int DirId, String Name);

            /**
             * Creates a new directory in the specified existing
             * instruments database directory. The directory name is
             * the base name of the specified file system path, if there is
             * no instrument in DbDir directory with that name. Otherwise,
             * a directory with unique name is created.
             * @returns The absolute path name of the newly created
             * instruments database directory.
             */
            String PrepareSubdirectory(String DbDir, String FsPath);

            void EnsureDBFileExists();

            /**
             * Adds the specified node to the specified database directory path.
             * @returns The newly created instruments database path.
             */
            static String AppendNode(String DbDir, String Node);

            /**
             * All '\0' chars in the string are replaced with '/'.
             */
            static String toDbName(String AbstractName);

            /**
             * All slashes are replaced with '\0'.
             */
            static String toAbstractName(String DbName);

            static String toEscapedFsPath(String FsPath);
            
            static String toNonEscapedFsPath(String FsPath);

            void FireDirectoryCountChanged(String Dir);
            void FireDirectoryInfoChanged(String Dir);
            void FireDirectoryNameChanged(String Dir, String NewName);
            void FireInstrumentCountChanged(String Dir);
            void FireInstrumentInfoChanged(String Instr);
            void FireInstrumentNameChanged(String Instr, String NewName);
            void FireJobStatusChanged(int JobId);

            /**
             * Strips the non-directory suffix from the file name. If the string
             * ends with `/' only the last character is removed. If the string
             * doesn't starts with `/' charater, an empty string is returned.
             */
            static String GetDirectoryPath(String File);

            /**
             * Returns the file name extracted from the specified absolute path
             * name. If the string doesn't starts with `/' or ends with `/',
             * an empty string is returned.
             */
            static String GetFileName(String Path);

            /**
             * Strips the last directory from the specified path name. If the 
             * string doesn't starts with `/'  an empty string is returned.
             */
            static String GetParentDirectory(String Dir);

            /**
             * Checks whether the specified path is valid.
             * @throws Exception - if the specified path is invalid.
             */
            static void CheckPathName(String Path);

            /**
             * Checks whether the specified file name is valid.
             * @throws Exception - if the specified file name is invalid.
             */
            static void CheckFileName(String File);

            static String GetDefaultDBLocation();

#ifndef WIN32
            /** SQLite user function for handling regular expressions */
            static void Regexp(sqlite3_context* pContext, int argc, sqlite3_value** ppValue);
#endif
    };

} // namespace LinuxSampler

#endif // __LS_INSTRUMENTSDB_H__
