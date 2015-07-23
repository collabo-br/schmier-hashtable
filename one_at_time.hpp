#ifndef __ONE_AT_TIME_HPP__
#define __ONE_AT_TIME_HPP__

#include <stdint.h>
#include <string.h>

/// \brief Buffer class.
///
/// Helps managing the buffers
struct Buffer {
    /// \Brief Pointer to buffer.
    ///
    /// This union is a helper to prevent the nessecity of casts, as all pointers
    /// holds the same value.
    union {
        void *buffer;
        unsigned char *cbuffer;
        const char *ccbuffer;
        uint32_t *ibuffer;
    };
    size_t len; ///< Buffer size

    /// Empty Constructor (NULL)
    Buffer(void):
        buffer(NULL), len(0)
    { }
    Buffer(void *b, size_t l):
        buffer(b), len(l)
    { }
    Buffer(unsigned char *b, size_t l):
        cbuffer(b), len(l)
    { }
    Buffer(const char *b, size_t l):
        ccbuffer(b), len(l)
    { }
    Buffer(uint32_t *b, size_t l):
        ibuffer(b), len(l)
    { }
};

/// \brief Holds hashes.
///
/// This class holds hashes of arbitrary sizes, helps manage them and prevent
/// memory leaks.
struct Hash {
    /// \Brief Pointer to hash.
    ///
    /// This union is a helper to prevent the nessecity of casts, as all pointers
    /// holds the same value.
    union 
    {
        unsigned char *chash;
        char *cchash;
        uint32_t *ihash;
        uint8_t *shash;
        void *hash;
    };
    size_t hash_size; ///< Hash size in Bytes

    /// NULL constructor.
    Hash(void):
        hash(NULL), hash_size(0)
    { }
    /// Create empty hash with buckets size.
    Hash(uint8_t buckets):
        hash(new unsigned char[buckets*4]()), hash_size(buckets*4)
    {
        bzero(this->hash, this->hash_size);
    }
    Hash(const Hash &other)
    {
        this->operator=(other);
    }
    ~Hash() 
    {
        this->clean();
    }
    Hash& operator=(const Hash &other) 
    {
        this->clean();
        this->hash_size = other.hash_size;
        this->chash = new unsigned char[other.hash_size]();
        memcpy(this->hash, other.hash, other.hash_size);
        return *this;
    }
    /// Cleans state.
    ///
    /// Delete this hash (if needed) and set it as a NULL hash.
    void clean(void)
    {
        if (this->chash)
            delete this->chash;
        this->hash = NULL;
        this->hash_size = 0;
    }
};

/// \brief Class to generate Bucket One at time hashes.
///
/// This class creates hashes of arbitrary sizes and buckets from buffers.
class BuckedOneAtTimeHash {
    public:
        /// Deletes a newly created hash.
        ///
        /// \param h hash to be deleted.
        static void freeHash(Hash *h)
        {
            if (h)
                delete h;
        }

        /// Returns a new hash from buffer with size buckets.
        ///
        /// \param buffer The buffer to be hashed.
        /// \param buckets The size of final hash.
        ///
        /// \return A new hash that mus be deleted ( preferably by void 
        /// freeHash(Hash *h) ), or NULL in case of error.
        static Hash* hash (
            const Buffer &buffer,
            uint8_t buckets
        ) {
            Hash *h = new Hash(buckets);
            if (!BuckedOneAtTimeHash::hash(buffer, h)) {
                delete h;
                return NULL;
            }
            return h;
        }

        /// Sets ret to hash.
        ///
        /// Hash the buffer and store its hash in ret.
        ///
        /// \param buffer The buffer to be hashed.
        /// \param ret the Hash where copy output to.
        /// 
        /// \return ret or NULL in case of error.
        static Hash* hash (
            const Buffer &buffer,
            Hash *ret
        ) {
            if (!ret->hash_size || !ret->hash)
                return NULL;

            uint8_t buckets = ret->hash_size/4;

            for (unsigned int i=0; i<buffer.len; ++i)
            {
                ret->ihash[i%buckets] += buffer.cbuffer[i];
                ret->ihash[i%buckets] += (ret->ihash[i%buckets] << 10);
                ret->ihash[i%buckets] ^= (ret->ihash[i%buckets] >> 6);
            }

            for (unsigned int i=0; i<buckets; ++i)
            {
                ret->ihash[i] += (ret->ihash[i] << 3);
                ret->ihash[i] ^= (ret->ihash[i] >> 11);
                ret->ihash[i] += (ret->ihash[i] << 15);
            }

            return ret;
        }
};

#endif