/* gcc -g3 -O1 -Wall -std=c99 -I/usr/local/ssl/darwin/include t-hmac.c /usr/local/ssl/darwin/lib/libcrypto.a -o t-hmac.exe */
/* gcc -g2 -Os -Wall -DNDEBUG=1 -std=c99 -I/usr/local/ssl/darwin/include t-hmac.c /usr/local/ssl/darwin/lib/libcrypto.a -o t-hmac.exe */

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

typedef unsigned char byte;
#define UNUSED(x) ((void)x)
const char hn[] = "SHA256";

/* Returns 0 for success, non-0 otherwise */
int make_keys(EVP_PKEY** skey, EVP_PKEY** vkey);

/* Returns 0 for success, non-0 otherwise */
int sign_it(const byte* msg, size_t mlen, byte** sig, size_t* slen, EVP_PKEY* pkey);

/* Returns 0 for success, non-0 otherwise */
int verify_it(const byte* msg, size_t mlen, const byte* sig, size_t slen, EVP_PKEY* pkey);

/* Prints a buffer to stdout. Label is optional */
void print_it(const char* label, const byte* buff, size_t len);

int main(int argc, char* argv[])
{
    printf("Testing HAMC functions with EVP_DigestSign\n");
    
    OpenSSL_add_all_algorithms();
    
    /* Sign and Verify HMAC keys */
    EVP_PKEY *skey = NULL, *vkey = NULL;
    
    int rc = make_keys(&skey, &vkey);
    assert(rc == 0);
    if(rc != 0)
        exit(1);
    
    assert(skey != NULL);
    if(skey == NULL)
        exit(1);
    
    assert(vkey != NULL);
    if(vkey == NULL)
        exit(1);
    
    const byte msg[] = "Now is the time for all good men to come to the aide of their country";
    byte* sig = NULL;
    size_t slen = 0;
    
    /* Using the skey or signing key */
    rc = sign_it(msg, sizeof(msg), &sig, &slen, skey);
    assert(rc == 0);
    if(rc == 0) {
        printf("Created signature\n");
    } else {
        printf("Failed to create signature, return code %d\n", rc);
        exit(1); /* Should cleanup here */
    }
    
    print_it("Signature", sig, slen);
    
#if 0
    /* Tamper with signature */
    printf("Tampering with signature\n");
    sig[0] ^= 0x01;
#endif
    
#if 0
    /* Tamper with signature */
    printf("Tampering with signature\n");
    sig[slen - 1] ^= 0x01;
#endif
    
    /* Using the vkey or verifying key */
    rc = verify_it(msg, sizeof(msg), sig, slen, vkey);
    if(rc == 0) {
        printf("Verified signature\n");
    } else {
        printf("Failed to verify signature, return code %d\n", rc);
    }

    if(sig)
        OPENSSL_free(sig);
    
    if(skey)
        EVP_PKEY_free(skey);
    
    if(vkey)
        EVP_PKEY_free(vkey);
    
    return 0;
}

int sign_it(const byte* msg, size_t mlen, byte** sig, size_t* slen, EVP_PKEY* pkey)
{
    /* Returned to caller */
    int result = -1;
    
    if(!msg || !mlen || !sig || !pkey) {
        assert(0);
        return -1;
    }
    
    if(*sig)
        OPENSSL_free(*sig);
    
    *sig = NULL;
    *slen = 0;
    
    EVP_MD_CTX* ctx = NULL;
    
    do
    {
        ctx = EVP_MD_CTX_create();
        assert(ctx != NULL);
        if(ctx == NULL) {
            printf("EVP_MD_CTX_create failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        const EVP_MD* md = EVP_get_digestbyname(hn);
        assert(md != NULL);
        if(md == NULL) {
            printf("EVP_get_digestbyname failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        int rc = EVP_DigestInit_ex(ctx, md, NULL);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestInit_ex failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        rc = EVP_DigestSignInit(ctx, NULL, md, NULL, pkey);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignInit failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        rc = EVP_DigestSignUpdate(ctx, msg, mlen);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignUpdate failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        size_t req = 0;
        rc = EVP_DigestSignFinal(ctx, NULL, &req);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignFinal failed (1), error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        assert(req > 0);
        if(!(req > 0)) {
            printf("EVP_DigestSignFinal failed (2), error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        *sig = (byte *)OPENSSL_malloc(req);
        assert(*sig != NULL);
        if(*sig == NULL) {
            printf("OPENSSL_malloc failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        *slen = req;
        rc = EVP_DigestSignFinal(ctx, *sig, slen);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignFinal failed (3), return code %d, error 0x%lx\n", rc, ERR_get_error());
            break; /* failed */
        }
        
        assert(req == *slen);
        if(rc != 1) {
            printf("EVP_DigestSignFinal failed, mismatched signature sizes %ld, %ld", req, *slen);
            break; /* failed */
        }
        
        result = 0;
        
    } while(0);
    
    if(ctx) {
        EVP_MD_CTX_destroy(ctx);
        ctx = NULL;
    }
    
    /* Convert to 0/1 result */
    return !!result;
}

int verify_it(const byte* msg, size_t mlen, const byte* sig, size_t slen, EVP_PKEY* pkey)
{
    /* Returned to caller */
    int result = -1;
    
    if(!msg || !mlen || !sig || !slen || !pkey) {
        assert(0);
        return -1;
    }

    EVP_MD_CTX* ctx = NULL;
    
    do
    {
        ctx = EVP_MD_CTX_create();
        assert(ctx != NULL);
        if(ctx == NULL) {
            printf("EVP_MD_CTX_create failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        const EVP_MD* md = EVP_get_digestbyname(hn);
        assert(md != NULL);
        if(md == NULL) {
            printf("EVP_get_digestbyname failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        int rc = EVP_DigestInit_ex(ctx, md, NULL);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestInit_ex failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        rc = EVP_DigestSignInit(ctx, NULL, md, NULL, pkey);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignInit failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        rc = EVP_DigestSignUpdate(ctx, msg, mlen);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignUpdate failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        byte buff[EVP_MAX_MD_SIZE];
        size_t size = sizeof(buff);
        
        rc = EVP_DigestSignFinal(ctx, buff, &size);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestVerifyFinal failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        assert(size > 0);
        if(!(size > 0)) {
            printf("EVP_DigestSignFinal failed (2)\n");
            break; /* failed */
        }
        
        const size_t m = (slen < size ? slen : size);
        result = !!CRYPTO_memcmp(sig, buff, m);
        
        OPENSSL_cleanse(buff, sizeof(buff));
        
    } while(0);
    
    if(ctx) {
        EVP_MD_CTX_destroy(ctx);
        ctx = NULL;
    }
    
    /* Convert to 0/1 result */
    return !!result;
}

void print_it(const char* label, const byte* buff, size_t len)
{
    if(!buff || !len)
        return;
    
    if(label)
        printf("%s: ", label);
    
    for(size_t i=0; i < len; ++i)
        printf("%02X", buff[i]);
    
    printf("\n");
}

/* Create an HMAC key */
int make_keys(EVP_PKEY** skey, EVP_PKEY** vkey)
{
    /* HMAC key */
    byte hkey[EVP_MAX_MD_SIZE];
    
    int result = -1;
    
    if(!skey || !vkey)
        return -1;
    
    if(*skey != NULL) {
        EVP_PKEY_free(*skey);
        *skey = NULL;
    }
    
    if(*vkey != NULL) {
        EVP_PKEY_free(*vkey);
        *vkey = NULL;
    }
    
    do
    {
        const EVP_MD* md = EVP_get_digestbyname(hn);
        assert(md != NULL);
        if(md == NULL) {
            printf("EVP_get_digestbyname failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        int size = EVP_MD_size(md);
        assert(size >= 16);
        if(!(size >= 16)) {
            printf("EVP_MD_size failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        assert(size <= sizeof(hkey));
        if(!(size <= sizeof(hkey))) {
            printf("EVP_MD_size is too large\n");
            break; /* failed */
        }
        
        /* Generate bytes */
        int rc = RAND_bytes(hkey, size);
        assert(rc == 1);
        if(rc != 1) {
            printf("RAND_bytes failed, error 0x%lx\n", ERR_get_error());
            break;
        }
        
        print_it("HMAC key", hkey, size);
        
        *skey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, hkey, size);
        assert(*skey != NULL);
        if(*skey == NULL) {
            printf("EVP_PKEY_new_mac_key failed, error 0x%lx\n", ERR_get_error());
            break;
        }
        
        *vkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, hkey, size);
        assert(*vkey != NULL);
        if(*vkey == NULL) {
            printf("EVP_PKEY_new_mac_key failed, error 0x%lx\n", ERR_get_error());
            break;
        }
        
        result = 0;
        
    } while(0);
    
    OPENSSL_cleanse(hkey, sizeof(hkey));
    
    /* Convert to 0/1 result */
    return !!result;
}
