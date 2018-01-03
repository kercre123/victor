#ifndef ANKI_FIXTURE_LOGGING_H_
#define ANKI_FIXTURE_LOGGING_H_



int fixture_log_init(void);
int fixture_log_start(const char* params, int len);
void fixture_log_writestring(const char* textstring);
void fixture_log_write(const char* buffer, int len);
int fixture_log_stop(const char* params, int len);
void fixture_log_terminate(void);

#endif//ANKI_FIXTURE_LOGGING_H_
