#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/decoder.h>
#include <openssl/encoder.h>

#include "myevp.h"

#define PUBLIC_KEY OSSL_KEYMGMT_SELECT_PUBLIC_KEY
#define PRIVATE_KEY OSSL_KEYMGMT_SELECT_PRIVATE_KEY

EVP_PKEY *generate_key(OSSL_LIB_CTX *lctx, char *private_key_filename, char *public_key_filename) {
	EVP_PKEY *pkey = NULL;
	EVP_PKEY_CTX *pctx = NULL;
	OSSL_ENCODER_CTX *ectx = NULL;
	FILE *keyfile;
	char *name = "rsaEncryption";

	pkey = EVP_PKEY_new();
	pctx = EVP_PKEY_CTX_new_from_name(lctx, name, NULL);
	if (EVP_PKEY_keygen_init(pctx) <= 0) {
		fprintf(stderr, "Error in `%s` key initialization\n", name);
		return NULL;
	}
	if (EVP_PKEY_keygen(pctx, &pkey) <= 0) {
		fprintf(stderr, "Error in `%s` key generation\n", name);
		EVP_PKEY_free(pkey);
		pkey = NULL;
	}

	keyfile = fopen(private_key_filename, "w");
	if (keyfile == NULL) {
		fprintf(stderr, "Error opening key %s\n", private_key_filename);
		return NULL;
	}
	ectx = OSSL_ENCODER_CTX_new_for_pkey(pkey,
		OSSL_KEYMGMT_SELECT_PRIVATE_KEY,
		"PEM",
		NULL,
		NULL
	);
	OSSL_ENCODER_to_fp(ectx, keyfile);
	fclose(keyfile);

	keyfile = fopen(public_key_filename, "w");
	if (keyfile == NULL) {
		fprintf(stderr, "Error opening key %s\n", public_key_filename);
		return NULL;
	}
	ectx = OSSL_ENCODER_CTX_new_for_pkey(pkey,
		OSSL_KEYMGMT_SELECT_PUBLIC_KEY,
		"PEM",
		NULL,
		NULL
	);
	OSSL_ENCODER_to_fp(ectx, keyfile);
	fclose(keyfile);


	return pkey;
}

EVP_PKEY *load_key(OSSL_LIB_CTX *lctx, char *filename, int type) {
	OSSL_DECODER_CTX *dctx;
	EVP_PKEY *pkey = NULL;
	FILE *keyfile;
	char *algo = "RSA";
	dctx = OSSL_DECODER_CTX_new_for_pkey(&pkey,
		"PEM",
		NULL,
		algo,
		type,
		lctx, NULL);
	if (dctx == NULL || OSSL_DECODER_CTX_get_num_decoders(dctx) == 0) {
		fprintf(stderr, "Error no suitable decoders found\n");
		return NULL;
	}

	keyfile = fopen(filename, "r");
	if (keyfile == NULL) {
		fprintf(stderr, "Error opening key %s\n", filename);
		return NULL;
	}
	if (!OSSL_DECODER_from_fp(dctx, keyfile)) {
	 	fprintf(stderr, "Error initializing key\n");
		ERR_print_errors_fp(stderr);
	 	return NULL;
	}
	return pkey;
}

int sign_data(unsigned char *plain, unsigned char **psign, char *private_key_file, const char *algo)
{
	
	EVP_MD_CTX *evp_md_ctx;
	EVP_PKEY *pkey = NULL;
	unsigned char *sigbuf = NULL;
	int i, siglen;

	OSSL_LIB_CTX *lctx = NULL;
	lctx = OSSL_LIB_CTX_new();

	if (private_key_file == NULL) {
		pkey = generate_key(lctx, "private.key", "public.key");
	} else {
		pkey = load_key(lctx, private_key_file, PRIVATE_KEY);
	}

	/* allocte memory for the sign */
	siglen = EVP_PKEY_size(pkey);
	if ((sigbuf = malloc(siglen)) == NULL) {
		perror("malloc");
		return -1;
	}
	(*psign) = sigbuf;

	/* signing stuff */
	evp_md_ctx = EVP_MD_CTX_new();
	if (!strcmp("md5", algo))
		EVP_SignInit(evp_md_ctx, EVP_md5());
	else if (!strcmp("sha1", algo))
		EVP_SignInit(evp_md_ctx, EVP_sha1());
	else if (!strcmp("sha256", algo))
		EVP_SignInit(evp_md_ctx, EVP_sha256());
	else if (!strcmp("sha512", algo))
		EVP_SignInit(evp_md_ctx, EVP_sha512());
	else
		perror("digest algorithm not found");
	
	EVP_SignUpdate(evp_md_ctx, plain, strlen(plain));
	i = EVP_SignFinal(evp_md_ctx, sigbuf, (unsigned int *) &siglen, pkey);
	EVP_MD_CTX_free(evp_md_ctx);

	if (i == 0)
		/* Signed Failure */
		return -1;
	else
		/* Signed OK */
		return siglen;
}

int verify_sign(unsigned char *databuf, void *sigbuf, int siglen, char *public_key_file, const char *algo)
{
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *evp_md_ctx;
	int i;
	OSSL_LIB_CTX *lctx = NULL;
	lctx = OSSL_LIB_CTX_new();

	pkey = load_key(lctx, public_key_file, PUBLIC_KEY);

	/* verifying stuff */
	evp_md_ctx = EVP_MD_CTX_new();
	if (!strcmp("md5", algo))
		EVP_VerifyInit(evp_md_ctx, EVP_md5());
	else if (!strcmp("sha1", algo))
		EVP_VerifyInit(evp_md_ctx, EVP_sha1());
	else if (!strcmp("sha256", algo))
		EVP_VerifyInit(evp_md_ctx, EVP_sha256());
	else if (!strcmp("sha512", algo))
		EVP_VerifyInit(evp_md_ctx, EVP_sha512());
	else
		perror("digest algorithm not found");
	
	i = EVP_VerifyUpdate(evp_md_ctx, databuf, strlen(databuf));
	if (i < 0) {
		fprintf(stderr, "EVP_VerifyUpdate()\n");
	}
	i = EVP_VerifyFinal(evp_md_ctx, sigbuf, (unsigned int) siglen, pkey);
	EVP_MD_CTX_free(evp_md_ctx);
	if (i > 0)
		/* Verified OK */
		return 1;
	else if (i == 0)
		/* Verification Failure */
		return 0;
	else {
		unsigned long err;
		/* Error Verifying Data */
		err = ERR_peek_last_error();
		fprintf(stderr, "ERR: %lu\n", err);
		fflush(stderr);
		ERR_print_errors_fp(stderr);
		//ERR_error_string(err, buf);
		//fprintf(stderr, "Error EVP_VerifyFinal(): %s: %s\n", buf, buf2);
		return -1;
	}
}

int enc_data(unsigned char *data)
{
	BIO *bpkey = NULL;
	EVP_PKEY *pkey = NULL;

	char *public_key_file = "/tmp/publica.pem";

	/* public key stuff */
	bpkey = BIO_new_file(public_key_file, "r");
	if (!bpkey) {
		fprintf(stderr, "Error 1 - inicializando public_key bio\n");
		return -1;
	}
	pkey = PEM_read_bio_PUBKEY(bpkey, NULL, NULL, NULL);
	BIO_free(bpkey);
	if (!pkey) {
		fprintf(stderr, "Error 2 - inicializando public_key evp\n");
		return -1;
	}
	return -1;
}
