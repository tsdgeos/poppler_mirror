//========================================================================
//
// gfile.cc
//
// Miscellaneous file and directory name manipulation.
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006 Takashi Iwai <tiwai@suse.de>
// Copyright (C) 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2008 Adam Batkin <adam@batkin.net>
// Copyright (C) 2008, 2010, 2012, 2013 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2009, 2012, 2014, 2017, 2018, 2021, 2022, 2024, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2009 Kovid Goyal <kovid@kovidgoyal.net>
// Copyright (C) 2013, 2018 Adam Reichold <adamreichold@myopera.com>
// Copyright (C) 2013, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Peter Breitenlohner <peb@mppmu.mpg.de>
// Copyright (C) 2013, 2017 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2017 Christoph Cullmann <cullmann@kde.org>
// Copyright (C) 2018 Mojca Miklavec <mojca@macports.org>
// Copyright (C) 2019, 2021 Christian Persch <chpe@src.gnome.org>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2026 Kleis Auke Wolthuizen <github@kleisauke.nl>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#ifndef _WIN32
#    include <sys/types.h>
#    include <sys/stat.h>
#    include <fcntl.h>
#    include <cstring>
#    include <pwd.h>
#endif // _WIN32
#include <cstdio>
#include <limits>
#include "GooString.h"
#include "gfile.h"

// Some systems don't define this, so just make it something reasonably
// large.
#ifndef PATH_MAX
#    define PATH_MAX 1024
#endif

#ifndef _WIN32

using namespace std::string_literals;

namespace {

template<typename...>
struct void_type
{
    using type = void;
};

template<typename... Args>
using void_t = typename void_type<Args...>::type;

template<typename Stat, typename = void_t<>>
struct StatMtim
{
    static const struct timespec &value(const Stat &stbuf) { return stbuf.st_mtim; }
};

// Mac OS X uses a different field name than POSIX and this detects it.
template<typename Stat>
struct StatMtim<Stat, void_t<decltype(Stat::st_mtimespec)>>
{
    static const struct timespec &value(const Stat &stbuf) { return stbuf.st_mtimespec; }
};

inline const struct timespec &mtim(const struct stat &stbuf)
{
    return StatMtim<struct stat>::value(stbuf);
}

}

#endif

//------------------------------------------------------------------------

GooString *appendToPath(GooString *path, const char *fileName)
{
#ifdef _WIN32
    //---------- Win32 ----------
    char buf[256];
    char *fp;

    auto tmp = path->copy();
    tmp->push_back('/');
    tmp->append(fileName);
    GetFullPathNameA(tmp->c_str(), sizeof(buf), buf, &fp);
    path->clear();
    path->append(buf);
    return path;

#else
    //---------- Unix ----------
    int i;

    // appending "." does nothing
    if (!strcmp(fileName, ".")) {
        return path;
    }

    // appending ".." goes up one directory
    if (!strcmp(fileName, "..")) {
        for (i = path->size() - 2; i >= 0; --i) {
            if (path->getChar(i) == '/') {
                break;
            }
        }
        if (i <= 0) {
            if (path->getChar(0) == '/') {
                path->erase(1, path->size() - 1);
            } else {
                path->clear();
                path->append("..");
            }
        } else {
            path->erase(i, path->size() - i);
        }
        return path;
    }

    // otherwise, append "/" and new path component
    if (!path->empty() && path->getChar(path->size() - 1) != '/') {
        path->push_back('/');
    }
    path->append(fileName);
    return path;
#endif
}

#ifndef _WIN32

static bool makeFileDescriptorCloexec(int fd)
{
#    ifdef FD_CLOEXEC
    int flags = fcntl(fd, F_GETFD);
    if (flags >= 0 && !(flags & FD_CLOEXEC)) {
        flags = fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
    }

    return flags >= 0;
#    else
    return true;
#    endif
}

int openFileDescriptor(const char *path, int flags)
{
#    ifdef O_CLOEXEC
    return open(path, flags | O_CLOEXEC);
#    else
    int fd = open(path, flags);
    if (fd == -1)
        return fd;

    if (!makeFileDescriptorCloexec(fd)) {
        close(fd);
        return -1;
    }

    return fd;
#    endif
}

#endif

FILE *openFile(const char *path, const char *mode)
{
#ifdef _WIN32
    OSVERSIONINFO version;
    wchar_t wPath[MAX_PATH + 1];
    char nPath[MAX_PATH + 1];
    wchar_t wMode[8];
    const char *p;
    size_t i;

    // NB: _wfopen is only available in NT
    version.dwOSVersionInfoSize = sizeof(version);
    GetVersionEx(&version);
    if (version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        for (p = path, i = 0; *p && i < MAX_PATH; ++i) {
            if ((p[0] & 0xe0) == 0xc0 && p[1] && (p[1] & 0xc0) == 0x80) {
                wPath[i] = (wchar_t)(((p[0] & 0x1f) << 6) | (p[1] & 0x3f));
                p += 2;
            } else if ((p[0] & 0xf0) == 0xe0 && p[1] && (p[1] & 0xc0) == 0x80 && p[2] && (p[2] & 0xc0) == 0x80) {
                wPath[i] = (wchar_t)(((p[0] & 0x0f) << 12) | ((p[1] & 0x3f) << 6) | (p[2] & 0x3f));
                p += 3;
            } else {
                wPath[i] = (wchar_t)(p[0] & 0xff);
                p += 1;
            }
        }
        wPath[i] = (wchar_t)0;
        for (i = 0; (i < sizeof(mode) - 1) && mode[i]; ++i) {
            wMode[i] = (wchar_t)(mode[i] & 0xff);
        }
        wMode[i] = (wchar_t)0;
        return _wfopen(wPath, wMode);
    } else {
        for (p = path, i = 0; *p && i < MAX_PATH; ++i) {
            if ((p[0] & 0xe0) == 0xc0 && p[1] && (p[1] & 0xc0) == 0x80) {
                nPath[i] = (char)(((p[0] & 0x1f) << 6) | (p[1] & 0x3f));
                p += 2;
            } else if ((p[0] & 0xf0) == 0xe0 && p[1] && (p[1] & 0xc0) == 0x80 && p[2] && (p[2] & 0xc0) == 0x80) {
                nPath[i] = (char)(((p[1] & 0x3f) << 6) | (p[2] & 0x3f));
                p += 3;
            } else {
                nPath[i] = p[0];
                p += 1;
            }
        }
        nPath[i] = '\0';
        return fopen(nPath, mode);
    }
#else
    // First try to atomically open the file with CLOEXEC
    const std::string modeStr = mode + "e"s;
    FILE *file = fopen(path, modeStr.c_str());
    if (file != nullptr) {
        return file;
    }

    // Fall back to the provided mode and apply CLOEXEC afterwards
    file = fopen(path, mode);
    if (file == nullptr) {
        return nullptr;
    }

    if (!makeFileDescriptorCloexec(fileno(file))) {
        fclose(file);
        return nullptr;
    }

    return file;
#endif
}

char *getLine(char *buf, int size, FILE *f)
{
    int c, i;

    i = 0;
    while (i < size - 1) {
        if ((c = fgetc(f)) == EOF) {
            break;
        }
        buf[i++] = (char)c;
        if (c == '\x0a') {
            break;
        }
        if (c == '\x0d') {
            c = fgetc(f);
            if (c == '\x0a' && i < size - 1) {
                buf[i++] = (char)c;
            } else if (c != EOF) {
                ungetc(c, f);
            }
            break;
        }
    }
    buf[i] = '\0';
    if (i == 0) {
        return nullptr;
    }
    return buf;
}

int Gfseek(FILE *f, Goffset offset, int whence)
{
#if HAVE_FSEEKO
    return fseeko(f, offset, whence);
#elif HAVE_FSEEK64
    return fseek64(f, offset, whence);
#elif defined(__MINGW32__)
    return fseeko64(f, offset, whence);
#elif defined(_WIN32)
    return _fseeki64(f, offset, whence);
#else
    return fseek(f, offset, whence);
#endif
}

Goffset Gftell(FILE *f)
{
#if HAVE_FSEEKO
    return ftello(f);
#elif HAVE_FSEEK64
    return ftell64(f);
#elif defined(__MINGW32__)
    return ftello64(f);
#elif defined(_WIN32)
    return _ftelli64(f);
#else
    return ftell(f);
#endif
}

Goffset GoffsetMax()
{
#if HAVE_FSEEKO
    return (std::numeric_limits<off_t>::max)();
#elif HAVE_FSEEK64 || defined(__MINGW32__)
    return (std::numeric_limits<off64_t>::max)();
#elif defined(_WIN32)
    return (std::numeric_limits<__int64>::max)();
#else
    return (std::numeric_limits<long>::max)();
#endif
}

//------------------------------------------------------------------------
// GooFile
//------------------------------------------------------------------------

#ifdef _WIN32

GooFile::GooFile(HANDLE handleA) : handle(handleA)
{
    GetFileTime(handleA, nullptr, nullptr, &modifiedTimeOnOpen);
}

int GooFile::read(char *buf, int n, Goffset offset) const
{
    DWORD m;

    LARGE_INTEGER largeInteger = {};
    largeInteger.QuadPart = offset;

    OVERLAPPED overlapped = {};
    overlapped.Offset = largeInteger.LowPart;
    overlapped.OffsetHigh = largeInteger.HighPart;

    return FALSE == ReadFile(handle, buf, n, &m, &overlapped) ? -1 : m;
}

Goffset GooFile::size() const
{
    LARGE_INTEGER size = { (DWORD)-1, -1 };

    GetFileSizeEx(handle, &size);

    return size.QuadPart;
}

std::unique_ptr<GooFile> GooFile::open(const std::string &fileName)
{
    HANDLE handle = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    return handle == INVALID_HANDLE_VALUE ? std::unique_ptr<GooFile>() : std::unique_ptr<GooFile>(new GooFile(handle));
}

std::unique_ptr<GooFile> GooFile::open(const wchar_t *fileName)
{
    HANDLE handle = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    return handle == INVALID_HANDLE_VALUE ? std::unique_ptr<GooFile>() : std::unique_ptr<GooFile>(new GooFile(handle));
}

bool GooFile::modificationTimeChangedSinceOpen() const
{
    struct _FILETIME lastModified;
    GetFileTime(handle, nullptr, nullptr, &lastModified);

    return modifiedTimeOnOpen.dwHighDateTime != lastModified.dwHighDateTime || modifiedTimeOnOpen.dwLowDateTime != lastModified.dwLowDateTime;
}

#else

int GooFile::read(char *buf, int n, Goffset offset) const
{
#    if HAVE_PREAD64
    return pread64(fd, buf, n, offset);
#    else
    return pread(fd, buf, n, offset);
#    endif
}

Goffset GooFile::size() const
{
#    if HAVE_LSEEK64
    return lseek64(fd, 0, SEEK_END);
#    else
    return lseek(fd, 0, SEEK_END);
#    endif
}

std::unique_ptr<GooFile> GooFile::open(const std::string &fileName)
{
    int fd = openFileDescriptor(fileName.c_str(), O_RDONLY);

    return GooFile::open(fd);
}

std::unique_ptr<GooFile> GooFile::open(int fdA)
{
    return fdA < 0 ? std::unique_ptr<GooFile>() : std::unique_ptr<GooFile>(new GooFile(fdA));
}

GooFile::GooFile(int fdA) : fd(fdA)
{
    struct stat statbuf;
    fstat(fd, &statbuf);
    modifiedTimeOnOpen = mtim(statbuf);
}

bool GooFile::modificationTimeChangedSinceOpen() const
{
    struct stat statbuf;
    fstat(fd, &statbuf);

    return modifiedTimeOnOpen.tv_sec != mtim(statbuf).tv_sec || modifiedTimeOnOpen.tv_nsec != mtim(statbuf).tv_nsec;
}

#endif // _WIN32
