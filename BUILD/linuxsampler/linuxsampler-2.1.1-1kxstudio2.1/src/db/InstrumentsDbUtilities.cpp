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

#include "InstrumentsDbUtilities.h"

#include "../common/File.h"
#include "../common/global_private.h"

#include <algorithm>
#include <errno.h>

#include "../common/Exception.h"
#include "InstrumentsDb.h"
#include "../engines/sfz/sfz.h"
#if HAVE_SF2
# if AC_APPLE_UNIVERSAL_BUILD
#  include <libgig/SF.h>
# else
#  include <SF.h>
# endif
#endif // HAVE_SF2

namespace LinuxSampler {

    class GigFileInfo : public InstrumentFileInfo {
    public:
        GigFileInfo(String fileName) : InstrumentFileInfo(fileName) {
            m_gig  = NULL;
            m_riff = NULL;
            try {
                m_riff = new RIFF::File(fileName);
                m_gig  = new gig::File(m_riff);
                m_gig->SetAutoLoad(false); // avoid time consuming samples scanning
            } catch (RIFF::Exception e) {
                throw Exception(e.Message);
            } catch (...) {
                throw Exception("Unknown exception while accessing gig file");
            }
        }

        virtual ~GigFileInfo() {
            if (m_gig)  delete m_gig;
            if (m_riff) delete m_riff;
        }

        String formatName() OVERRIDE {
            return "GIG";
        }

        String formatVersion() OVERRIDE {
            return (m_gig->pVersion) ? ToString(m_gig->pVersion->major) : "";
        }

        optional<InstrumentInfo> getInstrumentInfo(int index, ScanProgress* pProgress) OVERRIDE {
            InstrumentInfo info;
            try {
                ::gig::progress_t* progress = (pProgress) ? &pProgress->GigFileProgress : NULL;
                ::gig::Instrument* pInstrument = m_gig->GetInstrument(index, progress);
                if (!pInstrument)
                    return optional<InstrumentInfo>::nothing;

                info.instrumentName = pInstrument->pInfo->Name;
                info.product = (!pInstrument->pInfo->Product.empty()) ? pInstrument->pInfo->Product : m_gig->pInfo->Product;
                info.artists = (!pInstrument->pInfo->Artists.empty()) ? pInstrument->pInfo->Artists : m_gig->pInfo->Artists;
                info.keywords = (!pInstrument->pInfo->Keywords.empty()) ? pInstrument->pInfo->Keywords : m_gig->pInfo->Keywords;
                info.comments = (!pInstrument->pInfo->Comments.empty()) ? pInstrument->pInfo->Comments : m_gig->pInfo->Comments;
                info.isDrum = pInstrument->IsDrum;
            } catch (RIFF::Exception e) {
                throw Exception(e.Message);
            } catch (...) {
                throw Exception("Unknown exception while accessing gig file");
            }
            return info;
        }
    private:
        ::RIFF::File* m_riff;
        ::gig::File*  m_gig;
    };

    class SFZFileInfo : public InstrumentFileInfo {
    public:
        SFZFileInfo(String fileName) : InstrumentFileInfo(fileName) {
            m_sfz = NULL;
            try {
                m_sfz = new ::sfz::File(fileName);
            } catch (sfz::Exception e) {
                throw Exception(e.Message());
            } catch (...) {
                throw Exception("Unknown exception while accessing sfz file");
            }
        }

        virtual ~SFZFileInfo() {
            if (m_sfz) delete m_sfz;
        }

        String formatName() OVERRIDE {
            return "SFZ";
        }

        String formatVersion() OVERRIDE {
            return "";
        }

        optional<InstrumentInfo> getInstrumentInfo(int index, ScanProgress* pProgress) OVERRIDE {
            if (index != 0)
                return optional<InstrumentInfo>::nothing;

            InstrumentInfo info;
            // yeah, lousy info, but SFZ does not provide any meta info unfortunately yet
            return info;
        }
    private:
        ::sfz::File* m_sfz;
    };

#if HAVE_SF2

    class Sf2FileInfo : public InstrumentFileInfo {
    public:
        Sf2FileInfo(String fileName) : InstrumentFileInfo(fileName) {
            m_sf2  = NULL;
            m_riff = NULL;
            try {
                m_riff = new RIFF::File(fileName);
                m_sf2  = new sf2::File(m_riff);
            } catch (RIFF::Exception e) {
                throw Exception(e.Message);
            } catch (...) {
                throw Exception("Unknown exception while accessing sf2 file");
            }
        }

        virtual ~Sf2FileInfo() {
            if (m_sf2)  delete m_sf2;
            if (m_riff) delete m_riff;
        }

        String formatName() OVERRIDE {
            return "SF2";
        }

        String formatVersion() OVERRIDE {
            if (!m_sf2->pInfo || !m_sf2->pInfo->pVer) return "";
            String major = ToString(m_sf2->pInfo->pVer->Major);
            //String minor = ToString(m_sf2->pInfo->pVer->Minor);
            //return major + "." + minor;
            return major;
        }

        optional<InstrumentInfo> getInstrumentInfo(int index, ScanProgress* pProgress) OVERRIDE {
            if (index >= m_sf2->GetPresetCount())
                return optional<InstrumentInfo>::nothing;

            InstrumentInfo info;
            try {
                ::sf2::Preset* preset = m_sf2->GetPreset(index);
                if (!preset)
                    return optional<InstrumentInfo>::nothing;

                info.instrumentName = preset->Name;
                if (m_sf2->pInfo) {
                    info.product = m_sf2->pInfo->Product;
                    info.comments = m_sf2->pInfo->Comments;
                    info.artists = m_sf2->pInfo->Engineers;
                }
            } catch (RIFF::Exception e) {
                throw Exception(e.Message);
            } catch (...) {
                throw Exception("Unknown exception while accessing gig file");
            }
            return info;
        }
    private:
        ::RIFF::File* m_riff;
        ::sf2::File*  m_sf2;
    };

#endif // #if HAVE_SF2

    void DbInstrument::Copy(const DbInstrument& Instr) {
        if (this == &Instr) return;

        InstrFile = Instr.InstrFile;
        InstrNr = Instr.InstrNr;
        FormatFamily = Instr.FormatFamily;
        FormatVersion = Instr.FormatVersion;
        Size = Instr.Size;
        Created = Instr.Created;
        Modified = Instr.Modified;
        Description = Instr.Description;
        IsDrum = Instr.IsDrum;
        Product = Instr.Product;
        Artists = Instr.Artists;
        Keywords = Instr.Keywords;
    }


    void DbDirectory::Copy(const DbDirectory& Dir) {
        if (this == &Dir) return;

        Created = Dir.Created;
        Modified = Dir.Modified;
        Description = Dir.Description;
    }

    SearchQuery::SearchQuery() {
        MinSize = -1;
        MaxSize = -1;
        InstrType = BOTH;
    }

    void SearchQuery::SetFormatFamilies(String s) {
        if (s.length() == 0) return;
        int i = 0;
        int j = (int) s.find(',', 0);
        
        while (j != std::string::npos) {
            FormatFamilies.push_back(s.substr(i, j - i));
            i = j + 1;
            j = (int) s.find(',', i);
        }
        
        if (i < s.length()) FormatFamilies.push_back(s.substr(i));
    }

    void SearchQuery::SetSize(String s) {
        String s2 = GetMin(s);
        if (s2.length() > 0) MinSize = atoll(s2.c_str());
        else MinSize = -1;
        
        s2 = GetMax(s);
        if (s2.length() > 0) MaxSize = atoll(s2.c_str());
        else MaxSize = -1;
    }

    void SearchQuery::SetCreated(String s) {
        CreatedAfter = GetMin(s);
        CreatedBefore = GetMax(s);
    }

    void SearchQuery::SetModified(String s) {
        ModifiedAfter = GetMin(s);
        ModifiedBefore = GetMax(s);
    }

    String SearchQuery::GetMin(String s) {
        if (s.length() < 3) return "";
        if (s.at(0) == '.' && s.at(1) == '.') return "";
        int i = (int) s.find("..");
        if (i == std::string::npos) return "";
        return s.substr(0, i);
    }

    String SearchQuery::GetMax(String s) {
        if (s.length() < 3) return "";
        if (s.find("..", s.length() - 2) != std::string::npos) return "";
        int i = (int) s.find("..");
        if (i == std::string::npos) return "";
        return s.substr(i + 2);
    }

    void ScanJob::Copy(const ScanJob& Job) {
        if (this == &Job) return;

        JobId = Job.JobId;
        FilesTotal = Job.FilesTotal;
        FilesScanned = Job.FilesScanned;
        Scanning = Job.Scanning;
        Status = Job.Status;
    }

    int JobList::AddJob(ScanJob Job) {
        if (Counter + 1 < Counter) Counter = 0;
        else Counter++;
        Job.JobId = Counter;
        Jobs.push_back(Job);
        if (Jobs.size() > 3) {
            std::vector<ScanJob>::iterator iter = Jobs.begin();
            Jobs.erase(iter);
        }
        return Job.JobId;
    }

    ScanJob& JobList::GetJobById(int JobId) {
        for (int i = 0; i < Jobs.size(); i++) {
            if (Jobs.at(i).JobId == JobId) return Jobs.at(i);
        }
        
        throw Exception("Invalid job ID: " + ToString(JobId));
    }
    
    bool AbstractFinder::IsRegex(String Pattern) {
        if(Pattern.find('?') != String::npos) return true;
        if(Pattern.find('*') != String::npos) return true;
        return false;
    }

    void AbstractFinder::AddSql(String Col, String Pattern, std::stringstream& Sql) {
        if (Pattern.length() == 0) return;

        if (IsRegex(Pattern)) {
#ifndef WIN32
            Sql << " AND " << Col << " regexp ?";
#else
            for (int i = 0; i < Pattern.length(); i++) {
                if (Pattern.at(i) == '?') Pattern.at(i) = '_';
                else if (Pattern.at(i) == '*') Pattern.at(i) = '%';
            }
            Sql << " AND " << Col << " LIKE ?";
#endif
            Params.push_back(Pattern);
            return;
        }

        String buf;
        std::vector<String> tokens;
        std::vector<String> tokens2;
        std::stringstream ss(Pattern);
        while (ss >> buf) tokens.push_back(buf);

        if (tokens.size() == 0) {
            Sql << " AND " << Col << " LIKE ?";
            Params.push_back("%" + Pattern + "%");
            return;
        }

        bool b = false;
        for (int i = 0; i < tokens.size(); i++) {
            Sql << (i == 0 ? " AND (" : "");

            for (int j = 0; j < tokens.at(i).length(); j++) {
                if (tokens.at(i).at(j) == '+') tokens.at(i).at(j) = ' ';
            }

            ss.clear();
            ss.str("");
            ss << tokens.at(i);

            tokens2.clear();
            while (ss >> buf) tokens2.push_back(buf);

            if (b && tokens2.size() > 0) Sql << " OR ";
            if (tokens2.size() > 1) Sql << "(";
            for (int j = 0; j < tokens2.size(); j++) {
                if (j != 0) Sql << " AND ";
                Sql << Col << " LIKE ?";
                Params.push_back("%" + tokens2.at(j) + "%");
                b = true;
            }
            if (tokens2.size() > 1) Sql << ")";
        }
        if (!b) Sql << "0)";
        else Sql << ")";
    }

    DirectoryFinder::DirectoryFinder(SearchQuery* pQuery) : pDirectories(new std::vector<String>) {
        pStmt = NULL;
        this->pQuery = pQuery;
        std::stringstream sql;
        sql << "SELECT dir_name from instr_dirs WHERE dir_id!=0 AND parent_dir_id=?";

        if (pQuery->CreatedAfter.length() != 0) {
            sql << " AND created > ?";
            Params.push_back(pQuery->CreatedAfter);
        }
        if (pQuery->CreatedBefore.length() != 0) {
            sql << " AND created < ?";
            Params.push_back(pQuery->CreatedBefore);
        }
        if (pQuery->ModifiedAfter.length() != 0) {
            sql << " AND modified > ?";
            Params.push_back(pQuery->ModifiedAfter);
        }
        if (pQuery->ModifiedBefore.length() != 0) {
            sql << " AND modified < ?";
            Params.push_back(pQuery->ModifiedBefore);
        }

        AddSql("dir_name", pQuery->Name, sql);
        AddSql("description", pQuery->Description, sql);
        SqlQuery = sql.str();

        InstrumentsDb* idb = InstrumentsDb::GetInstrumentsDb();

        int res = sqlite3_prepare(idb->GetDb(), SqlQuery.c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(idb->GetDb())));
        }

        for(int i = 0; i < Params.size(); i++) {
            idb->BindTextParam(pStmt, i + 2, Params.at(i));
        }
    }
    
    DirectoryFinder::~DirectoryFinder() {
        if (pStmt != NULL) sqlite3_finalize(pStmt);
    }

    StringListPtr DirectoryFinder::GetDirectories() {
#if __cplusplus >= 201103L && !CONFIG_NO_CPP11STL
        return std::move(pDirectories);
#else
        return pDirectories;
#endif
    }
    
    void DirectoryFinder::ProcessDirectory(String Path, int DirId) {
        InstrumentsDb* idb = InstrumentsDb::GetInstrumentsDb();
        idb->BindIntParam(pStmt, 1, DirId);

        String s = Path;
        if(Path.compare("/") != 0) s += "/";
        int res = sqlite3_step(pStmt);
        while(res == SQLITE_ROW) {
            pDirectories->push_back(s + idb->toAbstractName(ToString(sqlite3_column_text(pStmt, 0))));
            res = sqlite3_step(pStmt);
        }
        
        if (res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(idb->GetDb())));
        }

        res = sqlite3_reset(pStmt);
        if (res != SQLITE_OK) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(idb->GetDb())));
        }
    }

    InstrumentFinder::InstrumentFinder(SearchQuery* pQuery) : pInstruments(new std::vector<String>) {
        pStmt = NULL;
        this->pQuery = pQuery;
        std::stringstream sql;
        sql << "SELECT instr_name from instruments WHERE dir_id=?";

        if (pQuery->CreatedAfter.length() != 0) {
            sql << " AND created > ?";
            Params.push_back(pQuery->CreatedAfter);
        }
        if (pQuery->CreatedBefore.length() != 0) {
            sql << " AND created < ?";
            Params.push_back(pQuery->CreatedBefore);
        }
        if (pQuery->ModifiedAfter.length() != 0) {
            sql << " AND modified > ?";
            Params.push_back(pQuery->ModifiedAfter);
        }
        if (pQuery->ModifiedBefore.length() != 0) {
            sql << " AND modified < ?";
            Params.push_back(pQuery->ModifiedBefore);
        }
        if (pQuery->MinSize != -1) sql << " AND instr_size > " << pQuery->MinSize;
        if (pQuery->MaxSize != -1) sql << " AND instr_size < " << pQuery->MaxSize;

        if (pQuery->InstrType == SearchQuery::CHROMATIC) sql << " AND is_drum = 0";
        else if (pQuery->InstrType == SearchQuery::DRUM) sql << " AND is_drum != 0";

        if (pQuery->FormatFamilies.size() > 0) {
            sql << " AND (format_family=?";
            Params.push_back(pQuery->FormatFamilies.at(0));
            for (int i = 1; i < pQuery->FormatFamilies.size(); i++) {
                sql << "OR format_family=?";
                Params.push_back(pQuery->FormatFamilies.at(i));
            }
            sql << ")";
        }

        AddSql("instr_name", pQuery->Name, sql);
        AddSql("description", pQuery->Description, sql);
        AddSql("product", pQuery->Product, sql);
        AddSql("artists", pQuery->Artists, sql);
        AddSql("keywords", pQuery->Keywords, sql);
        SqlQuery = sql.str();

        InstrumentsDb* idb = InstrumentsDb::GetInstrumentsDb();

        int res = sqlite3_prepare(idb->GetDb(), SqlQuery.c_str(), -1, &pStmt, NULL);
        if (res != SQLITE_OK) {
            throw Exception("DB error: " + ToString(sqlite3_errmsg(idb->GetDb())));
        }

        for(int i = 0; i < Params.size(); i++) {
            idb->BindTextParam(pStmt, i + 2, Params.at(i));
        }
    }
    
    InstrumentFinder::~InstrumentFinder() {
        if (pStmt != NULL) sqlite3_finalize(pStmt);
    }
    
    void InstrumentFinder::ProcessDirectory(String Path, int DirId) {
        InstrumentsDb* idb = InstrumentsDb::GetInstrumentsDb();
        idb->BindIntParam(pStmt, 1, DirId);

        String s = Path;
        if(Path.compare("/") != 0) s += "/";
        int res = sqlite3_step(pStmt);
        while(res == SQLITE_ROW) {
            pInstruments->push_back(s + idb->toAbstractName(ToString(sqlite3_column_text(pStmt, 0))));
            res = sqlite3_step(pStmt);
        }
        
        if (res != SQLITE_DONE) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(idb->GetDb())));
        }

        res = sqlite3_reset(pStmt);
        if (res != SQLITE_OK) {
            sqlite3_finalize(pStmt);
            throw Exception("DB error: " + ToString(sqlite3_errmsg(idb->GetDb())));
        }
    }

    StringListPtr InstrumentFinder::GetInstruments() {
#if __cplusplus >= 201103L && !CONFIG_NO_CPP11STL
        return std::move(pInstruments);
#else
        return pInstruments;
#endif
    }

    void DirectoryCounter::ProcessDirectory(String Path, int DirId) {
        count += InstrumentsDb::GetInstrumentsDb()->GetDirectoryCount(DirId);
    }

    void InstrumentCounter::ProcessDirectory(String Path, int DirId) {
        count += InstrumentsDb::GetInstrumentsDb()->GetInstrumentCount(DirId);
    }

    DirectoryCopier::DirectoryCopier(String SrcParentDir, String DestDir) {
        this->SrcParentDir = SrcParentDir;
        this->DestDir = DestDir;

        if (DestDir.at(DestDir.length() - 1) != '/') {
            this->DestDir.append("/");
        }
        if (SrcParentDir.at(SrcParentDir.length() - 1) != '/') {
            this->SrcParentDir.append("/");
        }
    }

    void DirectoryCopier::ProcessDirectory(String Path, int DirId) {
        InstrumentsDb* db = InstrumentsDb::GetInstrumentsDb();

        String dir = DestDir;
        String subdir = Path;
        if(subdir.length() > SrcParentDir.length()) {
            subdir = subdir.substr(SrcParentDir.length());
            dir += subdir;
            db->AddDirectory(dir);
        }

        int dstDirId = db->GetDirectoryId(dir);
        if(dstDirId == -1) {
            throw Exception("Unkown DB directory: " + InstrumentsDb::toEscapedPath(dir));
        }
        IntListPtr ids = db->GetInstrumentIDs(DirId);
        for (int i = 0; i < ids->size(); i++) {
            String name = db->GetInstrumentName(ids->at(i));
            db->CopyInstrument(ids->at(i), name, dstDirId, dir);
        }
    }

    ScanProgress::ScanProgress() {
        TotalFileCount = ScannedFileCount = Status = 0;
        CurrentFile = "";
        GigFileProgress.custom = this;
        GigFileProgress.callback = GigFileProgressCallback;
    }

    void ScanProgress::StatusChanged() {
        InstrumentsDb* db = InstrumentsDb::GetInstrumentsDb();
        db->Jobs.GetJobById(JobId).FilesTotal = GetTotalFileCount();
        db->Jobs.GetJobById(JobId).FilesScanned = GetScannedFileCount();
        db->Jobs.GetJobById(JobId).Scanning = CurrentFile;
        db->Jobs.GetJobById(JobId).Status = GetStatus();
        
        InstrumentsDb::GetInstrumentsDb()->FireJobStatusChanged(JobId);
    }

    int ScanProgress::GetTotalFileCount() {
        return TotalFileCount;
    }

    void ScanProgress::SetTotalFileCount(int Count) {
        if (TotalFileCount == Count) return;
        TotalFileCount = Count;
        StatusChanged();
    }

    int ScanProgress::GetScannedFileCount() {
        return ScannedFileCount;
    }

    void ScanProgress::SetScannedFileCount(int Count) {
        if (ScannedFileCount == Count) return;
        ScannedFileCount = Count;
        if (Count > TotalFileCount) TotalFileCount = Count;
        StatusChanged();
    }

    int ScanProgress::GetStatus() {
        return Status;
    }

    void ScanProgress::SetStatus(int Status) {
        if (this->Status == Status) return;
        if (Status < 0) this->Status = 0;
        else if (Status > 100) this->Status = 100;
        else this->Status = Status;
        StatusChanged();
    }

    void ScanProgress::SetErrorStatus(int Err) {
        if (Err > 0) Err *= -1;
        Status = Err;
        StatusChanged();
    }

    void ScanProgress::GigFileProgressCallback(gig::progress_t* pProgress) {
        if (pProgress == NULL) return;
        ScanProgress* sp = static_cast<ScanProgress*> (pProgress->custom);
        
        sp->SetStatus((int)(pProgress->factor * 100));
    }

    AddInstrumentsJob::AddInstrumentsJob(int JobId, ScanMode Mode, String DbDir, String FsDir, bool insDir) {
        this->JobId = JobId;
        Progress.JobId = JobId;
        this->Mode = Mode;
        this->DbDir = DbDir;
        this->FsDir = FsDir;
		this->insDir = insDir;
    }

    void AddInstrumentsJob::Run() {
        try {
            InstrumentsDb* db = InstrumentsDb::GetInstrumentsDb();

            switch (Mode) {
                case NON_RECURSIVE:
                    Progress.SetTotalFileCount(GetFileCount());
                    db->AddInstrumentsNonrecursive(DbDir, FsDir, insDir, &Progress);
                    break;
                case RECURSIVE:
                    db->AddInstrumentsRecursive(DbDir, FsDir, false, insDir, &Progress);
                    break;
                case FLAT:
                    db->AddInstrumentsRecursive(DbDir, FsDir, true, insDir, &Progress);
                    break;
                default:
                    throw Exception("Unknown scan mode");
            }

            // Just to be sure that the frontends will be notified about the job completion
            if (Progress.GetTotalFileCount() != Progress.GetScannedFileCount()) {
                Progress.SetTotalFileCount(Progress.GetScannedFileCount());
            }
            if (Progress.GetStatus() != 100) Progress.SetStatus(100);
        } catch(Exception e) {
            Progress.SetErrorStatus(-1);
            throw e;
        }
    }

    int AddInstrumentsJob::GetFileCount() {
        int count = 0;

        try {
            FileListPtr fileList = File::GetFiles(FsDir);

            for (int i = 0; i < fileList->size(); i++) {
                String s = fileList->at(i);
                if (InstrumentFileInfo::isSupportedFile(s)) count++;
            }
        } catch(Exception e) {
            e.PrintMessage();
            return 0;
        }
        
        return count;
    }

    AddInstrumentsFromFileJob::AddInstrumentsFromFileJob(int JobId, String DbDir, String FilePath, int Index, bool insDir) {
        this->JobId = JobId;
        Progress.JobId = JobId;
        Progress.SetTotalFileCount(1);

        this->DbDir = DbDir;
        this->FilePath = FilePath;
        this->Index = Index;
		this->insDir = insDir;
    }

    void AddInstrumentsFromFileJob::Run() {
        try {
            InstrumentsDb::GetInstrumentsDb()->AddInstruments(DbDir, insDir, FilePath, Index, &Progress);

            // Just to be sure that the frontends will be notified about the job completion
            if (Progress.GetTotalFileCount() != Progress.GetScannedFileCount()) {
                Progress.SetTotalFileCount(Progress.GetScannedFileCount());
            }
            if (Progress.GetStatus() != 100) Progress.SetStatus(100);
        } catch(Exception e) {
            Progress.SetErrorStatus(-1);
            throw e;
        }
    }


    void DirectoryScanner::Scan(String DbDir, String FsDir, bool Flat, bool insDir, ScanProgress* pProgress) {
        dmsg(2,("DirectoryScanner: Scan(DbDir=%s,FsDir=%s,Flat=%d,insDir=%d)\n", DbDir.c_str(), FsDir.c_str(), Flat, insDir));
        if (DbDir.empty() || FsDir.empty()) throw Exception("Directory expected");
        
        this->DbDir = DbDir;
        this->FsDir = FsDir;
        this->insDir = insDir;
        if (DbDir.at(DbDir.length() - 1) != '/') {
            this->DbDir.append("/");
        }
        if (FsDir.at(FsDir.length() - 1) != File::DirSeparator) {
            this->FsDir.push_back(File::DirSeparator);
        }
        this->Flat = Flat;
        this->pProgress = pProgress;
        
        File::WalkDirectoryTree(FsDir, this);
    }

    void DirectoryScanner::DirectoryEntry(std::string Path) {
        dmsg(2,("DirectoryScanner: DirectoryEntry(Path=%s)\n", Path.c_str()));

        String dir = DbDir;
        if (!Flat) {
            String subdir = Path;
            if(subdir.length() > FsDir.length()) {
                subdir = subdir.substr(FsDir.length());
#ifdef WIN32
                replace(subdir.begin(), subdir.end(), '\\', '/');
#endif
                dir += subdir;
            }
        }
        
        InstrumentsDb* db = InstrumentsDb::GetInstrumentsDb();

        if (HasInstrumentFiles(Path)) {
            if (!db->DirectoryExist(dir)) db->AddDirectory(dir);
            db->AddInstrumentsNonrecursive(dir, Path, insDir, pProgress);
        }
    };

    bool DirectoryScanner::HasInstrumentFiles(String Dir) {
        InstrumentFileCounter c;
        return c.Count(Dir) > 0;
    }

    int InstrumentFileCounter::Count(String FsDir) {
        dmsg(2,("InstrumentFileCounter: Count(FsDir=%s)\n", FsDir.c_str()));
        if (FsDir.empty()) throw Exception("Directory expected");
        FileCount = 0;

        File::WalkDirectoryTree(FsDir, this);
        return FileCount;
    }

    void InstrumentFileCounter::FileEntry(std::string Path) {
        dmsg(2,("InstrumentFileCounter: FileEntry(Path=%s)\n", Path.c_str()));
        if (InstrumentFileInfo::isSupportedFile(Path)) FileCount++;
    };


    InstrumentFileInfo* InstrumentFileInfo::getFileInfoFor(String filename) {
        if (filename.length() < 4) return NULL;
        String fileExtension = filename.substr(filename.length() - 4);
        if (!strcasecmp(".gig", fileExtension.c_str()))
            return new GigFileInfo(filename);
        if (!strcasecmp(".sfz", fileExtension.c_str()))
            return new SFZFileInfo(filename);
        #if HAVE_SF2
        if (!strcasecmp(".sf2", fileExtension.c_str()))
            return new Sf2FileInfo(filename);
        #endif
        return NULL;
    }

    bool InstrumentFileInfo::isSupportedFile(String filename) {
        if (filename.length() < 4) return false;
        String fileExtension = filename.substr(filename.length() - 4);
        if (!strcasecmp(".gig", fileExtension.c_str())) return true;
        if (!strcasecmp(".sfz", fileExtension.c_str())) return true;
        #if HAVE_SF2
        if (!strcasecmp(".sf2", fileExtension.c_str())) return true;
        #endif
        return false;
    }

} // namespace LinuxSampler
