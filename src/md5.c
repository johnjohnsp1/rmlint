/*
**********************************************************************
** md5.c                                                            **
** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
** Created: 2/17/90 RLR                                             **
** Revised: 1/91 SRD,AJ,BSK,JT Reference C Version                  **
**********************************************************************
*/

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */


/* #include changed */
#include "md5.h"
#include "rmlint.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <alloca.h>
#include <math.h>
/* Till MDPrintArr no change */
/* md5.c:
   1) Methods to calculate md5sums
   2) Method to build a short fingerprint of a file
   3) Method to build a 'full' (not quite) md5sum of a file
*/
/* forward declaration */
static void Transform ();

static unsigned char PADDING[64] =
{
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* F, G and H are basic MD5 functions: selection, majority, parity */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
  {(a) += F ((b), (c), (d)) + (x) + (nuint_t)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) \
  {(a) += G ((b), (c), (d)) + (x) + (nuint_t)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) \
  {(a) += H ((b), (c), (d)) + (x) + (nuint_t)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) \
  {(a) += I ((b), (c), (d)) + (x) + (nuint_t)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }

static void MD5Init (mdContext)
MD5_CTX *mdContext;
{
    mdContext->i[0] = mdContext->i[1] = (nuint_t)0;

    /* Load magic initialization constants.
     */
    mdContext->buf[0] = (nuint_t)0x67452301;
    mdContext->buf[1] = (nuint_t)0xefcdab89;
    mdContext->buf[2] = (nuint_t)0x98badcfe;
    mdContext->buf[3] = (nuint_t)0x10325476;
}

static void MD5Update (mdContext, inBuf, inLen)
MD5_CTX *mdContext;
unsigned char *inBuf;
unsigned int inLen;
{
    nuint_t in[16];
    int mdi;
    unsigned int i, ii;

    /* compute number of bytes mod 64 */
    mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

    /* update number of bits */
    if ((mdContext->i[0] + ((nuint_t)inLen << 3)) < mdContext->i[0])
    {
        mdContext->i[1]++;
    }
    mdContext->i[0] += ((nuint_t)inLen << 3);
    mdContext->i[1] += ((nuint_t)inLen >> 29);

    while (inLen--)
    {
        /* add new character to buffer, increment mdi */
        mdContext->in[mdi++] = *inBuf++;

        /* transform if necessary */
        if (mdi == 0x40)
        {
            for (i = 0, ii = 0; i < 16; i++, ii += 4)
                in[i] = (((nuint_t)mdContext->in[ii+3]) << 24) |
                        (((nuint_t)mdContext->in[ii+2]) << 16) |
                        (((nuint_t)mdContext->in[ii+1]) << 8) |
                        ((nuint_t)mdContext->in[ii]);
            Transform (mdContext->buf, in);
            mdi = 0;
        }
    }
}

static void MD5Final (mdContext)
MD5_CTX *mdContext;
{
    nuint_t in[16];
    int mdi;
    unsigned int i, ii;
    unsigned int padLen;

    /* save number of bits */
    in[14] = mdContext->i[0];
    in[15] = mdContext->i[1];

    /* compute number of bytes mod 64 */
    mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

    /* pad out to 56 mod 64 */
    padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
    MD5Update (mdContext, PADDING, padLen);

    /* append length in bits and transform */
    for (i = 0, ii = 0; i < 14; i++, ii += 4)
        in[i] = (((nuint_t)mdContext->in[ii+3]) << 24) |
                (((nuint_t)mdContext->in[ii+2]) << 16) |
                (((nuint_t)mdContext->in[ii+1]) << 8) |
                ((nuint_t)mdContext->in[ii]);
    Transform (mdContext->buf, in);

    /* store buffer in digest */
    for (i = 0, ii = 0; i < 4; i++, ii += 4)
    {
        mdContext->digest[ii] = (unsigned char)(mdContext->buf[i] & 0xFF);
        mdContext->digest[ii+1] =
            (unsigned char)((mdContext->buf[i] >> 8) & 0xFF);
        mdContext->digest[ii+2] =
            (unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
        mdContext->digest[ii+3] =
            (unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
    }
}

/* Basic MD5 step. Transform buf based on in.
 */
static void Transform (buf, in)
nuint_t *buf;
nuint_t *in;
{
    nuint_t a = buf[0], b = buf[1], c = buf[2], d = buf[3];

    /* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
    FF ( a, b, c, d, in[ 0], S11, 3614090360UL); /* 1 */
    FF ( d, a, b, c, in[ 1], S12, 3905402710UL); /* 2 */
    FF ( c, d, a, b, in[ 2], S13,  606105819UL); /* 3 */
    FF ( b, c, d, a, in[ 3], S14, 3250441966UL); /* 4 */
    FF ( a, b, c, d, in[ 4], S11, 4118548399UL); /* 5 */
    FF ( d, a, b, c, in[ 5], S12, 1200080426UL); /* 6 */
    FF ( c, d, a, b, in[ 6], S13, 2821735955UL); /* 7 */
    FF ( b, c, d, a, in[ 7], S14, 4249261313UL); /* 8 */
    FF ( a, b, c, d, in[ 8], S11, 1770035416UL); /* 9 */
    FF ( d, a, b, c, in[ 9], S12, 2336552879UL); /* 10 */
    FF ( c, d, a, b, in[10], S13, 4294925233UL); /* 11 */
    FF ( b, c, d, a, in[11], S14, 2304563134UL); /* 12 */
    FF ( a, b, c, d, in[12], S11, 1804603682UL); /* 13 */
    FF ( d, a, b, c, in[13], S12, 4254626195UL); /* 14 */
    FF ( c, d, a, b, in[14], S13, 2792965006UL); /* 15 */
    FF ( b, c, d, a, in[15], S14, 1236535329UL); /* 16 */

    /* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
    GG ( a, b, c, d, in[ 1], S21, 4129170786UL); /* 17 */
    GG ( d, a, b, c, in[ 6], S22, 3225465664UL); /* 18 */
    GG ( c, d, a, b, in[11], S23,  643717713UL); /* 19 */
    GG ( b, c, d, a, in[ 0], S24, 3921069994UL); /* 20 */
    GG ( a, b, c, d, in[ 5], S21, 3593408605UL); /* 21 */
    GG ( d, a, b, c, in[10], S22,   38016083UL); /* 22 */
    GG ( c, d, a, b, in[15], S23, 3634488961UL); /* 23 */
    GG ( b, c, d, a, in[ 4], S24, 3889429448UL); /* 24 */
    GG ( a, b, c, d, in[ 9], S21,  568446438UL); /* 25 */
    GG ( d, a, b, c, in[14], S22, 3275163606UL); /* 26 */
    GG ( c, d, a, b, in[ 3], S23, 4107603335UL); /* 27 */
    GG ( b, c, d, a, in[ 8], S24, 1163531501UL); /* 28 */
    GG ( a, b, c, d, in[13], S21, 2850285829UL); /* 29 */
    GG ( d, a, b, c, in[ 2], S22, 4243563512UL); /* 30 */
    GG ( c, d, a, b, in[ 7], S23, 1735328473UL); /* 31 */
    GG ( b, c, d, a, in[12], S24, 2368359562UL); /* 32 */

    /* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
    HH ( a, b, c, d, in[ 5], S31, 4294588738UL); /* 33 */
    HH ( d, a, b, c, in[ 8], S32, 2272392833UL); /* 34 */
    HH ( c, d, a, b, in[11], S33, 1839030562UL); /* 35 */
    HH ( b, c, d, a, in[14], S34, 4259657740UL); /* 36 */
    HH ( a, b, c, d, in[ 1], S31, 2763975236UL); /* 37 */
    HH ( d, a, b, c, in[ 4], S32, 1272893353UL); /* 38 */
    HH ( c, d, a, b, in[ 7], S33, 4139469664UL); /* 39 */
    HH ( b, c, d, a, in[10], S34, 3200236656UL); /* 40 */
    HH ( a, b, c, d, in[13], S31,  681279174UL); /* 41 */
    HH ( d, a, b, c, in[ 0], S32, 3936430074UL); /* 42 */
    HH ( c, d, a, b, in[ 3], S33, 3572445317UL); /* 43 */
    HH ( b, c, d, a, in[ 6], S34,   76029189UL); /* 44 */
    HH ( a, b, c, d, in[ 9], S31, 3654602809UL); /* 45 */
    HH ( d, a, b, c, in[12], S32, 3873151461UL); /* 46 */
    HH ( c, d, a, b, in[15], S33,  530742520UL); /* 47 */
    HH ( b, c, d, a, in[ 2], S34, 3299628645UL); /* 48 */

    /* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
    II ( a, b, c, d, in[ 0], S41, 4096336452UL); /* 49 */
    II ( d, a, b, c, in[ 7], S42, 1126891415UL); /* 50 */
    II ( c, d, a, b, in[14], S43, 2878612391UL); /* 51 */
    II ( b, c, d, a, in[ 5], S44, 4237533241UL); /* 52 */
    II ( a, b, c, d, in[12], S41, 1700485571UL); /* 53 */
    II ( d, a, b, c, in[ 3], S42, 2399980690UL); /* 54 */
    II ( c, d, a, b, in[10], S43, 4293915773UL); /* 55 */
    II ( b, c, d, a, in[ 1], S44, 2240044497UL); /* 56 */
    II ( a, b, c, d, in[ 8], S41, 1873313359UL); /* 57 */
    II ( d, a, b, c, in[15], S42, 4264355552UL); /* 58 */
    II ( c, d, a, b, in[ 6], S43, 2734768916UL); /* 59 */
    II ( b, c, d, a, in[13], S44, 1309151649UL); /* 60 */
    II ( a, b, c, d, in[ 4], S41, 4149444226UL); /* 61 */
    II ( d, a, b, c, in[11], S42, 3174756917UL); /* 62 */
    II ( c, d, a, b, in[ 2], S43,  718787259UL); /* 63 */
    II ( b, c, d, a, in[ 9], S44, 3951481745UL); /* 64 */

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

/* ------------------------------------------------------------- */


/*********************************************/
/** Original md5.c modified from now on ~CP **/
/*********************************************/

/* Prints a md5digest in human readable representation */
void MDPrintArr(unsigned char *digest)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        printf ("%02x", digest[i]);
    }
}

/* ------------------------------------------------------------- */

/* Mutexes to (pseudo-)"serialize" IO (avoid unnecessary jumping) */
pthread_mutex_t mutex_fp_IO = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ck_IO = PTHREAD_MUTEX_INITIALIZER;

/* Some init call */
void md5c_c_init(void)
{
    pthread_mutex_init(&mutex_ck_IO,NULL);
    pthread_mutex_init(&mutex_fp_IO,NULL);
}

/* ------------------------------------------------------------- */

/* used to calc the complete checksum of file & save it in File */
void md5_file(lint_t *file)
{
    /* Number of bytes read in */
    int bytes=0;

    /* Input stream */
    FILE *inFile=NULL;

    /* the md5buf */
    MD5_CTX mdContext;

    /* tmp buffer */
    unsigned char *data=NULL;

    /* Don't read the $already_read amount of bytes read by md5_fingerprint */
    nuint_t already_read = MD5_FPSIZE_FORM(file->fsize); 
    
    /* This is some rather seldom case, but skip checksum building here */
    if(file->fsize <= (already_read*2))
    {
        return;    
    }

    /*
     * Little note: valgrind might record 'data' as 'not stack'd, malloc'd or (recently) free'd',
     *              I guess (no, Im not 100% sure) this is just because of the normal stacksize of
     *              a thread, valgrind seems not to take note that thread's stacksize might enlarge,
     *              as this is what iirc alloca() does (incrementing the stackframe pointer)
     *              So for now I will ignore this one, just to let you know.
     *
     * Exact message valgrind's giving me:

            ==3118== Syscall param read(buf) points to unaddressable byte(s)
            ==3118==    at 0x538B7ED: read (in /lib/libc-2.12.1.so)
            ==3118==    by 0x53379C2: ??? (in /lib/libc-2.12.1.so)
            ==3118==    by 0x532C3B2: fread (in /lib/libc-2.12.1.so)
            ==3118==    by 0x403C63: md5_file (md5.c:357)
            ==3118==    by 0x404E93: cksum_cb (filter.c:567)
            ==3118==    by 0x404FAF: build_checksums (filter.c:595)
            ==3118==    by 0x4051EB: sheduler_cb (filter.c:667)
            ==3118==    by 0x4053CF: start_sheduler (filter.c:730)
            ==3118==    by 0x40613B: start_processing (filter.c:1090)
            ==3118==    by 0x4095E5: rmlint_main (rmlint.c:738)
            ==3118==    by 0x409643: main (main.c:32)
            ==3118==  Address 0x7ff001000 is not stack'd, malloc'd or (recently) free'd
            ==3118==
     * 
     */

    /* Allocate buffer on the thread's stack */
    data = alloca((MD5_IO_BLOCKSIZE > file->fsize) ? (file->fsize + 1) : MD5_IO_BLOCKSIZE);

    /*data = alloca(MD5_IO_BLOCKSIZE);*/
    inFile = fopen (file->path, "rq");

    /* Can't open file? */
    if(inFile == NULL)
    {
        return;
    }

    /* Init md5sum & jump to start */
    MD5Init (&mdContext);
    fseek(inFile, already_read, SEEK_SET);

    do
    {

/* If (pseudo-)serialized IO is requested lock a mutex, so other threads have to wait here
 * This is to prevent that several threads call fread() at the same time, what would case the
 * HD to jump a lot around without doing anything intelligent..
 * */
#if (MD5_SERIAL_IO == 1)
        pthread_mutex_lock(&mutex_ck_IO);
#endif
        /* The call to fread */
        bytes = fread (data, 1, MD5_IO_BLOCKSIZE, inFile);

/* Unlock */
#if (MD5_SERIAL_IO == 1)
        pthread_mutex_unlock(&mutex_ck_IO);
#endif

        /* Update the checksum with the current contents of &data */
        MD5Update (&mdContext, data, bytes);
    }
    while (bytes && (ftell(inFile) < (file->fsize - already_read)));

    /* Finalize, copy and close */
    MD5Final (&mdContext);
    memcpy(file->md5_digest, mdContext.digest, MD5_LEN);
    fclose (inFile);
}

/* ------------------------------------------------------------- */

/* Reads <readsize> bytes from each start and end + 8 bytes in the middle
   start and end gets converted into a 128bit md5sum and are written in <file>
   the 8 byte are stored in raw form
*/

void md5_fingerprint(lint_t *file, const nuint_t readsize)
{
    int bytes = 0;
    FILE *pF = fopen(file->path, "r");
    unsigned char *data = alloca(readsize);
    MD5_CTX con;

    /* empty? */
    if(!pF)
    {
        if(set->verbosity > 3)
        {
            warning(YEL"WARN: "NCO"Cannot open %s",file->path);
        }
        return;
    }

#if (MD5_SERIAL_IO == 1)
    pthread_mutex_lock(&mutex_fp_IO);
#endif

    /* Read the first block */
    bytes = fread(data,sizeof(char),readsize,pF);
    
#if (MD5_SERIAL_IO == 1)
    pthread_mutex_unlock(&mutex_fp_IO);
#endif

    /* Compute md5sum of this block */
    if(bytes)
    {
        MD5Init (&con);
        MD5Update (&con, data, bytes);
        MD5Final (&con);
        memcpy(file->fp[0],con.digest,MD5_LEN);
    }

#if (MD5_SERIAL_IO == 1)
    pthread_mutex_lock(&mutex_fp_IO);
#endif

    /* Jump to middle of file and read a couple of bytes there s*/
    fseek(pF, file->fsize/2 ,SEEK_SET);
    bytes = fread(file->bim, sizeof(char), BYTE_MIDDLE_SIZE, pF);

    /* Jump to end and read final block */
    fseek(pF, -readsize,SEEK_END);
    bytes = fread(data,sizeof(char),readsize,pF);
    
#if (MD5_SERIAL_IO == 1)
    pthread_mutex_unlock(&mutex_fp_IO);
#endif

    /* Compute checksum of this last block */
    if(bytes)
    {
        MD5Init (&con);
        MD5Update (&con, data, bytes);
        MD5Final (&con);
        memcpy(file->fp[1],con.digest,MD5_LEN);
    }

    /* kthxbai */
    fclose(pF);
}
