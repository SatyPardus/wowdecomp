#include "SRP6_Client.h"
#include <cstring>
#include "WoW/Source/WowServices/BigInteger.h"
#include "WoW/Source/WowServices/SRP6_Random.h"

// VERIFY: f3ad7c9f auto
// OFFSET: 0x009A8300
int32_t SRP6_Client::BeginAuthentication(const char* accountName, const char* password) {
    if (!accountName || !password) {
        return -1;
    }

    SHA1_Init(&this->m_proofContext);

    SHA1_CONTEXT context;
    SHA1_Init(&context);
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(accountName), strlen(accountName));
    SHA1_Final(this->m_accountNameHash, &context);

    SHA1_Init(&context);
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(accountName), strlen(accountName));
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(":"), 1);
    SHA1_Update(&context, reinterpret_cast<const uint8_t*>(password), strlen(password));
    SHA1_Final(this->m_credentialsHash, &context);

    return 0;
}

// VERIFY: 5d07ea4e auto
// OFFSET: 0x009A83E0
int32_t SRP6_Client::CalculateProof(const uint8_t* largeSafePrime, uint32_t largeSafePrimeLength, const uint8_t* generator, uint32_t generatorLength, const uint8_t* salt, uint32_t saltLength, const uint8_t* serverPublicKey, uint32_t serverPublicKeyLength, SRP6_Random& random) {
    BigInteger N = BigIntegerFromBytes(largeSafePrime, largeSafePrimeLength);
    BigInteger g = BigIntegerFromBytes(generator, generatorLength);
    BigInteger x = BigIntegerFromInt(0);
    BigInteger v = BigIntegerFromInt(0);
    BigInteger unused1 = BigIntegerFromInt(0);
    BigInteger unused2 = BigIntegerFromInt(0);
    BigInteger a = BigIntegerFromInt(0);
    BigInteger A = BigIntegerFromInt(0);
    BigInteger u = BigIntegerFromInt(0);
    BigInteger B = BigIntegerFromBytes(serverPublicKey, serverPublicKeyLength);
    BigInteger t = BigIntegerFromInt(0);
    BigInteger S = BigIntegerFromInt(0);

    int32_t result = -1;

    if (largeSafePrime && largeSafePrimeLength - 1 <= 31 && generator && generatorLength - 1 <= 31 && salt && saltLength - 1 <= 31 && serverPublicKey && serverPublicKeyLength >= 31) {
        SHA1_CONTEXT context;
        uint8_t hash[20];

        SHA1_Init(&context);
        SHA1_Update(&context, salt, saltLength);
        SHA1_Update(&context, this->m_credentialsHash, sizeof(this->m_credentialsHash));
        SHA1_Final(hash, &context);
        BigIntegerFree(x);
        x = BigIntegerFromBytes(hash, sizeof(hash));
        BigIntegerModExp(v, g, x, N);

        uint8_t primeHash[20];
        SHA1_Init(&context);
        SHA1_Update(&context, largeSafePrime, largeSafePrimeLength);
        SHA1_Final(primeHash, &context);

        uint8_t generatorHash[20];
        SHA1_Init(&context);
        SHA1_Update(&context, generator, generatorLength);
        SHA1_Final(generatorHash, &context);

        for (uint32_t i = 0; i < sizeof(primeHash); i++) {
            primeHash[i] ^= generatorHash[i];
        }

        SHA1_CONTEXT proof;
        SHA1_Init(&proof);
        SHA1_Update(&proof, primeHash, sizeof(primeHash));
        SHA1_Update(&proof, this->m_accountNameHash, sizeof(this->m_accountNameHash));
        SHA1_Update(&proof, salt, saltLength);

        if (BigIntegerBitLen(N) >= 256) {
            uint8_t privateKey[32];
            random.GenerateRandomBytes(privateKey, sizeof(privateKey));
            BigIntegerFree(a);
            a = BigIntegerFromBytes(privateKey, sizeof(privateKey));
            BigIntegerAddInt(a, a, BigIntegerBitLen(N));
            BigIntegerModExp(A, g, a, N);
            BigIntegerToBytes(A, this->m_publicKey, sizeof(this->m_publicKey));

            SHA1_Update(&proof, this->m_publicKey, sizeof(this->m_publicKey));
            SHA1_Update(&proof, serverPublicKey, serverPublicKeyLength);
            SHA1_Update(&this->m_proofContext, this->m_publicKey, sizeof(this->m_publicKey));

            SHA1_Init(&context);
            SHA1_Update(&context, this->m_publicKey, sizeof(this->m_publicKey));
            SHA1_Update(&context, serverPublicKey, serverPublicKeyLength);
            SHA1_Final(primeHash, &context);
            BigIntegerFree(u);
            u = BigIntegerFromBytes(primeHash, sizeof(primeHash));

            if (BigIntegerCmp(B, N) < 0 && BigIntegerCmpInt(B, 0)) {
                BigIntegerSub(t, N, v);
                BigIntegerAdd(B, B, t);
                BigIntegerAdd(B, B, t);
                BigIntegerAdd(B, B, t);
                BigIntegerMod(B, B, N);
                BigIntegerMul(t, x, u);
                BigIntegerAdd(t, t, a);
                BigIntegerModExp(S, B, t, N);

                uint8_t sharedSecret[32];
                BigIntegerToBytes(S, sharedSecret, sizeof(sharedSecret));
                SHA1_InterleaveHash(this->m_sessionKey, sharedSecret, sizeof(sharedSecret));

                SHA1_Update(&proof, this->m_sessionKey, sizeof(this->m_sessionKey));
                SHA1_Final(this->m_clientProof, &proof);
                SHA1_Update(&this->m_proofContext, this->m_clientProof, sizeof(this->m_clientProof));
                SHA1_Update(&this->m_proofContext, this->m_sessionKey, sizeof(this->m_sessionKey));

                result = 0;
            } else {
                result = -2;
            }
        }
    }

    BigIntegerFree(S);
    BigIntegerFree(t);
    BigIntegerFree(B);
    BigIntegerFree(u);
    BigIntegerFree(A);
    BigIntegerFree(a);
    BigIntegerFree(unused2);
    BigIntegerFree(unused1);
    BigIntegerFree(v);
    BigIntegerFree(x);
    BigIntegerFree(g);
    BigIntegerFree(N);

    return result;
}

// VERIFY: 0770cf9c manual
// OFFSET: 0x009A88C0
int32_t SRP6_Client::VerifyServerProof(const uint8_t* serverProof, uint32_t serverProofLength) {
    if (serverProofLength != 20) {
        return -2;
    }

    uint8_t proof[20];
    SHA1_Final(proof, &this->m_proofContext);

    return memcmp(proof, serverProof, sizeof(proof));
}
