#ifndef HIGHPRIEST_DEVICE_MD5_H
#define HIGHPRIEST_DEVICE_MD5_H




typedef unsigned char md5_byte_t;
typedef unsigned int md5_word_t;

typedef struct md5_state_s {
    md5_word_t count[2];
    md5_word_t abcd[4];
    md5_byte_t buf[64];
} md5_state_t;

class MD5 {
public:
    void init();
    void append(const md5_byte_t *data, int nbytes);
    void finish(md5_byte_t digest[16]);
    void result(const md5_byte_t *data, int nbytes, md5_byte_t digest[16])
    {
        init();
        append(data, nbytes);
        finish(digest);
    }
protected:
    void process(const md5_byte_t *data);
    md5_state_t	m_ms;
};

#endif
