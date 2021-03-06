/**
  *********************************************************************************************************
  * @file    main.c
  * @author  Movebroad -- KK
  * @version V1.0
  * @date    2018-12-24
  * @brief   1TAB = 5Speace
  *********************************************************************************************************
  * @attention
  *
  *
  *
  *********************************************************************************************************
  */

#include "bspatch.h"

#include <bzlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

//#define	DEBUG

static int64_t offtin(uint8_t *buf)
{
	int64_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

static int32_t offtin_4byte(uint8_t *buf)
{
	int32_t y;

	y=buf[3]&0x7F;
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[3]&0x80) y=-y;

	return y;
}

int bspatch(const uint8_t* old, int64_t oldsize, uint8_t* new, int64_t newsize, struct bspatch_stream* stream)
{
	uint8_t buf[4];
	int64_t oldpos,newpos;
	int64_t ctrl[3];
	int64_t i;

	oldpos=0;newpos=0;
	while(newpos<newsize) {
		/* Read control data */
		for(i=0;i<=2;i++) {
			if (stream->read(stream, buf, 4))
				return -1;
			ctrl[i]=offtin_4byte(buf);
		};

		/* Sanity-check */
		if(newpos+ctrl[0]>newsize)
			return -1;

		/* Read diff string */
		if (stream->read(stream, new + newpos, ctrl[0]))
			return -1;

		/* Add old data to diff string */
		for(i=0;i<ctrl[0];i++)
			if((oldpos+i>=0) && (oldpos+i<oldsize))
				new[newpos+i]+=old[oldpos+i];

		/* Adjust pointers */
		newpos+=ctrl[0];
		oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>newsize)
			return -1;

		/* Read extra string */
		if (stream->read(stream, new + newpos, ctrl[1]))
			return -1;

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

	return 0;
}

static int bz2_read(const struct bspatch_stream* stream, void* buffer, int length)
{
#if 0

	int n;
	int bz2err;
	BZFILE* bz2;

	bz2 = (BZFILE*)stream->opaque;
	n = BZ2_bzRead(&bz2err, bz2, buffer, length);
	if (n != length)
		return -1;

	return 0;

#else

	int n;
	FILE* f;

	f = (FILE*)stream->opaque;
	n = fread(buffer, 1, length, f);
	if (n != length)
		return -1;

	return 0;

#endif
}

/**********************************************************************************************************
 @Function			int main(int argc, char const *argv[])
 @Description			Main
 @Input				void
 @Return				int
**********************************************************************************************************/
int main(int argc, char const *argv[])
{
	FILE * f;
	int fd;
	int bz2err;
	uint8_t header[24];
	uint8_t *old, *new, *patchold, *patchnew;
	int64_t oldsize, newsize, patcholdsize, patchnewsize;
	BZFILE* bz2;
	struct bspatch_stream stream;
	struct stat sb;

	uint8_t decompressState = 0;
	uint16_t decompressZeroNum = 0;

	if(argc!=4) errx(1,"usage: %s oldfile newfile patchfile\n",argv[0]);

#if 1

	/* 私有解压算法 */
	/* step1 : 以读取方式打开patch文件,读取patch文件内容,并存在申请的patch内存中 */
	if ( ((fd = open(argv[3], O_RDONLY, 0)) < 0) ||
		((patcholdsize = lseek(fd, 0, SEEK_END)) == -1) ||
		((patchold = malloc(patcholdsize + 1)) == NULL) ||
		(lseek(fd, 0, SEEK_SET) != 0) ||
		(read(fd, patchold, patcholdsize) != patcholdsize) ||
		(close(fd) == -1) ) err(1, "%s", argv[3]);

#if 0
	printf("\r\npatcholdsize : %d\r\n", patcholdsize);
	for (int i = 0; i < patcholdsize; i++) {
		printf("%02X ", patchold[i]);
	}
	printf("\r\n\r\n");
#endif

	/* step2 : 创建patchnew内存,用来存放压缩结果 */
	if ((patchnew = malloc(512 * 1024)) == NULL) err(1, "patchnew");

	/* step3 : 解缩算法处理 */
	patchnewsize = 0;
	for (int64_t offset = 0; offset < patcholdsize; offset++) {

		if (decompressState == 0) {
			/* 不为0保留值 */
			if (patchold[offset] != 0x00) {
				patchnew[patchnewsize] = patchold[offset];
				patchnewsize++;
				continue;
			}
		}
		else {
			decompressZeroNum = patchold[offset];
			for (int64_t zeroNum = 0; zeroNum < decompressZeroNum; zeroNum++) {
				patchnew[patchnewsize++] = 0x00;
			}
			decompressState = 0;
			continue;
		}

		/* 解压0 */
		decompressState = 1;

	}

#if 0
	printf("\r\npatchnewsize : %d\r\n", patchnewsize);
	for (int i = 0; i < patchnewsize; i++) {
		printf("%02X ", patchnew[i]);
	}
	printf("\r\n\r\n");
#endif

	/* step4 : 压缩结果写入patch文件 */
	if ((f = fopen(argv[3], "w")) == NULL)
		err(1, "%s", argv[3]);
	
	if (fwrite(patchnew, patchnewsize, 1, f) != 1)
		err(1, "Failed to write patchnew");

	if (fclose(f))
		err(1, "fclose");

#endif

	/* Open patch file */
	if ((f = fopen(argv[3], "r")) == NULL)
		err(1, "fopen(%s)", argv[3]);

	/* Read header */
	if (fread(header, 1, 24, f) != 24) {
		if (feof(f))
			errx(1, "Corrupt patch\n");
		err(1, "fread(%s)", argv[3]);
	}

	/* Check for appropriate magic */
	if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0)
		errx(1, "Corrupt patch\n");

	/* Read lengths from header */
	newsize=offtin(header+16);
	if(newsize<0)
		errx(1,"Corrupt patch\n");

	/* Close patch file and re-open it via libbzip2 at the right places */
	if(((fd=open(argv[1],O_RDONLY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(fstat(fd, &sb)) ||
		(close(fd)==-1)) err(1,"%s",argv[1]);
	if((new=malloc(newsize+1))==NULL) err(1,NULL);


#if 0

	if (NULL == (bz2 = BZ2_bzReadOpen(&bz2err, f, 0, 0, NULL, 0)))
		errx(1, "BZ2_bzReadOpen, bz2err=%d", bz2err);

	stream.read = bz2_read;
	stream.opaque = bz2;
	if (bspatch(old, oldsize, new, newsize, &stream))
		errx(1, "bspatch");

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&bz2err, bz2);

#else

	stream.read = bz2_read;
	stream.opaque = f;
	if (bspatch(old, oldsize, new, newsize, &stream))
		errx(1, "bspatch");

#endif


	fclose(f);

	/* Write the new file */
	if(((fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,sb.st_mode))<0) ||
		(write(fd,new,newsize)!=newsize) || (close(fd)==-1))
		err(1,"%s",argv[2]);

	free(new);
	free(old);

#if 1

	/* step5 : 还原patch文件 */
	if ((f = fopen(argv[3], "w")) == NULL)
		err(1, "%s", argv[3]);
	
	if (fwrite(patchold, patcholdsize, 1, f) != 1)
		err(1, "Failed to write patchnew");

	if (fclose(f))
		err(1, "fclose");

	free(patchold);
	free(patchnew);

#endif

	return 0;
}

/********************************************** END OF FLEE **********************************************/
