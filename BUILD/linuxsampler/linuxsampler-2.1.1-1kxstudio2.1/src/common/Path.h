/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007-2017 Christian Schoenebeck                         *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef LS_PATH_H
#define LS_PATH_H

#include <vector>
#include <string>

namespace LinuxSampler {

/** @brief Portable Path Abstraction
 *
 * Correct path and file names are dependent to the underlying OS and FS.
 * Especially the set of forbidden characters (and their encodings / escape
 * sequences) in path and file names differ on the various systems. This
 * class is meant as a portable wrapper to circumvent these problems by
 * storing the file names in its raw, human readable (intended) form and
 * provides OS specific methods like @c toPosix() for converting the path into
 * the correct encoding, as expected by the respective system.
 *
 * This class is currently only used internally by the LSCP parser. A lot
 * generalization work would have to be done to use this class as a overall
 * replacement for all @c char* / @c std::string file name arguments in the
 * sampler or even its C++ API. Probably we'll use something like
 * @c boost::filesystem::path instead of this class in future.
 */
class Path {
public:
    /**
     * Default constructor.
     */
    Path();

    /**
     * Creates a Path object according to the local system's path conventions.
     * That means to construct this Path object it uses fromPosix() on POSIX
     * compliant systems (i.e. Linux, Mac) and fromWindows() on Windows systems.
     *
     * If you have no information in which file system encoding the path string
     * was encoded as, then rather use fromUnknownFS() instead.
     *
     * @see fromUnknownFS(), fromPosix(), fromWindows(), fromDbPath()
     */
    Path(std::string path);

    /**
     * Concatenate exactly one path node or filename to the end of this Path
     * object. This can be used to build up a full qualified path, directory
     * by directory.
     *
     * @param Name - name of the path node's name or filename in it's raw,
     *               human readable/expected form
     */
    void appendNode(std::string Name);

    /**
     * Sets the hardware drive of the path, for systems that use the concept
     * of drives in absolute pathes (i.e. Windows).
     */
    void setDrive(const char& Drive);

    /**
     * Returns file system path of this Path object as String in correct
     * encoding as expected by the local file system calls. Essentially that
     * means this software returns toWindows() on Windows systems, toPosix()
     * on POSIX compliant systems like Linux and Mac.
     */
    std::string toNativeFSPath() const;

    /**
     * Convert this Path into the correct encoding as expected by POSIX
     * compliant system calls.
     *
     * @see toNativeFSPath()
     */
    std::string toPosix() const;

    /**
     * Convert this Path into the correct encoding as expected
     * by the instruments database implementation.
     */
    std::string toDbPath() const;

    /**
     * Convert this Path into the correct encoding as expected and needed
     * for LSCP responses.
     */
    std::string toLscp() const;

    /**
     * Convert this Path into the correct encoding as expected by Windows
     * operating systems.
     *
     * @see toNativeFSPath()
     */
    std::string toWindows() const;

    /**
     * Concatenate two paths.
     */
    Path operator+(const Path& p);

    /**
     * Concatenate two paths.
     */
    Path operator+(const Path* p);

    /**
     * Attempts to auto detect the file system the supplied filename string
     * was encoded for, and then creates and returns a corresponding Path
     * object. This method only auto detects file system encodings of Windows
     * and POSIX (i.e. Linux and Mac). This method does @b NOT detect any other
     * encodings like DB path for example.
     */
    static Path fromUnknownFS(std::string path);

    /**
     * Create a Path object from a POSIX path / filename string.
     */
    static Path fromPosix(std::string path);

    /**
     * Create a Path object from a DB path.
     */
    static Path fromDbPath(std::string path);

    /**
     * Create a Path object from a Windows path / filename string.
     */
    static Path fromWindows(std::string path);

    /**
     * Returns the name of the file or directory represented by
     * this path in abstract/raw form. This is the last name in
     * the path's name sequence.
     */
    std::string getName() const;

    /**
     * Returns the name of the file or directory
     * represented by the specified path in abstract/raw form.
     * This is the last name in the path's name sequence.
     */
    static std::string getName(std::string path);

    /**
     * Returns the path with the last name
     * of the path's name sequence stripped off.
     */
    std::string stripLastName();

    /**
     * Returns the path with the last name
     * of the path's name sequence stripped off.
     */
    static std::string stripLastName(std::string path);

    /**
     * Returns the last name in the path's name sequence
     * of this path with the file extension stripped off.
     */
    std::string getBaseName() const;

    /**
     * Returns the last name in the path's name sequence
     * of the specified path with the file extension stripped off.
     */
    static std::string getBaseName(std::string path);

    /**
     * Returns true if the path is reflecting an absolute path, false if it is
     * rather a relative path.
     */
    bool isAbsolute() const;

    std::vector<std::string>& nodes();

private:
    std::vector<std::string> elements; ///< stores the path names raw = unencoded, each element is one node of the path
    char                     drive;
    bool                     absolute;
};

} // namespace LinuxSampler

#endif // LS_PATH_H
