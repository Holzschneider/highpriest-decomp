/* $Header: /Library.root/Servers/Character_net3/CharCipher.h 3     04-10-01 3:57p Kare $
 * $Log: /Library.root/Servers/Character_net3/CharCipher.h $
 * 
 * 3     04-10-01 3:57p Kare
 * Keyword Expansion Ãß°¡
 *
 */

#pragma once


#define USE_MACGUFFIN
#define USE_SNEFRU
#define USE_RMD128
#define USE_SAFER
#define USE_CAST128
#define USE_CRYPTON
#define USE_TURTLE
#define USE_TIGER
#define USE_FEAL8
#define USE_MISTY
#define USE_SEAL2


//============================================================================
#ifdef USE_MACGUFFIN
#define ROUNDS 32 
#define KSIZE (ROUNDS*3) 
/* expanded key structure */ 
typedef struct mcg_key {
    unsigned short val[KSIZE]; 
} mcg_key;


void mcg_keyset(unsigned char* key,mcg_key* ek);
void mcg_block_encrypt(unsigned char* blk,mcg_key* key);
void mcg_block_decrypt(unsigned char* blk,mcg_key* key);
#endif // USE_MACGUFFIN
//============================================================================
#ifdef USE_SNEFRU

void Hash512(unsigned int* output, unsigned int* input);
void HashN(unsigned int* output, unsigned int* input);
#endif // USE_SNEFRU
//============================================================================
#ifdef USE_RMD128
/* typedef 8 and 32 bit types, resp.  */
/* adapt these, if necessary, 
   for your operating system and compiler */
typedef    unsigned char        byte;
typedef    unsigned int        dword;

/* macro definitions */

/* collect four bytes into one word: */
#define BYTES_TO_DWORD(strptr)                    \
            (((dword) *((strptr)+3) << 24) | \
             ((dword) *((strptr)+2) << 16) | \
             ((dword) *((strptr)+1) <<  8) | \
             ((dword) *(strptr)))

/* ROL(x, n) cyclically rotates x over n bits to the left */
/* x must be of an unsigned 32 bits type and 0 <= n < 32. */
#define ROL(x, n)        (((x) << (n)) | ((x) >> (32-(n))))
/* the four basic functions F(), G() and H() */
#define F(x, y, z)        ((x) ^ (y) ^ (z)) 
#define G(x, y, z)        (((x) & (y)) | (~(x) & (z))) 
#define H(x, y, z)        (((x) | ~(y)) ^ (z))
#define I(x, y, z)        (((x) & (z)) | ((y) & ~(z))) 
  
/* the eight basic operations FF() through III() */
#define FF(a, b, c, d, x, s)        {\
      (a) += F((b), (c), (d)) + (x);\
      (a) = ROL((a), (s));\
   }
#define GG(a, b, c, d, x, s)        {\
      (a) += G((b), (c), (d)) + (x) + 0x5a827999UL;\
      (a) = ROL((a), (s));\
   }
#define HH(a, b, c, d, x, s)        {\
      (a) += H((b), (c), (d)) + (x) + 0x6ed9eba1UL;\
      (a) = ROL((a), (s));\
   }
#define II(a, b, c, d, x, s)        {\
      (a) += I((b), (c), (d)) + (x) + 0x8f1bbcdcUL;\
      (a) = ROL((a), (s));\
   }
#define FFF(a, b, c, d, x, s)        {\
      (a) += F((b), (c), (d)) + (x);\
      (a) = ROL((a), (s));\
   }
#define GGG(a, b, c, d, x, s)        {\
      (a) += G((b), (c), (d)) + (x) + 0x6d703ef3UL;\
      (a) = ROL((a), (s));\
   }
#define HHH(a, b, c, d, x, s)        {\
      (a) += H((b), (c), (d)) + (x) + 0x5c4dd124UL;\
      (a) = ROL((a), (s));\
   }
#define III(a, b, c, d, x, s)        {\
      (a) += I((b), (c), (d)) + (x) + 0x50a28be6UL;\
      (a) = ROL((a), (s));\
   }

/* function prototypes */

void MDinit(dword *MDbuf);
/*
 *  initializes MDbuffer to "magic constants"
 */


void compress(dword *MDbuf, dword *X);
/*
 *  the compression function.
 *  transforms MDbuf using message bytes X[0] through X[15]
 */


void MDfinish(dword *MDbuf, byte *strptr, dword lswlen, dword mswlen);
/*
 *  puts bytes from strptr into X and pad out; appends length 
 *  and finally, compresses the last block(s)
 *  note: length in bits == 8 * (lswlen + 2^32 mswlen).
 *  note: there are (lswlen mod 64) bytes left in strptr.
 */
#endif //USE_RMD128

//============================================================================
#ifdef USE_SAFER
/******************* Constants ************************************************/
#define SAFER_K64_DEFAULT_NOF_ROUNDS     6
#define SAFER_K128_DEFAULT_NOF_ROUNDS   10
#define SAFER_SK64_DEFAULT_NOF_ROUNDS    8
#define SAFER_SK128_DEFAULT_NOF_ROUNDS  10
#define SAFER_MAX_NOF_ROUNDS            13
#define SAFER_BLOCK_LEN                  8
#define SAFER_KEY_LEN     (1 + SAFER_BLOCK_LEN * (1 + 2 * SAFER_MAX_NOF_ROUNDS))

/******************* Assertions ***********************************************/

/******************* Macros ***************************************************/
 
/******************* Types ****************************************************/
typedef unsigned char safer_block_t[SAFER_BLOCK_LEN];
typedef unsigned char safer_key_t[SAFER_KEY_LEN];

/******************* Module Data **********************************************/

/******************* Prototypes ***********************************************/

/*******************************************************************************
* void Safer_Init_Module(void)
*
*   initializes this module.
*
********************************************************************************
* void Safer_Expand_Userkey(safer_block_t userkey_1,
*                           safer_block_t userkey_2,
*                           unsigned int nof_rounds,
*                           int strengthened,
*                           safer_key_t key)
*
*   expands a user-selected key of length 64 bits or 128 bits to a encryption /
*   decryption key. If your user-selected key is of length 64 bits, then give
*   this key to both arguments 'userkey_1' and 'userkey_2', e.g.
*   'Safer_Expand_Userkey(z, z, key)'. Note: SAFER K-64 and SAFER SK-64 with a
*   user-selected key 'z' of length 64 bits are identical to SAFER K-128 and
*   SAFER SK-128 with a user-selected key 'z z' of length 128 bits,
*   respectively.
*   pre:  'userkey_1'  contains the first 64 bits of user key.
*         'userkey_2'  contains the second 64 bits of user key.
*         'nof_rounds' contains the number of encryption rounds
*                      'nof_rounds' <= 'SAFER_MAX_NOF_ROUNDS'
*         'strengthened' is non-zero if the strengthened key schedule should be
*                      used and zero if the original key schedule should be
*                      used.
*   post: 'key'        contains the expanded key.
*
********************************************************************************
* void Safer_Encrypt_Block(safer_block_t block_in, safer_key_t key, 
*                          safer_block_t block_out)
*
*   encryption algorithm.
*   pre:  'block_in'  contains the plain-text block.
*         'key'       contains the expanded key.
*   post: 'block_out' contains the cipher-text block.
*
********************************************************************************
* void Safer_Decrypt_Block(safer_block_t block_in, safer_key_t key,
*                          safer_block_t block_out)
*
*   decryption algorithm.
*   pre:  'block_in'  contains the cipher-text block.
*         'key'       contains the expanded key.
*   post: 'block_out' contains the plain-text block.
*
*******************************************************************************/


extern void Safer_Init_Module(void);
extern void Safer_Expand_Userkey(safer_block_t userkey_1,
                                    safer_block_t userkey_2,
                                    unsigned int nof_rounds,
                                    int strengthened,
                                    safer_key_t key);
extern void Safer_Encrypt_Block (safer_block_t block_in, safer_key_t key, 
                                    safer_block_t block_out);
extern void Safer_Decrypt_Block (safer_block_t block_in, safer_key_t key,
                                    safer_block_t block_out);
#endif //USE_SAFER
//============================================================================

//============================================================================
#ifdef USE_CAST128
typedef unsigned char u8;	/* 8-bit unsigned */
typedef unsigned int u32;	/* 32-bit unsigned */

typedef struct {
	u32 xkey[32];	/* Key, after expansion */
	int rounds;		/* Number of rounds to use, 12 or 16 */
} cast_key;

void cast_setkey(cast_key* key, u8* rawkey, int keybytes);
void cast_encrypt(cast_key* key, u8* inblock, u8* outblock);
void cast_decrypt(cast_key* key, u8* inblock, u8* outblock);
#endif // USE_CAST128
//============================================================================
#ifdef USE_CRYPTON
typedef unsigned char   u1byte; /* an 8 bit unsigned character type */
typedef unsigned short  u2byte; /* a 16 bit unsigned integer type   */
typedef unsigned int   u4byte; /* a 32 bit unsigned integer type   */

typedef signed char     s1byte; /* an 8 bit signed character type   */
typedef signed short    s2byte; /* a 16 bit signed integer type     */
typedef int     s4byte; /* a 32 bit signed integer type     */
u4byte *crypton_set_key(const u4byte in_key[], const u4byte key_len);
void crypton_encrypt(const u4byte in_blk[4], u4byte out_blk[4]);
void crypton_decrypt(const u4byte in_blk[4], u4byte out_blk[4]);
#endif // USE_CRYPTON
//============================================================================
#ifdef USE_TURTLE
/* 
* turtle/hare reference implementation 
* Matt Blaze 
* September, 1996 
*/ 
#define TURTLEMAXN 8 /* max blocksize in TURTLEWORDs */

/* these next three must be changed together */ 
#define TURTLEBITS 8 /* word size in bits */ 
#define NTURTLEWORDS 256 /* 2^TURTLEBITS */ 
#define TURTLEWORD unsigned char /* unsigned data type with TURTLEBITS bits */ 

/* 
* basic turtle key data structure 
*/ 
typedef struct {
    int n; /* the blocksize (must be power of 2 and at least 2) */ 
    int rr[4]; /* the outer round subkey indexes */ 
    TURTLEWORD sbox[TURTLEMAXN*TURTLEMAXN][NTURTLEWORDS]; /* key tables */ 
} TK; 

/* 
* basic hare key data structure 
*/ 
typedef struct {
    TURTLEWORD r, l;/* current r and l counter state */ 
    int table; /* current table (0 or 1) */ 
    TURTLEWORD sbox[2][4][NTURTLEWORDS]; 
} HK; 

int turtle_key(TURTLEWORD *shortkey,int len, TK *key, int n);
int turtle_encrypt(TURTLEWORD *blk,TK *key);
int turtle_decrypt(TURTLEWORD *blk,TK *key); 

int hare_key(TURTLEWORD* shortkey,int len,HK *key);
TURTLEWORD hare_stream(HK* key); 
#endif // USE_TURTLE
//============================================================================
#ifdef USE_TIGER
typedef unsigned int word32;
typedef unsigned char byte;
void tiger(word32 *str, word32 length, word32 res[6]);
#endif // USE_TIGER
//============================================================================
#ifdef USE_SEAL2
#define WORDS_PER_SEAL_CALL 1024
typedef struct {
    unsigned int t[520]; /* 512 rounded up to a multiple of 5 + 5*/
    unsigned int s[265]; /* 256 rounded up to a multiple of 5 + 5*/
    unsigned int r[20]; /* 16 rounded up to multiple of 5 */
    unsigned int counter; /* 32-bit synch value. */
    unsigned int ks_buf[WORDS_PER_SEAL_CALL];
    int ks_pos;
} seal_ctx;

void seal_encrypt(seal_ctx *c, unsigned int *data_ptr, int w);
void seal_decrypt(seal_ctx *c, unsigned int *data_ptr, int w);
void seal_key(seal_ctx *c, unsigned char *key);
#endif // USE_SEAL2
//============================================================================
#ifdef USE_FEAL8
typedef unsigned char ByteType ;
void feal8_SetKey( ByteType *KP ) ;
void feal8_Decrypt( ByteType *Cipher, ByteType *Plain ) ;
void feal8_Encrypt( ByteType *Plain, ByteType *Cipher ) ;
#endif // USE_FEAL8
//============================================================================
#ifdef USE_MISTY

#define MISTY1_KEYSIZE 32

typedef unsigned int u4;
typedef unsigned char byte;

void misty1_keyinit(u4  *ek, u4  *k) ;
void misty1_decrypt_block(u4  *ek,u4  c[2], u4  p[2]) ;
void misty1_encrypt_block(u4  *ek, u4  p[2], u4  c[2]) ;

#endif // USE_MISTY
//============================================================================
