#ifndef BUILDSIG

typedef struct {
    int code;
    char msg[256];
} verify_err;


verify_err last_xmlsec_error();
void shutdown_xmlsec();
int init_xmlsec();
int verify_signed_xml(char* xmlMessage, const char* key);

#endif
