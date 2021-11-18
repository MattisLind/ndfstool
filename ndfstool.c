/* Tiny program that extracts files fomr ND file system disk images */
/* Mattis Lind (C) 2021 mattis@datormuseum.se */
/* This software may be used, modified, distributed in anyway as long ast to above copyright is not changed */


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct blockPointer {
  int pointer;
  int indexType;
};


int getBlockPointer (FILE * input, struct blockPointer * bp) {
  int ret;
  int tmp;
  ret = fgetc(input);
  if (ret == EOF) {
    return -1;
  }
  switch (0xC0 & ret) {
  case 0x00:
    bp->indexType=0;
    break;
  case 0x40:
    bp->indexType=1;
    break;
  case 0x80:
    bp->indexType=2;
    break;
  case 0xC0:
    return -1;
    break;
  }
  bp->pointer = (ret & 0x3F) << 24;
  ret = fgetc(input);
  if (ret == EOF) {
    return -1;
  }
  bp->pointer |= ret << 16;
  ret = fgetc(input);
  if (ret == EOF) {
    return -1;
  }
  bp->pointer |= ret << 8;
  ret = fgetc(input);
  if (ret == EOF) {
    return -1;
  }
  bp->pointer |= ret;
  return 0;
}

int getLongWord (FILE * input, int * value) {
  int ret;
  int tmp;
  ret = fgetc(input);
  if (ret == EOF) {
    return -1;
  }
  *value = ret  << 24;
  ret = fgetc(input);
  if (ret == EOF) {
    return -1;
  }
  *value |= ret << 16;
  ret = fgetc(input);
  if (ret == EOF) {
    return -1;
  }
  *value |= ret << 8;
  ret = fgetc(input);
  if (ret == EOF) {
    return -1;
  }
  *value |= ret;
  return 0;
}


int parseUserFile (FILE * input) {
  int enterCount = fgetc(input);
  char userName [17];
  char * ret2;
  if (enterCount == EOF) return -1;
  fprintf (stdout, "Enter count: %d\n", enterCount);
  ret2 = fgets (userName, 17, input);  
  if (ret2 == NULL) return -2;
  fprintf (stdout, "User name: %s\n", userName);
  return 0;
}

int readContigousFile (FILE * input, struct blockPointer fp, int numPagesInFile, int maxBytePointer, FILE * output) {
  char buffer [2048];
  int bytesToRead = MIN (2048, maxBytePointer+1);
  int ret;
  fprintf (stderr, "ENTRY readContiguosFile BlockPointer = %08X, maxBytePointer = %d\n", fp.pointer, maxBytePointer); 
  ret = fseek(input, fp.pointer * 2048, SEEK_SET);
  if (ret == -1) return -1;
  fread(buffer, 1, bytesToRead, input);
  fwrite(buffer, 1, bytesToRead, output);
  bytesToRead -= 2048;
  fp.pointer += 2048;
  if (bytesToRead <= 0) return 0;
  else readContigousFile (input, fp, numPagesInFile, bytesToRead, output); 
  return 0;
}

int readIndexedFile (FILE * input, struct blockPointer fp, int numPagesInFile, int maxBytePointer, FILE * output) {
  struct blockPointer np;
  int ret;
  int n=0;
  fprintf (stderr, "ENTRY readIndexedFile BlockPointer = %08X, maxBytePointer = %d\n", fp.pointer, maxBytePointer); 
  do {
    ret = fseek(input, fp.pointer * 2048 + n * 4, SEEK_SET);
    n++;
    getBlockPointer(input, &np);
    if (np.indexType == 0) {
      readContigousFile (input, np, numPagesInFile, MIN(maxBytePointer,2048),  output);
      maxBytePointer = maxBytePointer - 2048;
    } else {
      readIndexedFile (input, np, numPagesInFile, maxBytePointer, output);
    }
    fprintf (stderr, "LOOP readIndexedFile np.pointer = %08X maxBytePointer = %d\n", np.pointer, maxBytePointer );
  } while ( maxBytePointer > 0);
  return maxBytePointer;
}



int parseObjectFile (FILE * input) {
  FILE * output;
  int enterCount = fgetc(input);
  char userName [30];
  char type[5];
  char * ret2;
  int ret;
  int numPagesInFile;
  int maxBytePointer;
  struct blockPointer fp;
  if (enterCount == EOF) return -1;
  ret2 = fgets (userName, 17, input);  
  if (ret2 == NULL) return -2;
  fprintf (stdout, "Object name: %s\n", userName);
  ret2 = fgets (type, 5, input);  
  if (ret2 == NULL) return -2;
  fprintf (stdout, "Type: %s\n", type);
  ret = fseek (input, 30, SEEK_CUR);
  if (getLongWord (input, &numPagesInFile) < 0) return -7;
  fprintf(stdout, "# pages in file : %d\n", numPagesInFile );
  if (getLongWord (input, &maxBytePointer) < 0) return -7;
  fprintf(stdout, "max byte pointer : %d\n", maxBytePointer );
  if (getBlockPointer (input, &fp) < 0) return -7;
  fprintf(stdout, "File pointer type %d block %08X byte position %08X\n", fp.indexType, fp.pointer, fp.pointer * 2048 );
  ret2 = strchr(userName,'\'');
  if (ret2 != NULL) *ret2 = 0;
  strcat(userName,".");
  strcat(userName,type);
  fprintf (stdout, "File name: %s\n", userName);
  output = fopen (userName, "wb");
  if (output == NULL) return -9;
  if (fp.indexType == 0) {
    // Coniguous file
    readContigousFile (input, fp, numPagesInFile, maxBytePointer, output);
  } else {
    // Indexed file
    readIndexedFile (input, fp, numPagesInFile, maxBytePointer, output);
  }
  fclose(output);
  return 0;
}


int readInfo(FILE * input, struct blockPointer bp) {
  struct blockPointer newPointer;
  int n=0;
  int ret;
  char * ret2;
  ret = fseek (input, bp.pointer * 2048, SEEK_SET);
  if (ret == -1) return -1;
  if (bp.indexType != 0) {
    do {    
      ret = getBlockPointer(input, &newPointer);
      if (newPointer.pointer != 0) readInfo (input,newPointer);
    } while (newPointer.pointer !=0);
  } else {
    do {
      ret = fseek (input, bp.pointer * 2048 + n * 64, SEEK_SET);
      if (ret == -1) return -3;
      n++;
      ret = fgetc(input);
      if (ret == EOF) return -2;
      if (ret & 0x80) {
	if (ret & 0x1) {
	  if (parseUserFile (input) < 0) return -6;
	} else {
	  if (parseObjectFile (input) < 0) return -7;
	}
      } 
    } while (n < 32);
  }
  return 0;
}


int main (int argc, char ** argv) {
  FILE * input;
  int ret;
  char * ret2;
  char name [17];
  struct blockPointer userPointer, objectPointer, bitFilePointer;
  if (argc != 2) {
    fprintf (stderr, "Need one arument - the disk image file to process.\n");
    exit(1);
  }
  input = fopen (argv[1], "rb");
  if (input == NULL) {
    fprintf (stderr, "Unable to open input file %s.\n", argv[1]);
    exit(2);
  }
  ret = fseek (input, 0x7e0, SEEK_SET); // This is where the directory entry is
  if (ret == -1) {
    perror ("Seek failed: ");
    exit(3);
  }
  ret2 = fgets (name, 17, input);
  if (ret2==NULL) {
    fprintf(stderr, "Failed to seek to 0x7e0. image damaged.\n");
    exit(4);
  } 
  fprintf(stdout, "Name of disk: %s\n", name);

  ret = getBlockPointer(input, &objectPointer); 
  if (ret != 0) {
    fprintf (stderr, "Unable to read objectPointer.\n");
    exit(6);
  }
  fprintf (stdout, "objectPointer: mode=%d blockAddress=%08X byte position =%08X \n", objectPointer.indexType, objectPointer.pointer, objectPointer.pointer*2048); 

  ret = getBlockPointer(input, &userPointer); 
  if (ret != 0) {
    fprintf (stderr, "Unable to read userPointer.\n");
    exit(5);
  }
  fprintf (stdout, "userPointer: mode=%d blockAddress=%08X byte position =%08X \n", userPointer.indexType, userPointer.pointer, userPointer.pointer*2048); 

  ret = getBlockPointer(input, &bitFilePointer); 
  if (ret != 0) {
    fprintf (stderr, "Unable to read bitFilePointer.\n");
    exit(7);
  }
  fprintf (stdout, "bitFilePointer: mode=%d blockAddress=%08X byte position =%08X \n", bitFilePointer.indexType, bitFilePointer.pointer, bitFilePointer.pointer*2048); 
  ret = readInfo (input, objectPointer);
  ret = readInfo (input, userPointer);
  return 0;
}
