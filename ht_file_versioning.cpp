#include "ht_file_versioning.h"

#include "one_at_time.hpp"
#include "htb64.h"

#include <sstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <map>

uint8_t HTFileVersioning::bklenght = 12;

template < typename Iterator >
Iterator HTDataCompress::lzw_compress(const char *uncompressed, uint32_t size, Iterator result) 
{
    int dictSize = 256;
    std::map<std::string,int> dictionary;

    for (int i = 0; i < 256; i++)
        dictionary[std::string(1, i)] = i;

    std::string w;
    for (uint32_t it=0; it<size; ++it) {
        char c = uncompressed[it];
        std::string wc = w + c;
        if (dictionary.count(wc))
            w = wc;
        else {
            *result++ = dictionary[w];
            dictionary[wc] = dictSize++;
            w = std::string(1, c);
        }
    }

    if (!w.empty())
        *result++ = dictionary[w];
    return result;
}

void HTDataCompress::compress(uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len)
{
    uint16_t bits = 1;
    uint32_t max = 0;
    uint32_t bits_size = 0;

    std::vector<uint32_t> compressed;
    HTDataCompress::lzw_compress(
        (const char*)in,
        in_len,
        std::back_inserter(compressed)
    );

    for (unsigned a=0; a<compressed.size(); ++a)
        if (max < compressed[a]) 
            max = compressed[a];

    while (max > unsigned(1<<(bits-1)))
        ++bits;
    --bits;

    bits_size = bits*compressed.size();
    (*out_len) = bits_size/8;
    (*out_len) += (bits_size%8) ? 2 : 1;

    (*out) = new uint8_t[*out_len]();
    bzero(*out, *out_len);
    (**out) = bits;

    uint32_t bitindex = 8;
    for (unsigned a=0; a<compressed.size(); ++a) {
        uint32_t value = compressed[a];
        uint8_t *pvalue = (uint8_t*)(&value);

        for (unsigned b=0; b<bits;) {
            uint8_t rembi = bitindex%8;
            uint8_t remanting = bits-b;
            uint8_t consume = 8 - rembi;
            consume = (consume>remanting) ? remanting : consume;

            uint8_t c = pvalue[b/8]>>(b%8);
            c |= pvalue[b/8+1]<<(8-(b%8));

            uint16_t filter = (1<<(consume+1))-1;
            (*out)[(bitindex/8)] |= (c&filter)<<rembi;

            b += consume;
            bitindex += consume;
        }
    }
}

template < typename Iterator >
std::string HTDataCompress::lzw_decompress(Iterator begin, Iterator end)
{
    int dictSize = 256;
    std::map< int, std::string > dictionary;

    for (int i = 0; i < 256; i++) {
        dictionary[i] = std::string(1, i);
    }

    std::string w(1, *begin++);
    std::string result = w;
    std::string entry;

    for ( ; begin != end; begin++) {
        int k = *begin;
        if (dictionary.count(k)) {
            entry = dictionary[k];
        } else if (k == dictSize) {
            entry = w + w[0];
        } else
            throw "Bad compressed k";

        result += entry;
        dictionary[dictSize++] = w + entry[0];
        w = entry;
    }

    return result;
}

void HTDataCompress::decompress(uint8_t *in, size_t in_len, uint8_t *out, size_t out_len)
{
    bzero(out, out_len);

    std::vector< uint32_t > compressed;
    uint8_t bits_size = *in;

    for (uint32_t bitindex=8; (bitindex+8)/8<in_len;) {

        uint32_t value = 0;
        uint8_t  *pvalue = (uint8_t*)(&value);

        for (unsigned b=0; b<bits_size;) {
            uint8_t rembi = bitindex%8;

            uint8_t remanting = bits_size-b;
            uint8_t rem_to_in_byte = 8 - rembi;
            uint8_t rem_to_out_byte = 8 - (b%8);

            uint8_t consume = (remanting>rem_to_in_byte) ? rem_to_in_byte : remanting;
            consume = (consume>rem_to_out_byte) ? rem_to_out_byte : consume;

            uint8_t c = in[bitindex/8]>>(bitindex%8);
            c |= pvalue[bitindex/8+1]<<(8-(bitindex%8));

            uint16_t filter = (1<<(consume))-1;
            pvalue[(b/8)] |= (c&filter)<<(b%8);

            b += consume;
            bitindex += consume;
        }

        compressed.push_back(value);
    }

    std::string decompressed = HTDataCompress::lzw_decompress(compressed.begin(), compressed.end());
    for (unsigned a=0; a<decompressed.size() && a<out_len; ++a) 
        out[a] = decompressed[a];
}

HTFileVersioning::HTFileVersioning(void)
{
    this->shashtable = new uint8_t[getHTableBytesLen()]();
    this->reset();
}

HTFileVersioning::~HTFileVersioning()
{
    delete this->shashtable;
}

void HTFileVersioning::reset(void)
{
    bzero(this->hashtable, getHTableBytesLen());
}

uint8_t HTFileVersioning::getWord(uint8_t *ptr, uint32_t index)
{
    ptr = ptr+(index>>1);
    if (index%2)
        return ((*ptr)>>4)&0x0F;
    return (*ptr)&0x0F;
}

void HTFileVersioning::from3WtoIndex(uint8_t *_3w, uint32_t *dbytes_shift, 
    uint16_t *dbbits_shift)
{
    (*dbytes_shift) = _3w[0]*0xF + _3w[1];
    (*dbbits_shift) = 1<<_3w[2];
}

void HTFileVersioning::discoverHighLow(const char *fname, uint8_t *out)
{
    static uint8_t BUCKETS=2;
    static uint8_t PROCESS_BUCKETS=2;

    bzero(out, sizeof(uint8_t)*3);
    Hash h(BUCKETS*PROCESS_BUCKETS);
    Buffer b(fname, strlen(fname));

    BuckedOneAtTimeHash::hash(b, &h);

    // from 128b(32w) to 64b(16w)
    h.ihash[0] ^= h.ihash[3];
    h.ihash[1] ^= h.ihash[2];
    //from 64b(16w) to 12b(3w)
    for (int a=0; a<16; a++)
        out[a%3] ^= HTFileVersioning::getWord(h.shash, a);
}

void HTFileVersioning::addFile(const char *fname)
{
    uint8_t out[3];
    uint16_t bit=0;
    uint32_t byte=0;

    HTFileVersioning::discoverHighLow(fname, out);
    HTFileVersioning::from3WtoIndex(out, &byte, &bit);

    (this->dwhashtable[byte]) |= bit;
}

bool HTFileVersioning::checkFile(const char *fname) const
{
    uint8_t out[3];
    uint16_t bit=0;
    uint32_t byte=0;

    HTFileVersioning::discoverHighLow(fname, out);
    HTFileVersioning::from3WtoIndex(out, &byte, &bit);

    return bool(
        (this->dwhashtable[byte]) & bit
    );
}


void HTFileVersioning::getRawHTable(void *place, size_t len) const
{
    size_t tam = getHTableBytesLen();
    if (tam > len)
        tam = len;
    memcpy(place, this->hashtable, tam);
}

std::string HTFileVersioning::getHTable(void) const
{
    size_t len;
    uint8_t *out = NULL;
    size_t out_len = 0;

    HTDataCompress::compress(this->shashtable, this->getHTableBytesLen(), &out, &out_len);

    HT_B64 b64_encoder;
    unsigned char * ptr = b64_encoder.base64_encode(
        out,
        out_len,
        &len
    );

    std::string ret((const char*)ptr, len);

    delete ptr;
    delete out;
    return ret;
}

void HTFileVersioning::setHTable(std::string str) 
{
    size_t len = getHTableBytesLen()*3;
    uint8_t *temp = new uint8_t[len]();

    bzero(temp, len);
    bzero(this->hashtable, HTFileVersioning::getHTableBytesLen());

    HT_B64 b64_encoder;
    b64_encoder.base64_decode(
        (const unsigned char*)str.c_str(),
        str.size(),
        &len,
        temp
    );

    HTDataCompress::decompress(temp, len, this->shashtable, getHTableBytesLen());

    delete temp;
}

void HTFileVersioning::setHTable(void *place, size_t len)
{
    bzero(this->hashtable, HTFileVersioning::getHTableBytesLen());

    size_t t = len;
    if (t > getHTableBytesLen())
        t = getHTableBytesLen();
    memcpy(this->hashtable, place, t);
}

void HTFileVersioning::mergeHTable(std::string str)
{
    size_t len = HTFileVersioning::getHTableBytesLen()*3;
    size_t len2 = HTFileVersioning::getHTableBytesLen();
    unsigned char *buffer = new unsigned char[len]();
    unsigned char *buffer2 = new unsigned char[len2]();

    bzero(buffer, len);
    bzero(buffer2, len2);

    HT_B64 b64_encoder;
    b64_encoder.base64_decode(
        (const unsigned char*)str.c_str(),
        str.size(),
        &len,
        buffer
    );

    HTDataCompress::decompress(buffer, len, buffer2, len2);
    this->mergeHTable(buffer2, len2);

    delete buffer;
    delete buffer2;
}

void HTFileVersioning::mergeHTable(void *place, size_t len)
{
    unsigned char* buffer = (unsigned char*)place;
    for (unsigned int i=0; i<len; i++)
        this->chashtable[i] |= buffer[i];
}