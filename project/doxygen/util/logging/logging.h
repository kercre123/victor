namespace Anki {
namespace Util {

/*! \defgroup dasmsg Das Messages
*/

  class DasDoxMsg() {}


#define DAS_MSG(ezRef, eventName, documentation)  }}}}}}}}/** \ingroup dasmsg */ \
                                            /** \brief eventName */ \
                                            /** documentation */ \
                                            class ezRef(): public DasDoxMsg() { \
                                            public:
#define FILL_ITEM(dasEntry, value, comment) /*! @param dasEntry comment */
#define SEND_DAS_MSG_EVENT }; {{{{{{{{
#define SEND_DAS_MSG_WARN }; {{{{{{{{
#define SEND_DAS_MSG_ERROR }; {{{{{{{{
} // namespace Util
} // namespace Anki
