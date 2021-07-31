/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007-2013 Grigor Iliev, Benno Senoner                   *
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

#include "InstrumentsDb.h"

#include "../common/File.h"
#include "../common/Path.h"
#include "../common/global_private.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <errno.h>
#ifndef WIN32
#include <fnmatch.h>
#else
#include <direct.h>
#endif
#include "../common/Exception.h"

namespace LinuxSampler {

    InstrumentsDb InstrumentsDb::instance;

    void InstrumentsDb::CreateInstrumentsDb(String FilePath) {
        if (FilePath.empty()) {
            FilePath = GetDefaultDBLocation();
            dmsg(0,("InstrumentsDb: Creating database at default location '%s'\n", FilePath.c_str()));
        }

        File f = File(FilePath);
        if (f.Exist()) {
            throw Exception("File exists: " + FilePath);
        }

        SetDbFile(FilePath);

        String sql = 
            "  CREATE TABLE instr_dirs (                                      "
            "      dir_id         INTEGER PRIMARY KEY AUTOINCREMENT,          "
            "      parent_dir_id  INTEGER DEFAULT 0,                          "
            "      dir_name       TEXT,                                       "
            "      created        TIMESTAMP DEFAULT CURRENT_TIMESTAMP,        "
            "      modified       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,        "
            "      description    TEXT,                                       "
            "      FOREIGN KEY(parent_dir_id) REFERENCES instr_dirs(dir_id),  "
            "      UNIQUE (parent_dir_id,dir_name)                            "
            "  );                                                             ";
        
        ExecSql(sql);

        sql = "INSERT INTO instr_dirs (dir_id, parent_dir_id, dir_name) VALUES (0, -2, '/');";
        ExecSql(sql);

        sql =
            "  CREATE TABLE instruments (                                "
            "      instr_id        INTEGER PRIMARY KEY AUTOINCREMENT,    "
            "      dir_id          INTEGER DEFAULT 0,                    "
            "      instr_name      TEXT,                                 "
            "      instr_file      TEXT,                                 "
            "      instr_nr        INTEGER,                              "
            "      format_family   TEXT,                                 "
            "      format_version  TEXT,                                 "
            "      instr_size      INTEGER,                              "
            "      created         TIMESTAMP DEFAULT CURRENT_TIMESTAMP,  "
            "      modified        TIMESTAMP DEFAULT CURRENT_TIMESTAMP,  "
            "      description     TEXT,                                 "
            "      is_drum         INTEGER(1),                           "
            "      product         TEXT,                                 "
            "      artists         TEXT,                                 "
            "      keywords        TEXT,                                 "
            "      FOREIGN KEY(dir_id) REFERENCES instr_dirs(dir_id),    "
            "      UNIQUE (dir_id,instr_name)                            "
            "  );                                                        ";
        
        ExecSql(sql);
    }

    InstrumentsDb::InstrumentsDb() {
        db = NULL;
        InTransaction = false;
    }

    InstrumentsDb::~InstrumentsDb() {
        InstrumentsDbThread.StopThread();
        if (db != NULL) sqlite3_close(db);
    }
    
    void InstrumentsDb::AddInstrumentsDbListener(InstrumentsDb::Listener* l) {
        llInstrumentsDbListeners.AddListener(l);
    }
    
    void InstrumentsDb::RemoveInstrumentsDbListener(InstrumentsDb::Listener* l) {
        llInstrumentsDbListeners.RemoveListener(l);
    }
    
    InstrumentsDb* InstrumentsDb::GetInstrumentsDb() {
        return &instance;
    }
    
    void InstrumentsDb::SetDbFile(String File) {
        LockGuard lock(DbInstrumentsMutex);
        if (File.empty() || DbFile.length() > 0) {
            throw Exception("Failed to set the database file");
        }
        DbFile = File;
    }

    String InstrumentsDb::GetDefaultDBLocation() {
        #ifdef WIN32
        char* userprofile = getenv("USERPROFILE");
        if (userprofile) {
            String s = userprofile;
            s += "\\.linuxsampler\\instruments.db";
            return s;
        } else {
            // in case USERPROFILE is not set (which should not occur)
            return "instruments.db";
        }
        #else // POSIX ...
        String s = CONFIG_DEFAULT_INSTRUMENTS_DB_LOCATION;
        # if defined(__APPLE__)
        if (s.find("~") == 0)
            s.replace(0, 1, getenv("HOME"));
        # endif
        return s;
        #endif
    }

    void InstrumentsDb::EnsureDBFileExists() {
        if (DbFile.empty())
            DbFile = GetDefaultDBLocation();
		#if defined(__APPLE__)  /* 20071224 Toshi Nagata  */
		if (DbFile.find("~") == 0)
			DbFile.replace(0, 1, getenv("HOME"));
		#endif
        Path DbPath(DbFile);
        String DbDir = DbPath.stripLastName();
        // create directory if it does not exist yet
        if (!DbPath.nodes().empty()) {
            File d(DbDir);
            if (!d.Exist()) {
                #ifdef WIN32
                if (_mkdir(DbDir.c_str()))
                    throw Exception("Could not create instruments DB directory '" + DbDir + "'");
                #else
                if (mkdir(DbDir.c_str(), S_IRWXU))
                    throw Exception("Could not create instruments DB directory '" + DbDir + "'");
                #endif
            }
        }
        // create database file if it does not exist yet
        File f(DbFile);
        if (!f.Exist()) {
            // formats the DB, which creates a new instruments.db file
            Format();
        }
    }

    sqlite3* InstrumentsDb::GetDb() {
        if ( db != NULL) return db;

        if (DbFile.empty())
            DbFile = GetDefaultDBLocation();

        {
            // first check if the instruments DB's directory exists, if not give up
            Path path(DbFile);
            String sDir = path.stripLastName();
            File d(sDir);
            if (!d.Exist())
                throw Exception("Instruments DB directory '" + sDir + "' does not exist!");

            // just to give the user a notice about the DB file being created in case it does not exist yet
            File f(DbFile);
            if (!f.Exist())
                dmsg(0,("Instruments DB file '%s' does not exist yet. Trying to create it now.\n", DbFile.c_str()));
        }

        dmsg(0,("Opening instruments DB at '%s'\n", DbFile.c_str()));
        int rc = sqlite3_open(DbFile.c_str(), &db);
        if (rc) {
            sqlite3_close(db);
            db = NULL;
            throw Exception("Cannot open instruments database: " + DbFile);
        }
#ifndef WIN32
        rc = sqlite3_create_function(db, "regexp", 2, SQLITE_UTF8, NULL, Regexp, NULL, NULL);
        if (rc) { throw Exception("Failed to add user function for handling regular expressions."); }
#endif

        // TODO: remove this in the next version
        try {
            int i = ExecSqlInt("SELECT parent_dir_id FROM instr_dirs WHERE dir_id=0");
            // The parent ID of the root directory should be -2 now.
            if(i != -2) ExecSql("UPDATE instr_dirs SET parent_dir_id=-2 WHERE dir_id=0");
        } catch(Exception e) { }
        ////////////////////////////////////////
        
        return db;
    }
    
    int InstrumentsDb::GetDirectoryCount(int DirId) {
        dmsg(2,("InstrumentsDb: GetDirectoryCount(DirId=%d)\n", DirId));
        if(DirId == -1) return -1;

        std::stringstream sql;
        sql << "SELECT COUNT(*) FROM instr_dirs WHERE parent_dir_id=" <<  DirId;
        
        int count = ExecSqlInt(sql.str());

        return count;
    }

    int InstrumentsDb::GetDirectoryCount(String Dir, bool Recursive) {
        dmsg(2,("InstrumentsDb: GetDirectoryCount(Dir=%s,Recursive=%d)\n", Dir.c_str(), Recursive));
        int i;

        BeginTransaction();
        try {
            if (Recursive) {
                DirectoryCounter directoryCounter;
                DirectoryTreeWalk(Dir, &directoryCounter);
                i = directoryCounter.GetDirectoryCount();
            } else {
                i = GetDirectoryCount(GetDirectoryId(Dir));
            }
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        if (i == -1) throw Exception("Unkown DB directory: " + toEscapedPath(Dir));
        
        return i;
    }

    IntListPtr InstrumentsDb::GetDirectoryIDs(int DirId) {
        std::stringstream sql;
        sql << "SELECT dir_id FROM instr_dirs ";
        sql << "WHERE parent_dir_id=" << DirId << " AND dir_id!=0";

        return ExecSqlIntList(sql.str());
    }

    StringListPtr InstrumentsDb::GetDirectories(String Dir, bool Recursive) {
        dmsg(2,("InstrumentsDb: GetDirectories(Dir=%s,Recursive=%d)\n", Dir.c_str(), Recursive));

        BeginTransaction();
        try {
            int dirId = GetDirectoryId(Dir);
            if(dirId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));

            StringListPtr pDirs;
            if (Recursive) {
                SearchQuery q;
                DirectoryFinder directoryFinder(&q);
                DirectoryTreeWalk(Dir, &directoryFinder);
                pDirs = directoryFinder.GetDirectories();
            } else {
                pDirs = GetDirectories(dirId);
            }
            EndTransaction();
            return pDirs;
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
    }
    
    StringListPtr InstrumentsDb::GetDirectories(int DirId) {
        std::stringstream sql;
        sql << "SELECT dir_name FROM instr_dirs ";
        sql << "WHERE parent_dir_id=" << DirId << " AND dir_id!=0";
        StringListPtr dirs = ExecSqlStringList(sql.str());

        for (int i = 0; i < dirs->size(); i++) {
            for (int j = 0; j < dirs->at(i).length(); j++) {
                if (dirs->at(i).at(j) == '/') dirs->at(i).at(j) = '\0';
            }
        }

        return dirs;
    }

    int InstrumentsDb::GetDirectoryId(String Dir) {
        dmsg(2,("InstrumentsDb: GetDirectoryId(Dir=%s)\n", Dir.c_str()));
        CheckPathName(Dir);
        
        if (Dir.empty() || Dir.at(0) != '/') {
            return -1;
        } else if (Dir.length() == 1) {
            // We expect the root directory id to be always 0.
            return 0;
        }
        
        int id = 0, i = 1;
        int j = (int) Dir.find('/', i);

        while(j != std::string::npos) {
            id = GetDirectoryId(id, Dir.substr(i, j - i));
            i = j + 1;
            if (i >= Dir.length()) return id;
            j = (int) Dir.find('/', i);
        }
        
        return GetDirectoryId(id, Dir.substr(i));
    }

    int InstrumentsDb::GetDirectoryId(int ParentDirId, String DirName) {
        dmsg(2,("InstrumentsDb: GetDirectoryId(ParentDirId=%d, DirName=%s)\n", ParentDirId, DirName.c_str()));
        DirName = toDbName(DirName);
        std::stringstream sql;
        sql << "SELECT dir_id FROM instr_dirs WHERE parent_dir_id=";
        sql << ParentDirId << " AND dir_name=?";
        return ExecSqlInt(sql.str(), DirName);
    }

    int InstrumentsDb::GetDirectoryId(int InstrId) {
        dmsg(2,("InstrumentsDb: GetDirectoryId(InstrId=%d)\n", InstrId));
        std::stringstream sql;
        sql << "SELECT dir_id FROM instruments WHERE instr_id=" << InstrId;
        return ExecSqlInt(sql.str());
    }

    String InstrumentsDb::GetDirectoryName(int DirId) {
        String sql = "SELECT dir_name FROM instr_dirs WHERE dir_id=" + ToString(DirId);
        String name = ExecSqlString(sql);
        if (name.empty()) throw Exception("Directory ID not found");
        return name;
    }

    int InstrumentsDb::GetParentDirectoryId(int DirId) {
        if (DirId == 0) throw Exception("The root directory is specified");
        String sql = "SELECT parent_dir_id FROM instr_dirs WHERE dir_id=" + ToString(DirId);
        int parentId = ExecSqlInt(sql);
        if (parentId == -1) throw Exception("DB directory not found");
        return parentId;
    }

    String InstrumentsDb::GetDirectoryPath(int DirId) {
        String path = "";
        int count = 1000; // used to prevent infinite loops

        while(--count) {
            if (DirId == 0) {
                path = "/" + path;
                break;
            }
            path = GetDirectoryName(DirId) + "/" + path;
            DirId = GetParentDirectoryId(DirId);
        }

        if (!count) throw Exception("Possible infinite loop detected");

        return path;
    }
    
    StringListPtr InstrumentsDb::GetInstrumentsByFile(String File) {
        dmsg(2,("InstrumentsDb: GetInstrumentsByFile(File=%s)\n", File.c_str()));

        StringListPtr instrs(new std::vector<String>);
        
        BeginTransaction();
        try {
            File = toEscapedFsPath(File);
            IntListPtr ids = ExecSqlIntList("SELECT instr_id FROM instruments WHERE instr_file=?", File);
            
            for (int i = 0; i < ids->size(); i++) {
                String name = GetInstrumentName(ids->at(i));
                String dir = GetDirectoryPath(GetDirectoryId(ids->at(i)));
                instrs->push_back(dir + name);
            }
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        
        return instrs;
    }

    void InstrumentsDb::AddDirectory(String Dir) {
        dmsg(2,("InstrumentsDb: AddDirectory(Dir=%s)\n", Dir.c_str()));
        CheckPathName(Dir);
        String ParentDir = GetParentDirectory(Dir);
        
        BeginTransaction();
        try {
            if (Dir.length() > 1) {
                if (Dir.at(Dir.length() - 1) == '/') Dir.erase(Dir.length() - 1);
            }

            String dirName = GetFileName(Dir);
            if(ParentDir.empty() || dirName.empty()) {
                throw Exception("Failed to add DB directory: " + toEscapedPath(Dir));
            }

            int id = GetDirectoryId(ParentDir);
            if (id == -1) throw Exception("DB directory doesn't exist: " + toEscapedPath(ParentDir));
            int id2 = GetDirectoryId(id, dirName);
            if (id2 != -1) throw Exception("DB directory already exist: " + toEscapedPath(Dir));
            id2 = GetInstrumentId(id, dirName);
            if (id2 != -1) throw Exception("Instrument with that name exist: " + toEscapedPath(Dir));

            std::stringstream sql;
            sql << "INSERT INTO instr_dirs (parent_dir_id, dir_name) VALUES (";
            sql << id << ", ?)";

            ExecSql(sql.str(), toDbName(dirName));
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }

        EndTransaction();

        FireDirectoryCountChanged(ParentDir);
    }

    void InstrumentsDb::RemoveDirectory(String Dir, bool Force) {
        dmsg(2,("InstrumentsDb: RemoveDirectory(Dir=%s,Force=%d)\n", Dir.c_str(), Force));

        String ParentDir = GetParentDirectory(Dir);

        BeginTransaction();
        try {
            int dirId = GetDirectoryId(Dir);
            if (dirId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));
            if (dirId == 0) throw Exception("Cannot delete the root directory: " + Dir);
            if(ParentDir.empty()) throw Exception("Unknown parent directory");
            if (Force) RemoveDirectoryContent(dirId);
            RemoveDirectory(dirId);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }

        EndTransaction();
        FireDirectoryCountChanged(ParentDir);
    }

    void InstrumentsDb::RemoveDirectoryContent(int DirId, int Level) {
        dmsg(2,("InstrumentsDb: RemoveDirectoryContent(DirId=%d,Level=%d)\n", DirId, Level));
        if (Level > 1000) throw Exception("Directory level too deep: " + ToString(Level));
        IntListPtr dirIds = GetDirectoryIDs(DirId);

        for (int i = 0; i < dirIds->size(); i++) {
            RemoveDirectoryContent(dirIds->at(i), Level + 1);
        }

        RemoveAllDirectories(DirId);
        RemoveAllInstruments(DirId);
    }

    void InstrumentsDb::RemoveDirectory(int DirId) {
        dmsg(2,("InstrumentsDb: RemoveDirectory(DirId=%d)\n", DirId));
        if (GetInstrumentCount(DirId) > 0 || GetDirectoryCount(DirId) > 0) {
            throw Exception("The specified DB directory is not empty");
        }

        std::stringstream sql;
        sql << "DELETE FROM instr_dirs WHERE dir_id=" << DirId;

        ExecSql(sql.str());
    }

    void InstrumentsDb::RemoveAllDirectories(int DirId) {
        dmsg(2,("InstrumentsDb: RemoveAllDirectories(DirId=%d)\n", DirId));
        IntListPtr dirIds = GetDirectoryIDs(DirId);

        for (int i = 0; i < dirIds->size(); i++) {
            if (!IsDirectoryEmpty(dirIds->at(i))) {
                throw Exception("DB directory not empty!");
            }
        }
        std::stringstream sql;
        sql << "DELETE FROM instr_dirs WHERE parent_dir_id=" << DirId;
        sql << " AND dir_id!=0";

        ExecSql(sql.str());
    }

    bool InstrumentsDb::IsDirectoryEmpty(int DirId) {
        dmsg(2,("InstrumentsDb: IsDirectoryEmpty(DirId=%d)\n", DirId));
        int dirCount = GetDirectoryCount(DirId);
        int instrCount = GetInstrumentCount(DirId);
        dmsg(3,("InstrumentsDb: IsDirectoryEmpty: dirCount=%d,instrCount=%d\n", dirCount, instrCount));
        if (dirCount == -1 || instrCount == -1) return false;
        return dirCount == 0 && instrCount == 0;
    }

    bool InstrumentsDb::DirectoryExist(String Dir) {
        dmsg(2,("InstrumentsDb: DirectoryExist(Dir=%s)\n", Dir.c_str()));
        {
            LockGuard lock(DbInstrumentsMutex);
            return GetDirectoryId(Dir) != -1;
        }
    }

    DbDirectory InstrumentsDb::GetDirectoryInfo(String Dir) {
        dmsg(2,("InstrumentsDb: GetDirectoryInfo(Dir=%s)\n", Dir.c_str()));
        DbDirectory d;

        BeginTransaction();

        try {
            int id = GetDirectoryId(Dir);
            if(id == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));

            sqlite3_stmt *pStmt = NULL;
            std::stringstream sql;
            sql << "SELECT created,modified,description FROM instr_dirs ";
            sql << "WHERE dir_id=" << id;

            int res = sqlite3_prepare(GetDb(), sql.str().c_str(), -1, &pStmt, NULL);
            if (res != SQLITE_OK) {
                throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
            }

            res = sqlite3_step(pStmt);
            if(res == SQLITE_ROW) {
                d.Created = ToString(sqlite3_column_text(pStmt, 0));
                d.Modified = ToString(sqlite3_column_text(pStmt, 1));
                d.Description = ToString(sqlite3_column_text(pStmt, 2));
            } else {
                sqlite3_finalize(pStmt);

                if (res != SQLITE_DONE) {
                    throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
                } else {
                    throw Exception("Unknown DB directory: " + toEscapedPath(Dir));
                }
            }
            
            sqlite3_finalize(pStmt);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }

        EndTransaction();
        return d;
    }

    void InstrumentsDb::RenameDirectory(String Dir, String Name) {
        dmsg(2,("InstrumentsDb: RenameDirectory(Dir=%s,Name=%s)\n", Dir.c_str(), Name.c_str()));
        CheckFileName(Name);
        String dbName = toDbName(Name);

        BeginTransaction();
        try {
            int dirId = GetDirectoryId(Dir);
            if (dirId == -1) throw Exception("Unknown DB directory: " + toEscapedText(Dir));

            std::stringstream sql;
            sql << "SELECT parent_dir_id FROM instr_dirs WHERE dir_id=" <<  dirId;

            int parent = ExecSqlInt(sql.str());
            if (parent == -1) throw Exception("Unknown parent directory: " + toEscapedPath(Dir));

            if (GetDirectoryId(parent, dbName) != -1) {
                String s = toEscapedPath(Name);
                throw Exception("Cannot rename. Directory with that name already exists: " + s);
            }

            if (GetInstrumentId(parent, dbName) != -1) {
                throw Exception("Cannot rename. Instrument with that name exist: " + toEscapedPath(Dir));
            }

            sql.str("");
            sql << "UPDATE instr_dirs SET dir_name=? WHERE dir_id=" << dirId;
            ExecSql(sql.str(), dbName);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }

        EndTransaction();
        FireDirectoryNameChanged(Dir, toAbstractName(Name));
    }

    void InstrumentsDb::MoveDirectory(String Dir, String Dst) {
        dmsg(2,("InstrumentsDb: MoveDirectory(Dir=%s,Dst=%s)\n", Dir.c_str(), Dst.c_str()));

        if(Dir.compare("/") == 0) throw Exception("Cannot move the root directory");
        String ParentDir = GetParentDirectory(Dir);
        if(ParentDir.empty()) throw Exception("Unknown parent directory");

        BeginTransaction();
        try {
            int dirId = GetDirectoryId(Dir);
            if (dirId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));
            int dstId = GetDirectoryId(Dst);
            if (dstId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dst));
            if (dirId == dstId) {
                throw Exception("Cannot move directory to itself");
            }

            if (Dir.at(Dir.length() - 1) != '/') Dir.append("/");
            if (Dst.length() > Dir.length()) {
                if (Dir.compare(Dst.substr(0, Dir.length())) == 0) {
                    throw Exception("Cannot move a directory to a subdirectory of itself.");
                }
            }
            
            Dir.erase(Dir.length() - 1);
            String dirName = GetFileName(Dir);

            int id2 = GetDirectoryId(dstId, dirName);
            if (id2 != -1) throw Exception("DB directory already exist: " + toEscapedPath(dirName));
            id2 = GetInstrumentId(dstId, dirName);
            if (id2 != -1) throw Exception("Instrument with that name exist: " + toEscapedPath(dirName));

            std::stringstream sql;
            sql << "UPDATE instr_dirs SET parent_dir_id=" << dstId;
            sql << " WHERE dir_id=" << dirId;
            ExecSql(sql.str());
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }

        EndTransaction();
        FireDirectoryCountChanged(ParentDir);
        FireDirectoryCountChanged(Dst);
    }

    void InstrumentsDb::CopyDirectory(String Dir, String Dst) {
        dmsg(2,("InstrumentsDb: CopyDirectory(Dir=%s,Dst=%s)\n", Dir.c_str(), Dst.c_str()));

        if(Dir.compare("/") == 0) throw Exception("Cannot copy the root directory");
        String ParentDir = GetParentDirectory(Dir);
        if(ParentDir.empty()) throw Exception("Unknown parent directory");

        BeginTransaction();
        try {
            int dirId = GetDirectoryId(Dir);
            if (dirId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));
            int dstId = GetDirectoryId(Dst);
            if (dstId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dst));
            if (dirId == dstId) {
                throw Exception("Cannot copy directory to itself");
            }

            if (Dir.at(Dir.length() - 1) != '/') Dir.append("/");
            if (Dst.length() > Dir.length()) {
                if (Dir.compare(Dst.substr(0, Dir.length())) == 0) {
                    throw Exception("Cannot copy a directory to a subdirectory of itself.");
                }
            }
            
            Dir.erase(Dir.length() - 1);
            String dirName = GetFileName(Dir);

            int id2 = GetDirectoryId(dstId, dirName);
            if (id2 != -1) throw Exception("DB directory already exist: " + toEscapedPath(dirName));
            id2 = GetInstrumentId(dstId, dirName);
            if (id2 != -1) throw Exception("Instrument with that name exist: " + toEscapedPath(dirName));

            DirectoryCopier directoryCopier(ParentDir, Dst);
            DirectoryTreeWalk(Dir, &directoryCopier);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }

        EndTransaction();
    }

    void InstrumentsDb::SetDirectoryDescription(String Dir, String Desc) {
        dmsg(2,("InstrumentsDb: SetDirectoryDescription(Dir=%s,Desc=%s)\n", Dir.c_str(), Desc.c_str()));
        
        BeginTransaction();
        try {
            int id = GetDirectoryId(Dir);
            if(id == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));

            std::stringstream sql;
            sql << "UPDATE instr_dirs SET description=?,modified=CURRENT_TIMESTAMP ";
            sql << "WHERE dir_id="<< id;
        
            ExecSql(sql.str(), Desc);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        
        FireDirectoryInfoChanged(Dir);
    }

    int InstrumentsDb::AddInstruments(ScanMode Mode, String DbDir, String FsDir, bool bBackground, bool insDir) {
        dmsg(2,("InstrumentsDb: AddInstruments(Mode=%d,DbDir=%s,FsDir=%s,bBackground=%d,insDir=%d)\n", Mode, DbDir.c_str(), FsDir.c_str(), bBackground, insDir));
        if(!bBackground) {
            switch (Mode) {
                case NON_RECURSIVE:
                    AddInstrumentsNonrecursive(DbDir, FsDir, insDir);
                    break;
                case RECURSIVE:
                    AddInstrumentsRecursive(DbDir, FsDir, false, insDir);
                    break;
                case FLAT:
                    AddInstrumentsRecursive(DbDir, FsDir, true, insDir);
                    break;
                default:
                    throw Exception("Unknown scan mode");
            }

            return -1;
        }

        ScanJob job;
        int jobId = Jobs.AddJob(job);
        InstrumentsDbThread.Execute(new AddInstrumentsJob(jobId, Mode, DbDir, FsDir, insDir));

        return jobId;
    }
    
    int InstrumentsDb::AddInstruments(String DbDir, String FilePath, int Index, bool bBackground) {
        dmsg(2,("InstrumentsDb: AddInstruments(DbDir=%s,FilePath=%s,Index=%d,bBackground=%d)\n", DbDir.c_str(), FilePath.c_str(), Index, bBackground));
        if(!bBackground) {
            AddInstruments(DbDir, false, FilePath, Index);
            return -1;
        }

        ScanJob job;
        int jobId = Jobs.AddJob(job);
        InstrumentsDbThread.Execute(new AddInstrumentsFromFileJob(jobId, DbDir, FilePath, Index, false));

        return jobId;
    } 

    void InstrumentsDb::AddInstruments(String DbDir, bool insDir, String FilePath, int Index, ScanProgress* pProgress) {
        dmsg(2,("InstrumentsDb: AddInstruments(DbDir=%s,insDir=%d,FilePath=%s,Index=%d)\n", DbDir.c_str(), insDir, FilePath.c_str(), Index));
        if (DbDir.empty() || FilePath.empty()) return;
        
        {
            LockGuard lock(DbInstrumentsMutex);

            int dirId = GetDirectoryId(DbDir);
            if (dirId == -1) throw Exception("Invalid DB directory: " + toEscapedText(DbDir));

            File f = File(FilePath);
            if (!f.Exist()) {
                std::stringstream ss;
                ss << "Fail to stat `" << FilePath << "`: " << f.GetErrorMsg();
                throw Exception(ss.str());
            }

            if (!f.IsFile()) {
                std::stringstream ss;
                ss << "`" << FilePath << "` is not an instrument file";
                throw Exception(ss.str());
            }

            String dir = insDir ? PrepareSubdirectory(DbDir, FilePath) : DbDir;
            AddInstrumentsFromFile(dir, FilePath, Index, pProgress);
        }
    }

    void InstrumentsDb::AddInstrumentsNonrecursive(String DbDir, String FsDir, bool insDir, ScanProgress* pProgress) {
        dmsg(2,("InstrumentsDb: AddInstrumentsNonrecursive(DbDir=%s,FsDir=%s,insDir=%d)\n", DbDir.c_str(), FsDir.c_str(), insDir));
        if (DbDir.empty() || FsDir.empty()) return;
        
        {
            LockGuard lock(DbInstrumentsMutex);

            int dirId = GetDirectoryId(DbDir);
            if (dirId == -1) throw Exception("Invalid DB directory: " + toEscapedPath(DbDir));

            File f = File(FsDir);
            if (!f.Exist()) {
                std::stringstream ss;
                ss << "Fail to stat `" << FsDir << "`: " << f.GetErrorMsg();
                throw Exception(ss.str());
            }

            if (!f.IsDirectory()) {
                throw Exception("Directory expected: " + FsDir);
            }
            
            if (FsDir.at(FsDir.length() - 1) != File::DirSeparator) {
                FsDir.push_back(File::DirSeparator);
            }
            
            try {
                FileListPtr fileList = File::GetFiles(FsDir);
                for (int i = 0; i < fileList->size(); i++) {
                    String dir = insDir ? PrepareSubdirectory(DbDir, fileList->at(i)) : DbDir;
					AddInstrumentsFromFile(dir, FsDir + fileList->at(i), -1, pProgress);
                }
            } catch(Exception e) {
                e.PrintMessage();
            }
        }
    }

    void InstrumentsDb::AddInstrumentsRecursive(String DbDir, String FsDir, bool Flat, bool insDir, ScanProgress* pProgress) {
        dmsg(2,("InstrumentsDb: AddInstrumentsRecursive(DbDir=%s,FsDir=%s,Flat=%d,insDir=%d)\n", DbDir.c_str(), FsDir.c_str(), Flat, insDir));
        if (pProgress != NULL) {
            InstrumentFileCounter c;
            pProgress->SetTotalFileCount(c.Count(FsDir));
        }

        DirectoryScanner d;
        d.Scan(DbDir, FsDir, Flat, insDir, pProgress);
    }

    int InstrumentsDb::GetInstrumentCount(int DirId) {
        dmsg(2,("InstrumentsDb: GetInstrumentCount(DirId=%d)\n", DirId));
        if(DirId == -1) return -1;
        
        std::stringstream sql;
        sql << "SELECT COUNT(*) FROM instruments WHERE dir_id=" <<  DirId;
        
        return ExecSqlInt(sql.str());
    }

    int InstrumentsDb::GetInstrumentCount(String Dir, bool Recursive) {
        dmsg(2,("InstrumentsDb: GetInstrumentCount(Dir=%s,Recursive=%d)\n", Dir.c_str(), Recursive));
        int i;
        
        BeginTransaction();
        try {
            if (Recursive) {
                InstrumentCounter instrumentCounter;
                DirectoryTreeWalk(Dir, &instrumentCounter);
                i = instrumentCounter.GetInstrumentCount();
            } else {
                i = GetInstrumentCount(GetDirectoryId(Dir));
            }
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();

        if (i == -1) throw Exception("Unknown Db directory: " + toEscapedPath(Dir));
        return i;
    }

    IntListPtr InstrumentsDb::GetInstrumentIDs(int DirId) {
        std::stringstream sql;
        sql << "SELECT instr_id FROM instruments WHERE dir_id=" << DirId;

        return ExecSqlIntList(sql.str());
    }

    StringListPtr InstrumentsDb::GetInstruments(String Dir, bool Recursive) {
        dmsg(2,("InstrumentsDb: GetInstruments(Dir=%s,Recursive=%d)\n", Dir.c_str(), Recursive));
        BeginTransaction();
        try {
            int dirId = GetDirectoryId(Dir);
            if(dirId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));

            StringListPtr pInstrs;

            if(Recursive) {
                SearchQuery q;
                InstrumentFinder instrumentFinder(&q);
                DirectoryTreeWalk(Dir, &instrumentFinder);
                pInstrs = instrumentFinder.GetInstruments();
            } else {
                std::stringstream sql;
                sql << "SELECT instr_name FROM instruments WHERE dir_id=" << dirId;

                pInstrs = ExecSqlStringList(sql.str());
                // Converting to abstract names
                for (int i = 0; i < pInstrs->size(); i++) {
                    for (int j = 0; j < pInstrs->at(i).length(); j++) {
                        if (pInstrs->at(i).at(j) == '/') pInstrs->at(i).at(j) = '\0';
                    }
                }
            }
            EndTransaction();
            return pInstrs;
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
    }

    int InstrumentsDb::GetInstrumentId(String Instr) {
        dmsg(2,("InstrumentsDb: GetInstrumentId(Instr=%s)\n", Instr.c_str()));
        String Dir = GetDirectoryPath(Instr);
        if (Dir.empty()) return -1;

        return GetInstrumentId(GetDirectoryId(Dir), GetFileName(Instr));
    }

    int InstrumentsDb::GetInstrumentId(int DirId, String InstrName) {
        dmsg(2,("InstrumentsDb: GetInstrumentId(DirId=%d,InstrName=%s)\n", DirId, InstrName.c_str()));
        if (DirId == -1 || InstrName.empty()) return -1;
        
        std::stringstream sql;
        sql << "SELECT instr_id FROM instruments WHERE dir_id=";
        sql << DirId << " AND instr_name=?";
        return ExecSqlInt(sql.str(), toDbName(InstrName));
    }

    String InstrumentsDb::GetInstrumentName(int InstrId) {
        dmsg(2,("InstrumentsDb: GetInstrumentName(InstrId=%d)\n", InstrId));
        std::stringstream sql;
        sql << "SELECT instr_name FROM instruments WHERE instr_id=" << InstrId;
        return toAbstractName(ExecSqlString(sql.str()));
    }
    
    void InstrumentsDb::RemoveInstrument(String Instr) {
        dmsg(2,("InstrumentsDb: RemoveInstrument(Instr=%s)\n", Instr.c_str()));
        String ParentDir = GetDirectoryPath(Instr);
        if(ParentDir.empty()) throw Exception("Unknown parent directory");
        
        BeginTransaction();
        try {
            int instrId = GetInstrumentId(Instr);
            if(instrId == -1) {
                throw Exception("The specified instrument does not exist: " + toEscapedPath(Instr));
            }
            RemoveInstrument(instrId);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        FireInstrumentCountChanged(ParentDir);
    }

    void InstrumentsDb::RemoveInstrument(int InstrId) {
        dmsg(2,("InstrumentsDb: RemoveInstrument(InstrId=%d)\n", InstrId));

        std::stringstream sql;
        sql << "DELETE FROM instruments WHERE instr_id=" << InstrId;

        ExecSql(sql.str());
    }

    void InstrumentsDb::RemoveAllInstruments(int DirId) {
        dmsg(2,("InstrumentsDb: RemoveAllInstruments(DirId=%d)\n", DirId));

        std::stringstream sql;
        sql << "DELETE FROM instruments WHERE dir_id=" << DirId;
        ExecSql(sql.str());
    }

    DbInstrument InstrumentsDb::GetInstrumentInfo(String Instr) {
        dmsg(2,("InstrumentsDb: GetInstrumentInfo(Instr=%s)\n", Instr.c_str()));
        DbInstrument i;
        
        BeginTransaction();
        try {
            int id = GetInstrumentId(Instr);
            if(id == -1) throw Exception("Unknown DB instrument: " + toEscapedPath(Instr));
            i = GetInstrumentInfo(id);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        
        return i;
    }

    DbInstrument InstrumentsDb::GetInstrumentInfo(int InstrId) {
        sqlite3_stmt *pStmt = NULL;
        std::stringstream sql;
        sql << "SELECT instr_file,instr_nr,format_family,format_version,";
        sql << "instr_size,created,modified,description,is_drum,product,";
        sql << "artists,keywords FROM instruments WHERE instr_id=" << InstrId;

        int res = sqlite3_prepare(GetDb(), sql.str().c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        DbInstrument i;
        res = sqlite3_step(pStmt);
        if(res == SQLITE_ROW) {
            i.InstrFile = ToString(sqlite3_column_text(pStmt, 0));
            i.InstrNr = sqlite3_column_int(pStmt, 1);
            i.FormatFamily = ToString(sqlite3_column_text(pStmt, 2));
            i.FormatVersion = ToString(sqlite3_column_text(pStmt, 3));
            i.Size = sqlite3_column_int64(pStmt, 4);
            i.Created = ToString(sqlite3_column_text(pStmt, 5));
            i.Modified = ToString(sqlite3_column_text(pStmt, 6));
            i.Description = ToString(sqlite3_column_text(pStmt, 7));
            i.IsDrum = sqlite3_column_int(pStmt, 8);
            i.Product = ToString(sqlite3_column_text(pStmt, 9));
            i.Artists = ToString(sqlite3_column_text(pStmt, 10));
            i.Keywords = ToString(sqlite3_column_text(pStmt, 11));
        } else {
            sqlite3_finalize(pStmt);

            if (res != SQLITE_DONE) {
                throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
            } else {
                throw Exception("Unknown DB instrument");
            }
        }

        sqlite3_finalize(pStmt);
        return i;
    }

    void InstrumentsDb::RenameInstrument(String Instr, String Name) {
        dmsg(2,("InstrumentsDb: RenameInstrument(Instr=%s,Name=%s)\n", Instr.c_str(), Name.c_str()));
        CheckFileName(Name);

        BeginTransaction();
        try {
            int dirId = GetDirectoryId(GetDirectoryPath(Instr));
            if (dirId == -1) throw Exception("Unknown DB instrument: " + toEscapedPath(Instr));

            int instrId = GetInstrumentId(dirId, GetFileName(Instr));
            if (instrId == -1) throw Exception("Unknown DB instrument: " + toEscapedPath(Instr));

            if (GetInstrumentId(dirId, Name) != -1) {
                String s = toEscapedPath(Name);
                throw Exception("Cannot rename. Instrument with that name already exists: " + s);
            }

            if (GetDirectoryId(dirId, Name) != -1) {
                String s = toEscapedPath(Name);
                throw Exception("Cannot rename. Directory with that name already exists: " + s);
            }

            std::stringstream sql;
            sql << "UPDATE instruments SET instr_name=? WHERE instr_id=" << instrId;
            ExecSql(sql.str(), toDbName(Name));
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        FireInstrumentNameChanged(Instr, toAbstractName(Name));
    }

    void InstrumentsDb::MoveInstrument(String Instr, String Dst) {
        dmsg(2,("InstrumentsDb: MoveInstrument(Instr=%s,Dst=%s)\n", Instr.c_str(), Dst.c_str()));
        String ParentDir = GetDirectoryPath(Instr);
        if(ParentDir.empty()) throw Exception("Unknown parent directory");

        BeginTransaction();
        try {
            int dirId = GetDirectoryId(ParentDir);
            if (dirId == -1) throw Exception("Unknown DB instrument: " + toEscapedPath(Instr));

            String instrName = GetFileName(Instr);
            int instrId = GetInstrumentId(dirId, instrName);
            if (instrId == -1) throw Exception("Unknown DB instrument: " + toEscapedPath(Instr));

            int dstId = GetDirectoryId(Dst);
            if (dstId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dst));
            if (dirId == dstId) {
                EndTransaction();
                return;
            }

            if (GetInstrumentId(dstId, instrName) != -1) {
                String s = toEscapedPath(instrName);
                throw Exception("Cannot move. Instrument with that name already exists: " + s);
            }

            if (GetDirectoryId(dstId, instrName) != -1) {
                String s = toEscapedPath(instrName);
                throw Exception("Cannot move. Directory with that name already exists: " + s);
            }

            std::stringstream sql;
            sql << "UPDATE instruments SET dir_id=" << dstId;
            sql << " WHERE instr_id=" << instrId;
            ExecSql(sql.str());
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        FireInstrumentCountChanged(ParentDir);
        FireInstrumentCountChanged(Dst);
    }

    void InstrumentsDb::CopyInstrument(String Instr, String Dst) {
        dmsg(2,("InstrumentsDb: CopyInstrument(Instr=%s,Dst=%s)\n", Instr.c_str(), Dst.c_str()));
        String ParentDir = GetDirectoryPath(Instr);
        if(ParentDir.empty()) throw Exception("Unknown parent directory");

        BeginTransaction();
        try {
            int dirId = GetDirectoryId(GetDirectoryPath(Instr));
            if (dirId == -1) throw Exception("Unknown DB instrument: " + toEscapedPath(Instr));

            String instrName = GetFileName(Instr);
            int instrId = GetInstrumentId(dirId, instrName);
            if (instrId == -1) throw Exception("Unknown DB instrument: " + toEscapedPath(Instr));

            int dstId = GetDirectoryId(Dst);
            if (dstId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dst));
            if (dirId == dstId) {
                EndTransaction();
                return;
            }

            CopyInstrument(instrId, instrName, dstId, Dst);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        
    }

    void InstrumentsDb::CopyInstrument(int InstrId, String InstrName, int DstDirId, String DstDir) {
        if (GetInstrumentId(DstDirId, InstrName) != -1) {
            String s = toEscapedPath(InstrName);
            throw Exception("Cannot copy. Instrument with that name already exists: " + s);
        }

        if (GetDirectoryId(DstDirId, InstrName) != -1) {
            String s = toEscapedPath(InstrName);
            throw Exception("Cannot copy. Directory with that name already exists: " + s);
        }

        DbInstrument i = GetInstrumentInfo(InstrId);
        sqlite3_stmt *pStmt = NULL;
        std::stringstream sql;
        sql << "INSERT INTO instruments (dir_id,instr_name,instr_file,instr_nr,format_family,";
        sql << "format_version,instr_size,description,is_drum,product,artists,keywords) ";
        sql << "VALUES (" << DstDirId << ",?,?," << i.InstrNr << ",?,?," << i.Size << ",?,";
        sql << i.IsDrum << ",?,?,?)";

        int res = sqlite3_prepare(GetDb(), sql.str().c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        String s = toDbName(InstrName);
        BindTextParam(pStmt, 1, s);
        BindTextParam(pStmt, 2, i.InstrFile);
        BindTextParam(pStmt, 3, i.FormatFamily);
        BindTextParam(pStmt, 4, i.FormatVersion);
        BindTextParam(pStmt, 5, i.Description);
        BindTextParam(pStmt, 6, i.Product);
        BindTextParam(pStmt, 7, i.Artists);
        BindTextParam(pStmt, 8, i.Keywords);

        res = sqlite3_step(pStmt);
        if(res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }
        
        sqlite3_finalize(pStmt);
        FireInstrumentCountChanged(DstDir);
    }

    void InstrumentsDb::SetInstrumentDescription(String Instr, String Desc) {
        dmsg(2,("InstrumentsDb: SetInstrumentDescription(Instr=%s,Desc=%s)\n", Instr.c_str(), Desc.c_str()));

        BeginTransaction();
        try {
            int id = GetInstrumentId(Instr);
            if(id == -1) throw Exception("Unknown DB instrument: " + toEscapedPath(Instr));

            std::stringstream sql;
            sql << "UPDATE instruments SET description=?,modified=CURRENT_TIMESTAMP ";
            sql << "WHERE instr_id="<< id;

            ExecSql(sql.str(), Desc);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        FireInstrumentInfoChanged(Instr);
    }

    void InstrumentsDb::AddInstrumentsFromFile(String DbDir, String file, int Index, ScanProgress* pProgress) {
        dmsg(2,("InstrumentsDb: AddInstrumentsFromFile(DbDir=%s,File=%s,Index=%d)\n", DbDir.c_str(), file.c_str(), Index));

        if (!InstrumentFileInfo::isSupportedFile(file)) return;

        try {
            if (pProgress != NULL) {
                pProgress->SetStatus(0);
                pProgress->CurrentFile = file;
            }

            int dirId = GetDirectoryId(DbDir);
            if (dirId == -1) throw Exception("Invalid DB directory: " + toEscapedPath(DbDir));

            File f = File(file);
            if (!f.Exist()) {
                std::stringstream ss;
                ss << "Fail to stat `" << file << "`: " << f.GetErrorMsg();
                throw Exception(ss.str());
            }

            if (!f.IsFile()) {
                std::stringstream ss;
                ss << "`" << file << "` is not a regular file";
                throw Exception(ss.str());
            }

            AddInstrumentsFromFilePriv(DbDir, dirId, file, f, Index, pProgress);

            if (pProgress != NULL) {
                pProgress->SetScannedFileCount(pProgress->GetScannedFileCount() + 1);
            }
        } catch(Exception e) {
            e.PrintMessage();
        }
    }

    void InstrumentsDb::AddInstrumentsFromFilePriv(String DbDir, const int dirId, String FilePath, File file, int Index, ScanProgress* pProgress) {
        dmsg(2,("InstrumentsDb: AddGigInstruments(DbDir=%s,FilePath=%s,Index=%d)\n", DbDir.c_str(), FilePath.c_str(), Index));

        bool unlocked = false;
        InstrumentFileInfo* fileInfo = NULL;
        sqlite3_stmt* pStmt = NULL;
        try {
            fileInfo = InstrumentFileInfo::getFileInfoFor(FilePath);
            if (!fileInfo) return;

            std::stringstream sql;
            sql << "INSERT INTO instruments (dir_id,instr_name,instr_file,";
            sql << "instr_nr,format_family,format_version,instr_size,";
            sql << "description,is_drum,product,artists,keywords) VALUES (";
            sql << dirId << ",?,?,?,?,?," << file.GetSize() << ",?,?,?,?,?)";

            // instr_name 1
            // instr_file 2
            // instr_nr 3
            // format_family 4
            // format_version 5
            // description 6
            // is_drum 7
            // product 8
            // artists 9
            // keywords 10

            int res = sqlite3_prepare(GetDb(), sql.str().c_str(), -1, &pStmt, NULL);
            if (res != SQLITE_OK) {
                throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
            }

            BindTextParam(pStmt, 2, toEscapedFsPath(FilePath));
            BindTextParam(pStmt, 4, fileInfo->formatName());
            BindTextParam(pStmt, 5, fileInfo->formatVersion());

            int instrIndex = (Index == -1) ? 0 : Index;

            // Assume that it's locked and should be unlocked at this point
            // to be able to use the database from another threads
            if (!InTransaction) {
                DbInstrumentsMutex.Unlock();
                unlocked = true;
            } else {
                std::cerr << "Shouldn't be in transaction when adding instruments." << std::endl;
            }

            optional<InstrumentInfo> info = fileInfo->getInstrumentInfo(0, pProgress);
            if (!InTransaction) DbInstrumentsMutex.Lock();
            while (info) {
                String instrumentName = info->instrumentName;
                if (instrumentName.empty())
                    instrumentName = Path::getBaseName(FilePath);
                instrumentName = GetUniqueName(dirId, instrumentName);

                BindTextParam(pStmt, 8, info->product);
                BindTextParam(pStmt, 9, info->artists);
                BindTextParam(pStmt, 10, info->keywords);

                std::stringstream sql2;
                sql2 << "SELECT COUNT(*) FROM instruments WHERE instr_file=? AND ";
                sql2 << "instr_nr=" << instrIndex;
                String s = toEscapedFsPath(FilePath);
                if (ExecSqlInt(sql2.str(), s) > 0) goto next;

                BindTextParam(pStmt, 1, instrumentName);
                BindIntParam(pStmt, 3, instrIndex);

                BindTextParam(pStmt, 6, info->comments);
                BindIntParam(pStmt, 7, info->isDrum);

                res = sqlite3_step(pStmt);
                if (res != SQLITE_DONE) {
                    throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
                }
                res = sqlite3_reset(pStmt);
                FireInstrumentCountChanged(DbDir);

            next:
                if (Index != -1) break;

                instrIndex++;
                info = fileInfo->getInstrumentInfo(instrIndex, pProgress);
            }
        } catch (Exception e) {
            if (pStmt) sqlite3_finalize(pStmt);
            if (fileInfo) delete fileInfo;
            if (unlocked) DbInstrumentsMutex.Lock();
            std::stringstream ss;
            ss << "Failed to scan `" << FilePath << "`: " << e.Message();
            throw Exception(ss.str());
        } catch (...) {
            if (pStmt) sqlite3_finalize(pStmt);
            if (fileInfo) delete fileInfo;
            if (unlocked) DbInstrumentsMutex.Lock();
            throw Exception("Failed to scan `" + FilePath + "`");
        }
        if (pStmt) sqlite3_finalize(pStmt);
        if (fileInfo) delete fileInfo;
        if (unlocked) DbInstrumentsMutex.Lock();
    }

    void InstrumentsDb::DirectoryTreeWalk(String AbstractPath, DirectoryHandler* pHandler) {
        int DirId = GetDirectoryId(AbstractPath);
        if(DirId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(AbstractPath));
        DirectoryTreeWalk(pHandler, AbstractPath, DirId, 0);
    }

    void InstrumentsDb::DirectoryTreeWalk(DirectoryHandler* pHandler, String AbstractPath, int DirId, int Level) {
        if(Level == 1000) throw Exception("Possible infinite loop detected");
        pHandler->ProcessDirectory(AbstractPath, DirId);
        
        String s;
        StringListPtr pDirs = GetDirectories(DirId);
        for(int i = 0; i < pDirs->size(); i++) {
            if (AbstractPath.length() == 1 && AbstractPath.at(0) == '/') {
                s = "/" + pDirs->at(i);
            } else {
                s = AbstractPath + "/" + pDirs->at(i);
            }
            DirectoryTreeWalk(pHandler, s, GetDirectoryId(DirId, pDirs->at(i)), Level + 1);
        }
    }

    StringListPtr InstrumentsDb::FindDirectories(String Dir, SearchQuery* pQuery, bool Recursive) {
        dmsg(2,("InstrumentsDb: FindDirectories(Dir=%s)\n", Dir.c_str()));
        DirectoryFinder directoryFinder(pQuery);
        
        BeginTransaction();
        try {
            int DirId = GetDirectoryId(Dir);
            if(DirId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));

            if (Recursive) DirectoryTreeWalk(Dir, &directoryFinder);
            else directoryFinder.ProcessDirectory(Dir, DirId);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();

        return directoryFinder.GetDirectories();
    }

    StringListPtr InstrumentsDb::FindInstruments(String Dir, SearchQuery* pQuery, bool Recursive) {
        dmsg(2,("InstrumentsDb: FindInstruments(Dir=%s)\n", Dir.c_str()));
        InstrumentFinder instrumentFinder(pQuery);
        
        BeginTransaction();
        try {
            int DirId = GetDirectoryId(Dir);
            if(DirId == -1) throw Exception("Unknown DB directory: " + toEscapedPath(Dir));

            if (Recursive) DirectoryTreeWalk(Dir, &instrumentFinder);
            else instrumentFinder.ProcessDirectory(Dir, DirId);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();

        return instrumentFinder.GetInstruments();
    }
    
    StringListPtr InstrumentsDb::FindLostInstrumentFiles() {
        dmsg(2,("InstrumentsDb: FindLostInstrumentFiles()\n"));

        BeginTransaction();
        try {
            StringListPtr files = ExecSqlStringList("SELECT DISTINCT instr_file FROM instruments");
            StringListPtr result(new std::vector<String>);
            for (int i = 0; i < files->size(); i++) {
                File f(toNonEscapedFsPath(files->at(i)));
                if (!f.Exist()) result->push_back(files->at(i));
            }
            EndTransaction();
            return result;
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
    }
    
    void InstrumentsDb::SetInstrumentFilePath(String OldPath, String NewPath) {
        if (OldPath == NewPath) return;
        StringListPtr instrs;
        BeginTransaction();
        try {
            std::vector<String> params(2);
            params[0] = toEscapedFsPath(NewPath);
            params[1] = toEscapedFsPath(OldPath);
            instrs = GetInstrumentsByFile(OldPath);
            ExecSql("UPDATE instruments SET instr_file=? WHERE instr_file=?", params);
        } catch (Exception e) {
            EndTransaction();
            throw e;
        }
        EndTransaction();
        
        for (int i = 0; i < instrs->size(); i++) {
            FireInstrumentInfoChanged(instrs->at(i));
        }
    }

    void InstrumentsDb::BeginTransaction() {
        dmsg(2,("InstrumentsDb: BeginTransaction(InTransaction=%d)\n", InTransaction));
        DbInstrumentsMutex.Lock();
        if (InTransaction) return;
        
        if(db == NULL) return;
        sqlite3_stmt *pStmt = NULL;
        
        InTransaction = true;
        int res = sqlite3_prepare(db, "BEGIN TRANSACTION", -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            std::cerr << ToString(sqlite3_errmsg(db)) << std::endl;
            return;
        }
        
        res = sqlite3_step(pStmt);
        if(res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            std::cerr << ToString(sqlite3_errmsg(db)) << std::endl;
            return;
        }

        sqlite3_finalize(pStmt);
    }

    void InstrumentsDb::EndTransaction() {
        dmsg(2,("InstrumentsDb: EndTransaction(InTransaction=%d)\n", InTransaction));
        if (!InTransaction) {
            DbInstrumentsMutex.Unlock();
            return;
        }
        InTransaction = false;
        
        if (db == NULL) {
            DbInstrumentsMutex.Unlock();
            return;
        }
        sqlite3_stmt *pStmt = NULL;
        
        int res = sqlite3_prepare(db, "END TRANSACTION", -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            std::cerr << ToString(sqlite3_errmsg(db)) << std::endl;
            DbInstrumentsMutex.Unlock();
            return;
        }
        
        res = sqlite3_step(pStmt);
        if(res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            std::cerr << ToString(sqlite3_errmsg(db)) << std::endl;
            DbInstrumentsMutex.Unlock();
            return;
        }

        sqlite3_finalize(pStmt);
        DbInstrumentsMutex.Unlock();
    }

    void InstrumentsDb::ExecSql(String Sql) {
        dmsg(2,("InstrumentsDb: ExecSql(Sql=%s)\n", Sql.c_str()));
        std::vector<String> Params;
        ExecSql(Sql, Params);
    }

    void InstrumentsDb::ExecSql(String Sql, String Param) {
        dmsg(2,("InstrumentsDb: ExecSql(Sql=%s,Param=%s)\n", Sql.c_str(), Param.c_str()));
        std::vector<String> Params;
        Params.push_back(Param);
        ExecSql(Sql, Params);
    }

    void InstrumentsDb::ExecSql(String Sql, std::vector<String>& Params) {
        dmsg(2,("InstrumentsDb: ExecSql(Sql=%s,Params)\n", Sql.c_str()));
        sqlite3_stmt *pStmt = NULL;
        
        int res = sqlite3_prepare(GetDb(), Sql.c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        for(int i = 0; i < Params.size(); i++) {
            BindTextParam(pStmt, i + 1, Params[i]);
        }

        res = sqlite3_step(pStmt);
        if (res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        sqlite3_finalize(pStmt);
    }

    int InstrumentsDb::ExecSqlInt(String Sql) {
        dmsg(2,("InstrumentsDb: ExecSqlInt(Sql=%s)\n", Sql.c_str()));
        sqlite3_stmt *pStmt = NULL;

        int res = sqlite3_prepare(GetDb(), Sql.c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }
        
        int i = -1;
        res = sqlite3_step(pStmt);
        if(res == SQLITE_ROW) {
            i = sqlite3_column_int(pStmt, 0);
        } else if (res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        sqlite3_finalize(pStmt);
        
        return i;
    }

    int InstrumentsDb::ExecSqlInt(String Sql, String Param) {
        dmsg(2,("InstrumentsDb: ExecSqlInt(Sql=%s,Param=%s)\n", Sql.c_str(), Param.c_str()));
        sqlite3_stmt *pStmt = NULL;

        int res = sqlite3_prepare(GetDb(), Sql.c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        BindTextParam(pStmt, 1, Param);

        int i = -1;
        res = sqlite3_step(pStmt);
        if(res == SQLITE_ROW) {
            i = sqlite3_column_int(pStmt, 0);
        } else if (res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        sqlite3_finalize(pStmt);
        return i;
    }

    String InstrumentsDb::ExecSqlString(String Sql) {
        dmsg(2,("InstrumentsDb: ExecSqlString(Sql=%s)\n", Sql.c_str()));
        sqlite3_stmt *pStmt = NULL;

        int res = sqlite3_prepare(GetDb(), Sql.c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }
        
        String s;
        res = sqlite3_step(pStmt);
        if(res == SQLITE_ROW) {
            s = ToString(sqlite3_column_text(pStmt, 0));
        } else if (res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        sqlite3_finalize(pStmt);
        
        return s;
    }

    IntListPtr InstrumentsDb::ExecSqlIntList(String Sql) {
        dmsg(2,("InstrumentsDb: ExecSqlIntList(Sql=%s)\n", Sql.c_str()));
        std::vector<String> Params;
        return ExecSqlIntList(Sql, Params);
    }

    IntListPtr InstrumentsDb::ExecSqlIntList(String Sql, String Param) {
        dmsg(2,("InstrumentsDb: ExecSqlIntList(Sql=%s,Param=%s)\n", Sql.c_str(), Param.c_str()));
        std::vector<String> Params;
        Params.push_back(Param);
        return ExecSqlIntList(Sql, Params);
    }

    IntListPtr InstrumentsDb::ExecSqlIntList(String Sql, std::vector<String>& Params) {
        dmsg(2,("InstrumentsDb: ExecSqlIntList(Sql=%s)\n", Sql.c_str()));
        IntListPtr intList(new std::vector<int>);
        
        sqlite3_stmt *pStmt = NULL;

        int res = sqlite3_prepare(GetDb(), Sql.c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }
        
        for(int i = 0; i < Params.size(); i++) {
            BindTextParam(pStmt, i + 1, Params[i]);
        }
        
        res = sqlite3_step(pStmt);
        while(res == SQLITE_ROW) {
            intList->push_back(sqlite3_column_int(pStmt, 0));
            res = sqlite3_step(pStmt);
        }
        
        if (res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        sqlite3_finalize(pStmt);
        
        return intList;
    }
    
    StringListPtr InstrumentsDb::ExecSqlStringList(String Sql) {
        dmsg(2,("InstrumentsDb: ExecSqlStringList(Sql=%s)\n", Sql.c_str()));
        StringListPtr stringList(new std::vector<String>);
        
        sqlite3_stmt *pStmt = NULL;

        int res = sqlite3_prepare(GetDb(), Sql.c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }
        
        res = sqlite3_step(pStmt);
        while(res == SQLITE_ROW) {
            stringList->push_back(ToString(sqlite3_column_text(pStmt, 0)));
            res = sqlite3_step(pStmt);
        }
        
        if (res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }

        sqlite3_finalize(pStmt);
        
        return stringList;
    }

    void InstrumentsDb::BindTextParam(sqlite3_stmt* pStmt, int Index, String Text) {
        if (pStmt == NULL) return;
        int res = sqlite3_bind_text(pStmt, Index, Text.c_str(), -1, SQLITE_TRANSIENT);
        if (res != SQLITE_OK) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }
    }

    void InstrumentsDb::BindIntParam(sqlite3_stmt* pStmt, int Index, int Param) {
        if (pStmt == NULL) return;
        int res = sqlite3_bind_int(pStmt, Index, Param);
        if (res != SQLITE_OK) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(db)));
        }
    }

#ifndef WIN32
    void InstrumentsDb::Regexp(sqlite3_context* pContext, int argc, sqlite3_value** ppValue) {
        if (argc != 2) return;

        String pattern = ToString(sqlite3_value_text(ppValue[0]));
        String str = ToString(sqlite3_value_text(ppValue[1]));

        if(!fnmatch(pattern.c_str(), str.c_str(), FNM_CASEFOLD)) {
            sqlite3_result_int(pContext, 1);
        }
    }
#endif

    String InstrumentsDb::GetDirectoryPath(String File) {
        if (File.empty()) return String("");
        if (File.at(0) != '/') String("");
        if (File.length() == 1) return File;
        if (File.at(File.length() - 1) == '/') return File.substr(0, File.length() - 1);
        int i = (int) File.rfind('/', File.length() - 1);
        if(i == std::string::npos) return String("");
        if(i == 0) return String("/");
        return File.substr(0, i);
    }

    String InstrumentsDb::GetFileName(String Path) {
        if (Path.length() < 2) return String("");
        if (Path.at(0) != '/') String("");
        if (Path.at(Path.length() - 1) == '/') return String("");
        int i = (int) Path.rfind('/', Path.length() - 1);
        return Path.substr(i + 1);
    }

    void InstrumentsDb::CheckPathName(String Path) {
        if (Path.empty()) return;

        int i = 0, j = (int)Path.find('/', i);

        while(j != std::string::npos) {
            if (j + 1 >= Path.length()) return;
            if (Path.at(j + 1) == '/') throw Exception("Invalid path name: " + Path);
            
            i = j + 1;
            j = (int) Path.find('/', i);
        }
    }
    
    String InstrumentsDb::GetParentDirectory(String Dir) {
        if (Dir.length() < 2) return String("");
        if (Dir.at(0) != '/') String("");
        int i = (int) Dir.rfind('/', Dir.length() - 2);
        if (i == 0) return "/";
        return Dir.substr(0, i);
    }

    void InstrumentsDb::Format() {
        {
            LockGuard lock(DbInstrumentsMutex);

            if (db != NULL) {
                sqlite3_close(db);
                db = NULL;
            }

            if (DbFile.empty()) DbFile = GetDefaultDBLocation();
            String bkp = DbFile + ".bkp";
            remove(bkp.c_str());
            if (rename(DbFile.c_str(), bkp.c_str()) && errno != ENOENT) {
                throw Exception(String("Failed to backup database: ") + strerror(errno));
            }
        
            String f = DbFile;
            DbFile = "";
            CreateInstrumentsDb(f);
        }
        FireDirectoryCountChanged("/");
        FireInstrumentCountChanged("/");
    }

    void InstrumentsDb::CheckFileName(String File) {
        if (File.empty()) throw Exception("Invalid file name: " + File);
    }

    String InstrumentsDb::GetUniqueName(int DirId, String Name) {
        dmsg(2,("InstrumentsDb: GetUniqueInstrumentName(DirId=%d,Name=%s)\n", DirId, Name.c_str()));

        if (GetInstrumentId(DirId, Name) == -1 && GetDirectoryId(DirId, Name) == -1) return Name;
        std::stringstream ss;
        for(int i = 2; i < 1001; i++) {
            ss.str("");
            ss << Name << '[' << i << ']';
            if (GetInstrumentId(DirId, ss.str()) == -1 && GetInstrumentId(DirId, ss.str()) == -1) {
                return ss.str();
            }
        }

        throw Exception("Unable to find an unique name: " + Name);
    }
    
    String InstrumentsDb::PrepareSubdirectory(String DbDir, String FsPath) {
        std::string dir = Path::getBaseName(FsPath);
        dir = toAbstractName(dir);
        if(dir.empty()) dir = "New Directory";
        dir = GetUniqueName(GetDirectoryId(DbDir), dir);
        dir = AppendNode(DbDir, dir);
        AddDirectory(dir);
        return dir;
    }

    String InstrumentsDb::AppendNode(String DbDir, String Node) {
        if(DbDir.length() == 1 && DbDir.at(0) == '/') return DbDir + Node;
        if(DbDir.at(DbDir.length() - 1) == '/') return DbDir + Node;
        return DbDir + "/" + Node;
    }

    String InstrumentsDb::toDbName(String AbstractName) {
        for (int i = 0; i < AbstractName.length(); i++) {
            if (AbstractName.at(i) == '\0') AbstractName.at(i) = '/';
        }
        return AbstractName;
    }

    String InstrumentsDb::toEscapedPath(String AbstractName) {
        for (int i = 0; i < AbstractName.length(); i++) {
            if (AbstractName.at(i) == '\0')      AbstractName.replace(i++, 1, "\\x2f");
            else if (AbstractName.at(i) == '\\') AbstractName.replace(i++, 1, "\\\\");
            else if (AbstractName.at(i) == '\'') AbstractName.replace(i++, 1, "\\'");
            else if (AbstractName.at(i) == '"')  AbstractName.replace(i++, 1, "\\\"");
            else if (AbstractName.at(i) == '\r') AbstractName.replace(i++, 1, "\\r");
            else if (AbstractName.at(i) == '\n') AbstractName.replace(i++, 1, "\\n");
        }
        return AbstractName;
    }
    
    String InstrumentsDb::toEscapedText(String text) {
        for (int i = 0; i < text.length(); i++) {
            if (text.at(i) == '\\')      text.replace(i++, 1, "\\\\");
            else if (text.at(i) == '\'') text.replace(i++, 1, "\\'");
            else if (text.at(i) == '"')  text.replace(i++, 1, "\\\"");
            else if (text.at(i) == '\r') text.replace(i++, 1, "\\r");
            else if (text.at(i) == '\n') text.replace(i++, 1, "\\n");
        }
        return text;
    }
    
    String InstrumentsDb::toNonEscapedText(String text) {
        String sb;
        for (int i = 0; i < text.length(); i++) {
            char c = text.at(i);
			if(c == '\\') {
				if(i >= text.length()) {
					std::cerr << "Broken escape sequence!" << std::endl;
					break;
				}
				char c2 = text.at(++i);
				if(c2 == '\'')      sb.push_back('\'');
				else if(c2 == '"')  sb.push_back('"');
				else if(c2 == '\\') sb.push_back('\\');
				else if(c2 == 'r')  sb.push_back('\r');
				else if(c2 == 'n')  sb.push_back('\n');
				else std::cerr << "Unknown escape sequence \\" << c2 << std::endl;
			} else {
				sb.push_back(c);
			}
        }
        return sb;
    }
    
    String InstrumentsDb::toEscapedFsPath(String FsPath) {
#ifdef WIN32
        replace(FsPath.begin(), FsPath.end(), '\\', '/');
#endif
        return toEscapedText(FsPath);
    }
    
    String InstrumentsDb::toNonEscapedFsPath(String FsPath) {
        FsPath = toNonEscapedText(FsPath);
#ifdef WIN32
        replace(FsPath.begin(), FsPath.end(), '/', '\\');
#endif
        return FsPath;
    }
    
    String InstrumentsDb::toAbstractName(String DbName) {
        for (int i = 0; i < DbName.length(); i++) {
            if (DbName.at(i) == '/') DbName.at(i) = '\0';
        }
        return DbName;
    }

    void InstrumentsDb::FireDirectoryCountChanged(String Dir) {
        for (int i = 0; i < llInstrumentsDbListeners.GetListenerCount(); i++) {
            llInstrumentsDbListeners.GetListener(i)->DirectoryCountChanged(Dir);
        }
    }

    void InstrumentsDb::FireDirectoryInfoChanged(String Dir) {
        for (int i = 0; i < llInstrumentsDbListeners.GetListenerCount(); i++) {
            llInstrumentsDbListeners.GetListener(i)->DirectoryInfoChanged(Dir);
        }
    }

    void InstrumentsDb::FireDirectoryNameChanged(String Dir, String NewName) {
        for (int i = 0; i < llInstrumentsDbListeners.GetListenerCount(); i++) {
            llInstrumentsDbListeners.GetListener(i)->DirectoryNameChanged(Dir, NewName);
        }
    }

    void InstrumentsDb::FireInstrumentCountChanged(String Dir) {
        for (int i = 0; i < llInstrumentsDbListeners.GetListenerCount(); i++) {
            llInstrumentsDbListeners.GetListener(i)->InstrumentCountChanged(Dir);
        }
    }

    void InstrumentsDb::FireInstrumentInfoChanged(String Instr) {
        for (int i = 0; i < llInstrumentsDbListeners.GetListenerCount(); i++) {
            llInstrumentsDbListeners.GetListener(i)->InstrumentInfoChanged(Instr);
        }
    }

    void InstrumentsDb::FireInstrumentNameChanged(String Instr, String NewName) {
        for (int i = 0; i < llInstrumentsDbListeners.GetListenerCount(); i++) {
            llInstrumentsDbListeners.GetListener(i)->InstrumentNameChanged(Instr, NewName);
        }
    }

    void InstrumentsDb::FireJobStatusChanged(int JobId) {
        for (int i = 0; i < llInstrumentsDbListeners.GetListenerCount(); i++) {
            llInstrumentsDbListeners.GetListener(i)->JobStatusChanged(JobId);
        }
    }

} // namespace LinuxSampler
