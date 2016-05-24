//
// Created by maxxie on 16-5-24.
//

#ifndef DAMGARD_JURIK_DAMGARD_JURIK_H
#define DAMGARD_JURIK_DAMGARD_JURIK_H
#include <gmp.h>

typedef struct
{
    int bits;  /* e.g., 1024 */
    mpz_t g;
    mpz_t n;   /* public modulus n = p q */
    //Cache for n^j
    mpz_t *n_j;
} damgard_jurik_pubkey_t;


typedef struct
{
    mpz_t lambda;    /* lambda(n), i.e., lcm(p-1,q-1) */
} damgard_jurik_prvkey_t;

typedef void (*damgard_jurik_get_rand_t) ( void* buf, size_t len );

class damgard_jurik;
class damgard_jurik_text_t;
class damgard_jurik_ciphertext_t;
class damgard_jurik_plaintext_t;

void damgard_jurik_function_l(mpz_t result ,mpz_t b, mpz_t n);

typedef void (*damgard_jurik_get_rand_t) ( void* buf, size_t len );

class damgard_jurik {
private:
    unsigned long s_max;
    damgard_jurik_pubkey_t *pubkey;
    damgard_jurik_prvkey_t *prvkey;
    damgard_jurik_get_rand_t rand_func;
    void key_gen(int);
    void init_rand(gmp_randstate_t rand, int bytes);

public:
    damgard_jurik(unsigned long s, int bitsmodule, damgard_jurik_get_rand_t rand_func);

    damgard_jurik_ciphertext_t encrypt(damgard_jurik_plaintext_t* pt);

    damgard_jurik_ciphertext_t encrypt(damgard_jurik_plaintext_t* pt, unsigned long s);

    damgard_jurik_plaintext_t decrypt(damgard_jurik_ciphertext_t* ct);

    unsigned long get_s(damgard_jurik_text_t *te);

};

class damgard_jurik_text_t {
public:
    mpz_t text;
    damgard_jurik_text_t();
    damgard_jurik_text_t(unsigned long integer);
    damgard_jurik_text_t(const char *str);
    damgard_jurik_text_t(void *bytes, int len);

    char* to_str();
    void* to_bytes();
    size_t size();
};

class damgard_jurik_plaintext_t: public damgard_jurik_text_t {
public:
    damgard_jurik_plaintext_t():damgard_jurik_text_t() {}
    damgard_jurik_plaintext_t(unsigned long integer):damgard_jurik_text_t(integer) {}
    damgard_jurik_plaintext_t(const char *str):damgard_jurik_text_t(str) {}
    damgard_jurik_plaintext_t(void *bytes, int len):damgard_jurik_text_t(bytes ,len) {}
};

class damgard_jurik_ciphertext_t: public damgard_jurik_text_t {
public:
    damgard_jurik *dj;
    mpz_t n_s;
    unsigned long s;
    damgard_jurik_ciphertext_t(): damgard_jurik_text_t(){ mpz_init(n_s); }
    damgard_jurik_ciphertext_t(void *bytes, int len): damgard_jurik_text_t(bytes, len){mpz_init(n_s);}
};

damgard_jurik_ciphertext_t operator*(damgard_jurik_ciphertext_t a, damgard_jurik_ciphertext_t b);

damgard_jurik_ciphertext_t operator^(damgard_jurik_ciphertext_t &a, damgard_jurik_text_t &b);


#endif //DAMGARD_JURIK_DAMGARD_JURIK_H
