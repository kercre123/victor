#include "zipReader.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: zipReaderTest file.zip\n");
    return -1;
  }

  char* zipFileName = argv[1];
  ZipReader reader;
  bool result = reader.Init(zipFileName);
  if (!result) {
    printf("Failed to initialize with %s\n", zipFileName);
    return -2;
  }

  printf("%s has %zd file entries\n", zipFileName, reader.Size());

  for (int i = 2; i < argc; i++) {
    uint32_t handle;
    uint32_t size;
    result = reader.Find(argv[i], handle, size);
    if (result) {
      printf("'%s' - offset = %d, size = %d\n", argv[i], handle, size);
      unsigned char* data = (unsigned char *) malloc(size);
      result = reader.Read(handle, 0, size, data);
      if (result) {
        std::string filename = std::to_string(i) + ".bin";
        int outfd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
        write(outfd, data, size);
        close(outfd);
      } else {
        printf("error reading data from '%s'!\n", argv[i]);
      }
      free(data); data = nullptr;
    } else {
      printf("'%s' - not found!\n", argv[i]);
    }
  }

  reader.Term();
  return 0;
}
