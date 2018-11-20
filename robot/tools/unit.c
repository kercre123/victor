#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef uint_fast16_t idx_t;


#define MIN(a,b) ((a)<(b)?(a):(b))

#define TXTBUF_CAPACITY (1<<12)
#define TXTBUF_SIZE_MASK (TXTBUF_CAPACITY-1)


struct text_buffer_t {
//  struct ContactData buffer[TXTBUF_CAPACITY];
  unsigned char text[TXTBUF_CAPACITY];
  idx_t head;
  idx_t tail;
};

#pragma clang diagnostic ignored "-Wmissing-braces" //f'in Clang doesn't know how to init.
static struct text_buffer_t gOutgoingText = {0};

static inline idx_t contact_text_buffer_count() {
  return ((gOutgoingText.head-gOutgoingText.tail) & TXTBUF_SIZE_MASK);
}

idx_t contact_text_buffer_put(const unsigned char* text, idx_t len) {
  idx_t available = TXTBUF_CAPACITY-1 - contact_text_buffer_count();
  idx_t count = MIN(available, len);
  while (count-->0) {
    gOutgoingText.text[gOutgoingText.head++]=*text++;
    gOutgoingText.head &= TXTBUF_SIZE_MASK;
    len--;
  }
  return len;
}

idx_t contact_text_buffer_get(unsigned char data[], idx_t len) {
  idx_t available = contact_text_buffer_count();
  idx_t count = 0;
  len = MIN(available, len);
  while (count < len) {
    *data++ = gOutgoingText.text[gOutgoingText.tail++];
    gOutgoingText.tail &= TXTBUF_SIZE_MASK;
    count++;
  }
return count;
}


const unsigned char* sample_data =
  (unsigned char*)"hello world eleven eighteen two";  //nty-seven";

#define BUFSZ 32

int contact_text_buffer_unit_test(void) {
  unsigned char buf[BUFSZ];
  unsigned char vec[BUFSZ];
  assert(contact_text_buffer_count() == 0);
  vec[0] = 'h';
  assert(contact_text_buffer_put(vec,1) == 0);
  assert(contact_text_buffer_count() == 1);
  assert(contact_text_buffer_get(buf, BUFSZ) == 1);
  assert(buf[0]=='h');

  strcpy((char*)vec, (char*)sample_data);

  /* int i = contact_text_buffer_put(vec,BUFSZ); */
  /* printf("%d, %d\n", i, contact_text_buffer_count()); */


  assert(contact_text_buffer_put(vec,BUFSZ) == 0);
  assert(contact_text_buffer_count() == BUFSZ);
  assert(contact_text_buffer_get(buf, BUFSZ) == BUFSZ);
  printf("%.*s\n", BUFSZ, buf);
  assert(strncmp((char*)buf, (char*)sample_data, BUFSZ)==0);


  memset(buf, 0, sizeof(buf));
  assert(contact_text_buffer_get(buf, BUFSZ) == 0);
  printf("%s\n",buf);
  assert(strcmp((char*)buf, (char*)sample_data)!=0);



  int nrepeats = 10;
  while (nrepeats-->0) {
    int add = rand() % (BUFSZ +1);
    int rem = rand() % (BUFSZ+1);
    printf("Addding %d chars\n", add);
    int i;
    for (i=0;i<add;i++) {
      vec[i]='0'+i;
    }
    assert(contact_text_buffer_put(vec, add)==0);

    printf("Pulling %d chars\n", rem);
    int actual = contact_text_buffer_get(buf, rem);
    assert(actual <=rem);
    for (i=0;i<actual;i++) {
      printf("%c",buf[i]);
      //assert in sequence.
    }
    printf("\n%d remain\n\n", contact_text_buffer_count());


  }

  printf("all tests passed\n");
  return 0;

}



int main(int argc, const char*argv[]) {

  clock_t now = clock();
  srand(now);
  contact_text_buffer_unit_test();
  return 0;
}
