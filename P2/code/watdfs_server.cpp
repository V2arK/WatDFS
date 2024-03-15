//
// Starter code for CS 454/654
// You SHOULD change this file
//

#include "rpc.h"
#include "debug.h"
INIT_LOG

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <fuse.h>

// --------------------------- P2 ---------------------------
#include "rw_lock.h"
#include <string>
#include <map>

enum class OpType {
    RD, // read
    WR  // write (possibly also read)
};

struct FileInfo {
    OpType     opType; // Type
    rw_lock_t *lock;   // Pointer to the lock

    // Constructor to initialize the structure
    FileInfo(OpType opType, rw_lock_t *lockPtr) :
        opType(opType), lock(lockPtr) {
    }
};

// short path : lock * / OpType
std::map<std::string, rw_lock *> global_lock_info;
std::map<std::string, OpType>    global_open_info;

// ----------------------------------------------------------

// Global state server_persist_dir.
char *server_persist_dir = nullptr;

// Important: the server needs to handle multiple concurrent client requests.
// You have to be careful in handling global variables, especially for updating them.
// Hint: use locks before you update any global variable.

// We need to operate on the path relative to the server_persist_dir.
// This function returns a path that appends the given short path to the
// server_persist_dir. The character array is allocated on the heap, therefore
// it should be freed after use.
// Tip: update this function to return a unique_ptr for automatic memory management.
char *get_full_path(char *short_path) {
    int short_path_len = strlen(short_path);
    int dir_len        = strlen(server_persist_dir);
    int full_len       = dir_len + short_path_len + 1;

    char *full_path = (char *)malloc(full_len);

    // First fill in the directory.
    strcpy(full_path, server_persist_dir);
    // Then append the path.
    strcat(full_path, short_path);
    DLOG("Full path: %s\n", full_path);

    return full_path;
}

// The server implementation of getattr.
int watdfs_getattr(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];
    // The second argument is the stat structure, which should be filled in
    // by this function.
    struct stat *statbuf = (struct stat *)args[1];
    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // TODO: Make the stat system call, which is the corresponding system call needed
    // to support getattr. You should use the statbuf as an argument to the stat system call.
    // Let sys_ret be the return code from the stat system call.
    int sys_ret = stat(full_path, statbuf);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    } else {
        *ret = sys_ret;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    DLOG("Returning code for getattr: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of mknod.
int watdfs_mknod(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // Argument 2: mode (input, ARG_INT)
    mode_t *mode = (mode_t *)args[1];

    // Argument 3: dev (input, ARG_LONG)
    dev_t *dev = (dev_t *)args[2];

    // The fourth argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[3];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = mknod(full_path, *mode, *dev);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    } else {
        *ret = sys_ret;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    DLOG("Returning code for mknod: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of open.
int watdfs_open(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // Argument 2: fuse_file_info (input/output, char array)
    struct fuse_file_info *fi = (struct fuse_file_info *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    DLOG("watdfs_open: getting fileInfo for file %s", short_path);

    // mutual exclusion of open
    auto it = global_open_info.find(std::string(short_path));
    if (it == global_open_info.end()) { // non exist
        DLOG("watdfs_open: fileInfo not exist for file %s", short_path);
        if (((fi->flags & O_ACCMODE) == O_RDWR) || ((fi->flags & O_ACCMODE) == O_WRONLY)) {
            // file has no reader / writer, add to it
            DLOG("watdfs_open: fileInfo adding with WR for file %s", short_path);
            global_open_info.insert(std::make_pair(std::string(short_path), OpType::WR));
        } else {
            // file has no reader / writer, add to it
            DLOG("watdfs_open: fileInfo adding with RD for file %s", short_path);
            global_open_info.insert(std::make_pair(std::string(short_path), OpType::RD));
        }
    } else { // exist
        DLOG("watdfs_open: fileInfo exist for file %s", short_path);
        // fileInfo exists.
        // When a file is opened, it is opened with a file access mode (i.e. O_RDONLY, O_WRONLY, O_RDWR).
        if (((fi->flags & O_ACCMODE) == O_RDWR) || ((fi->flags & O_ACCMODE) == O_WRONLY)) {
            // the server should return an error(-EACCES) if a client attempts to open a file(wit hwrite mode)
            // and the file is already opened (in write mode)
            DLOG("watdfs_open: fileInfo exist for file %s, and tring to WR", short_path);
            if (it->second == OpType::WR) {
                *ret = -EACCES;
                // Clean up the full path, it was allocated on the heap.
                free(full_path);
                // The RPC call succeeded, so return 0.
                DLOG("watdfs_open: double write prohibited for file %s", short_path);
                return 0;
            } else {
                // some reader, now i am a writer
                global_open_info[std::string(short_path)] = OpType::WR;
            }
        }
    }

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = open(full_path, fi->flags);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
        DLOG("Returning code for open: %d", *ret);
    } else {
        // Open FILE and return a new file descriptor for it, or -1 on error.
        fi->fh = sys_ret;
        *ret   = sys_ret;
        DLOG("Open file succeed with file handler number: %d", *ret);
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of release.
int watdfs_release(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // Argument 2: fuse_file_info (input/output, char array)
    struct fuse_file_info *fi = (struct fuse_file_info *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // mutual exclusion of open
    auto it = global_open_info.find(std::string(short_path));
    if (it == global_open_info.end()) { // non exist
        // case not possible
    } else {
        // fileInfo exists.
        // When a file is opened, it is opened with a file access mode (i.e. O_RDONLY, O_WRONLY, O_RDWR).
        if (((fi->flags & O_ACCMODE) == O_RDWR) || ((fi->flags & O_ACCMODE) == O_WRONLY)) {
            // the server should return an error(-EACCES) if a client attempts to open a file(wit hwrite mode)
            // and the file is already opened (in write mode)
            global_open_info[std::string(short_path)] = OpType::RD;
            DLOG("watdfs_release: fileInfo reset to RD for file %s", short_path);
            // previously it must be WR
            // we just don't delete the entry, leave it in there forever.
        } else {
            DLOG("watdfs_release: fileInfo kept as RD for file %s", short_path);
        }
    }

    // Let sys_ret be the return code from the close system call.
    int sys_ret = close(fi->fh);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
        DLOG("Returning code for close/release: %d", *ret);
    } else {
        // Open FILE and return a new file descriptor for it, or -1 on error.
        *ret = sys_ret;
        DLOG("Close/release file succeed with code: %d", *ret);
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of read.
int watdfs_read(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // Argument 2: buf (output, char array)
    char *buf = (char *)args[1];

    // Argument 3: size (input, LONG)
    size_t *size = (size_t *)args[2];

    // Argument 4: offset (input, LONG)
    off_t *offset = (off_t *)args[3];

    // Argument 5: fuse_file_info (input, char array)
    struct fuse_file_info *fi = (struct fuse_file_info *)args[4];

    // The sixth argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[5];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // TODO: Make the stat system call, which is the corresponding system call needed
    // to support getattr. You should use the statbuf as an argument to the stat system call.
    // (void)statbuf;

    // Let sys_ret be the return code from the read system call.
    DLOG("Read size %ld, offset %ld", *size, *offset);
    int sys_ret = pread(fi->fh, buf, *size, *offset);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
        DLOG("Returning error code for read: %d", *ret);
    } else {
        // Open FILE and return a new file descriptor for it, or -1 on error.
        *ret = sys_ret;
        DLOG("Read file succeed with %d byte read.", *ret);
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of write.
int watdfs_write(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // Argument 2: buf (output, char array)
    char *buf = (char *)args[1];

    // Argument 3: size (input, LONG)
    size_t *size = (size_t *)args[2];

    // Argument 4: offset (input, LONG)
    off_t *offset = (off_t *)args[3];

    // Argument 5: fuse_file_info (input, char array)
    struct fuse_file_info *fi = (struct fuse_file_info *)args[4];

    // The sixth argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[5];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // TODO: Make the stat system call, which is the corresponding system call needed
    // to support getattr. You should use the statbuf as an argument to the stat system call.
    // (void)statbuf;

    // Let sys_ret be the return code from the read system call.
    DLOG("write size %ld, offset %ld", *size, *offset);
    int sys_ret = pwrite(fi->fh, buf, *size, *offset);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
        DLOG("Returning error code for write: %d", *ret);
    } else {
        // Open FILE and return a new file descriptor for it, or -1 on error.
        *ret = sys_ret;
        DLOG("write file succeed with %d byte read.", *ret);
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of truncate.
int watdfs_truncate(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // Argument 2: newsize (input, long)
    off_t *newsize = (off_t *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = truncate(full_path, *newsize);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
        DLOG("Returning code for truncate: %d", *ret);
    } else {
        // Open FILE and return a new file descriptor for it, or -1 on error.
        *ret = sys_ret;
        DLOG("truncate succeed with: %d", *ret);
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of fsync.
int watdfs_fsync(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // Argument 2: fuse_file_info (input, char array)
    struct fuse_file_info *fi = (struct fuse_file_info *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the close system call.
    int sys_ret = fsync(fi->fh);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
        DLOG("Returning code for fsync: %d", *ret);
    } else {
        // Open FILE and return a new file descriptor for it, or -1 on error.
        *ret = sys_ret;
        DLOG("Fsync file succeed with code: %d", *ret);
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of utimensat.
int watdfs_utimensat(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // Argument 2: ts (input, char array)
    struct timespec *ts = (struct timespec *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the close system call.

    // If pathname is absolute (true for our case), then dirfd is ignored.
    // The flags field is a bit mask that may be 0,
    // or include the following constant, defined in <fcntl.h>
    int sys_ret = utimensat(0, full_path, ts, 0);

    if (sys_ret < 0) {
        // If an error occurs, system calls set errno (global val) to a positive number to
        // indicate what went wrong.
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
        DLOG("Returning code for utimensat: %d", *ret);
    } else {
        // Open FILE and return a new file descriptor for it, or -1 on error.
        *ret = sys_ret;
        DLOG("utimensat file succeed with code: %d", *ret);
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call succeeded, so return 0.
    return 0;
}

// --------------------------- P2 ---------------------------

// The server implementation of lock.
int watdfs_lock(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    std::string short_path((char *)args[0]);

    // The second argument is the rw_lock_mode_t mode.
    rw_lock_mode_t mode = *(rw_lock_mode_t *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret     = (int *)args[2];
    int  sys_ret = 0;
    // Initially we set the return code to be 0.
    *ret = 0;

    DLOG("watdfs_lock called on file %s", short_path.c_str());

    // get lock info
    auto it = global_lock_info.find(short_path);

    if (it == global_lock_info.end()) { // non exist
        // create entry
        // global_lock_info[short_path].opType = OpType::N; // right now there is no operations
        DLOG("watdfs_lock creating lock on file %s", short_path.c_str());
        global_lock_info[short_path] = new rw_lock_t;
        // init lock
        DLOG("watdfs_lock initing lock on file %s", short_path.c_str());
        sys_ret = rw_lock_init(global_lock_info[short_path]);

        if (sys_ret < 0) {
            *ret = -errno;
            DLOG("Failed to create lock on file %s with errno %d", short_path.c_str(), errno);
            // The RPC call succeeded, so return 0.
            return 0;
        }

        // get lock again
        it = global_lock_info.find(short_path);
    }

    // now lock exists.
    DLOG("watdfs_lock trying to acquire lock on file %s", short_path.c_str());
    // try to acquire the lock in the given mode
    sys_ret = rw_lock_lock(it->second, mode);

    if (sys_ret < 0) {
        *ret = -errno;
        DLOG("Failed to obtain lock on file %s with errno %d", short_path.c_str(), errno);
    }

    // The RPC call succeeded, so return 0.
    return 0;
}

// The server implementation of unlock.
int watdfs_unlock(int *argTypes, void **args) {
    // Get the arguments.

    // The first argument is the path relative to the mountpoint.
    std::string short_path((char *)args[0]);

    // The second argument is the rw_lock_mode_t mode.
    rw_lock_mode_t mode = *(rw_lock_mode_t *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret     = (int *)args[2];
    int  sys_ret = 0;
    // Initially we set the return code to be 0.
    *ret = 0;

    DLOG("watdfs_unlock called on file %s", short_path.c_str());

    // get lock info
    auto it = global_lock_info.find(short_path);

    if (it == global_lock_info.end()) { // non exist
        *ret = -EPERM;                  // operation not permitted
        DLOG("Failed to unlock file %s, lock DNE", short_path.c_str());
        // The RPC call succeeded, so return 0.
        return 0;
    }

    // try to unlock the lock in the given mode
    sys_ret = rw_lock_unlock(it->second, mode);

    if (sys_ret < 0) {
        *ret = -errno;
        DLOG("Failed to release lock on file %s with errno %d", short_path.c_str(), errno);
    }

    // The RPC call succeeded, so return 0.
    return 0;
}
// ------------------------------------------------------------
// --------------------------- Main ---------------------------
// ------------------------------------------------------------

// The main function of the server.
int main(int argc, char *argv[]) {
    // argv[1] should contain the directory where you should store data on the
    // server. If it is not present it is an error, that we cannot recover from.
    if (argc != 2) {
        // In general, you shouldn't print to stderr or stdout, but it may be
        // helpful here for debugging. Important: Make sure you turn off logging
        // prior to submission!
        // See watdfs_client.cpp for more details
        // # ifdef PRINT_ERR
        // std::cerr << "Usage:" << argv[0] << " server_persist_dir";
        // #endif
        return -1;
    }
    // Store the directory in a global variable.
    server_persist_dir = argv[1];

    // TODO: Initialize the rpc library by calling `rpcServerInit`.
    // Important: `rpcServerInit` prints the 'export SERVER_ADDRESS' and
    // 'export SERVER_PORT' lines. Make sure you *do not* print anything
    // to *stdout* before calling `rpcServerInit`.
    DLOG("Initializing server...");

    int ret = rpcServerInit();
    // Initialize the RPC library.
    if (ret != 0) {
        // Handle error, e.g., print an error message.
        // Optionally, you can use DLOG for debugging.
        DLOG("Failed to initialize RPC server!");
        return ret;
    }
    DLOG("RPC server initialized.");
    // TODO: If there is an error with `rpcServerInit`, it maybe useful to have
    // debug-printing here, and then you should return.

    // TODO: Register your functions with the RPC library.
    // Note: The braces are used to limit the scope of `argTypes`, so that you can
    // reuse the variable for multiple registrations. Another way could be to
    // remove the braces and use `argTypes0`, `argTypes1`, etc.
    {
        // There are 3 args for the function (see watdfs_client.cpp for more
        // detail).
        int argTypes[4];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        // The second argument is the statbuf.
        argTypes[1] =
            (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        // The third argument is the retcode.
        argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        // Finally we fill in the null terminator.
        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"getattr", argTypes, watdfs_getattr);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register getattr failed.");
            return ret;
        }
        DLOG("Register getattr succeed.");
    }

    // on the server side, you must specify the length of the array as
    // 1 in the call to rpcRegister.

    /* IMPORTANT: only ARRAY needs to specify length 1u. */

    /* mknod */
    {
        // There are 4 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[5];
        // First is the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: mode (input, ARG_INT)
        arg_types[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);

        // Argument 3: dev (input, ARG_LONG)
        arg_types[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

        // Argument 4: return code (output, int)
        arg_types[3] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally we fill in the null terminator.
        arg_types[4] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"mknod", arg_types, watdfs_mknod);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register mknod failed.");
            return ret;
        }
        DLOG("Register mknod succeed.");
    }

    /* open */
    {
        // There are 3 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[4];
        // First is the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: fuse_file_info (input/output, char array)
        arg_types[1] =
            ((1u << ARG_INPUT) | (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | 1u;

        // Argument 3: return code (output, int)
        arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally we fill in the null terminator.
        arg_types[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"open", arg_types, watdfs_open);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register open failed.");
            return ret;
        }
        DLOG("Register open succeed.");
    }

    /* release */
    {
        // There are 3 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[4];
        // First is the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: fuse_file_info (input, char array)
        arg_types[1] =
            ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | 1u;

        // Argument 3: return code (output, int)
        arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally we fill in the null terminator.
        arg_types[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"release", arg_types, watdfs_release);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register release failed.");
            return ret;
        }
        DLOG("Register release succeed.");
    }

    /* Read */
    {
        // There are 6 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[7];

        // The first argument is the path, it is an input only argument, and a char
        // array. The length of the array is the length of the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: buf (output, char array)
        // Assuming the entire structure is passed as a char array.
        arg_types[1] = ((1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | 1u;

        // Argument 3: size (input, LONG)
        arg_types[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

        // Argument 4: offset (input, LONG)
        arg_types[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

        // Argument 5: fi (input, char array)
        // Assuming the entire structure is passed as a char array.
        arg_types[4] = ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | 1u;

        // Argument 6: return code (output, int)
        arg_types[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally, the last position of the arg types is 0. There is no
        // corresponding arg.
        arg_types[6] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"read", arg_types, watdfs_read);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register read failed.");
            return ret;
        }
        DLOG("Register read succeed.");
    }

    /* Write */
    {
        // There are 6 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[7];

        // The first argument is the path, it is an input only argument, and a char
        // array. The length of the array is the length of the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: buf (input, char array)
        // Assuming the entire structure is passed as a char array.
        arg_types[1] = ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | 1u;

        // Argument 3: size (input, LONG)
        arg_types[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

        // Argument 4: offset (input, LONG)
        arg_types[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

        // Argument 5: fi (input, char array)
        // Assuming the entire structure is passed as a char array.
        arg_types[4] = ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | 1u;

        // Argument 6: return code (output, int)
        arg_types[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally, the last position of the arg types is 0. There is no
        // corresponding arg.
        arg_types[6] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"write", arg_types, watdfs_write);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register write failed.");
            return ret;
        }
        DLOG("Register write succeed.");
    }

    /* Truncate */
    {
        // There are 3 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[4];
        // First is the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: newsize (input, LONG)
        arg_types[1] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

        // Argument 3: return code (output, int)
        arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally we fill in the null terminator.
        arg_types[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"truncate", arg_types, watdfs_truncate);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register truncate failed.");
            return ret;
        }
        DLOG("Register truncate succeed.");
    }

    /* fsync */
    {
        // There are 3 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[4];
        // First is the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: fuse_file_info (input, char array)
        arg_types[1] =
            ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | 1u;

        // Argument 3: return code (output, int)
        arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally we fill in the null terminator.
        arg_types[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"fsync", arg_types, watdfs_fsync);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register fsync failed.");
            return ret;
        }
        DLOG("Register fsync succeed.");
    }

    /* utimensat */
    {
        // There are 3 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[4];
        // First is the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: ts (input, array, char)
        arg_types[1] =
            ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | 1u;

        // Argument 3: return code (output, int)
        arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally we fill in the null terminator.
        arg_types[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"utimensat", arg_types, watdfs_utimensat);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register utimensat failed.");
            return ret;
        }
        DLOG("Register utimensat succeed.");
    }

    /* ------------------------ P2 ------------------------ */

    /* lock */
    {
        // There are 3 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[4];
        // First is the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: mode (input, ARG_INT)
        arg_types[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);

        // Argument 3: return code (output, int)
        arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally we fill in the null terminator.
        arg_types[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"lock", arg_types, watdfs_lock);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register lock failed.");
            return ret;
        }
        DLOG("Register lock succeed.");
    }

    /* unlock */
    {
        // There are 3 args for the function (see watdfs_client.cpp for more
        // detail).
        int arg_types[4];
        // First is the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;

        // Argument 2: mode (input, ARG_INT)
        arg_types[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);

        // Argument 3: return code (output, int)
        arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

        // Finally we fill in the null terminator.
        arg_types[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *)"unlock", arg_types, watdfs_unlock);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register unlock failed.");
            return ret;
        }
        DLOG("Register unlock succeed.");
    }

    // TODO: Hand over control to the RPC library by calling `rpcExecute`.
    // rpcExecute could fail, so you may want to have debug-printing here, and
    // then you should return.
    ret = rpcExecute();

    // clean all locks
    for (auto it : global_lock_info) {
        int lock_ret = rw_lock_destroy(it.second);

        if (lock_ret < 0) {
            DLOG("Failed to destroy lock on file %s", it.first.c_str());
        }
    }

    if (ret != 0) {
        // Handle error.
        DLOG("RPC execution failed.");
        return ret;
    }
    return ret;
}
