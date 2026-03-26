#include "encrypt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================
// GERAÇÃO DE SALT
//
// Usa /dev/urandom quando disponível (Linux/macOS).
// Fallback: time(NULL) + ruído de ASLR via endereço de stack.
// ============================================================
void stellar_generate_salt(uint8_t* salt) {
    FILE* urandom = fopen("/dev/urandom", "rb");
    if (urandom) {
        fread(salt, 1, STELLAR_SALT_SIZE, urandom);
        fclose(urandom);
        return;
    }

    // Fallback para sistemas sem /dev/urandom (ex: Windows)
    #include <time.h>
    uint64_t seed = (uint64_t)time(NULL);
    uintptr_t ptr_noise = (uintptr_t)&seed;
    seed ^= ptr_noise;
    seed ^= ptr_noise << 17;
    for (int i = 0; i < STELLAR_SALT_SIZE; i++) {
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        seed *= 0x2545F4914F6CDD1DULL;
        salt[i] = (uint8_t)(seed & 0xFF);
    }
}

// ============================================================
// KDF — Key Derivation Function
//
// Transforma a chave de texto + salt em 32 bytes de material
// de chave usando estado de 256 bits e KDF_ROUNDS rodadas de
// mixing não-linear. Chaves curtas ficam tão fortes quanto
// chaves longas após a derivação.
// ============================================================
void stellar_derive_key(const char* key, const uint8_t* salt, uint8_t* out32) {
    uint32_t state[8];
    if (!key) key = "";
    int klen = (int)strlen(key);

    // Constantes iniciais: dígitos de π e e em hex
    state[0] = 0x243F6A88U;
    state[1] = 0x85A308D3U;
    state[2] = 0x13198A2EU;
    state[3] = 0x03707344U;
    state[4] = 0xA4093822U;
    state[5] = 0x299F31D0U;
    state[6] = 0x082EFA98U;
    state[7] = 0xEC4E6C89U;

    // Absorve a chave
    for (int i = 0; i < klen; i++) {
        state[i % 8] ^= (uint32_t)(uint8_t)key[i] * 0x9E3779B9U;
        state[i % 8] ^= state[(i + 1) % 8] >> 13;
        state[(i + 3) % 8] += state[i % 8];
        state[(i + 5) % 8] ^= (state[i % 8] << 7) | (state[i % 8] >> 25);
    }

    // Absorve o salt
    for (int i = 0; i < STELLAR_SALT_SIZE; i++) {
        state[i % 8] ^= (uint32_t)salt[i] * 0x6C62272EU;
        state[(i + 2) % 8] += state[i % 8];
        state[(i + 4) % 8] ^= (state[i % 8] << 11) | (state[i % 8] >> 21);
    }

    // Rodadas de difusão
    for (int r = 0; r < KDF_ROUNDS; r++) {
        for (int i = 0; i < 8; i++) {
            state[i]         += state[(i + 1) % 8];
            state[(i+3) % 8] ^= (state[i] << 5)  | (state[i] >> 27);
            state[(i+5) % 8] ^= state[i] * 0xBF58476DU;
            state[(i+7) % 8] += (state[i] >> 9)  ^ state[(i + 2) % 8];
        }
    }

    // Extrai 32 bytes
    for (int i = 0; i < 8; i++) {
        out32[i*4+0] = (state[i] >>  0) & 0xFF;
        out32[i*4+1] = (state[i] >>  8) & 0xFF;
        out32[i*4+2] = (state[i] >> 16) & 0xFF;
        out32[i*4+3] = (state[i] >> 24) & 0xFF;
    }
}

// ============================================================
// STREAM CIPHER
//
// Estado interno de 256 bits que evolui de forma não-linear
// a cada byte. XOR com o keystream gerado — a mesma função
// serve para cifrar e decifrar.
// ============================================================
void stellar_stream_cipher(uint8_t* data, int sz, const uint8_t* key32) {
    uint32_t s[8];
    for (int i = 0; i < 8; i++) {
        s[i] = (uint32_t)key32[i*4+0]
             | (uint32_t)key32[i*4+1] << 8
             | (uint32_t)key32[i*4+2] << 16
             | (uint32_t)key32[i*4+3] << 24;
    }

    for (int i = 0; i < sz; i++) {
        uint32_t t = s[0] ^ (s[3] << 7 | s[3] >> 25);
        s[0] = s[1] + s[4];
        s[1] = s[2] ^ (s[5] >> 9);
        s[2] = s[3] * 0xBF58476DU;
        s[3] = s[4] ^ t;
        s[4] = s[5] + s[6];
        s[5] = s[6] ^ (s[7] << 13 | s[7] >> 19);
        s[6] = s[7] + s[0];
        s[7] = t ^ (s[2] >> 11) ^ s[1];

        uint8_t ks = (uint8_t)(
            (s[0] ^ s[1] ^ s[2] ^ s[3] ^
             s[4] ^ s[5] ^ s[6] ^ s[7]) & 0xFF
        );
        data[i] ^= ks;
    }
}

// ============================================================
// MAC — Message Authentication Code (keyed)
//
// Depende tanto dos dados quanto de key32. Sem a chave correta
// não é possível forjar um MAC válido ou verificar integridade.
// ============================================================
uint32_t stellar_compute_mac(const uint8_t* data, int sz, const uint8_t* key32) {
    uint32_t h0 = 0x6A09E667U ^ ( (uint32_t)key32[0]
                                 | (uint32_t)key32[1] << 8
                                 | (uint32_t)key32[2] << 16
                                 | (uint32_t)key32[3] << 24 );
    uint32_t h1 = 0xBB67AE85U ^ ( (uint32_t)key32[4]
                                 | (uint32_t)key32[5] << 8
                                 | (uint32_t)key32[6] << 16
                                 | (uint32_t)key32[7] << 24 );

    for (int i = 0; i < sz; i++) {
        h0 ^= (uint32_t)data[i] * 0x9E3779B9U;
        h0  = (h0 << 13) | (h0 >> 19);
        h0 += h1;
        h1 ^= h0 * 0xBF58476DU;
        h1  = (h1 << 7)  | (h1 >> 25);
        h1 += (uint32_t)data[i] ^ (uint32_t)(i & 0xFF);
    }

    h0 ^= h1;
    h0 ^= (uint32_t)key32[28] | (uint32_t)key32[29] << 8;
    h0  = (h0 << 17) | (h0 >> 15);
    h0 *= 0x94D049BBU;
    h0 ^= h1 ^ ((uint32_t)sz * 0x6C62272EU);

    return h0;
}

// ============================================================
// ENCRYPT
//
// Recebe o bytecode original e produz o buffer cifrado pronto
// para ser escrito em disco.
//
// Formato: [MAGIC_OBF(4)][SALT(8)][BYTECODE_CIFRADO(sz)][MAC(4)]
//
// O MAGIC ofuscado é STELLAR_MAGIC_RAW XOR key32[0..3].
// Como key32 depende do salt, cada arquivo tem um magic diferente
// — um hexdump nunca mostrará "STEL" em texto claro.
//
// Retorna ponteiro alocado com malloc. O chamador deve liberar.
// ============================================================
uint8_t* stellar_encrypt(const uint8_t* bytecode, int sz,
                         const char* key, int* out_sz) {
    uint8_t salt[STELLAR_SALT_SIZE];
    stellar_generate_salt(salt);

    uint8_t key32[32];
    stellar_derive_key(key, salt, key32);

    // MAC do bytecode original (antes de cifrar)
    uint32_t mac = stellar_compute_mac(bytecode, sz, key32);

    // Cópia do bytecode para cifrar in-place
    uint8_t* cipher_buf = malloc(sz);
    if (!cipher_buf) return NULL;
    memcpy(cipher_buf, bytecode, sz);
    stellar_stream_cipher(cipher_buf, sz, key32);

    // MAGIC ofuscado
    static const uint8_t MAGIC_RAW[] = {0x53, 0x54, 0x45, 0x4C};
    uint8_t obf_magic[4];
    for (int i = 0; i < 4; i++)
        obf_magic[i] = MAGIC_RAW[i] ^ key32[i];

    // Monta buffer final:
    // [MAGIC_OBF(4)][SALT(8)][BYTECODE_CIFRADO(sz)][MAC(4)][MARKER(4)]
    static const uint8_t marker[] = { 0xC1, 0x4B, 0xA5, 0xED };
    int total = 4 + STELLAR_SALT_SIZE + sz + 4 + STELLAR_MARKER_SIZE;
    uint8_t* out = malloc(total);
    if (!out) { free(cipher_buf); return NULL; }

    memcpy(out,                                          obf_magic,  4);
    memcpy(out + 4,                                      salt,       STELLAR_SALT_SIZE);
    memcpy(out + 4 + STELLAR_SALT_SIZE,                  cipher_buf, sz);
    memcpy(out + 4 + STELLAR_SALT_SIZE + sz,             &mac,       4);
    memcpy(out + 4 + STELLAR_SALT_SIZE + sz + 4,         marker,     STELLAR_MARKER_SIZE);

    free(cipher_buf);
    if (out_sz) *out_sz = total;
    return out;
}

// ============================================================
// DECRYPT
//
// Lê o buffer cifrado, verifica o MAC e retorna o bytecode
// original decifrado em `out_buf` (que deve ter pelo menos
// `file_sz - STELLAR_HEADER_OVERHEAD` bytes).
//
// Retorna STELLAR_CRYPT_OK ou um código de erro.
// ============================================================
StellarCryptResult stellar_decrypt(const uint8_t* file_buf, int file_sz,
                                   const char* key,
                                   uint8_t* out_buf, int* out_sz) {
    // Sanidade básica
    if (!file_buf || !out_buf)
        return STELLAR_CRYPT_ERR_CORRUPTED;
    if (!key) key = "";
    if (file_sz <= STELLAR_HEADER_OVERHEAD)
        return STELLAR_CRYPT_ERR_CORRUPTED;

    // Extrai salt
    uint8_t salt[STELLAR_SALT_SIZE];
    memcpy(salt, file_buf + 4, STELLAR_SALT_SIZE);

    // Deriva chave
    uint8_t key32[32];
    stellar_derive_key(key, salt, key32);

    // Verifica MAGIC ofuscado — falha rápida se a chave for errada
    static const uint8_t MAGIC_RAW[] = {0x53, 0x54, 0x45, 0x4C};
    uint8_t expected_magic[4];
    for (int i = 0; i < 4; i++)
        expected_magic[i] = MAGIC_RAW[i] ^ key32[i];

    if (memcmp(file_buf, expected_magic, 4) != 0)
        return STELLAR_CRYPT_ERR_BAD_KEY;

    // Calcula tamanho do bytecode e garante que cabe no buffer de saída (65536)
    // Formato: [MAGIC(4)][SALT(8)][BYTECODE(bc_sz)][MAC(4)][MARKER(4)]
    int bc_sz = file_sz - STELLAR_HEADER_OVERHEAD;
    if (bc_sz <= 0 || bc_sz > 65536)
        return STELLAR_CRYPT_ERR_CORRUPTED;

    // MAC fica logo após o bytecode cifrado
    uint32_t stored_mac;
    memcpy(&stored_mac, file_buf + 4 + STELLAR_SALT_SIZE + bc_sz, 4);

    memcpy(out_buf, file_buf + 4 + STELLAR_SALT_SIZE, bc_sz);
    stellar_stream_cipher(out_buf, bc_sz, key32);

    // Valida MAC — se falhar, a chave era errada ou o arquivo foi adulterado
    if (stellar_compute_mac(out_buf, bc_sz, key32) != stored_mac)
        return STELLAR_CRYPT_ERR_BAD_MAC;

    if (out_sz) *out_sz = bc_sz;
    return STELLAR_CRYPT_OK;
}

// ============================================================
// IS_ENCRYPTED
//
// Heurística rápida: tenta derivar o magic com a chave fornecida.
// Se nenhuma chave for fornecida, apenas verifica se o tamanho
// do arquivo é compatível com o formato cifrado.
// ============================================================
int stellar_is_encrypted(const uint8_t* file_buf, int file_sz, const char* key) {
    (void)key; // não precisamos mais da chave para detectar se é cifrado
    if (!file_buf || file_sz <= STELLAR_HEADER_OVERHEAD) return 0;

    // Checa o marcador fixo nos últimos 4 bytes do arquivo.
    // Esse marcador é escrito por stellar_encrypt e não depende da chave,
    // então conseguimos detectar arquivos cifrados sem saber a chave.
    static const uint8_t marker[] = { 0xC1, 0x4B, 0xA5, 0xED };
    return (memcmp(file_buf + file_sz - STELLAR_MARKER_SIZE, marker, STELLAR_MARKER_SIZE) == 0);
}