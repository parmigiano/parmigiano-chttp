#include "encryption.h"

#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/buffer.h>

static char* base64_encode(const unsigned char* input, int len)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    BIO_write(bio, input, len);
    BIO_flush(bio);

    BUF_MEM* ptr;
    BIO_get_mem_ptr(bio, &ptr);

    char* out = strndup(ptr->data, ptr->length);
    BIO_free_all(bio);
    return out;
}

static unsigned char* base64_decode(const char* input, int* out_len)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new_mem_buf(input, -1);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    int len = strlen(input) * 3 / 4;
    unsigned char* buffer = malloc(len);

    *out_len = BIO_read(bio, buffer, len);
    BIO_free_all(bio);
    return buffer;
}

char* encrypt(const char* plaintext)
{
    int key_len, iv_len;
    unsigned char* key = base64_decode(getenv("SUPER_SECRET_KEY"), &key_len);
    unsigned char* iv = base64_decode(getenv("IV"), &iv_len);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    unsigned char ciphertext[1024];
    int len, clen;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char*)plaintext, strlen(plaintext));
    clen = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    clen += len;

    unsigned char hmac[32];
    unsigned int hlen;
    HMAC(EVP_sha256(), key, key_len, ciphertext, clen, hmac, &hlen);

    unsigned char* out = malloc(clen + hlen);
    memcpy(out, ciphertext, clen);
    memcpy(out + clen, hmac, hlen);

    char* b64 = base64_encode(out, clen + hlen);

    EVP_CIPHER_CTX_free(ctx);
    free(key);
    free(iv);
    free(out);

    return b64;
}

char* decrypt(const char* cipher_b64)
{
    int data_len;
    unsigned char* data = base64_decode(cipher_b64, &data_len);

    int key_len, iv_len;
    unsigned char* key = base64_decode(getenv("SUPER_SECRET_KEY"), &key_len);
    unsigned char* iv = base64_decode(getenv("IV"), &iv_len);

    int clen = data_len - 32;
    unsigned char* ciphertext = data;
    unsigned char* hmac = data + clen;

    unsigned char check[32];
    unsigned int hlen;
    HMAC(EVP_sha256(), key, key_len, ciphertext, clen, check, &hlen);

    if (CRYPTO_memcmp(hmac, check, 32) != 0)
    {
        free(data);
        free(key);
        free(iv);
        return NULL;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    unsigned char plaintext[1024];
    int len, plen;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, clen);
    plen = len;

    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plen += len;

    char* out = strndup((char*)plaintext, plen);

    EVP_CIPHER_CTX_free(ctx);
    free(data);
    free(key);
    free(iv);

    return out;
}