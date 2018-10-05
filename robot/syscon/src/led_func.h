// 11000001
static void led_1x0(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11000010
static void led_2x0(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11000011
static void led_3x0(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11000100
static void led_4x0(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11000101
static void led_5x0(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11000110
static void led_6x0(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11000111
static void led_7x0(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 10100001
static void led_1x1(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 10100010
static void led_2x1(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 10100011
static void led_3x1(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 10100100
static void led_4x1(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 10100101
static void led_5x1(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 10100110
static void led_6x1(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 10100111
static void led_7x1(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 01100001
static void led_1x2(void) {
    LEDOE::set();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 01100010
static void led_2x2(void) {
    LEDOE::set();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 01100011
static void led_3x2(void) {
    LEDOE::set();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 01100100
static void led_4x2(void) {
    LEDOE::set();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 01100101
static void led_5x2(void) {
    LEDOE::set();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 01100110
static void led_6x2(void) {
    LEDOE::set();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 01100111
static void led_7x2(void) {
    LEDOE::set();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11101000
static void led_1x3(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11110000
static void led_2x3(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
// 11111000
static void led_3x3(void) {
    LEDOE::set();
    LED_DAT::set();    LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    LED_DAT::reset();  LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    wait();            LED_CLK::set(); wait(); LED_CLK::reset();
    kick_off();
}
