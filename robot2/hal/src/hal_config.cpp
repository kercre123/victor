#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "anki/types.h"
#include "anki/cozmo/robot/logging.h"
#include "../spine/spine_hal.h"

#include "anki/cozmo/robot/hal_config.h"

namespace Anki {
  namespace Cozmo {

    //forward Declarations
    namespace HALConfig {
      void ParseValue(const char* valstr, const char* end, const HALConfig::Item* item);
      void ParseConfigLine(const char* line, int sz, const HALConfig::Item config[]);
    }
 
    namespace {
      static const char CONFIG_SEPARATOR = ':';
      
    }
    

      
    void HALConfig::ParseValue(const char* valstr, const char* end, const HALConfig::Item* item)
    {
      char* endconv = NULL;
      assert(strlen(valstr) <= end-valstr);
      double val = strtod(valstr, &endconv);
      if (endconv > valstr) //valid conversion of at least some chars. 
        //Open Question: do we reject partially valid entries like "2.0pi", which would get scanned as 2.0?
      {
        switch (item->type)
        {
        case HALConfig::DOUBLE:
          *(double*)(item->address) = val;
          break;
        case HALConfig::FLOAT:
          *(float*)(item->address) = val;
          break;
        default:
          assert(!"Invalid Conversion Type");
          break;
        }
      }


    }
    
    void HALConfig::ParseConfigLine(const char* line, int sz, const HALConfig::Item config[])
    {
      int i;
      const char* end = line+sz;
      if (sz <1 ) { return; }
      while (isspace(*line) ) {if (++line>= end) { return; } } //find first non-whitespace
      const char* sep = line;
      while (*sep != CONFIG_SEPARATOR) { if (++sep >= end) { return;} } //find end of key.
      for (i=0;config[i].address != NULL;i++) {
        if (strncmp(line, config[i].key, sep-line) == 0) { //match
          ParseValue(sep+1, end, &config[i]);
          LOGD("Set %s to %f\n", config[i].key, *(float*)(config[i].address));
          break;
        }
      }
      return;
    }

    Result HALConfig::ReadConfigFile(const char* path, const HALConfig::Item config[] )
    {
      FILE* fp = fopen(path, "r");
      if (!fp) {
        LOGE("Can't open %s\n", path);
        return RESULT_FAIL_FILE_OPEN;
      }
      else {
        while (1) {
          char* line = NULL;
          size_t bufsz = 0;
          ssize_t nread = getline(&line, &bufsz, fp);
          if (nread > 0) {
            ParseConfigLine(line,nread, config);
          }
          else {
            break; //EOF;
          }
          free(line);
        }
      }
      fclose(fp);
      return RESULT_OK;
    }
  }
};


#ifdef SELF_TEST

#define TEST_FILE "CONFIG_TEST.DAT"

int main(int argc, const char* argv[])
{
  using namespace Anki::Cozmo;
  const char* DATA = "One:1\nSpace In Name:2.0\n Leading_space: 3e3\nTrailing_space :4 \n5:0xF.8\nNoValue:\nNoColon\n";

  FILE* fp = fopen(TEST_FILE,"w+");
  fwrite(DATA, 1, strlen(DATA), fp);
  fclose(fp);

    double one = 0xBADDDA7A;
    double two = 0xBADDDA7A;
    double three = 0xBADDDA7A;
    double four = 0xBADDDA7A;
    double five = 0xBADDDA7A;
    double six = 0xBADDDA7A;
    double seven = 0xBADDDA7A;

HALConfig::Item  configitems[]  = {
    {"5", HALConfig::DOUBLE, &five},
    {"NoValue", HALConfig::DOUBLE, &six},
    {"NoColon", HALConfig::DOUBLE, &seven},
    {"One", HALConfig::DOUBLE, &one},
    {"Space In Name", HALConfig::DOUBLE, &two},
    {"Leading_space", HALConfig::DOUBLE, &three},
    {"Trailing_space", HALConfig::DOUBLE, &four},
    {0}};  //Need zeros as end-of-list marker

    HALConfig::ReadConfigFile(TEST_FILE, configitems);

    assert(one == 1.0);
    assert(two == 2.0);
    assert(three == 3000);
    assert(four == 0xBADDDA7A);
    assert(five == 15.5);
    assert(six == 0xBADDDA7A);
    assert(seven == 0xBADDDA7A);
    printf("SUCCESS\n");

    unlink(TEST_FILE);

}

#endif
