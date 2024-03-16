//
// Starter code for CS 454/654
// You SHOULD change this file
//

#include "watdfs_client.h"
#include "debug.h"
#include <algorithm>
INIT_LOG

#include "rpc.h"

// --------------------------- P2 ---------------------------
#include "rw_lock.h"
#include <string>
#include <map>

struct Metadata {
    int      client_flag;
    int      fileDesc_client;
    uint64_t fileHandle_server;

    // Constructor
    Metadata(int flag, int fd_client, uint64_t fh_server) :
        client_flag(flag), fileDesc_client(fd_client), fileHandle_server(fh_server) {
        // Initialization list sets the values of the members
    }
};

// global variables
struct Userdata {
    char  *cache_path;
    time_t cache_interval;
    // short path -> metadata
    std::map<std::string, time_t>          Tc;
    std::map<std::string, struct Metadata> files_opened;
};

// func declarations
int rpc_getattr(void *userdata, const char *path, struct stat *statbuf);

int rpc_mknod(void *userdata, const char *path, mode_t mode, dev_t dev);

int rpc_open(void *userdata, const char *path, struct fuse_file_info *fi);

int rpc_release(void *userdata, const char *path, struct fuse_file_info *fi);

int rpc_read(void *userdata, const char *path, char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi);

int rpc_write(void *userdata, const char *path, const char *buf,
              size_t size, off_t offset, struct fuse_file_info *fi);

int rpc_truncate(void *userdata, const char *path, off_t newsize);

int rpc_fsync(void *userdata, const char *path, struct fuse_file_info *fi);

int rpc_utimensat(void *userdata, const char *path, const struct timespec ts[2]);

// helpers

// 7.2.4

// lock the file for transfering using the read or write lock_mode, respectively.
int lock(const char *path, rw_lock_mode_t mode) {
    // SET UP THE RPC CALL
    DLOG("lock called for '%s'", path);

    // getattr has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the output arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments

    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // Argument 2: mode (input, ARG_INT)
    arg_types[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);
    args[1]      = (void *)&mode;

    // Argument 3: return code (output, int)
    int retcode  = 0;
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    args[2]      = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"lock", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("lock rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("lock failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    return fxn_ret;
}

// unlock the file for transfering using the read or write lock_mode, respectively.
int unlock(const char *path, rw_lock_mode_t mode) {
    // SET UP THE RPC CALL
    DLOG("unlock called for '%s'", path);

    // getattr has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the output arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments

    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // Argument 2: mode (input, ARG_INT)
    arg_types[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);
    args[1]      = (void *)&mode;

    // Argument 3: return code (output, int)
    int retcode  = 0;
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    args[2]      = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"unlock", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("unlock rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("unlock failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    return fxn_ret;
}

// Copy from watdfs_server. Used to extend the path on to cache path.
char *get_full_path(void *userdata, const char *short_path) {
    int short_path_len = strlen(short_path);
    int dir_len        = strlen(((struct Userdata *)userdata)->cache_path);
    int full_len       = dir_len + short_path_len + 1;

    char *full_path = (char *)malloc(full_len);

    // First fill in the directory.
    strcpy(full_path, ((struct Userdata *)userdata)->cache_path);
    // Then append the path.
    strcat(full_path, short_path);
    DLOG("Full path: %s\n", full_path);

    return full_path;
}

// return NULL if file not exist in userdata (not opened)
// otherwize return the Metadata.
struct Metadata *get_metadata_opened(void *userdata, const char *path) {
    auto it = ((struct Userdata *)userdata)->files_opened.find(std::string(path));

    if (it != ((struct Userdata *)userdata)->files_opened.end()) { // exists
        return &it->second;
    } else { // non exist
        DLOG("Metadata not found for %s", path);
        return NULL;
    }
}

// return NULL if file not exist in userdata (not opened)
// otherwize return the Metadata.
time_t *get_Tc(void *userdata, const char *path) {
    auto it = ((struct Userdata *)userdata)->Tc.find(std::string(path));

    if (it != ((struct Userdata *)userdata)->Tc.end()) { // exists
        return &(it->second);
    } else { // non exist
        return NULL;
    }
}

int get_flags(void *userdata, const char *path) {
    int  flags = -1;
    auto it    = ((struct Userdata *)userdata)->files_opened.find(std::string(path));
    if (it != ((struct Userdata *)userdata)->files_opened.end()) { // exists
        flags = it->second.client_flag & O_ACCMODE;
    }

    return flags;
}

// check if the file at given path is fresh.
// ASSUME file is opened.
bool is_fresh(void *userdata, const char *path) {
    time_t *Tc = get_Tc(userdata, path);
    time_t  T  = time(NULL);

    if (Tc == NULL) {
        // file is not cached, we just assume its not fresh.
        return false;
    }

    if ((T - *Tc) < ((struct Userdata *)userdata)->cache_interval) {
        // case (i)
        DLOG("case (i): %s", path);
        return true;
    }

    // --- get file attributes from the server ---
    struct stat *statbuf_remote = new struct stat;
    int          rpc_ret        = rpc_getattr(userdata, path, statbuf_remote);

    if (rpc_ret < 0) {
        DLOG("Failed to getattr from server on %s", path);
        // nothing we can do really
    }

    // --- get file attributes from the client ---
    char        *full_path     = get_full_path(userdata, path);
    struct stat *statbuf_local = new struct stat;
    int          fxn_ret       = stat(full_path, statbuf_local);

    if (fxn_ret < 0) {
        fxn_ret = -errno;
        DLOG("Failed on getattr on %s with error code %d", full_path, errno);
        // nothing we can do really
    }

    if (statbuf_local->st_mtime == statbuf_remote->st_mtime) {
        // case (ii)
        DLOG("case (ii): %s", path);
        free(full_path);
        delete (statbuf_local);
        delete (statbuf_remote);
        return true;
    }

    free(full_path);
    delete (statbuf_local);
    delete (statbuf_remote);

    DLOG("not fresh: %s", path);
    return false;
}

// update the file given by 'path''s Tc to current time.
void update_Tc(void *userdata, const char *path) {
    time_t *Tc = get_Tc(userdata, path);
    // update the Tc to current time.
    if (Tc != NULL) {
        *Tc = time(NULL);
    } else {
        DLOG("File %s not cached, will add new entry on Tc.", path);
        // such file is not cached before, add to our list
        ((struct Userdata *)userdata)->Tc[std::string(path)] = time(NULL);
    }
}

// we will open file on O_WRONLY.
int download_file(void *userdata, const char *path) {
    DLOG("Start to download file %s", path);

    // The integer value that the actual function will return.
    int fxn_ret = 0;

    // Firstly, we check if the file is opened locally
    /*
    struct Metadata *file_metadata = get_metadata_opened(str::string(path));

    if (file_metadata != NULL) {
        DLOG("Failed to download file %s, file already opened", path);
        fxn_ret = -EMFILE;
        return fxn_ret;
    }
    */

    // Next, we attempt to get the statbuf from the server.

    // --- get file attributes from the server ---
    struct stat *statbuf_remote = new struct stat;
    int          rpc_ret        = rpc_getattr(userdata, path, statbuf_remote);

    if (rpc_ret < 0) {
        fxn_ret = -errno;
        delete (statbuf_remote);
        DLOG("RPC failed on rpc_getattr file %s with error code %d", path, errno);
        return fxn_ret;
    }

    // Lock
    rpc_ret = lock(path, RW_READ_LOCK);
    
    if (rpc_ret < 0) {
        fxn_ret = rpc_ret;
        delete (statbuf_remote);
        DLOG("RPC failed on getting read lock on file %s with error code %d", path, fxn_ret);
        return fxn_ret;
    }

    // --- Read file from server ---
    // firstly open file from server, we know it exists since we getattr.
    struct fuse_file_info *fi = new struct fuse_file_info;
    fi->flags                 = O_RDONLY;
    // we just want to read the file and download to client.
    rpc_ret = rpc_open(userdata, path, fi);

    // We just open READ ONLY, so it should not be causing any issue even if someone opened it for WRITE.
    if (rpc_ret < 0) {
        unlock(path, RW_READ_LOCK); // release lock, don't bother to check result

        fxn_ret = -errno;
        delete (fi);
        delete (statbuf_remote);
        DLOG("RPC failed on watdfs_cli_open while downloading file on file %s with error code %d", path, errno);
        return fxn_ret;
    }

    // read file into
    char *buf_content = new char[statbuf_remote->st_size];
    rpc_ret           = rpc_read(userdata, path, buf_content, statbuf_remote->st_size, 0, fi);

    if (rpc_ret < 0) {
        unlock(path, RW_READ_LOCK); // release lock, don't bother to check result

        fxn_ret = -errno;
        delete (fi);
        delete (statbuf_remote);
        delete (buf_content);
        DLOG("RPC failed on watdfs_cli_read while reading content on file %s with error code %d", path, errno);
        return fxn_ret;
    }

    // --- Release server file ---
    rpc_ret = rpc_release(userdata, path, fi);

    if (rpc_ret < 0) {
        unlock(path, RW_READ_LOCK); // release lock, don't bother to check result

        DLOG("Failed to release file %s from server with error code %d", path, errno);
        fxn_ret = -errno;
        delete (fi);
        delete (statbuf_remote);
        delete (buf_content);
        return fxn_ret;
    }

    // Unlock
    rpc_ret = unlock(path, RW_READ_LOCK);

    if (rpc_ret < 0) {
        fxn_ret = rpc_ret;
        delete (statbuf_remote);
        DLOG("RPC failed on releasing read lock on file %s with error code %d", path, fxn_ret);
        return fxn_ret;
    }

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(userdata, path);
    // Open for reading and writing.
    // the file descriptor shall not share it with any other process in the system.
    int fileDesc_local = open(full_path, O_WRONLY);

    // Upon successful completion, the function shall open the file and
    // return a non-negative integer representing the lowest numbered unused file descriptor.
    // Otherwise, -1 shall be returned and errno set to indicate the error.
    // No files shall be created or modified if the function returns -1.
    if (fileDesc_local == -1) {
        // failed to open file.
        // There could be many cases why, here is a list:
        // https://pubs.opengroup.org/onlinepubs/007904875/functions/open.html
        // Don't know how to handle the issues other than the file DNE.
        // TODO: Maybe need to add other check?

        // assume file DNE, we create the local file that has
        // the same MODE and DEVice identifier.
        fxn_ret = mknod(full_path, statbuf_remote->st_mode, statbuf_remote->st_dev);

        if (fxn_ret < 0) {
            DLOG("Failed to create file %s with error code %d", path, errno);
            fxn_ret = -errno;
            delete (statbuf_remote);
            free(full_path);
            return fxn_ret;
        }

        // now we created the file, try again.
        fileDesc_local = open(full_path, O_RDWR);

        if (fileDesc_local == -1) {
            DLOG("Failed to open existing file %s with error code %d", path, errno);
            fxn_ret = -errno;
            delete (statbuf_remote);
            free(full_path);
            return fxn_ret;
        }
    }

    // --- truncate the file at the client to make sure its empty ---
    fxn_ret = truncate(full_path, 0);

    if (fxn_ret < 0) {
        DLOG("Failed to truncate file %s with error code %d", full_path, errno);
        fxn_ret = -errno;
        delete (fi);
        delete (statbuf_remote);
        delete (buf_content);
        free(full_path);
        close(fileDesc_local);
        return fxn_ret;
    }

    // --- write the file to the client ---
    // write the buf_content to fileDesc_local starting -, for st_size length
    fxn_ret = pwrite(fileDesc_local, buf_content, statbuf_remote->st_size, 0);

    if (fxn_ret < 0) {
        DLOG("Failed to write into file %s with error code %d", full_path, errno);
        fxn_ret = -errno;
        delete (fi);
        delete (statbuf_remote);
        delete (buf_content);
        free(full_path);
        close(fileDesc_local);
        return fxn_ret;
    }

    // --- update the file metadata at the client to match server ---
    struct timespec ts[2] = {statbuf_remote->st_atim, statbuf_remote->st_mtim};
    fxn_ret               = futimens(fileDesc_local, ts);

    if (fxn_ret < 0) {
        DLOG("Failed to utimensat on local file %s with error code %d", full_path, errno);
        fxn_ret = -errno;
        delete (statbuf_remote);
        delete (buf_content);
        free(full_path);
        delete (fi);
        close(fileDesc_local);
        return fxn_ret;
    }

    // --- Close local file ---
    close(fileDesc_local);

    // --- Update Tc ---
    update_Tc(userdata, path);

    delete (fi);
    delete (statbuf_remote);
    delete (buf_content);
    free(full_path);

    DLOG("download_file on %s exit successfully", path);

    return fxn_ret;
}

// we will open file on O_RDWR.
int upload_file(void *userdata, const char *path) {
    DLOG("Start to upload file %s", path);

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---
        DLOG("Upload: failed to upload un-opened file local file %s ", path);
        return -ENOENT; // No such file or directory
    } else {
        if ((metadata->client_flag & O_ACCMODE) == O_RDONLY) {
            // Only read calls are allowed and should perform freshness
            // checks before reads, as usual. Write calls should fail and return -EMFILE.
            DLOG("watdfs_cli_write: cannot write read only file '%s'", path);
            return -EMFILE; // Too many open files
        }
    }

    // The integer value that the actual function will return.
    int fxn_ret = 0;
    int rpc_ret = 0;
    // --- get file attributes from the client ---

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char        *full_path     = get_full_path(userdata, path);
    struct stat *statbuf_local = new struct stat;
    fxn_ret                    = stat(full_path, statbuf_local);

    if (fxn_ret < 0) {
        fxn_ret = -errno;
        delete (statbuf_local);
        free(full_path);
        DLOG("RPC failed on getting stat on file %s with error code %d", path, errno);
        return fxn_ret;
    }

    // --- Open local file for reading and writing ---

    // the file descriptor shall not share it with any other process in the system.
    int fileDesc_local = open(full_path, O_RDONLY);

    if (fileDesc_local == -1) {
        // failed to open file.
        fxn_ret = -errno;
        delete (statbuf_local);
        free(full_path);
        DLOG("RPC failed on open (O_RDWR) local file %s with error code %d", path, errno);
        return fxn_ret;
    }

    // --- Read local file ---
    char *buf_content = new char[statbuf_local->st_size];
    fxn_ret           = pread(fileDesc_local, buf_content, statbuf_local->st_size, 0);

    if (fxn_ret < 0) {
        fxn_ret = -errno;
        delete (statbuf_local);
        delete (buf_content);
        free(full_path);
        DLOG("RPC failed on reading local content on file %s with fileDesc %d with error code %d", path, fileDesc_local, errno);
        close(fileDesc_local);
        return fxn_ret;
    }

    // --- Close local file, we done reading ---
    close(fileDesc_local); // didn't bother to check

    // Lock
    rpc_ret = lock(path, RW_WRITE_LOCK);

    if (rpc_ret < 0) {
        fxn_ret = rpc_ret;
        delete (buf_content);
        DLOG("RPC failed on getting write lock on file %s with error code %d", path, fxn_ret);
        return fxn_ret;
    }

    // --- open file on server ---
    // firstly open file from server, we know it exists since we created it otherwize.
    struct fuse_file_info *fi = new struct fuse_file_info;
    // fill out fi
    fi->fh    = metadata->fileHandle_server;
    fi->flags = metadata->client_flag;

    // --- truncate the file at the server to make sure its at the right size ---
    rpc_ret = rpc_truncate(userdata, path, 0);

    if (rpc_ret < 0) {
        unlock(path, RW_WRITE_LOCK); // release lock, don't bother to check result
        DLOG("Failed to truncate server file %s with error code %d", full_path, errno);
        fxn_ret = -errno;
        delete (statbuf_local);
        delete (buf_content);
        free(full_path);
        delete (fi);
        return fxn_ret;
    }

    // --- write the file to the server ---
    // write the buf_content to remote server starting from 0, for st_size length
    rpc_ret = rpc_write(userdata, path, buf_content, statbuf_local->st_size, 0, fi);

    if (rpc_ret < 0) {
        unlock(path, RW_WRITE_LOCK); // release lock, don't bother to check result
        DLOG("upload: Failed to write into remote file %s with error code %d", path, rpc_ret);
        fxn_ret = rpc_ret;
        delete (statbuf_local);
        delete (buf_content);
        free(full_path);
        delete (fi);
        return fxn_ret;
    }

    // --- update the file metadata at the server to match client ---
    struct timespec ts[2] = {statbuf_local->st_atim, statbuf_local->st_mtim};
    rpc_ret               = rpc_utimensat(userdata, path, ts);

    if (rpc_ret < 0) {
        DLOG("Failed to utimensat on remote file %s with error code %d", full_path, rpc_ret);
        fxn_ret = rpc_ret;
        unlock(path, RW_WRITE_LOCK);
        delete (statbuf_local);
        delete (buf_content);
        free(full_path);
        delete (fi);
        return fxn_ret;
    }

    // Unlock
    rpc_ret = unlock(path, RW_WRITE_LOCK);

    if (rpc_ret < 0) {
        fxn_ret = rpc_ret;
        delete (statbuf_local);
        delete (buf_content);
        free(full_path);
        delete (fi);
        DLOG("RPC failed on releasing write lock on file %s with error code %d", path, fxn_ret);
        return fxn_ret;
    }

    // update Tc
    update_Tc(userdata, path);

    // delete (statbuf_remote);
    delete (statbuf_local);
    delete (buf_content);
    free(full_path);
    delete (fi);
    // close(fileDesc_local);

    DLOG("upload_file on %s exit successfully", path);

    return fxn_ret;
}

// this function update file if not fresh,
// and download + update Tc if file is not cached.
// fail if file not exist on server.
int update_file(void *userdata, const char *path) {
    // The integer value function will return.
    int fxn_ret = 0;
    int rpc_ret = 0;
    DLOG("update_file called on '%s'", path);

    // You should try to open and transfer the file from the server,
    // perform the operation locally, transfer the file back to the server
    // (for write calls), and close the file.

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(userdata, path);

    // firstly, check if local file exists + is fresh
    if (!is_fresh(userdata, path)) {
        DLOG("update_file accessing new file '%s', sending RPC ...", path);

        // sucessfully get file attr from server
        // try to open and transfer the file from the server.
        rpc_ret = download_file(userdata, path);

        if (rpc_ret < 0) {
            // some error encountered.
            DLOG("update_file failed to download file '%s' info.", path);
            // free memories
            free(full_path);
            // exit
            return rpc_ret;
        }
    } else {
        DLOG("update_file: file '%s' up to date already", path);
    }

    // is fresh, just return
    free(full_path);
    return fxn_ret;
}

// ---------------------- CLI functions ----------------------

// SETUP AND TEARDOWN
void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache,
                      time_t cache_interval, int *ret_code) {
    // TODO: set up the RPC library by calling `rpcClientInit`.
    int rpc_init_status = rpcClientInit();

    // TODO: check the return code of the `rpcClientInit` call
    // `rpcClientInit` may fail, for example, if an incorrect port was exported.
    *ret_code = rpc_init_status; // Set error code

    if (rpc_init_status != 0) {
        DLOG("Failed to initialize RPC Client");
        *ret_code = -1;
        return nullptr;
    }

    // It may be useful to print to stderr or stdout during debugging.
    // Important: Make sure you turn off logging prior to submission!
    // One useful technique is to use pre-processor flags like:
    // # ifdef PRINT_ERR
    // std::cerr << "Failed to initialize RPC Client" << std::endl;
    // #endif
    // Tip: Try using a macro for the above to minimize the debugging code.

    // TODO Initialize any global state that you require for the assignment and return it.
    // The value that you return here will be passed as userdata in other functions.
    // In A1, you might not need it, so you can return `nullptr`.
    // void *userdata = nullptr;
    struct Userdata *userdata = new struct Userdata;
    // TODO: save `path_to_cache` and `cache_interval` (for A3).
    userdata->cache_interval = cache_interval;

    userdata->cache_path = (char *)malloc(strlen(path_to_cache) + 1);

    if (userdata->cache_path == NULL) {
        *ret_code = -1;
        DLOG("Failed to initialize usetdata->cache_path");

        delete (userdata);
        return nullptr;
    }

    strcpy(userdata->cache_path, path_to_cache);

    // TODO: set `ret_code` to 0 if everything above succeeded else some appropriate
    // non-zero value.
    *ret_code = 0;

    // Return pointer to global state data.
    return userdata;
}

void watdfs_cli_destroy(void *userdata) {
    // TODO: clean up your userdata state.
    // TODO: tear down the RPC library by calling `rpcClientDestroy`.

    // The client should call rpcClientDestroy when they are finished interacting with the server.
    // This will terminate connections with the server.
    rpcClientDestroy();

    // clean all stuff left in cache

    // free userdata
    free(((struct Userdata *)userdata)->cache_path);
    delete ((struct Userdata *)userdata);
}

// GET FILE ATTRIBUTES
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf) {
    // SET UP THE RPC CALL
    DLOG("watdfs_cli_getattr called for '%s'", path);

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(userdata, path);

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    time_t *tc = get_Tc(userdata, path);
    if ((tc != NULL) && (is_fresh(userdata, path))) {
        // we cached the file and fresh, then we can just return its getattr
        DLOG("watdfs_cli_getattr: returning attr on cached file %s", path);
        // fill statbuf
        rpc_ret = stat(full_path, statbuf);
        free(full_path);
        if (rpc_ret < 0) {
            DLOG("watdfs_cli_getattr: Failed to read stat from %s with error code %d", path, errno);
            fxn_ret = -errno;
            return fxn_ret;
        }
        return rpc_ret;
    } else {
        // file not exist or not fresh
        if (metadata == NULL) {
            // --- File not opened ---

            // firstly get info from remote.
            struct stat statbuf_server;
            rpc_ret = rpc_getattr(userdata, path, &statbuf_server);

            if (rpc_ret < 0) {
                fxn_ret = rpc_ret;
                DLOG("watdfs_cli_getattr: Failed to obtain info on remote file '%s'", path);

                free(full_path);
                // appearently if file not exist, mknod will get called next after receive -2
                // from the next line.
                return fxn_ret;
            }

            rpc_ret = update_file(userdata, path);

            if (rpc_ret < 0) {
                // some error encountered.
                DLOG("watdfs_cli_getattr: failed to download file '%s'", path);
                // free memories
                free(full_path);
                return rpc_ret;
            }
        } else {
            // --- File opened ---
            if ((metadata->client_flag & O_ACCMODE) == O_RDONLY) {
                // Only read calls are allowed and should perform freshness
                // checks before reads, as usual. Write calls should fail and return -EMFILE.

                // download / update file if it's not fresh, fail if file not exist
                rpc_ret = update_file(userdata, path);

                if (rpc_ret < 0) {
                    fxn_ret = rpc_ret;
                    DLOG("watdfs_cli_getattr: Failed to update local file '%s'", path);
                    free(full_path);
                    return fxn_ret;
                }
            } else {
                // Read calls should not perform freshness checks, as there
                // would be no updates on the server due to write exclusion and this prevents
                // overwriting local file updates if freshness condition has expired.
                // Write calls should perform the freshness checks at the end of writes, as usual.
            }
        }
    }

    // now file must exists.

    // fill statbuf
    rpc_ret = stat(full_path, statbuf);

    free(full_path);

    if (rpc_ret < 0) {
        DLOG("Failed to read stat from %s with error code %d", path, errno);
        fxn_ret = -errno;
        return fxn_ret;
    }

    return fxn_ret;
}

// CREATE, OPEN AND CLOSE
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {
    // Called to create a file.
    DLOG("watdfs_cli_mknod called for '%s'", path);

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(userdata, path);

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---

        // check if file exists on server
        struct stat *statbuf_remote = new struct stat;
        rpc_ret                     = rpc_getattr(userdata, path, statbuf_remote);

        if (rpc_ret >= 0) {
            // we successfully get the file,  meaning we should not be able to mknod.
            DLOG("watdfs_cli_mknod: File %s already exist", path);
            fxn_ret = -EEXIST; // File Exist
            free(full_path);
            return fxn_ret;
        }

        // We couldn't get statbuf for remote file, meaning such file not exist (is not ?)

        // so we remove the local cached file (if it exists)
        rpc_ret = unlink(full_path);

        if (rpc_ret < 0) {
            DLOG("watdfs_cli_mknod warning: Failed to remove cached file %s with error code %d", full_path, errno);
            // we don't really care if we removed it or not, because the file may not exists
        }

        // TODO: Need to remote Tc?

        // create the file on the server
        rpc_ret = rpc_mknod(userdata, path, mode, dev);

        if (rpc_ret < 0) {
            DLOG("watdfs_cli_mknod: Failed to mknod remote file %s with error code %d", path, rpc_ret);
            fxn_ret = rpc_ret;
            free(full_path);
            return fxn_ret;
        }

        // just download it
        rpc_ret = download_file(userdata, path);

        if (rpc_ret < 0) {
            // download cache fail
            DLOG("watdfs_cli_read failed to download cache file '%s' info.", path);
            fxn_ret = rpc_ret;
            return fxn_ret;
        }

        return fxn_ret;

    } else {
        // --- File opened ---
        DLOG("watdfs_cli_mknod: cannot mknod opened file %s", path);
        fxn_ret = -EEXIST; // File Exist
        free(full_path);
        return fxn_ret;
    }

    free(full_path);
    // return final results, should be 0
    return fxn_ret;
}

int watdfs_cli_open(void *userdata, const char *path, struct fuse_file_info *fi) {
    // Called during open.
    // You should fill in fi->fh.
    DLOG("watdfs_cli_open called for '%s'", path);

    // When a file is opened, it is opened with a file access mode (i.e. O_RDONLY, O_WRONLY, O_RDWR).
    // However, since files opened on the server will be read and files opened on the client will be
    // written to while creating local cached copies, these modes cannot be passed directly to
    // open on the client or server.

    // When watdfs_cli_open is called, you should (try to) copy the file from the server to the client
    // so the client can apply operations locally. Therefore, open should be called at both the client
    // and the server as part of caching the file locally, resulting in two different file descriptors
    // which you should track at the client. As part of these open calls,
    // you should satisfy the mutual exclusion requirements (suggestions in Section 7.2.3).

    // Once the file has been copied to the client, the original flags received by watdfs_cli_open must be used,
    // such that the file handle returned from this call respects the flag properties (e.g., read-only, write-only).

    // Opening a file should also initialize metadata at the client that is needed to check the freshness condition
    // for the file ( Tc). You can use the file modification time of the file to track T_client and T_server.

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(userdata, path);

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---

        // the server should keep track of open files. The server should maintain a
        // thread synchronized data structure that maps filenames to their status
        // (open for write, open for read, etc.).
        // If the server receives an open request for write to a file that has already
        // been opened in write mode,the server will use this data structure to discover
        // the conflict a nd return - EACCES.
        // When the server receives a message to close a file it has opened in write mode,
        // the data structure should be modified to indicate that the file is now available to a writer.

        // get file stat from server
        
        //struct stat *statbuf_remote = new struct stat;
        //rpc_ret                     = rpc_getattr(userdata, path, statbuf_remote);

        if (rpc_ret < 0) {
            if (rpc_ret != -ENOENT) {
                // Not sure whats happened
                DLOG("watdfs_cli_open: Failed to getattr on remote file %s, with errno %d", path, rpc_ret);
                fxn_ret = -rpc_ret;
                free(full_path);
                return fxn_ret;
            }
            // No such file or directory
            // only use the following flags:
            // O_CREAT, O_APPEND, O_EXCL, O_RDONLY, O_WRONLY, and O_RDWR.

            if (fi->flags != O_CREAT) {
                // we are not creating, and file not exist.
                DLOG("watdfs_cli_open: Failed to open file %s, file not exist", path);
                free(full_path);
                return -ENOENT;
            }

            // from P1:
            // If an application calls open with the O_CREAT flag and the file does not exist (how? by getattr?),
            // watdfs_cli_mknod is called by FUSE before the actual watdfs_cli_open call.
            // So we don't need to mknod, and watdfs_cli_mknod handles create file then upload.
            // so we should be able to just open directly, and this case it should be fresh as well.

            // TODO: Guess we just return?
            DLOG("watdfs_cli_open: file %s not exist, but with O_CREAT", path);
            free(full_path);
            return -ENOENT;
        }


        DLOG("watdfs_cli_open: updating file %s", path);
        // download / update file if it's not fresh, fail if file not exist
        rpc_ret = update_file(userdata, path);

        if (rpc_ret < 0) {
            fxn_ret = rpc_ret;
            free(full_path);
            DLOG("watdfs_cli_getattr: Failed to update local file '%s'", path);
            return fxn_ret;
        }

        // Opening a file should also initialize metadata at the client that is needed to check the freshness condition
        // for the file ( Tc). You can use the file modification time of the file to track T_client and T_server.

        // now we estabilished that the file should exist and fresh, so we proceed with open now.

        // When watdfs_cli_open is called, you should (try to) copy the file from the server to the client
        // so the client can apply operations locally. Therefore, open should be called at both the client
        // and the server as part of caching the file locally, resulting in two different file descriptors
        // which you should track at the client. As part of these open calls,
        // you should satisfy the mutual exclusion requirements (suggestions in Section 7.2.3).

        // --- Open local file ---
        // Once the file has been copied to the client, the original flags received by watdfs_cli_open must be used,
        // such that the file handle returned from this call respects the flag properties (e.g., read-only, write-only).
        int fileDesc_local = open(full_path, fi->flags);

        if (fileDesc_local == -1) {
            DLOG("watdfs_cli_open: Failed to open existing local file %s with error code %d", path, errno);
            fxn_ret = -errno;
            // clear this entry
            //((struct Userdata *)userdata)->files_opened.erase(std::string(path));
            free(full_path);
            return fxn_ret;
        }
        DLOG("watdfs_cli_open: local file %s opened", path);
        // --- Open remote file ---

        rpc_ret = rpc_open(userdata, path, fi);

        if (rpc_ret < 0) {
            DLOG("watdfs_cli_open: Failed to open existing remote file %s with error code %d", path, rpc_ret);
            fxn_ret = rpc_ret;

            // close local opened file
            rpc_ret = close(fileDesc_local);
            if (rpc_ret < 0) {
                DLOG("watdfs_cli_open: Failed to close local opened cached file %s with error code %d", path, rpc_ret);
            }

            // clear this entry
            //((struct Userdata *)userdata)->files_opened.erase(std::string(path));
            free(full_path);
            return fxn_ret;
        }

        DLOG("watdfs_cli_open: remote file %s opened", path);

        // --- Create  Metadata ---
        ((struct Userdata *)userdata)->files_opened.insert(std::make_pair(std::string(path), Metadata{fi->flags, fileDesc_local, fi->fh}));
        DLOG("watdfs_cli_open: Metadata created for file %s", path);
        DLOG("watdfs_cli_open: Local cached file open with fileDesc %d", fileDesc_local);
        DLOG("watdfs_cli_open: Remote file open with fileHandle %ld", fi->fh);
        DLOG("watdfs_cli_open: flag %d", fi->flags);
    } else {
        // --- File opened ---
        // we cannot open an opened file..
        DLOG("watdfs_cli_open: Failed to open already opend file '%s'", path);
        free(full_path);
        // You should use your already tracked metadata to determine
        // if the file has already been opened; if it has return -EMFILE.
        return -EMFILE;
    }

    // IMPORTANT: We will leave fi->fh as the remote file handler.
    free(full_path);
    return fxn_ret;
}

int watdfs_cli_release(void *userdata, const char *path, struct fuse_file_info *fi) {
    // Called during close.

    // libfuse will call watdfs_cli_release, but will not wait for watdfs_cli_release to return
    //  before close returns.

    // subsequent open/close calls to the same file may not succeed because watdfs_cli_release
    // has not completed.

    // we require that if release has not yet completed on the same file, open should fail with EMFILE.

    DLOG("watdfs_cli_release called for '%s'", path);

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---
        DLOG("watdfs_cli_release: File '%s' not opened", path);
        return -ENOENT;

    } else {
        // --- File opened ---

        if ((metadata->client_flag & O_ACCMODE) != O_RDONLY) {
            // watdfs_cli_release is responsible for transferring a writable file from the client to
            // server and unlocking it.

            DLOG("watdfs_cli_release: uploading file '%s'", path);
            rpc_ret = upload_file(userdata, path);

            if (rpc_ret < 0) {
                fxn_ret = rpc_ret;
                DLOG("watdfs_cli_release: Failed to upload file '%s' with errno %d before close", path, fxn_ret);
                // not sure why it failed to upload. maybe the check for flags is not good enough
                return fxn_ret;
            }
        }

        // --- Close Server File ---

        // IMPORTANT: We left fi->fh as the remote file handler in watdfs_cli_open().
        rpc_ret = rpc_release(userdata, path, fi);

        if (rpc_ret < 0) {
            fxn_ret = rpc_ret;
            DLOG("watdfs_cli_release: Server file '%s' close failed with errno %d", path, fxn_ret);
            return fxn_ret;
        }

        // --- Close Local File ---
        rpc_ret = close(metadata->fileDesc_client);

        if (rpc_ret < 0) {
            fxn_ret = -errno;
            DLOG("watdfs_cli_release: File '%s' close failed with errno %d", path, fxn_ret);
            return fxn_ret;
        }

        // On watdfs_cli_release, the client copy of the file should be closed(close system call), but it should remain in the cache directory.The cached copy of the file should not be deleted.
        // clear this entry
        ((struct Userdata *)userdata)->files_opened.erase(std::string(path));
        DLOG("watdfs_cli_release: File '%s' closed, removed entry in userdata", path);
        return fxn_ret;
    }
}

int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    // Read size amount of data at offset of file into buf.

    // This function reads into buf at most size bytes from the specified offset of the file.
    // It should return the number of bytes requested to be read,
    // except on EOF (return the number of bytes actually read) or error (return -errno).

    // Remember that size may be greater than the maximum array size of the RPC
    // library.

    DLOG("watdfs_cli_read called for '%s'", path);

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---
        DLOG("watdfs_cli_read: File '%s' not opened", path);
        return -EPERM; /* operation not permitted */
    } else {
        // --- File opened ---
        if ((metadata->client_flag & O_ACCMODE) == O_RDONLY) {
            DLOG("watdfs_cli_read: file %s opened in RDONLY mode", path);
            // Only read calls are allowed and should perform freshness
            // checks before reads, as usual. Write calls should fail and return -EMFILE.
            rpc_ret = update_file(userdata, path);

            if (rpc_ret < 0) {
                fxn_ret = rpc_ret;
                DLOG("watdfs_cli_truncate: Failed to update local file '%s'", path);
                return fxn_ret;
            }

        } else {
            // Read calls should not perform freshness checks, as there
            // would be no updates on the server due to write exclusion and this prevents
            // overwriting local file updates if freshness condition has expired.
            // Write calls should perform the freshness checks at the end of writes, as usual.
        }

        // file is fresh now (also possible if failed to download, don't know how to handle)
        rpc_ret = pread(metadata->fileDesc_client, buf, size, offset);

        if (rpc_ret < 0) {
            fxn_ret = -errno;
            DLOG("watdfs_cli_read: Failed to read cached file '%s' with errno %d", path, fxn_ret);
            return fxn_ret;
        } else {
            fxn_ret = rpc_ret;
            // update how many byte written
        }
    }
    return fxn_ret;
}

int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {
    // Write size amount of data at offset of file from buf.

    // This function writes size number of bytes from buf into the file at the specified offset.
    // It should return the number of bytes requested to be written, except on error (-errno).

    // Remember that size may be greater than the maximum array size of the RPC
    // library.
    DLOG("watdfs_cli_write called for '%s'", path);

    // Writes by a client should be applied to the local copy of the file, and periodically written back to the server.
    // You will do this by checking the freshness condition for writes at the end of a write/truncate call.
    // If at that time T, [(T - Tc) < t] or [T_client == T_server], then you can return immediately.
    // Otherwise, you must (synchronously) write client’s copy of the file back to the server and update T_server
    // to T_client. When the client is done with the file, as indicated by watdfs_cli_release,
    // the file should always be written back to the server.

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---
        DLOG("watdfs_cli_write: File '%s' not opened", path);
        return -EPERM; /* operation not permitted */
    } else {
        // --- File opened ---
        if ((metadata->client_flag & O_ACCMODE) == O_RDONLY) {
            // Only read calls are allowed and should perform freshness
            // checks before reads, as usual. Write calls should fail and return -EMFILE.
            DLOG("watdfs_cli_write: cannot write read only file '%s'", path);
            return -EMFILE; /* Too many open files */
        } else {
            // Read calls should not perform freshness checks, as there
            // would be no updates on the server due to write exclusion and this prevents
            // overwriting local file updates if freshness condition has expired.
            // Write calls should perform the freshness checks at the end of writes, as usual.

            rpc_ret = pwrite(metadata->fileDesc_client, buf, size, offset);

            if (rpc_ret < 0) {
                fxn_ret = -errno;
                DLOG("watdfs_cli_write failed to write to cache file '%s', errno %d.", path, fxn_ret);
                return fxn_ret;
            } else {
                fxn_ret = rpc_ret;
                // update how many byte written
            }

            DLOG("watdfs_cli_write succeed to write to cache file '%s'", path);

            // update Tc
            // update_Tc(userdata, path);

            // freshness check
            if (!is_fresh(userdata, path)) {
                rpc_ret = upload_file(userdata, path);

                if (rpc_ret < 0) {
                    // Upload failed.
                    DLOG("watdfs_cli_write failed to upload cache file '%s' info.", path);
                    fxn_ret = rpc_ret;
                    return fxn_ret;
                }
            }
        }
    }
    DLOG("watdfs_cli_write: finished on file %s", path);
    // Return, we are done
    return fxn_ret;
}

int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) {
    // Change the file size to newsize.

    // This function changes the size of the file to newsize.
    // If the file previously was larger than this size, the extra data is deleted.
    // If the file previously was shorter, it is extended,
    // and the extended part is filled in with null bytes (‘\0’).

    DLOG("watdfs_cli_truncate called for '%s'", path);

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---
        DLOG("watdfs_cli_truncate: File not opened '%s', update next ", path);
        // download / update file if it's not fresh, fail if file not exist
        rpc_ret = update_file(userdata, path);

        if (rpc_ret < 0) {
            fxn_ret = rpc_ret;
            DLOG("watdfs_cli_truncate: Failed to update local file '%s'", path);
            return fxn_ret;
        }

    } else {
        // --- File opened ---
        if ((metadata->client_flag & O_ACCMODE) == O_RDONLY) {
            // Only read calls are allowed and should perform freshness
            // checks before reads, as usual. Write calls should fail and return -EMFILE.
            DLOG("watdfs_cli_truncate: cannot write read only file '%s'", path);
            return -EMFILE; /* Too many open files */
        } else {
            // Read calls should not perform freshness checks, as there
            // would be no updates on the server due to write exclusion and this prevents
            // overwriting local file updates if freshness condition has expired.
            // Write calls should perform the freshness checks at the end of writes, as usual.
        }
    }

    // at this point, file should exists.

    // --- get file attributes from the client ---
    char *full_path = get_full_path(userdata, path);

    rpc_ret = truncate(full_path, newsize);

    if (rpc_ret < 0) {
        fxn_ret = -errno;
        DLOG("watdfs_cli_truncate failed to write to truncate file '%s', errno %d.", path, fxn_ret);
        free(full_path);
        return fxn_ret;
    }

    // update Tc
    // update_Tc(userdata, path);

    // freshness check
    if (!is_fresh(userdata, path)) {
        DLOG("watdfs_cli_truncate: upload file '%s'", path);
        rpc_ret = upload_file(userdata, path);

        if (rpc_ret < 0) {
            // Upload failed.
            DLOG("watdfs_cli_truncate failed to upload cache file '%s' info.", path);
            fxn_ret = rpc_ret;
            return fxn_ret;
        }
    }

    // Return, we are done
    return fxn_ret;
}

int watdfs_cli_fsync(void *userdata, const char *path, struct fuse_file_info *fi) {
    // Force a flush of file data.
    DLOG("watdfs_cli_fsync called for '%s'", path);

    // If a client application issues fsync, then the client’s copy of the file
    // must be written to the server immediately and T_server and Tc should be updated.
    // If the file is opened in read only mode, return an error.

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---
        DLOG("watdfs_cli_fstnc: File '%s' not opened", path);
        return -EPERM; /* operation not permitted */
    } else {
        // --- File opened ---
        if ((metadata->client_flag & O_ACCMODE) == O_RDONLY) {
            // Only read calls are allowed and should perform freshness
            // checks before reads, as usual. Write calls should fail and return -EMFILE.
            DLOG("watdfs_cli_fsync: cannot write read only file '%s'", path);
            return -EMFILE; /* Too many open files */
        } else {
            // Read calls should not perform freshness checks, as there
            // would be no updates on the server due to write exclusion and this prevents
            // overwriting local file updates if freshness condition has expired.
            // Write calls should perform the freshness checks at the end of writes, as usual.

            rpc_ret = upload_file(userdata, path);

            if (rpc_ret < 0) {
                // Upload failed.
                DLOG("watdfs_cli_fsync failed to upload cache file '%s' info.", path);
                fxn_ret = rpc_ret;
                return fxn_ret;
            }

            // update Tc
            // update_Tc(userdata, path);
        }
    }
    // Return, we are done
    return fxn_ret;
}

// CHANGE METADATA
int watdfs_cli_utimensat(void *userdata, const char *path, const struct timespec ts[2]) {
    // Change file access and modification times.
    DLOG("watdfs_cli_utimensat called for '%s'", path);

    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    int rpc_ret = 0;

    struct Metadata *metadata = get_metadata_opened(userdata, path);

    char *full_path = get_full_path(userdata, path);

    if (metadata == NULL) {
        // --- File not opened ---

        // download / update file if it's not fresh, fail if file not exist
        rpc_ret = update_file(userdata, path);

        if (rpc_ret < 0) {
            fxn_ret = rpc_ret;
            DLOG("watdfs_cli_utimensat: Failed to update local file '%s'", path);
            return fxn_ret;
        }

    } else {
        // --- File opened ---
        if ((metadata->client_flag & O_ACCMODE) == O_RDONLY) {
            // Only read calls are allowed and should perform freshness
            // checks before reads, as usual. Write calls should fail and return -EMFILE.
            DLOG("watdfs_cli_utimensat: cannot write read only file '%s'", path);
            free(full_path);
            return -EMFILE; /* Too many open files */
        }
    }

    DLOG("watdfs_cli_utimensat: Local cached file open with fileDesc %d", metadata->fileDesc_client);
    DLOG("watdfs_cli_utimensat: Remote file open with fileHandle %ld", metadata->fileHandle_server);
    DLOG("watdfs_cli_utimensat: flag %d", metadata->client_flag);

    // Read calls should not perform freshness checks, as there
    // would be no updates on the server due to write exclusion and this prevents
    // overwriting local file updates if freshness condition has expired.
    // Write calls should perform the freshness checks at the end of writes, as usual.

    DLOG("watdfs_cli_utimensat: Now change local file %s.", path);

    // If pathname is absolute (true for our case), then dirfd is ignored.
    rpc_ret = utimensat(0, full_path, ts, 0);

    if (rpc_ret < 0) {
        fxn_ret = -errno;
        DLOG("watdfs_cli_utimensat failed to write to truncate file '%s', errno %d.", path, fxn_ret);
        free(full_path);
        return fxn_ret;
    }

    DLOG("watdfs_cli_utimensat: Now update Tc on file %s.", path);

    // update Tc
    // update_Tc(userdata, path);

    // freshness check
    if (!is_fresh(userdata, path)) {
        DLOG("watdfs_cli_utimensat: Now upload file %s.", path);
        rpc_ret = upload_file(userdata, path);

        if (rpc_ret < 0) {
            // Upload failed.
            DLOG("watdfs_cli_utimensat failed to upload cache file '%s' info.", path);
            fxn_ret = rpc_ret;
            free(full_path);
            return fxn_ret;
        }
    }

    // Return, we are done
    free(full_path);
    return fxn_ret;
}

// -------------------- P1 RPC functions --------------------

// the getattr RPC function
int rpc_getattr(void *userdata, const char *path, struct stat *statbuf) {
    // SET UP THE RPC CALL
    DLOG("rpc_getattr called for '%s'", path);

    // getattr has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the output arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // The second argument is the stat structure. This argument is an output
    // only argument, and we treat it as a char array. The length of the array
    // is the size of the stat structure, which we can determine with sizeof.
    arg_types[1] = (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)sizeof(struct stat); // statbuf
    args[1]      = (void *)statbuf;

    // The third argument is the return code, an output only argument, which is
    // an integer.
    // TODO: fill in this argument type.

    int retcode;
    // by 4.1.1, retcode is not input, is output, is not array, type ARG_INT.
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u); // retcode
    // you cannot add the  (uint) sizeof(int); to the end,
    // you will get wrong type on RPC return (-205).

    // The return code is not an array, so we need to hand args[2] an int*.
    // The int* could be the address of an integer located on the stack, or use
    // a heap allocated integer, in which case it should be freed.
    // TODO: Fill in the argument
    args[2] = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"getattr", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("getattr rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        // If the return code of watdfs_cli_getattr is negative (an error), then
        // we need to make sure that the stat structure is filled with 0s. Otherwise,
        // FUSE will be confused by the contradicting return values.
        memset(statbuf, 0, sizeof(struct stat));
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    return fxn_ret;
}

// CREATE, OPEN AND CLOSE
int rpc_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {
    // Called to create a file.
    DLOG("rpc_mknod called for '%s'", path);

    // mknod has 4 arguments.
    int ARG_COUNT = 4;

    // Allocate space for the arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // Argument 2: mode (input, ARG_INT)
    arg_types[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);
    args[1]      = (void *)&mode;

    // Argument 3: dev (input, ARG_LONG)
    arg_types[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
    args[2]      = (void *)&dev;

    // Argument 4: return code (output, int)
    int retcode  = 0;
    arg_types[3] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    args[3]      = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[4] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"mknod", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("rpc_mknod failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("rpc_mknod failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    return fxn_ret;
}

int rpc_open(void *userdata, const char *path, struct fuse_file_info *fi) {
    // Called during open.
    // You should fill in fi->fh.
    DLOG("rpc_open called for '%s'", path);

    // open has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // Argument 2: fuse_file_info (input/output, char array)

    // fill in fi->fh
    //  this is done on the server side

    // Assuming the entire structure is passed as a char array.
    arg_types[1] = ((1u << ARG_INPUT) | (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | (uint)sizeof(struct fuse_file_info);
    args[1]      = (void *)fi;

    // Argument 3: return code (output, int)
    int retcode  = 0;
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    args[2]      = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"open", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("open rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("open failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    return fxn_ret;
}

int rpc_release(void *userdata, const char *path, struct fuse_file_info *fi) {
    // Called during close, but possibly asynchronously.
    DLOG("rpc_release called for '%s'", path);

    // release has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // Argument 2: fuse_file_info (input, char array)

    // Assuming the entire structure is passed as a char array.
    arg_types[1] = ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | (uint)sizeof(struct fuse_file_info);
    args[1]      = (void *)fi;

    // Argument 3: return code (output, int)
    int retcode  = 0;
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    args[2]      = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"release", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("release rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("release failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    return fxn_ret;
}

// READ AND WRITE DATA
int rpc_read(void *userdata, const char *path, char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi) {
    // Read size amount of data at offset of file into buf.

    // This function reads into buf at most size bytes from the specified offset of the file.
    // It should return the number of bytes requested to be read,
    // except on EOF (return the number of bytes actually read) or error (return -errno).

    // Remember that size may be greater than the maximum array size of the RPC
    // library.

    DLOG("rpc_read called for '%s'", path);

    // we might need to split read request to multiple RPC call.
    size_t max_size     = (unsigned long)MAX_ARRAY_LEN;
    size_t remain_size  = size;
    off_t  next_start   = offset;
    size_t read         = 0; // total # of byte we read in
    size_t cur_pkg_size = 0;
    // We can only send up to MAX_ARRAY_LEN
    int retcode = 0;
    int rpc_ret = 0;
    int fxn_ret = 0;

    // read has 6 arguments.
    int ARG_COUNT = 6;

    // Allocate space for the arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    do {
        // update values
        cur_pkg_size = std::min(max_size, remain_size);
        next_start += retcode;
        // update buf to write to next available byte
        buf += retcode;

        DLOG("max_size = %ld", max_size);
        DLOG("remain_size = %ld", remain_size);
        DLOG("next_start = %ld", next_start);
        DLOG("read = %ld", read);
        DLOG("cur_pkg_size = %ld", cur_pkg_size);
        DLOG("retcode = %d", retcode);
        DLOG("rpc_ret = %d", rpc_ret);
        DLOG("fxn_ret = %d", fxn_ret);

        // Fill in the arguments
        // The first argument is the path, it is an input only argument, and a char
        // array. The length of the array is the length of the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
        // For arrays the argument is the array pointer, not a pointer to a pointer.
        args[0] = (void *)path;

        // Argument 2: buf (output, char array)
        // Assuming the entire structure is passed as a char array.
        arg_types[1] = ((1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | (uint)cur_pkg_size;
        args[1]      = (void *)buf;

        // Argument 3: size (input, LONG)
        arg_types[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        args[2]      = (void *)&cur_pkg_size;

        // Argument 4: offset (input, LONG)
        arg_types[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        args[3]      = (void *)&next_start;

        // Argument 5: fi (input, char array)
        // Assuming the entire structure is passed as a char array.
        arg_types[4] =
            ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u))
            | (uint)sizeof(struct fuse_file_info);
        args[4] = (void *)fi;

        // Argument 6: return code (output, int)
        retcode      = 0;
        arg_types[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        args[5]      = (void *)&retcode;

        // Finally, the last position of the arg types is 0. There is no
        // corresponding arg.
        arg_types[6] = 0;

        // MAKE THE RPC CALL
        DLOG("Sending read size %ld, offset %ld", cur_pkg_size, next_start);
        rpc_ret = rpcCall((char *)"read", arg_types, args);

        // HANDLE THE RETURN
        // The integer value watdfs_cli_getattr will return.
        if (rpc_ret < 0) {
            DLOG("read rpc failed with error '%d'", rpc_ret);
            // Something went wrong with the rpcCall, return a sensible return
            // value. In this case lets return, -EINVAL
            fxn_ret = -EINVAL;

            // Clean up the memory we have allocated.
            delete[] args;

            return fxn_ret;
            // TODO: we just return since there is an error, maybe gonna change later
        } else {
            // Our RPC call succeeded. However, it's possible that the return code
            // from the server is not 0, that is it may be -errno. Therefore, we
            // should set our function return value to the retcode from the server.

            fxn_ret = retcode; // Set function return value to the server's return cod
        }

        if (fxn_ret < 0) {
            DLOG("read failed with code: %d", fxn_ret);

            // Clean up the memory we have allocated.
            delete[] args;

            return fxn_ret;
            // We cannot handle function call issue, just exit.
        }

        // Now we update the variables and get ready for the next RPC call.
        // Remember if retcode is positive, then it means the # of byte read in.
        remain_size -= retcode;
        read += retcode;
        DLOG("sub-read succeed with length: %ld", (unsigned long)retcode);
        DLOG("Byte remaining is: %ld", (unsigned long)remain_size);

    } while ((remain_size >= 0) && ((long unsigned int)retcode == max_size));
    // we will keep sending RPC packages if not finished
    // remaining size might mean how much byte we can fit in the buf.
    // if retcode is max_size, means it's likely that we still have more.
    DLOG("Loop terminated! ");
    DLOG("max_size = %ld", max_size);
    DLOG("remain_size = %ld", remain_size);
    DLOG("next_start = %ld", next_start);
    DLOG("read = %ld", read);
    DLOG("cur_pkg_size = %ld", cur_pkg_size);
    DLOG("retcode = %d", retcode);
    DLOG("rpc_ret = %d", rpc_ret);
    DLOG("fxn_ret = %d", fxn_ret);

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    errno = read;
    // It should return the number of bytes requested to be read.
    DLOG("read succeed with length: %ld", (unsigned long)read);

    // return read to register that we read how many byte.
    return read;
}

int rpc_write(void *userdata, const char *path, const char *buf,
              size_t size, off_t offset, struct fuse_file_info *fi) {
    // Write size amount of data at offset of file from buf.

    // This function writes size number of bytes from buf into the file at the specified offset.
    // It should return the number of bytes requested to be written, except on error (-errno).

    // Remember that size may be greater than the maximum array size of the RPC
    // library.
    DLOG("rpc_write called for '%s'", path);

    // we might need to split write request to multiple RPC call.
    size_t max_size     = (unsigned long)MAX_ARRAY_LEN;
    size_t remain_size  = size;
    off_t  next_start   = offset;
    size_t write        = 0; // total # of byte we wrote
    size_t cur_pkg_size = std::min(max_size, remain_size);
    // We can only send up to MAX_ARRAY_LEN
    int retcode = 0;
    int rpc_ret = 0;
    int fxn_ret = 0;

    // read has 6 arguments.
    int ARG_COUNT = 6;

    // Allocate space for the arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    while (remain_size > 0) {
        // we will keep sending RPC packages if not finished
        // remaining size might mean how much byte we can fit in the buf.
        // if retcode is max_size, means it's likely that we still have more.

        DLOG("max_size = %ld", max_size);
        DLOG("remain_size = %ld", remain_size);
        DLOG("next_start = %ld", next_start);
        DLOG("write = %ld", write);
        DLOG("cur_pkg_size = %ld", cur_pkg_size);
        DLOG("retcode = %d", retcode);
        DLOG("rpc_ret = %d", rpc_ret);
        DLOG("fxn_ret = %d", fxn_ret);

        // Fill in the arguments
        // The first argument is the path, it is an input only argument, and a char
        // array. The length of the array is the length of the path.
        arg_types[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
        // For arrays the argument is the array pointer, not a pointer to a pointer.
        args[0] = (void *)path;

        // Argument 2: buf (input, char array)
        // Assuming the entire structure is passed as a char array.
        arg_types[1] = ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | (uint)cur_pkg_size;
        args[1]      = (void *)buf;

        // Argument 3: size (input, LONG)
        arg_types[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        args[2]      = (void *)&cur_pkg_size;

        // Argument 4: offset (input, LONG)
        arg_types[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        args[3]      = (void *)&next_start;

        // Argument 5: fi (input, char array)
        // Assuming the entire structure is passed as a char array.
        arg_types[4] =
            ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u))
            | (uint)sizeof(struct fuse_file_info);
        args[4] = (void *)fi;

        // Argument 6: return code (output, int)
        retcode      = 0;
        arg_types[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        args[5]      = (void *)&retcode;

        // Finally, the last position of the arg types is 0. There is no
        // corresponding arg.
        arg_types[6] = 0;

        // MAKE THE RPC CALL
        DLOG("Sending write size %ld, offset %ld", cur_pkg_size, next_start);
        rpc_ret = rpcCall((char *)"write", arg_types, args);

        // HANDLE THE RETURN
        // The integer value watdfs_cli_getattr will return.
        if (rpc_ret < 0) {
            DLOG("write rpc failed with error '%d'", rpc_ret);
            // Something went wrong with the rpcCall, return a sensible return
            // value. In this case lets return, -EINVAL
            fxn_ret = -EINVAL;

            // Clean up the memory we have allocated.
            delete[] args;

            return fxn_ret;
            // TODO: we just return since there is an error, maybe gonna change later
        } else {
            // Our RPC call succeeded. However, it's possible that the return code
            // from the server is not 0, that is it may be -errno. Therefore, we
            // should set our function return value to the retcode from the server.

            fxn_ret = retcode; // Set function return value to the server's return cod
        }

        if (fxn_ret < 0) {
            DLOG("write failed with code: %d", fxn_ret);

            // Clean up the memory we have allocated.
            delete[] args;

            return fxn_ret;
            // We cannot handle function call issue, just exit.
        }

        // Now we update the variables and get ready for the next RPC call.
        // Remember if retcode is positive, then it means the # of byte wrote in.
        remain_size -= retcode;
        write += retcode;
        cur_pkg_size = std::min(max_size, remain_size);
        next_start += retcode;
        // update buf to write to next available byte
        buf += retcode;

        DLOG("sub-write succeed with length: %ld", (unsigned long)retcode);
        DLOG("Byte remaining is: %ld", (unsigned long)remain_size);
    }

    DLOG("Loop terminated! ");
    DLOG("max_size = %ld", max_size);
    DLOG("remain_size = %ld", remain_size);
    DLOG("next_start = %ld", next_start);
    DLOG("write = %ld", write);
    DLOG("cur_pkg_size = %ld", cur_pkg_size);
    DLOG("retcode = %d", retcode);
    DLOG("rpc_ret = %d", rpc_ret);
    DLOG("fxn_ret = %d", fxn_ret);

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    errno = write;
    // It should return the number of bytes requested to be read.
    DLOG("write succeed with length: %ld", (unsigned long)write);

    // return read to register that we read how many byte.
    return write;
}

int rpc_truncate(void *userdata, const char *path, off_t newsize) {
    // Change the file size to newsize.

    // This function changes the size of the file to newsize.
    // If the file previously was larger than this size, the extra data is deleted.
    // If the file previously was shorter, it is extended,
    // and the extended part is filled in with null bytes (‘\0’).

    DLOG("rpc_truncate called for '%s'", path);

    // open has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // Argument 2: newsize (input, LONG)

    // Assuming the entire structure is passed as a char array.
    arg_types[1] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
    args[1]      = (void *)&newsize;

    // Argument 3: return code (output, int)
    int retcode  = 0;
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    args[2]      = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"truncate", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("truncate rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("truncate failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    return fxn_ret;
}

int rpc_fsync(void *userdata, const char *path, struct fuse_file_info *fi) {
    // Force a flush of file data.
    DLOG("rpc_fsync called for '%s'", path);

    // release has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // Argument 2: fuse_file_info (input, char array)

    // Assuming the entire structure is passed as a char array.
    arg_types[1] = ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | (uint)sizeof(struct fuse_file_info);
    args[1]      = (void *)fi;

    // Argument 3: return code (output, int)
    int retcode  = 0;
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    args[2]      = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"fsync", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("fsync rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("fsync failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    return fxn_ret;
}

// CHANGE METADATA
int rpc_utimensat(void *userdata, const char *path, const struct timespec ts[2]) {
    // Change file access and modification times.
    DLOG("rpc_open called for '%s'", path);

    // open has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the arguments.
    void **args = new void *[ARG_COUNT];

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // Argument 2: ts (input/, char array)

    // fill in fi->fh
    //  this is done on the server side

    // Assuming the entire structure is passed as a char array.
    arg_types[1] = ((1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u)) | ((uint)sizeof(struct timespec) * 2);
    // we have ts[2]
    args[1] = (void *)ts;

    // Argument 3: return code (output, int)
    int retcode  = 0;
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    args[2]      = (void *)&retcode;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"utimensat", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        DLOG("utimensat rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("utimensat failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    return fxn_ret;
}
