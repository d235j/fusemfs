/*
 * FuseMFS - Macintosh File System implementation (read-only)
 * Copyright (C) 2008-2009 Jesus A. Alvarez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define __USE_BSD
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <iconv.h>
#include <libgen.h>
#include "fusemfs.h"

static struct {
    char    *path;
    int     use_folders;
} options = {
    .path =         NULL,
    .use_folders =  0
};

static MFSVolume *_fusemfs_vol;
static iconv_t _fusemfs_iconv;
static iconv_t _fusemfs_vnoci;

enum {
    KEY_VERSION,
    KEY_HELP,
    KEY_FOLDERS
};

#define FUSEMFS_OPT(t, p, v) { t, offsetof(struct options, p), v }
#define COMMENT_XATTR "com.apple.metadata:kMDItemFinderComment"

static struct fuse_opt fusemfs_opts[] =
{
    FUSE_OPT_KEY("-V",          KEY_VERSION),
    FUSE_OPT_KEY("--version",   KEY_VERSION),
    FUSE_OPT_KEY("-h",          KEY_HELP),
    FUSE_OPT_KEY("--help",      KEY_HELP),
    FUSE_OPT_KEY("--folders",   KEY_FOLDERS),
    FUSE_OPT_END
};

static struct fuse_operations fusemfs_ops = {
    .getattr   = fusemfs_getattr,
    .readdir   = fusemfs_readdir,
    .open      = fusemfs_open,
    .release   = fusemfs_release,
    .read      = fusemfs_read,
    .statfs    = fusemfs_statfs,
};

static int fusemfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
    switch (key) {
    case FUSE_OPT_KEY_NONOPT:
        if (options.path == NULL) {
            options.path = strdup(arg);
            return 0;
        }
        return 1;
    case KEY_VERSION:
        printf("fuseMFS %s, (c)2008-2009 Jesus A. Alvarez, namedfork.net\n", FUSEMFS_VERSION);
        exit(1);
    case KEY_HELP:
        printf("usage: fusemfs [--folders] [fuse options] device mountpoint\n");
        exit(0);
    case KEY_FOLDERS:
        options.use_folders = 1;
        return 0;
    }
}

char * mfs_to_utf8 (const char * in, char * out, size_t outlen) {
    size_t len = strlen(in);
    size_t outleft;
    char * outp = out;
    if (out == NULL) {
        outlen = (len*3)+1;
        out = malloc(outlen);
    }
    outleft = outlen-1;
    iconv(_fusemfs_iconv, (char **restrict)&in, &len, &outp, &outleft);
    iconv(_fusemfs_iconv, NULL, NULL, NULL, NULL);
    out[outlen-outleft-1] = '\0';
    
    // replace /:
    for(outp=out;*outp;outp++) {
        if (*outp == ':') *outp = '/';
        else if (*outp == '/') *outp = ':';
    }
    
    return out;
}

char * utf8_to_mfs (const char * in) {
    size_t len, outlen, outleft;
    len = outleft = strlen(in);
    outlen = len+1;
    char * out = malloc(outlen);
    char * outp = out;
    iconv(_fusemfs_vnoci, (char **restrict)&in, &len, &outp, &outleft);
    iconv(_fusemfs_vnoci, NULL, NULL, NULL, NULL);
    out[outlen-outleft-1] = '\0';
    
    // replace /:
    for(outp=out;*outp;outp++) {
        if (*outp == ':') *outp = '/';
        else if (*outp == '/') *outp = ':';
    }
    
    return out;
}

int main(int argc, char *argv[])
{
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    
    bzero(&options, sizeof options);
    if (fuse_opt_parse(&args, &options, fusemfs_opts, fusemfs_opt_proc) == -1) return -1;
    
    // open volume
    if (options.path == NULL) {
        fprintf(stderr, "invalid volume\n");
        return 1;
    }
    
    int mfsflags = 0;
    if (options.use_folders) mfsflags |= MFS_FOLDERS;
    _fusemfs_iconv = iconv_open("UTF-8", "Macintosh");
    if (_fusemfs_iconv == (iconv_t)-1) {
        perror("iconv_open");
        return 1;
    }
    _fusemfs_vnoci = iconv_open("Macintosh", "UTF-8");
    if (_fusemfs_vnoci == (iconv_t)-1) {
        perror("iconv_open");
        return 1;
    }
    
    _fusemfs_vol = mfs_vopen(options.path, 0, mfsflags);
    if (_fusemfs_vol == NULL) {
        perror("mfs_vopen");
        return 1;
    }
    
    // fuse options
    fuse_opt_add_arg(&args, "-o");
    fuse_opt_add_arg(&args, "ro");
    
    // MacFUSE options
    #if defined(__APPLE__)
    fuse_opt_add_arg(&args, "-o");
    char volnameOption[128] = "volname=";
    mfs_to_utf8(_fusemfs_vol->name, volnameOption+8, 120);
    fuse_opt_add_arg(&args, volnameOption);
    fuse_opt_add_arg(&args, "-o");
    fuse_opt_add_arg(&args, "fstypename=MFS");
    fuse_opt_add_arg(&args, "-o");
    fuse_opt_add_arg(&args, "local");
    #endif
    
    // run fuse
    ret = fuse_main(args.argc, args.argv, &fusemfs_ops, NULL);
    
    mfs_vclose(_fusemfs_vol);
    iconv_close(_fusemfs_iconv);
    fuse_opt_free_args(&args);
    return ret;
}

#if 0
#pragma mark -
#pragma mark FUSE Callbacks
#endif

static int fusemfs_getattr (const char *path, struct stat *stbuf) {
    MFSDirectoryRecord *rec;
    MFSFolder *folder;
    int ret = -ENOENT;
    char *fpath = utf8_to_mfs(path);
    char *fname = strrchr(fpath, ':')+1;
    int pathType;
    
    if (strcmp(path, "/") == 0) {
        // root dir info
        mfs_root_stat(&_fusemfs_vol->mdb, stbuf);
        ret = 0;
    } else if (rec = mfs_directory_find_name(_fusemfs_vol->directory, fname)) {
        // data fork
        if ((options.use_folders == 0) || (mfs_path_info(_fusemfs_vol, fpath) == kMFSPathFile)) {
            mfs_record_stat(&_fusemfs_vol->mdb, rec, stbuf, kMFSForkData);
            ret = 0;
        }
    } else if ((strncmp(fname, "._", 2) == 0) && (rec = mfs_directory_find_name(_fusemfs_vol->directory, fname+2))) {
        // AppleDouble headerfile
        if (options.use_folders) {
            memmove(fname, fname+2, strlen(fname)-2);
            fname[strlen(fname)-2] = '\0';
            pathType = mfs_path_info(_fusemfs_vol, fpath);
            memmove(fname+2, fname, strlen(fname));
            memcpy(fname, "._", 2);
        } else {
            pathType = kMFSPathFile;
        };
        if (pathType == kMFSPathFile) {
            mfs_record_stat(&_fusemfs_vol->mdb, rec, stbuf, kMFSForkAppleDouble);
            ret = 0;
        }
    } else if (options.use_folders && (mfs_path_info(_fusemfs_vol, fpath) == kMFSPathFolder)) {
        // folder
        folder = mfs_folder_find_name(_fusemfs_vol, fname);
        if (folder) {
            mfs_folder_stat(folder, stbuf);
            ret = 0;
        }
    }
    
    free(fpath);
    return ret;
}

static int fusemfs_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    MFSDirectoryRecord *rec;
    MFSFolder *dir, *subdir;
    struct stat stbuf;
    char fname[256];
    int16_t folderID;
    int i;
    
    if (strcmp(path, "/") == 0) {
        // root directory
        mfs_root_stat(&_fusemfs_vol->mdb, &stbuf);
        filler(buf, ".", &stbuf, 0);
        filler(buf, "..", NULL, 0);
        for(i=0; _fusemfs_vol->directory[i]; i++) {
            rec = _fusemfs_vol->directory[i];
            folderID = ntohs(rec->flUsrWds.folder);
            if (options.use_folders && 
                (folderID != kMFSFolderRoot) && 
                (folderID != kMFSFolderDesktop)) 
                // if using folders, skip files that aren't on the root
                // directory or on the desktop
                continue;
            
            // data file
            mfs_record_stat(&_fusemfs_vol->mdb, rec, &stbuf, kMFSForkData);
            mfs_to_utf8(rec->flCName, fname, sizeof fname);
            filler(buf, fname, &stbuf, 0);
            // AppleDouble headerfile
            char *appleDoubleName = malloc(strlen(fname)+3);
            strcpy(appleDoubleName, "._");
            strcat(appleDoubleName, fname);
            mfs_record_stat(&_fusemfs_vol->mdb, rec, &stbuf, kMFSForkAppleDouble);
            filler(buf, appleDoubleName, &stbuf, 0);
            free(appleDoubleName);
        }
        // subdirectories
        if (options.use_folders && (dir = mfs_folder_find(_fusemfs_vol, kMFSFolderRoot))) {
            for(i = 0; i < _fusemfs_vol->numFolders; i++) {
                subdir = &_fusemfs_vol->folders[i];
                if (subdir->fdID == kMFSFolderRoot) continue;
                if ((subdir->fdParent != kMFSFolderRoot) && (subdir->fdParent != kMFSFolderDesktop)) continue;
                // directories in root or desktop
                mfs_folder_stat(subdir, &stbuf);
                mfs_to_utf8(subdir->fdCNam, fname, sizeof fname);
                filler(buf, fname, &stbuf, 0);
            }
        }
    } else if (options.use_folders == 0) {
        return -ENOENT;
    } else {
        // a folder
        char *pathdup = strdup(path);
        char *dirname = utf8_to_mfs(basename(pathdup));
        dir = mfs_folder_find_name(_fusemfs_vol, dirname);
        free(pathdup); free(dirname);
        if (dir == NULL) return -ENOENT;
        // TODO check that the folder really exists at that path
        // by constructing folder path and comparing it with the asked one
        
        mfs_folder_stat(dir, &stbuf);
        filler(buf, ".", &stbuf, 0);
        filler(buf, "..", NULL, 0);
        
        // files
        for(i=0; _fusemfs_vol->directory[i]; i++) {
            rec = _fusemfs_vol->directory[i];
            folderID = ntohs(rec->flUsrWds.folder);
            if (folderID != dir->fdID) continue;
            
            // data file
            mfs_record_stat(&_fusemfs_vol->mdb, rec, &stbuf, kMFSForkData);
            mfs_to_utf8(rec->flCName, fname, sizeof fname);
            filler(buf, fname, &stbuf, 0);
            
            // AppleDouble headerfile
            char *appleDoubleName = malloc(strlen(fname)+3);
            strcpy(appleDoubleName, "._");
            strcat(appleDoubleName, fname);
            mfs_record_stat(&_fusemfs_vol->mdb, rec, &stbuf, kMFSForkAppleDouble);
            filler(buf, appleDoubleName, &stbuf, 0);
            free(appleDoubleName);
        }
        
        // subdirectories
        for(i = 0; i < _fusemfs_vol->numFolders; i++) {
            subdir = &_fusemfs_vol->folders[i];
            if (subdir->fdParent != dir->fdID) continue;
            mfs_folder_stat(subdir, &stbuf);
            mfs_to_utf8(subdir->fdCNam, fname, sizeof fname);
            filler(buf, fname, &stbuf, 0);
        }
    }
    
    return 0;
}

static int fusemfs_open (const char *path, struct fuse_file_info *fi) {
    // find record and fork
    // TODO check that path really exists if folders are on
    char *pathdup = strdup(path);
    char *fname = utf8_to_mfs(basename(pathdup));
    MFSDirectoryRecord *rec = mfs_directory_find_name(_fusemfs_vol->directory, fname);
    free(pathdup);
    int mode = kMFSForkData;
    if ((rec == NULL) && (strncmp(fname, "._", 2) == 0)) {
        // open as AppleDouble
        rec = mfs_directory_find_name(_fusemfs_vol->directory, fname+2);
        mode = kMFSForkAppleDouble;
    }
    free(fname);
    if (rec == NULL) return -ENOENT;
    
    // open
    MFSFork *fk = mfs_fkopen(_fusemfs_vol, rec, mode, 0);
    if (fk == NULL) return (errno*-1);
    fi->fh = (uint64_t)fk;
    
    return 0;
}

static int fusemfs_release (const char *path, struct fuse_file_info *fi) {
    MFSFork *fk = (MFSFork*)fi->fh;
    if (mfs_fkclose(fk) == -1) return (errno*-1);
    fi->fh = 0;
    return 0;
}

static int fusemfs_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    MFSFork *fk = (MFSFork*)fi->fh;
    return mfs_fkread_at(fk, size, (size_t)offset, (void*)buf);
}

static int fusemfs_statfs (const char *path, struct statvfs *stbuf) {
    bzero(stbuf, sizeof(struct statvfs));    
    stbuf->f_bsize = (unsigned long)_fusemfs_vol->mdb.drAlBlkSiz;
    stbuf->f_frsize = (unsigned long)_fusemfs_vol->mdb.drAlBlkSiz;
    stbuf->f_blocks = (fsblkcnt_t)_fusemfs_vol->mdb.drNmAlBlks;
    stbuf->f_bfree = stbuf->f_bavail = (fsblkcnt_t)_fusemfs_vol->mdb.drFreeBks;
    stbuf->f_files = (fsfilcnt_t)_fusemfs_vol->mdb.drNmFls;
    stbuf->f_namemax = 255;
    return 0;
}

#if 0
#pragma mark -
#pragma mark Glue
#endif

int mfs_record_stat (MFSMasterDirectoryBlock *mdb, MFSDirectoryRecord *rec, struct stat *stbuf, int mode) {
    bzero(stbuf, sizeof(struct stat));
    if (rec == NULL) return -1;
    stbuf->st_ino = rec->flFlNum;
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    if (mode == kMFSForkData) stbuf->st_size = rec->flLgLen;
    if (mode == kMFSForkRsrc) stbuf->st_size = rec->flRLgLen;
    if (mode == kMFSForkAppleDouble) stbuf->st_size = rec->flRLgLen + kAppleDoubleHeaderLength;
    stbuf->st_mtime = mfs_time(rec->flMdDat);
    return 0;
}

int mfs_root_stat (MFSMasterDirectoryBlock *mdb, struct stat *stbuf) {
    bzero(stbuf, sizeof(struct stat));
    if (options.use_folders) {
        MFSFolder *root = mfs_folder_find(_fusemfs_vol, kMFSFolderRoot);
        stbuf->st_nlink = root->fdSubdirs + 2;
    } else stbuf->st_nlink = 2;
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_mtime = mfs_time(mdb->drCrDate);
    stbuf->st_size = mdb->drAlBlkSiz * mdb->drNmAlBlks;
    stbuf->st_blksize = mdb->drAlBlkSiz;
    stbuf->st_blocks = mdb->drNmAlBlks - mdb->drFreeBks;
    return 0;
}

int mfs_folder_stat (MFSFolder *folder, struct stat *stbuf) {
    bzero(stbuf, sizeof(struct stat));
    stbuf->st_nlink = folder->fdSubdirs + 2;
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_mtime = mfs_time(folder->fdMdDat);
    return 0;
}
