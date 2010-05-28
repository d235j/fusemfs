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

#ifndef _FUSEMFS_H_
#define _FUSEMFS_H_

#define FUSEMFS_VERSION "0.1"
#define FUSE_USE_VERSION  26

#include <sys/time.h>
#include <fuse.h>
#include <fcntl.h>
#include <stdint.h>
#include <libmfs/mfs.h>

// FUSE callbacks
static int fusemfs_getattr (const char *path, struct stat *stbuf);
static int fusemfs_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int fusemfs_open (const char *path, struct fuse_file_info *fi);
static int fusemfs_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fusemfs_release (const char *path, struct fuse_file_info *);
static int fusemfs_statfs (const char *path, struct statvfs *buf);
static void * fusemfs_init (struct fuse_conn_info *conn);

// glue
int mfs_record_stat (MFSMasterDirectoryBlock *mdb, MFSDirectoryRecord *rec, struct stat *stbuf, int mode);
int mfs_root_stat (MFSMasterDirectoryBlock *mdb, struct stat *stbuf);
int mfs_folder_stat (MFSFolder *folder, struct stat *stbuf);

#endif /* _FUSEMFS_H_ */
