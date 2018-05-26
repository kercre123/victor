// 
#define _GNU_SOURCE
#include <sched.h>
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>

#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))  
void btea(uint32_t *v, int n, uint32_t const key[4]) {
  uint32_t y, z, sum;
  unsigned p, rounds, e;
  if (n > 1) {          /* Coding Part */
    rounds = 6 + 52/n;
    sum = 0;
    z = v[n-1];
    do {
      sum += DELTA;
      e = (sum >> 2) & 3;
      for (p=0; p<n-1; p++) {
        y = v[p+1]; 
        z = v[p] += MX;
      }
      y = v[0];
      z = v[n-1] += MX;
    } while (--rounds);
  }
}

#define SIZE 1024*1024
uint32_t buf[SIZE];

void setcpu(int core)
{
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(core, &set);
  if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &set))
  {
    printf("Can't sched_setaffinity for core %d\n", core);
    exit(254);
  }
}

int main(int argc, const char* argv[])
{
  int i, j;
  char name[256];
  FILE* fp;
  if (argc >= 2)
  {
    setcpu(argv[1][0]-'0');
  }
  sprintf(name, "/data/burn/%d", getpid());
  fp = fopen(name, "wb");
  if (!fp)
  {
    printf("Can't write %s\n", name);
    exit(255);
  }
  fclose(fp);
  printf("Burner writing to %s\n", name);
  while (1)
  {
    for (i = 16384; i <= SIZE; i <<= 1)
    {
      for (j = 0; j < SIZE; j += i)
        btea(buf+j, i, buf);
      if (argc == 3)
      {
        fp = fopen(name, "wb");
        fseek(fp, 0, SEEK_SET);
        fwrite(buf, sizeof(buf), 1, fp);
        fwrite(buf, sizeof(buf), 1, fp);
        fwrite(buf, sizeof(buf), 1, fp);
        fwrite(buf, sizeof(buf), 1, fp);     
        fclose(fp);
      }
    }
  }
}
