// TRSA.h: RSA encryption with a public key, used for the login packet. Errors are thrown as const char*.

#pragma once

#include "vlong.h"
#include "common.h"
#include "compat/span.h"

class TRSA {
public:
	TRSA();
	virtual ~TRSA();
	// Decimal text up to its NUL. It stays a C string: the digits end at the first NUL, which tools/patch_client.py
	// relies on (a shorter modulus is padded with NULs in the literal's place), and a null pointer is its own error.
	void SetPublicKey(const char *ModulusM);
	void Encrypt(patella::TSpan<uint8, 128> Data); // in place, exponent 65537; a null pointer is an error

private:
	vlong m_ModulusM;
};
