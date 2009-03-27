PROD = fusemfs

CC = gcc
AR = ar
LIBMFS_DIR = libmfs
LIBRES_DIR = libres
CFLAGS = `pkg-config fuse --cflags` -std=c99 -I. -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-multichar
LDFLAGS = `pkg-config fuse --libs` $(LIBMFS_DIR)/mfs.o $(LIBRES_DIR)/res.o

all: fusemfs.c fusemfs.h libmfs-lib libres-lib
	$(CC) $(CFLAGS) $(LDFLAGS) fusemfs.c -o $(PROD)

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