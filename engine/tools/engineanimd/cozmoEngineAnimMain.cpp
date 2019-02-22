/**
* File: cozmoEngineAnimMain.cpp
*
* Author: Richard Gale
* Created: 2/6/19
*
* Description: Combine Engine+Anim Process on Victor
*
* Copyright: Anki, inc. 2019
*
*/

#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
  typedef void *(threadPtr)(void *);

  char *error;

  void *vicanim_handle = dlopen("libvic-anim.so", RTLD_LAZY);
  if (!vicanim_handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(1);
  }

  threadPtr* vicanim_fn = (threadPtr*)dlsym(vicanim_handle, "threadmain");
  if ((error = dlerror()) != NULL)  {
    fprintf(stderr, "%s\n", error);
    exit(1);
  }

  void *vicengine_handle = dlopen("libvic-engine.so", RTLD_LAZY);
  if (!vicengine_handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(1);
  }

  threadPtr* vicengine_fn = (threadPtr*)dlsym(vicengine_handle, "threadmain");
  if ((error = dlerror()) != NULL)  {
    fprintf(stderr, "%s\n", error);
    exit(1);
  }

  struct argcv {
    int argc;
    char **argv;
  } p;

  p.argc = argc;
  p.argv = argv;

  pthread_t vicanim_thread;
  pthread_attr_t vicanim_attr;
  pthread_attr_init(&vicanim_attr);
  pthread_create(&vicanim_thread, &vicanim_attr, vicanim_fn, &p);

  pthread_t vicengine_thread;
  pthread_attr_t vicengine_attr;
  pthread_attr_init(&vicengine_attr);
  pthread_create(&vicengine_thread, &vicengine_attr, vicengine_fn, &p);

  pthread_join(vicengine_thread, NULL);

  return 1;
}
