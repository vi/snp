#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>


#include <snappy.h>        
#include <snappy-sinksource.h>

#define BUFSIZE 65536

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

class FdSink : public snappy::Sink {
 public:
  FdSink(int fd): fd(fd) { }
  virtual void Append(const char* data, size_t n) {
    int ret;
    while(n) {
	again:
	ret = write(fd, data, n);
	if(ret==-1) {
	    if (errno == EAGAIN || errno == EINTR) {
		goto again;
	    }
	    perror("write");
	    exit(3);
	}
	n-=ret;
    }
  }
  virtual char* GetAppendBuffer(size_t len, char* scratch) {
      return scratch;
  }

 private:
  int fd;
};

int main(int argc, char* argv[]) {
    if(argc>3 || (argc==2 && strcmp(argv[1],"-d"))){
	printf("Only \"snappy\" to encode and \"snappy -d\" to decode.\n");
	return 2;
    }

    if(argc==3) {
	// snappy -d
	FdSink si(1);

	for(;;) {
	    FdSource so(0);
	    if(!so.Available()) {
		break;
	    }
	    snappy::Uncompress(&so, &si);
	}
    } else {
	FdSink si(1);

	for(;;) {
	    FdSource so(0);
	    if(!so.Available()) {
		break;
	    }
	    snappy::Compress(&so, &si);
	}
    }
}
