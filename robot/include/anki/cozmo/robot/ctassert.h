// ct_assert is a compile time assertion, useful for checking sizeof() and other compile time knowledge
#ifndef ct_assert
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_( a, b)
#ifdef __COUNTER__
#define ct_assert(e) enum { ASSERT_CONCAT(assert_counter_, __COUNTER__) = 1/(!!(e)) }
#else
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#endif
#define static_assert(e, msg) \
    enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#define ASSERT_IS_POWER_OF_TWO(e) ct_assert((e & (e-1)) == 0)
#define ASSERT_IS_MULTIPLE_OF_TWO(e) ct_assert((e % 2) == 0)
#endif
