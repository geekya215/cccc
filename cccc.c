#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define int long

int token,
    poolsize,
    line,
    *text,
    *stack;

int tval,
    *current_id,
    *symbols;

int *pc, *bp, *sp, ax, cycle; // vm registers

char *src,
     *data;

// instructions
enum { 
  LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
  OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
  OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT
};

// tokens and classes (operators last and in precedence order)
enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// fields of identifier
enum { Token, Hash, Name, Type, Class, Value, GType, GClass, GValue, IdSize };

enum { CHAR, INT, PTR };

void next() {
  char *pos;
  int hash;
  while (token = *src) {
    ++src;

    // skip new line
    if (token == '\n') {
      ++line;
    }
    // skip marco
    else if (token == '#') {
      while (*src != 0 && *src != '\n') {
        src++;
      }
    }
    // parse identifier
    else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {
      pos = src - 1;
      hash = token;

      while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
        hash = hash * 147 + *src;
        src++;
      }

      current_id = symbols;
      while (current_id[Token]) {
        if (current_id[Hash] == hash && !memcmp((char *)current_id[Name], pos, src - pos)) {
          token = current_id[Token];
          return;
        }
        current_id = current_id + IdSize;
      }

      current_id[Token] = (int)pos;
      current_id[Hash] = hash;
      token = current_id[Token] = Id;
      return;
    }
    // parse number(only support dec)
    else if (token >= '0' && token <= '9') {
      tval = token - '0';
      while (*src >= '0' && *src <= '9') {
        tval = tval * 10 + (*src++ - '0');
      }
      token = Num;
      return;
    }
    // parse string or char
    else if (token == '"' || token == '\'') {
      pos = data;
      while (*src != 0 && *src != token) {
        tval = *src++;
        if (tval == '\\') {
          // escape character
          tval = *src++;
          if (tval == 'n') {
            tval = '\n';
          }
        }
        if (token == '"') {
          *data++ = tval;
        }
      }
      src++;
      // if it is a single character, return Num token
      if (token == '"') {
        tval = (int)pos;
      } else {
        token = Num;
      }
      return;
    }
    else if (token == '/') {
      // parse comment
      if (*src == '/') {
        while (*src != 0 && *src != '\n') {
          src++;
        }
      } else {
        // parse divide
        token = Div;
        return;
      }
    }
    else if (token == '=') {
      // parse '==' and '='
      if (*src == '=') {
        src ++;
        token = Eq;
      } else {
        token = Assign;
      }
      return;
    }
    else if (token == '+') {
      // parse '+' and '++'
      if (*src == '+') {
        src ++;
        token = Inc;
      } else {
        token = Add;
      }
      return;
    }
    else if (token == '-') {
      // parse '-' and '--'
      if (*src == '-') {
        src ++;
        token = Dec;
      } else {
        token = Sub;
      }
      return;
    }
    else if (token == '!') {
      // parse '!='
      if (*src == '=') {
        src++;
        token = Ne;
      }
      return;
    }
    else if (token == '<') {
      // parse '<=', '<<' or '<'
      if (*src == '=') {
        src ++;
        token = Le;
      } else if (*src == '<') {
        src ++;
        token = Shl;
      } else {
        token = Lt;
      }
      return;
    }
    else if (token == '>') {
      // parse '>=', '>>' or '>'
      if (*src == '=') {
        src ++;
        token = Ge;
      } else if (*src == '>') {
        src ++;
        token = Shr;
      } else {
        token = Gt;
      }
      return;
    }
    else if (token == '|') {
      // parse '|' or '||'
      if (*src == '|') {
        src ++;
        token = Lor;
      } else {
        token = Or;
      }
      return;
    }
    else if (token == '&') {
      // parse '&' and '&&'
      if (*src == '&') {
        src ++;
        token = Lan;
      } else {
        token = And;
      }
      return;
    }
    else if (token == '^') {
      token = Xor;
      return;
    }
    else if (token == '%') {
      token = Mod;
      return;
    }
    else if (token == '*') {
      token = Mul;
      return;
    }
    else if (token == '[') {
      token = Brak;
      return;
    }
    else if (token == '?') {
      token = Cond;
      return;
    }
    else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
      // directly return the character as token;
      return;
    }
  }
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
  int op, *tmp;
  while (1) {
    op = *pc++; // get operation code

    if (op == IMM)       { ax = *pc++; }                                     // load immediate value to ax
    else if (op == LC)   { ax = *(char *)ax; }                               // load character to ax, address in ax
    else if (op == LI)   { ax = *(int *)ax; }                                // load integer to ax, address in ax
    else if (op == SC)   { *(char *)*sp++ = ax; }                            // save character to address, value in ax, address on stack
    else if (op == SI)   { *(int *)*sp++ = ax; }                             // save integer to address, value in ax, address on stack
    else if (op == PUSH) { *--sp = ax; }                                     // push the value of ax onto the stack
    else if (op == JMP)  { pc = (int *)*pc; }                                // jump to the address
    else if (op == JZ)   { pc = ax ? pc + 1 : (int *)*pc; }                  // jump if ax is zero
    else if (op == JNZ)  { pc = ax ? (int *)*pc : pc + 1; }                  // jump if ax is not zero
    else if (op == CALL) { *--sp = (int)(pc+1); pc = (int *)*pc; }           // call subroutine
    // else if (op == RET)  { pc = (int *)*sp++; }                           // return from subroutine
    else if (op == ENT)  { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }      // make new stack frame
    else if (op == ADJ)  { sp = sp + *pc++; }                                // add esp, <size>
    else if (op == LEV)  { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; }  // restore call frame and PC
    else if (op == ENT)  { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }      // make new stack frame
    else if (op == ADJ)  { sp = sp + *pc++; }                                // add esp, <size>
    else if (op == LEV)  { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; }  // restore call frame and PC
    else if (op == LEA)  { ax = (int)(bp + *pc++); }                         // load address for arguments

    else if (op == OR)  ax = *sp++ | ax;
    else if (op == XOR) ax = *sp++ ^ ax;
    else if (op == AND) ax = *sp++ & ax;
    else if (op == EQ)  ax = *sp++ == ax;
    else if (op == NE)  ax = *sp++ != ax;
    else if (op == LT)  ax = *sp++ < ax;
    else if (op == LE)  ax = *sp++ <= ax;
    else if (op == GT)  ax = *sp++ >  ax;
    else if (op == GE)  ax = *sp++ >= ax;
    else if (op == SHL) ax = *sp++ << ax;
    else if (op == SHR) ax = *sp++ >> ax;
    else if (op == ADD) ax = *sp++ + ax;
    else if (op == SUB) ax = *sp++ - ax;
    else if (op == MUL) ax = *sp++ * ax;
    else if (op == DIV) ax = *sp++ / ax;
    else if (op == MOD) ax = *sp++ % ax;

    else if (op == EXIT) { printf("exit(%d)", *sp); return *sp; }
    else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
    else if (op == CLOS) { ax = close(*sp); }
    else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
    else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
    else if (op == MALC) { ax = (int)malloc(*sp); }
    else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp); }
    else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp); }
    else {
        printf("unknown instruction:%d\n", op);
        return -1;
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int i, fd;

  // initial 
  poolsize = 256 * 1024;
  line = 1;
  argc--, argv++;

  // allocate memory for vm
  if (!(text = malloc(poolsize))) {
    printf("could not malloc %d bytes memory for text area\n", poolsize);
    return -1;
  }

  if (!(data = malloc(poolsize))) {
    printf("could not malloc %d bytes memory for data area\n", poolsize);
    return -1;
  }

  if (!(stack = malloc(poolsize))) {
    printf("could not malloc %d bytes memory for stack area\n", poolsize);
    return -1;
  }
  if (!(symbols = malloc(poolsize))) {
    printf("could not malloc(%d) for symbol table\n", poolsize);
    return -1;
  }

  // initial vm
  memset(text, 0, poolsize);
  memset(data, 0, poolsize);
  memset(stack, 0, poolsize);
  memset(symbols, 0, poolsize);
  bp = sp = (int *)((int)stack + poolsize);
  ax = 0;

  // create fd
  if ((fd = open(*argv, 0)) < 0) {
    printf("could not open %s\n", *argv);
    return -1;
  }
  // allocate memory for source code
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

  // simulate 10 * 20 - 100
  i = 0;
  text[i++] = IMM;
  text[i++] = 10;
  text[i++] = PUSH;
  text[i++] = IMM;
  text[i++] = 20;
  text[i++] = MUL;
  text[i++] = PUSH;
  text[i++] = IMM;
  text[i++] = 100;
  text[i++] = SUB;
  text[i++] = PUSH;
  text[i++] = EXIT;
  pc = text;

  program();
  return eval();
}
