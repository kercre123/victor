// Based on code supplied in the xmlsec-ruby package: https://github.com/wonnage/xmlsec-ruby
// which is released under the MIT license
//
#ifndef NOBUILDSIG


#include "sig.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <xmlsec/xmlsec.h>
#include <xmlsec/xmltree.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/xmlenc.h>
#include <xmlsec/crypto.h>
#include <xmlsec/bn.h>


#define UNABLE_TO_PARSE 1000
#define NO_START_NODE 1001
#define ID_DEFINE_FAILED 1002
#define XMLSEC_CONTEXT_FAILED 1003
#define PUB_KEY_READ_FAILED 1004
#define BAD_XMLDSIG_FORMAT 1005
#define INIT_FAILED 1006
#define BAD_VERSION 1007
#define CRYPTO_LOAD_FAILED 1008
#define CRYPTO_INIT_FAILED 1009


void cleanup(xmlSecDSigCtxPtr dsigCtx) ;
int verify_document(xmlDocPtr doc, const char* key);
void xmlSecErrorCallback(const char* file, int line, const char* func, const char* errorObject, const char* errorSubject, int reason, const char* msg);

__thread verify_err _last_xmlsec_err;

int set_error(int code, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(_last_xmlsec_err.msg, sizeof(_last_xmlsec_err.msg), fmt, args);
    _last_xmlsec_err.code = code;
    va_end(args);
    return 0;
}

verify_err last_xmlsec_error() {
    return _last_xmlsec_err;
}

static int xmlSecAppAddIDAttr(xmlNodePtr node, const xmlChar* attrName, const xmlChar* nodeName, const xmlChar* nsHref) {
    xmlAttrPtr attr, tmpAttr;
    xmlNodePtr cur;
    xmlChar* id;

    if((node == NULL) || (attrName == NULL) || (nodeName == NULL)) {
        return(-1);
    }

    /* process children first because it does not matter much but does simplify code */
    cur = xmlSecGetNextElementNode(node->children);
    while(cur != NULL) {
        if(xmlSecAppAddIDAttr(cur, attrName, nodeName, nsHref) < 0) {
            return(-1);
        }
        cur = xmlSecGetNextElementNode(cur->next);
    }

    /* node name must match */
    if(!xmlStrEqual(node->name, nodeName)) {
        return(0);
    }

    /* if nsHref is set then it also should match */
    if((nsHref != NULL) && (node->ns != NULL) && (!xmlStrEqual(nsHref, node->ns->href))) {
        return(0);
    }

    /* the attribute with name equal to attrName should exist */
    for(attr = node->properties; attr != NULL; attr = attr->next) {
        if(xmlStrEqual(attr->name, attrName)) {
            break;
        }
    }
    if(attr == NULL) {
        return(0);
    }

    /* and this attr should have a value */
    id = xmlNodeListGetString(node->doc, attr->children, 1);
    if(id == NULL) {
        return(0);
    }

    /* check that we don't have same ID already */
    tmpAttr = xmlGetID(node->doc, id);
    if(tmpAttr == NULL) {
        xmlAddID(NULL, node->doc, id, attr);
    } else if(tmpAttr != attr) {
        fprintf(stderr, "Error: duplicate ID attribute \"%s\"\n", id);
        xmlFree(id);
        return(-1);
    }
    xmlFree(id);
    return(0);
}

/* functions */
//int verify_file(const char* xmlMessage, const char* key) {
int verify_signed_xml(char* xmlMessage, const char* key) {
    xmlDocPtr doc = NULL;
    /* Init libxml and libxslt libraries */
    xmlInitParser();
    LIBXML_TEST_VERSION
    xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefault(1);
    doc = xmlParseDoc((xmlChar *) xmlMessage) ;
    return verify_document(doc, key);
}

int verify_document(xmlDocPtr doc, const char* key) {
    xmlNodePtr node = NULL;
    xmlSecDSigCtxPtr dsigCtx = NULL;
    int res = 0;

    if ((doc == NULL) || (xmlDocGetRootElement(doc) == NULL)) {
        cleanup(dsigCtx);
        return set_error(UNABLE_TO_PARSE, "Unable to parse XML document");
    }

    /* find start node */
    node = xmlSecFindNode(xmlDocGetRootElement(doc), xmlSecNodeSignature, xmlSecDSigNs);
    if(node == NULL) {
        cleanup(dsigCtx);
        return set_error(NO_START_NODE, "Could not find start node in XML document");
    }

    xmlNodePtr cur = xmlSecGetNextElementNode(doc->children);
    while(cur != NULL) {
        if(xmlSecAppAddIDAttr(cur, "ID", "Response", "urn:oasis:names:tc:SAML:2.0:protocol") < 0) {
            cleanup(dsigCtx);
            return set_error(ID_DEFINE_FAILED, "Could not define ID attribute");
        }
        cur = xmlSecGetNextElementNode(cur->next);
    }

    /* create signature context */
    dsigCtx = xmlSecDSigCtxCreate(NULL);
    if(dsigCtx == NULL) {
        cleanup(dsigCtx);
        return set_error(XMLSEC_CONTEXT_FAILED, "Could not create signature context");
    }

    /* load public key */
    dsigCtx->signKey = xmlSecCryptoAppKeyLoadMemory(key, strlen(key), xmlSecKeyDataFormatCertPem, NULL, NULL, NULL);
    if(dsigCtx->signKey == NULL) {
        cleanup(dsigCtx);
        return set_error(PUB_KEY_READ_FAILED, "Could not read public pem key %s", key);
    }

    /* Verify signature */
    if(xmlSecDSigCtxVerify(dsigCtx, node) < 0) {
        cleanup(dsigCtx);
        return set_error(BAD_XMLDSIG_FORMAT, "Document does not seem to be in an XMLDsig format");
    }

    /* print verification result to stdout */
    if(dsigCtx->status == xmlSecDSigStatusSucceeded) {
        res = 1;
    } else {
        res = 0;
    }
    cleanup(dsigCtx);
    return res;
}

void cleanup(xmlSecDSigCtxPtr dsigCtx) {
    if(dsigCtx != NULL) {
        xmlSecDSigCtxDestroy(dsigCtx);
    }
}

int init_xmlsec(verify_err *err) {

    /* Init xmlsec library */
    if(xmlSecInit() < 0) {
        return set_error(INIT_FAILED, "xmlsec initialization failed");
    }

    /* Check loaded library version */
    if(xmlSecCheckVersion() != 1) {
        return set_error(BAD_VERSION, "Incompatible xmlsec library");
    }

    /* Load default crypto engine if we are supporting dynamic
     * loading for xmlsec-crypto libraries. Use the crypto library
     * name ("openssl", "nss", etc.) to load corresponding
     * xmlsec-crypto library.
     */
#ifdef XMLSEC_CRYPTO_DYNAMIC_LOADING
    if(xmlSecCryptoDLLoadLibrary(BAD_CAST XMLSEC_CRYPTO) < 0) {
        return set_serror(CRYPTO_LOAD_FAILED, , "Error: unable to load default xmlsec-crypto library. Make sure "
                          "that you have it installed and check shared libraries path "
                          "(LD_LIBRARY_PATH) envornment variable.");
    }
#endif /* XMLSEC_CRYPTO_DYNAMIC_LOADING */

    /* Init xmlsec-crypto library */
    if(xmlSecCryptoInit() < 0) {
        return set_error(CRYPTO_INIT_FAILED, "xmlsec-crypto initialization failed.");
    }
    xmlSecErrorsSetCallback(xmlSecErrorCallback);
    return 0;
}

void xmlSecErrorCallback(const char* file, int line, const char* func, const char* errorObject, const char* errorSubject, int reason, const char* msg) {
    //printf("file: %s\nline: %d\nfunc: %s\nobject: %s\nsubject: %s\nreason: %d\nmsg: %s\n", file, line, func, errorObject, errorSubject, reason, msg);
    set_error(reason, "XLMSec error in %s: %s", func, msg);
}

void shutdown_xmlsec() {
    /* Shutdown xmlsec-crypto library */
    xmlSecCryptoShutdown();

    /* Shutdown xmlsec library */
    xmlSecShutdown();

    /* Shutdown libxslt/libxml */
#ifndef XMLSEC_NO_XSLT
    xsltCleanupGlobals();
#endif /* XMLSEC_NO_XSLT */
    xmlCleanupParser();
    return ;
}

#endif
