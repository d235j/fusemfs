/*
 *  util.c
 *  FuseMFS
 *
 *  Created by Zydeco on 28/5/2010.
 *  Copyright 2010 namedfork.net. All rights reserved.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libmfs/mfs.h>
#include <sys/loadable_fs.h>
#include <sys/stat.h>
#include <iconv.h>
#include <sys/errno.h>

int usage() {
	fprintf(stderr, "usage: fusemfs.util [-p|-m] <options>\n");
	return EXIT_FAILURE;
}

int have_macfuse() {
	struct stat us;
	return (stat("/usr/local/lib/libfuse_ino64.dylib", &us) == 0);
}

int probe (const char *device, int removable, int readonly) {
	char *path;
	asprintf(&path, "/dev/%s", device);
	MFSVolume *vol = mfs_vopen(path, 0, 0);
	free(path);
	if (vol == NULL) return FSUR_UNRECOGNIZED;
	
	printf("%s\n", vol->name); // TODO: convert to UTF8
	
	mfs_vclose(vol);
	return FSUR_RECOGNIZED;
}


int mount (const char *device, const char *mountpoint) {
	char *cmd;
	asprintf(&cmd, "/System/Library/Filesystems/fusefs_mfs.fs/Contents/Resources/fusemfs \"%s\" \"%s\"", device, mountpoint);
	int ret = system(cmd);
	free(cmd);
	return ret?FSUR_IO_FAIL:FSUR_IO_SUCCESS;
}

int main (int argc, char * argv[], char * envp[], char * apple[]) {	
	// check arguments
	if (argc < 3) return usage();
#ifdef DEBUG
	freopen("/mfs.util.log", "a", stderr);
	fprintf(stderr, "mfs.util %d\n", getpid());
	for(int i=0; i<argc; i++) fprintf(stderr, "%d: %s\n", i, argv[i]);
	fflush(stderr);
#endif
	int ret = 0;
	switch (argv[1][1]) {
		case FSUC_PROBE:
		case FSUC_PROBEFORINIT:
			ret = probe(argv[2], !strcmp(argv[3],DEVICE_REMOVABLE), !strcmp(argv[4],DEVICE_READONLY));
			break;
		case FSUC_MOUNT:
		case FSUC_MOUNT_FORCE:
			ret = mount(argv[argc-2], argv[argc-1]);
			break;
		case 'q': // quick verify
			printf("No verification done, assuming it's ok\n");
			ret = EXIT_SUCCESS;
			break;
		case FSUC_INITIALIZE:
		case FSUC_REPAIR:
		case 'v': // verify
		case 'k': // get UUID
		case 's': // set UUID
		default:
			ret = FSUR_INVAL;
			break;
	}
	return ret;
}
