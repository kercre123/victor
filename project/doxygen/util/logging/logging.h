namespace Anki {
namespace Util {

/*! \defgroup dasmsg Das Messages
*/

class DasDoxMsg() {}

#define DASMSG(ezRef, eventName, documentation)  }}}}}}}}/** \ingroup dasmsg */ \
                                            /** \brief eventName */ \
                                            /** documentation */ \
                                            class ezRef(): public DasDoxMsg() { \
                                            public:
#define DASMSG_SET(dasEntry, value, comment) /*! @param dasEntry comment */
#define DASMSG_SEND }; {{{{{{{{
#define DASMSG_SEND_WARNING }; {{{{{{{{
#define DASMSG_SEND_ERROR }; {{{{{{{{

} // namespace Util
} // namespace Anki
