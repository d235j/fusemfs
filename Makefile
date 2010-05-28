PROD = fusemfs

CC = gcc
AR = ar
LIBMFS_DIR = libmfs
LIBRES_DIR = libres
ARCH = -arch i386 -arch ppc
CFLAGS = $(ARCH) -D__FreeBSD__=10 -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse -std=c99 -I. -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-multichar
LDFLAGS = -L/usr/lib -liconv -L/usr/local/lib -lfuse_ino64 -L$(LIBMFS_DIR) -L$(LIBRES_DIR) -lmfs -lres

all: fusemfs.c fusemfs.h libmfs-lib libres-lib
	$(CC) $(CFLAGS) fusemfs.c $(LDFLAGS) -o $(PROD)

libmfs-lib:
	@make -eC $(LIBMFS_DIR)

libres-lib:
	@make -eC $(LIBRES_DIR)

clean:
	@echo Cleaning libmfs
	@make -eC $(LIBMFS_DIR) clean
	@echo Cleaning libres
	@make -eC $(LIBRES_DIR) clean
	@echo Cleaning fusemfs
	rm -rf $(PROD) $(PROD).dSYM