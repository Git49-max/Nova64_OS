#ifndef STELLAR_ENCRYPT_H
#define STELLAR_ENCRYPT_H

#include <stdint.h>

// ============================================================
// Constantes do formato cifrado
// ============================================================

#define STELLAR_SALT_SIZE       8
#define KDF_ROUNDS              64

// Marcador fixo no FIM do arquivo cifrado — não depende da chave.
// Permite detectar "esse arquivo é cifrado" sem precisar da chave.
#define STELLAR_MARKER_SIZE     4
static const uint8_t STELLAR_CIPHER_MARKER[4] = { 0xC1, 0x4B, 0xA5, 0xED };

// Overhead total: MAGIC(4) + SALT(8) + MAC(4) + MARKER(4)
#define STELLAR_HEADER_OVERHEAD (4 + STELLAR_SALT_SIZE + 4 + STELLAR_MARKER_SIZE)

// ============================================================
// Códigos de retorno de stellar_decrypt
// ============================================================
typedef enum {
    STELLAR_CRYPT_OK             =  0,
    STELLAR_CRYPT_ERR_BAD_KEY    = -1,  // MAGIC não bateu — chave errada
    STELLAR_CRYPT_ERR_BAD_MAC    = -2,  // MAC inválido — chave errada ou arquivo corrompido
    STELLAR_CRYPT_ERR_CORRUPTED  = -3,  // Arquivo muito pequeno / malformado
} StellarCryptResult;

// ============================================================
// API pública
// ============================================================

// Gera STELLAR_SALT_SIZE bytes aleatórios usando /dev/urandom
// (ou fallback via time + ASLR em sistemas sem /dev/urandom).
void stellar_generate_salt(uint8_t* salt);

// Deriva 32 bytes de material de chave a partir de `key` + `salt`.
// Resultado escrito em `out32[32]`.
void stellar_derive_key(const char* key, const uint8_t* salt, uint8_t* out32);

// Stream cipher reversível: cifra ou decifra `data[sz]` com `key32[32]`.
void stellar_stream_cipher(uint8_t* data, int sz, const uint8_t* key32);

// MAC keyed de 32 bits sobre `data[sz]` usando `key32[32]`.
uint32_t stellar_compute_mac(const uint8_t* data, int sz, const uint8_t* key32);

// Cifra `bytecode[sz]` com `key`. Retorna buffer alocado (caller libera).
// Escreve o tamanho total em `*out_sz` se não-NULL.
uint8_t* stellar_encrypt(const uint8_t* bytecode, int sz,
                         const char* key, int* out_sz);

// Decifra e verifica `file_buf[file_sz]`. Escreve bytecode em `out_buf`.
// `out_buf` deve ter pelo menos `file_sz - STELLAR_HEADER_OVERHEAD` bytes.
// Retorna um StellarCryptResult.
StellarCryptResult stellar_decrypt(const uint8_t* file_buf, int file_sz,
                                   const char* key,
                                   uint8_t* out_buf, int* out_sz);

// Retorna 1 se o arquivo parece ser um binário Stellar cifrado.
// Se `key` for não-NULL, verifica usando a chave real.
int stellar_is_encrypted(const uint8_t* file_buf, int file_sz, const char* key);

#endif // STELLAR_ENCRYPT_H