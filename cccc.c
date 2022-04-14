#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int token,
    poolsize,
    line;
char *src;

enum {
  IMM ,LEA ,LI  ,SI  ,LC  ,SC  ,PUSH,
  CALL,JMP ,JZ  ,JNZ ,ENT ,ADJ ,LEV ,
  ADD ,SUB ,MUL ,DIV ,MOD ,SHL ,SHR ,AND ,OR  ,XOR ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,
  OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT
};

enum {
  CHAR, INT, PTR
};

void next() {
  token = *src++;
}

void expression(int level) {
}

void program() {
  next();
  while (token > 0) {
    printf("read token: %c\n", token);
    next();
  }
}

int eval() {
  return 0;
}

int main(int argc, char *argv[]) {
  int i, fd;

  // initial 
  poolsize = 256 * 1024;
  line = 1;
  argc--, argv++;

  // create fd
  if ((fd = open(*argv, 0)) < 0) {
    printf("could not open %s\n", *argv);
    return -1;
  }

  // malloc memory
  if (!(src = malloc(poolsize))) {
    printf("could not malloc %d bytes memory\n", poolsize);
    return -1;
  }

  // read source code to buffer
  if ((i = read(fd, src, poolsize - 1)) <= 0) {
    printf("load %d bytes to memory\n", i);
    return -1;
  }

  src[i] = 0;
  close(fd);

  program();
  return eval();
}
