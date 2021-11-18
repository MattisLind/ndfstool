/* Tiny program that parses the ND BPUN fromat */
/* Mattis Lind (C) 2021 mattis@datormuseum.se */
/* This software may be used, modified, distributed in anyway as long ast to above copyright is not changed */

#include <stdio.h>
#include <stdlib.h>

int main (int argc, char ** argv) {
  FILE * input;
  int blockStart;
  int wordCount; 
  unsigned int data;
  int ch; 
  char  action;
  int sum=0, checksum;
  int state=0;
  if (argc != 2) {
    fprintf (stderr, "Need at least one argument.");
    exit(1);
  }
  input = fopen (argv[1], "rb");
  while (EOF != (ch = fgetc(input))) {
    //fprintf (stderr, "state=%d  ch=%02X\n ", state, 0xff & ch);
    switch (state) {
    case 0:
      switch (ch) {
      case '!':
	state = 1;
	break;
      case 0:
	break;
      default:
	fputc(ch & 0x7f, stdout);
	break;
      }
      break;
    case 1:
      blockStart = (0xff & ch) << 8;
      state = 2;
      break;
    case 2:
      blockStart |= (ch & 0xff)  ;
      fprintf(stdout, "\nblockStart=%04X\n", blockStart);
      state = 3;
      break;
    case 3:
      wordCount = (ch & 0xff) << 8;
      state = 4;
      break;
    case 4:
      wordCount |= (ch & 0xff);
      state = 5;
      fprintf(stdout, "wordCount=%d\n", wordCount);
      break;
    case 5:
      data = (ch & 0xff) << 8;
      state =6;
      break;
    case 6:
      data |= (ch & 0xff);
      fprintf (stdout, "data=%04X\n", 0xffff & data);
      //fprintf (stdout, "wordcount=%d\n", wordCount);
      wordCount--;
      sum += data;
      if (wordCount == 0) state = 7;
      else state = 5;
      break;
    case 7:
      checksum = ch << 8;
      state = 8;
      break;
    case 8:
      checksum |= ch;
      state = 9;
      fprintf (stdout, "checksum=%04X sum=%04X \n", 0xffff & checksum, 0xffff & sum);
      break;
    case 9: 
      action = ch;
      fprintf (stdout, "action=%02X\n", action);
      state = 10;
      break;
    case 10:
      break;
    }
  }
  return 0;
}
