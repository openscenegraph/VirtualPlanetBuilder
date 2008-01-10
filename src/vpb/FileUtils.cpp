#include <vpb/FileUtils>

#ifdef WIN32

    #include <windows.h>
    #include <stdarg.h>
    #include <varargs.h>
    #include <io.h>
    #include <process.h>
    #include <direct.h>
    #include <fcntl.h>
    #include <sys/types.h>

    #define ssize_t int     // return type of read() is int on Win32
    #define __const const
    #define __off_t off_t

    int     vpb::access(const char *path, int amode)              { return ::_access(path, amode); }
    int     vpb::open(const char *path, int oflag)                { return ::_open(path, oflag); }
    FILE*   vpb::fopen(const char* filename, const char* mode)    { return ::fopen(filename, mode); }
    ssize_t vpb::read(int fildes, void *buf, size_t nbyte)        { return ::_read(fildes, buf, nbyte); }
    ssize_t vpb::write(int fildes, const void *buf, size_t nbyte) { return ::_write(fildes, buf, nbyte); }
    int     vpb::close(int fildes)                                { return ::_close(fildes); }
    int     vpb::fclose(FILE* stream)                             { return ::fclose(stream); }
    int     vpb::fchmod(int fildes, mode_t mode)                  { return 0; }
    off_t   vpb::lseek(int fildes, off_t offset, int whence)      { return ::_lseek(fildes, offset, whence); }
    int     vpb::lockf(int fildes, int function, off_t size)      { return 0; }
    int     vpb::ftruncate(int fildes, off_t length)              { return ::_chsize(fildes, length); }

    // No equivalent to sync() on Win32, could use fsync(fd) on each file.
    void    vpb::sync()                                           { (void) _flushall(); }
    int     vpb::fsync(int fd)                                    { if (fd) return ::_commit(fd); return 0; }
    int     vpb::getpid()                                         { return ::_getpid(); }
    int     vpb::gethostname(char *name, size_t namelen)          { return ::gethostname(name, namelen); }

    // See http://cvsweb.xfree86.org/cvsweb/xc/include/Xpoll.h?rev=3.11 
    // variable XFD_SETSIZE for precedent
    int     vpb::getdtablesize()                                  { return 256; }

#else // WIN32

    #include <sys/stat.h>

    int     vpb::access(const char *path, int amode)              { return ::access(path, amode); }
    int     vpb::open(const char *path, int oflag)                { return ::open(path, oflag); }
    FILE*   vpb::fopen(const char* filename, const char* mode)    { return ::fopen(filename, mode); }
    ssize_t vpb::read(int fildes, void *buf, size_t nbyte)        { return ::read(fildes, buf, nbyte); }
    ssize_t vpb::write(int fildes, const void *buf, size_t nbyte) { return ::write(fildes, buf, nbyte); }
    int     vpb::close(int fildes)                                { return ::close(fildes); }
    int     vpb::fclose(FILE* stream)                             { return ::fclose(stream); }
    int     vpb::fchmod(int fildes, mode_t mode)                  { return ::fchmod(fildes, mode); }
    off_t   vpb::lseek(int fildes, off_t offset, int whence)      { return ::lseek(fildes, offset, whence); }
    int     vpb::lockf(int fildes, int function, off_t size)      { return ::lockf(fildes, function, size); }
    int     vpb::ftruncate(int fildes, off_t length)              { return ::ftruncate(fildes, length); }
    void    vpb::sync()                                           { ::sync(); }
    int     vpb::fsync(int fildes)                                { return ::fsync(fildes); }
    int     vpb::getpid()                                         { return ::getpid(); }
    int     vpb::gethostname(char *name, size_t namelen)          { return ::gethostname(name, namelen); }
    int     vpb::getdtablesize()                                  { return ::getdtablesize(); }

#endif  // WIN32
