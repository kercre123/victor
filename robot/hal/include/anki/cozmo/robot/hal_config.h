
namespace Anki {
  namespace Cozmo {
    

    namespace HALConfig {
        typedef enum {
          INVALID,
          FLOAT,
          DOUBLE,
          //Add more types here
        } ValueType;
      
      typedef struct {
        const char* key;
        ValueType type;
        void* address;
      } Item;

      Result ReadConfigFile(const char* path, const HALConfig::Item config[]);
      
    }

  }
}
