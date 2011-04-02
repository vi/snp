/**
 * Implemented by Vitaly "_Vi" Shukela in year 2011
 * MIT license
 *
 * Compresed file format:
 * 	(Block_length Compressed_block)* Zero_byte
 *	Block length is either one byte <128 if compressed block length is less than 128
 *	or big and then little parts of (compressed_block_length|0x8000)
 *	\x40 - compressed block length is 0x40
 *	\x81\xFE - compressed block length is 0x01FE
 *
 *	Compressed block is compressed using Snappy library
 */


#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>


#include <snappy.h>        
#include <snappy-sinksource.h>

#define BUFSIZE 0x6D80

class FdSource : public snappy::Source {
 public:
  FdSource(int fd) : fd(fd), len(0), cursor(buf) {
    readMore();
  }
  virtual size_t Available() const {
    return len;
  }
  virtual const char* Peek(size_t* len) {
    *len = this->len;
    return cursor;  
  }
  virtual void Skip(size_t n) {
      cursor+=n;
      len-=n;
  }
 private:
  int fd;
  char buf[BUFSIZE];
  int len;
  char* cursor;

  void readMore() {
    reread:
    len = read(fd, buf, BUFSIZE);
    if (len==-1) {
       if (errno == EAGAIN || errno == EINTR) {
	   goto reread;
       }
       perror("read");
       exit(1);
    }
  }
};

int main(int argc, char* argv[]) {
    if(argc>2 || (argc==2 && strcmp(argv[1],"-d"))){
	printf("Only \"snp\" to encode and \"snp -d\" to decode.\n");
	return 2;
    }

    char compressed[BUFSIZE*2]; // 32 + source_len + source_len/6
    if(argc==2) {
	// "snp -d" - decompressing

	for(;;) {
	    size_t len;
	    
	    {
		unsigned char l1;
		unsigned char l2;
		l1 = fgetc(stdin);
		if(l1<128) {
		    len = l1;
		} else {
		    l2 = fgetc(stdin);
		    len = l2 + 0x100*(l1&0x7F);
		}
	    }

	    if(len==0) {
		break;
	    }

	    ssize_t ret;
	    ret = fread(compressed, 1, len, stdin);
	    if(ret!=len) {
		fprintf(stderr, "Chopped input data\n");
		exit(3);
	    }

	    {
		char decompressed[BUFSIZE];
		size_t uc_length;
		char* decompressed2 = decompressed;

		if (!snappy::GetUncompressedLength(compressed, len, &uc_length)) {
		    fprintf(stderr, "Invalid input data\n");
		    exit(3);
		}

		if (uc_length > BUFSIZE) {
		    decompressed2 = (char*)malloc(uc_length);
		}
		
		if(!snappy::RawUncompress(compressed, len, decompressed2)) {
		    fprintf(stderr, "Invalid input date 2\n");
		    exit(3);
		}

		fwrite(decompressed2, 1, uc_length, stdout);

		if (uc_length > BUFSIZE) {
		    free(decompressed2);
		}
	    }

	}
    } else {
	// "snp" - compressing

	for(;;) {
	    FdSource so(0);
	    if(!so.Available()) {
		break;
	    }
	    snappy::UncheckedByteArraySink si(compressed);
	    size_t len = snappy::Compress(&so, &si);

	    if(len<128) {
		unsigned char l=len;
		fputc(l, stdout);
	    } else
	    if(len>=128 || len<0x7FFF) {
		unsigned char l1=(len/0x100) | 0x80;
		unsigned char l2=len&0xFF;
		fputc(l1, stdout);
		fputc(l2, stdout);
	    } else {
		fprintf(stderr, "error: compressed size too big. Miscalculation of input buffer size.\n");
		exit(3);
	    }

	    fwrite(compressed, 1, len, stdout);
	}
	fputc(0, stdout);
    }
}
