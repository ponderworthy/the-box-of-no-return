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

#include "../common/global_private.h"

#ifndef __LS_INSTRUMENTSDBUTILITIES_H__
#define __LS_INSTRUMENTSDBUTILITIES_H__

#include <memory>
#include <vector>
#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/gig.h>
#else
# include <gig.h>
#endif
#include <sqlite3.h>

#include "../common/File.h"
#include "../common/optional.h"

namespace LinuxSampler {

    class DbInstrument {
        public:
            String InstrFile;
            int    InstrNr;
            String FormatFamily;
            String FormatVersion;
            long long int Size;
            String Created;
            String Modified;
            String Description;
            bool   IsDrum;
            String Product;
            String Artists;
            String Keywords;

            DbInstrument() { }
            DbInstrument(const DbInstrument& Instr) { Copy(Instr); }
            void operator=(const DbInstrument& Instr) { Copy(Instr); }
            void Copy(const DbInstrument&);
    };

    class DbDirectory {
        public:
            String Created;
            String Modified;
            String Description;

            DbDirectory() { }
            DbDirectory(const DbDirectory& Dir) { Copy(Dir); }
            void operator=(const DbDirectory& Dir) { Copy(Dir); }
            void Copy(const DbDirectory&);
    };

    class SearchQuery {
        public:
            enum InstrumentType {
                CHROMATIC = 0,
                DRUM = 1,
                BOTH = 2
            };

            String Name;
            std::vector<String> FormatFamilies;
            long long MinSize;
            long long MaxSize;
            String CreatedBefore;
            String CreatedAfter;
            String ModifiedBefore;
            String ModifiedAfter;
            String Description;
            String Product;
            String Artists;
            String Keywords;
            InstrumentType InstrType;
            
            SearchQuery();
            void SetFormatFamilies(String s);
            void SetSize(String s);
            void SetCreated(String s);
            void SetModified(String s);

        private:
            String GetMin(String s);
            String GetMax(String s);
    };

    enum ScanMode {
        RECURSIVE = 0,
        NON_RECURSIVE = 1,
        FLAT = 2
    };

#if __cplusplus >= 201103L && !CONFIG_NO_CPP11STL
    typedef std::unique_ptr<std::vector<int>> IntListPtr;
    typedef std::unique_ptr<std::vector<String>> StringListPtr;
#else
    typedef std::auto_ptr<std::vector<int> > IntListPtr;
    typedef std::auto_ptr<std::vector<String> > StringListPtr;
#endif

    class ScanJob {
        public:
            int JobId;
            int FilesTotal;
            int FilesScanned;
            String Scanning;
            int Status;

            ScanJob() : FilesTotal(0), FilesScanned(0), Status(0) { }
            ScanJob(const ScanJob& Job) { Copy(Job); }
            void operator=(const ScanJob& Job) { Copy(Job); }
            void Copy(const ScanJob&);
    };
    
    class JobList {
        public:
            JobList() { Counter = 0; }

            /**
             * Adds the specified job to the list and keeps the list
             * to have maximum 3 jobs (the oldest one is removed).
             * @returns The ID used to identify this job.
             */
            int AddJob(ScanJob Job);

            /**
             * Returns the job with ID JobId.
             * @throws Exception If job with ID JobId doesn't exist.
             */
            ScanJob& GetJobById(int JobId);

        private:
            std::vector<ScanJob> Jobs;
            int Counter;
    };

    class DirectoryHandler {
        public:
            virtual void ProcessDirectory(String Path, int DirId) = 0;
    };

    class AbstractFinder : public DirectoryHandler {
        public:
            virtual void ProcessDirectory(String Path, int DirId) = 0;

            bool IsRegex(String Pattern);
            void AddSql(String Col, String Pattern, std::stringstream& Sql);

        protected:
            std::vector<String> Params;
    };

    class DirectoryFinder : public AbstractFinder {
        public:
            DirectoryFinder(SearchQuery* pQuery);
            ~DirectoryFinder();
            StringListPtr GetDirectories();
            virtual void ProcessDirectory(String Path, int DirId);

        private:
            sqlite3_stmt* pStmt;
            String SqlQuery;
            SearchQuery* pQuery;
            StringListPtr pDirectories;
    };

    class InstrumentFinder : public AbstractFinder {
        public:
            InstrumentFinder(SearchQuery* pQuery);
            ~InstrumentFinder();
            StringListPtr GetInstruments();
            virtual void ProcessDirectory(String Path, int DirId);

        private:
            sqlite3_stmt* pStmt;
            String SqlQuery;
            SearchQuery* pQuery;
            StringListPtr pInstruments;
    };

    class DirectoryCounter : public DirectoryHandler {
        public:
            DirectoryCounter() { count = 0; }
            virtual void ProcessDirectory(String Path, int DirId);
            int GetDirectoryCount() { return count; }

        private:
            int count;
    };

    class InstrumentCounter : public DirectoryHandler {
        public:
            InstrumentCounter() { count = 0; }
            virtual void ProcessDirectory(String Path, int DirId);
            int GetInstrumentCount() { return count; }

        private:
            int count;
    };

    class DirectoryCopier : public DirectoryHandler {
        public:
            DirectoryCopier(String SrcParentDir, String DestDir);
            virtual void ProcessDirectory(String Path, int DirId);

        private:
            String SrcParentDir;
            String DestDir;
    };

    /**
     * Used to monitor the scan progress of instrument files
     */
    class ScanProgress {
        public:
            ScanProgress();
            
            /**
             * Invoked when there is some progress on the monitored task.
             */
            void StatusChanged();

            /**
             * Gets the total number of files scheduled for scanning.
             */
            int GetTotalFileCount();

            /**
             * Sets the total number of files scheduled for scanning.
             */
            void SetTotalFileCount(int Count);

            /**
             * Gets the current nuber of scanned files.
             */
            int GetScannedFileCount();

            /**
             * Sets the current nuber of scanned files.
             */
            void SetScannedFileCount(int Count);

            /**
             * Returns an integer value between 0 and 100 indicating the
             * scanning progress percentage of the file which is currently
             * being scanned. Negative value indicates an error.
             */
            int GetStatus();

            /**
             * Sets the scanning progress percentage of the file which is
             * currently being scanned. If negative value is specified, the
             * status is set to 0. If value greater then 100 is specified,
             * the status is set to 100.
             */
            void SetStatus(int Status);
            
            /**
             * Sets and error code to indicate task failure.
             * @param Err Should be negative value. If positive value
             * is suplied, it is reverted to negative.
             */
            void SetErrorStatus(int Err);
            
            /**
             * The absolute path name of the file which is currently being scanned.
             */
            String CurrentFile;

            int JobId;
            ::gig::progress_t GigFileProgress;

        private:
            int TotalFileCount;
            int ScannedFileCount;
            int Status;

            static void GigFileProgressCallback(::gig::progress_t* pProgress);
    };

    /**
     * A job which adds the instruments in the specified
     * file to the specified instruments database directory.
     */
    class AddInstrumentsFromFileJob : public Runnable {
        public:
            /**
             * param JobId the ID of this job.
             * @param DbDir The absolute path name of a directory in the
             * instruments database in which only the new instruments
             * (that are not already in the database) will be added.
             * @param FilePath The absolute path name of the instrument file.
             * @param Index The index of the instrument (in the given
             * instrument file) to add. If -1 is specified, all instruments in
             * the supplied instrument file will be added.
			 * @param insDir if true a separate directory will be created for
			 * the gig file.
             */
            AddInstrumentsFromFileJob(int JobId, String DbDir, String FilePath, int Index = -1, bool insDir = false);
            
            /**
             * The entry point of the job.
             */
            virtual void Run();

            private:
                int JobId;
                String DbDir;
                String FilePath;
                int Index;
                ScanProgress Progress;
				bool insDir;
    };

    /**
     * A job which adds all supported instruments in the specified
     * directory to the specified instruments database directory.
     */
    class AddInstrumentsJob : public Runnable {
        public:
            /**
             * param JobId the ID of this job.
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
             * @param FsDir The absolute path name of a directory in the
             * file system.
			 * @param insDir If true a separate directory will be created for 
			 * each gig file
             */
            AddInstrumentsJob(int JobId, ScanMode Mode, String DbDir, String FsDir, bool insDir = false);
            
            /**
             * The entry point of the job.
             */
            virtual void Run();

            private:
                int JobId;
                ScanMode Mode;
                String DbDir;
                String FsDir;
                ScanProgress Progress;
				bool insDir;

                int GetFileCount();
    };
    
    class DirectoryScanner: public File::DirectoryWalker {
        public:
            /**
             * Recursively scans all subdirectories of the specified file
             * system directory and adds the supported instruments to the database.
			 * @param insDir is true this will create a directory for each gig file.
             * @param pProgress The progress used to monitor the scan process.
             * @throws Exception - if the specified directories are invalid.
             */
           void Scan(String DbDir, String FsDir, bool Flat, bool insDir = false, ScanProgress* pProgress = NULL);

           virtual void DirectoryEntry(std::string Path);
           virtual void FileEntry(std::string Path) { }

        private:
            String DbDir;
            String FsDir;
            bool Flat;
            ScanProgress* pProgress;
            bool HasInstrumentFiles(String Dir);
			bool insDir;
    };
    
    class InstrumentFileCounter: public File::DirectoryWalker {
        public:
            /**
             * Recursively scans all subdirectories of the specified file
             * system directory and returns the number of supported instrument files.
             * @param FsDir The file system directory to process.
             * @throws Exception - if the specified directory is invalid.
             */
            int Count(String FsDir);

            virtual void DirectoryEntry(std::string Path) { }
            virtual void FileEntry(std::string Path);

        private:
            int FileCount;
    };

    struct InstrumentInfo {
        String instrumentName;
        String product;
        String artists;
        String keywords;
        String comments;
        bool   isDrum;

        InstrumentInfo() : isDrum(false) {}
    };

    class InstrumentFileInfo {
    public:
        virtual ~InstrumentFileInfo() {}
        virtual optional<InstrumentInfo> getInstrumentInfo(int index, ScanProgress* pProgress) = 0;
        virtual String formatName() = 0;
        virtual String formatVersion() = 0;

        static InstrumentFileInfo* getFileInfoFor(String filename);
        static bool isSupportedFile(String filename);

    protected:
        InstrumentFileInfo(String filename) : m_fileName(filename) {}

        String m_fileName;
    };

} // namespace LinuxSampler

#endif // __LS_INSTRUMENTSDBUTILITIES_H__
