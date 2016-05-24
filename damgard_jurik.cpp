//
// Created by maxxie on 16-5-24.
//

#include <cstdlib>
#include <cstring>
#include "damgard_jurik.h"

damgard_jurik_text_t::damgard_jurik_text_t() {
    mpz_init(text);
}

damgard_jurik_text_t::damgard_jurik_text_t(unsigned long integer) {
    mpz_init(text);
    mpz_init_set_ui(text, integer);
}

damgard_jurik_text_t::damgard_jurik_text_t(void *bytes, int len) {
    mpz_init(text);
    mpz_import(text, len, 1, 1, 0, 0, bytes);
}

damgard_jurik_text_t::damgard_jurik_text_t(const char str[]) {
    mpz_init(text);
    mpz_import(text, strlen(str), 1, 1, 0, 0, str);
}

char* damgard_jurik_text_t::to_str() {
    char *buf;
    size_t len;
    buf = (char *)mpz_export(NULL, &len, 1, 1, 0, 0, text);
    buf[len] = 0;
    return buf;
}

void* damgard_jurik_text_t::to_bytes() {
    void *buf;
    size_t written;
    buf = mpz_export(NULL, &written, 1, 1, 0, 0, text);
    return buf;
}

size_t damgard_jurik_text_t::size() {
    return mpz_size(text) * sizeof(mp_limb_t);
}

damgard_jurik_ciphertext_t operator*(damgard_jurik_ciphertext_t a, damgard_jurik_ciphertext_t b) {
    damgard_jurik_ciphertext_t return_object;
    mpz_init(return_object.text);
    mpz_mul(return_object.text, a.text, b.text);
    mpz_mod(return_object.text, return_object.text, a.n_s);
    mpz_set(return_object.n_s, a.n_s);
    return_object.s = a.s;
    return return_object;
}

damgard_jurik_ciphertext_t operator^(damgard_jurik_ciphertext_t &a, damgard_jurik_text_t &b) {
    damgard_jurik_ciphertext_t return_object;
    mpz_init(return_object.text);
    mpz_powm(return_object.text, a.text, b.text, a.n_s);
    mpz_set(return_object.n_s, a.n_s);
    return_object.s = a.s;
    return return_object;
}

void damgard_jurik::init_rand(gmp_randstate_t rand, int bytes) {
    void* buf;
    mpz_t s;

    buf = new char[bytes];
    this->rand_func(buf, bytes);

    gmp_randinit_default(rand);
    mpz_init(s);
    mpz_import(s, bytes, 1, 1, 0, 0, buf);
    gmp_randseed(rand, s);
    mpz_clear(s);

    delete(buf);
}

void damgard_jurik_function_l(mpz_t result ,mpz_t b, mpz_t n) {
    mpz_sub_ui(result, b, 1);
    mpz_div(result, result, n);
}

void damgard_jurik::key_gen(int modulusbits) {
    mpz_t p, q;

    gmp_randstate_t rand;
    mpz_init(pubkey->n);
    mpz_init(pubkey->g);
    mpz_init(prvkey->lambda);
    mpz_init(p);
    mpz_init(q);

    init_rand(rand, modulusbits / 8 + 1);
    do
    {
        do
            mpz_urandomb(p, rand, modulusbits / 2);
        while( !mpz_probab_prime_p(p, 10) );

        do
            mpz_urandomb(q, rand, modulusbits / 2);
        while( !mpz_probab_prime_p(q, 10) );

        /* compute the public modulus n = p q */

        mpz_mul(pubkey->n, p, q);
    } while( !mpz_tstbit(pubkey->n, modulusbits - 1));
    pubkey->bits = modulusbits;

    mpz_add_ui(pubkey->g, pubkey->n, 1);
    pubkey->n_j = new mpz_t[s_max + 2];
    mpz_init(pubkey->n_j[1]);
    mpz_set(pubkey->n_j[1], pubkey->n);
    for (int i = 2;i < s_max;i++) {
        mpz_init(pubkey->n_j[i]);
        mpz_mul(pubkey->n_j[i], pubkey->n_j[i - 1], pubkey->n);
    }

    /* Compute Private Key */
    mpz_sub_ui(p, p, 1);
    mpz_sub_ui(q, q, 1);
    mpz_lcm(prvkey->lambda, p, q);

    mpz_clear(p);
    mpz_clear(q);
    gmp_randclear(rand);
}

damgard_jurik::damgard_jurik(unsigned long s, int bitsmodule, damgard_jurik_get_rand_t rand_func) {
    pubkey = new damgard_jurik_pubkey_t();
    prvkey = new damgard_jurik_prvkey_t();
    this->s_max = s;
    this->rand_func = rand_func;
    key_gen(bitsmodule);
}

damgard_jurik_ciphertext_t damgard_jurik::encrypt(damgard_jurik_plaintext_t* pt, unsigned long s) {
    mpz_t r;
    gmp_randstate_t rand;
    mpz_t x;
    damgard_jurik_ciphertext_t res;
    /* pick random blinding factor */

    mpz_init(r);

    init_rand(rand, pubkey->bits / 8 + 1);
    do
        mpz_urandomb(r, rand, pubkey->bits);
    while( mpz_cmp(r, pubkey->n) >= 0 );

    mpz_init(x);
    mpz_powm(res.text, pubkey->g, pt->text, pubkey->n_j[s + 1]);
    mpz_powm(x, r, pubkey->n_j[s], pubkey->n_j[s + 1]);

    mpz_mul(res.text, res.text, x);
    mpz_mod(res.text, res.text, pubkey->n_j[s + 1]);

    mpz_set(res.n_s, pubkey->n_j[s + 1]);
    res.s = s;
    mpz_clear(x);
    mpz_clear(r);
    gmp_randclear(rand);

    return res;
}

damgard_jurik_ciphertext_t damgard_jurik::encrypt(damgard_jurik_plaintext_t* pt) {
    return encrypt(pt, get_s(pt));
}

damgard_jurik_plaintext_t damgard_jurik::decrypt(damgard_jurik_ciphertext_t* ct) {
    damgard_jurik_plaintext_t res;
    mpz_t c_r;
    mpz_init(c_r);

    mpz_powm(c_r, ct->text, prvkey->lambda, pubkey->n_j[ct->s + 1]);

    int i, j;
    int k_n;
    mpz_t t1, t2, t3, i_lamda;
    mpz_init(t1);
    mpz_init(t2);
    mpz_init(t3);
    mpz_init(i_lamda);
    mpz_set_ui(i_lamda, 0);
    for (i = 1;i <= ct->s;++i) {
        mpz_mod(t1, c_r, pubkey->n_j[i + 1]);
        damgard_jurik_function_l(t1, t1, pubkey->n_j[1]);
        mpz_set(t2, i_lamda);
        k_n = 1;
        for (j = 2;j <= i;j++) {
            k_n *= j;
            mpz_sub_ui(i_lamda, i_lamda, 1);
            mpz_mul(t2, t2, i_lamda);
            mpz_mod(t2, t2, pubkey->n_j[i]);
            mpz_set_ui(t3, k_n);
            mpz_invert(t3, t3, pubkey->n_j[i]);
            mpz_mul(t3, t2, t3);
            mpz_mul(t3, t3, pubkey->n_j[j - 1]);
            mpz_sub(t1, t1, t3);
            mpz_mod(t1, t1, pubkey->n_j[i]);
        }
        mpz_set(i_lamda, t1);
    }
    mpz_invert(t3, prvkey->lambda, pubkey->n_j[ct->s]);
    mpz_mul(res.text, i_lamda, t3);
    mpz_mod(res.text, res.text, pubkey->n_j[ct->s]);
    return res;
}

unsigned long damgard_jurik::get_s(damgard_jurik_text_t *te) {
    size_t len = te->size();
    return len / pubkey->bits * 8 + 1;
}