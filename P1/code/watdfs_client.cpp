//
// Starter code for CS 454/654
// You SHOULD change this file
//

#include "watdfs_client.h"
#include "debug.h"
#include <algorithm>
INIT_LOG

#include "rpc.h"

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
    void *userdata = nullptr;

    // TODO: save `path_to_cache` and `cache_interval` (for A3).

    // TODO: set `ret_code` to 0 if everything above succeeded else some appropriate
    // non-zero value.

    // Return pointer to global state data.
    return userdata;
}

void watdfs_cli_destroy(void *userdata) {
    // TODO: clean up your userdata state.
    // TODO: tear down the RPC library by calling `rpcClientDestroy`.

    // The client should call rpcClientDestroy when they are finished interacting with the server.
    // This will terminate connections with the server.
    rpcClientDestroy();
}

// GET FILE ATTRIBUTES
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf) {
    // SET UP THE RPC CALL
    DLOG("watdfs_cli_getattr called for '%s'", path);

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

        // TODO: set the function return value to the return code from the server.
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
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {
    // Called to create a file.
    DLOG("watdfs_cli_mknod called for '%s'", path);

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
        DLOG("mknod rpc failed with error '%d'", rpc_ret);
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        // TODO: set the function return value to the return code from the server.
        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("mknod failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_open(void *userdata, const char *path,
                    struct fuse_file_info *fi) {
    // Called during open.
    // You should fill in fi->fh.
    DLOG("watdfs_cli_open called for '%s'", path);

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

        // TODO: set the function return value to the return code from the server.
        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("open failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    errno = fxn_ret;
    DLOG("open succeed with file handler: %ld", (unsigned long)fxn_ret);

    return 0;
}

int watdfs_cli_release(void *userdata, const char *path,
                       struct fuse_file_info *fi) {
    // Called during close, but possibly asynchronously.
    DLOG("watdfs_cli_release called for '%s'", path);

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

        // TODO: set the function return value to the return code from the server.
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
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    // Read size amount of data at offset of file into buf.

    // This function reads into buf at most size bytes from the specified offset of the file.
    // It should return the number of bytes requested to be read,
    // except on EOF (return the number of bytes actually read) or error (return -errno).

    // Remember that size may be greater than the maximum array size of the RPC
    // library.

    DLOG("watdfs_cli_read called for '%s'", path);

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

            // TODO: set the function return value to the return code from the server.
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

int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {
    // Write size amount of data at offset of file from buf.

    // This function writes size number of bytes from buf into the file at the specified offset.
    // It should return the number of bytes requested to be written, except on error (-errno).

    // Remember that size may be greater than the maximum array size of the RPC
    // library.
    DLOG("watdfs_cli_write called for '%s'", path);

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

            // TODO: set the function return value to the return code from the server.
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

int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) {
    // Change the file size to newsize.

    // This function changes the size of the file to newsize.
    // If the file previously was larger than this size, the extra data is deleted.
    // If the file previously was shorter, it is extended,
    // and the extended part is filled in with null bytes (‘\0’).

    DLOG("watdfs_cli_truncate called for '%s'", path);

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

        // TODO: set the function return value to the return code from the server.
        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("truncate failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    errno = fxn_ret;
    DLOG("truncate succeed with code: %ld", (unsigned long)fxn_ret);

    return 0;
}

int watdfs_cli_fsync(void *userdata, const char *path,
                     struct fuse_file_info *fi) {
    // Force a flush of file data.
    DLOG("watdfs_cli_fsync called for '%s'", path);

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

        // TODO: set the function return value to the return code from the server.
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
int watdfs_cli_utimensat(void *userdata, const char *path,
                         const struct timespec ts[2]) {
    // Change file access and modification times.
    DLOG("watdfs_cli_open called for '%s'", path);

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
    args[1]      = (void *)ts;

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

        // TODO: set the function return value to the return code from the server.
        fxn_ret = retcode; // Set function return value to the server's return cod
    }

    if (fxn_ret < 0) {
        DLOG("utimensat failed with code: %d", fxn_ret);
    }

    // Clean up the memory we have allocated.
    delete[] args;

    // Finally return the value we got from the server.
    errno = fxn_ret;
    DLOG("utimensat succeed with file handler: %ld", (unsigned long)fxn_ret);

    return 0;
}
