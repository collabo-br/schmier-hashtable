#ifndef __HT_FILE_VERSIONING__
#define __HT_FILE_VERSIONING__

#include <string>
#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
/// \brief Symetric compression.
///
/// This class is used to compress buffers using LZMA algorithm.
////////////////////////////////////////////////////////////////////////////////
class HTDataCompress {
    public:
        /// \brief Compresss function.
        ///
        /// Compress the given input buffer and returns its output and size.
        ///
        /// \param in the pointer to input buffer.
        /// \param in_len the size of the input buffer.
        /// \param out a pointer to pointer, allowing retrieve the pointer to
        /// the the new compressed buffer.
        /// \param out_len the compressed size.
        static void compress(uint8_t *in, size_t in_len, uint8_t **out,
            size_t *out_len);

        /// \brief Decompress function.
        ///
        /// Decompress the given input buffer and returns its output and size.
        ///
        /// \param in the pointer to input buffer.
        /// \param in_len the size of the input buffer.
        /// \param out a pointer to pointer, allowing retrieve the pointer to
        /// the the new uncompressed buffer.
        /// \param out_len the uncompressed size.
        static void decompress(uint8_t *in, size_t in_len, uint8_t *out,
            size_t out_len);

    protected:
        template < typename Iterator >
        static Iterator lzw_compress(const char *uncompressed, uint32_t size,
            Iterator result);
        template < typename Iterator >
        static std::string lzw_decompress(Iterator begin, Iterator end);
};

////////////////////////////////////////////////////////////////////////////////
/// \brief Main versioning class.
///
/// This class is THE class, where you interact with the module, its quite simple
/// and provides the funcionalities to add and check files, merge and set tables.
////////////////////////////////////////////////////////////////////////////////
class HTFileVersioning {
    public:
        static uint8_t bklenght; ///< The lenght in bits of the hash key

        /// \brief Returns the size of the table.
        ///
        /// Returns the size (in bits) of the hash table.
        ///
        /// \return number of bits of the hash table.
        static uint64_t getHTableBitsLen(void)
        {
            return 1<<(bklenght);
        }
        /// \brief Returns the size of the table.
        ///
        /// Returns the size (in bytes) of the hash table.
        ///
        /// \return number of bytes of the hash table.
        static uint32_t getHTableBytesLen(void)
        {
            return divRoundUp(getHTableBitsLen(), 8);
        }

        HTFileVersioning(void);
        ~HTFileVersioning();

        /// \brief Reset the table.
        ///
        /// Set all bits of the table to zero, effectively marking all files as
        /// non existent on the table.
        void reset(void);

        /// \brief Add a file.
        ///
        /// Hash the filepath given and use the hash as key to mark it on the
        /// hashtable.
        ///
        /// \param fname null terminated std string.
        void addFile(const std::string &fname)
            { this->addFile(fname.c_str()); }

        /// \brief Add a file.
        /// 
        /// Hash the filepath given and use the hash as key to mark it on the
        /// hashtable.
        ///
        /// \param fname null terminated c style string (buffer/array).
        void addFile(const char *fname);

        /// \brief Check a file.
        ///
        /// Hash the filepath given and use the hash as key to check if marked
        /// in the hashtable.
        ///
        /// \param fname null terminated std string.
        /// \return true if present, false otherwise.
        bool checkFile(const std::string &fname) const
            { return this->checkFile(fname.c_str()); }

        /// \brief Check a file.
        ///
        /// Hash the filepath given and use the hash as key to check if marked
        /// in the hashtable.
        ///
        /// \param fname null terminated c style string (buffer/array).
        /// \return true if present, false otherwise.
        bool checkFile(const char *fname) const;

        /// \brief Copy the raw table.
        ///
        /// Copy the raw table to *place respecting its size of len.
        ///
        /// \param place pointer to buffer where to copy to.
        /// \param len size of destination buffer.
        void getRawHTable(void *place, size_t len) const;

        /// \brief Returns the compressed table.
        ///
        /// This call returns the compressed table compressed and encoded in
        /// B64, this is the function used to export tables.
        ///
        /// \return std string with table compressed and encoded.
        std::string getHTable(void) const;

        /// \brief Set the table.
        ///
        /// Decode and decompress the table in str, than set it as current table.
        ///
        /// \param str Compressed and B64 encoded table
        void setHTable(std::string str);

        /// \brief Set the table.
        ///
        /// Copy raw table from place to current table.
        ///
        /// \param place Pointer to source of the raw table to copy
        /// \param len sanity check limit of source
        void setHTable(void *place, size_t len);

        /// \brief Merge table
        ///
        /// Decode and decompress the table in str, than merge with current
        /// table.
        ///
        /// \param str Compressed and B64 encoded table
        void mergeHTable(std::string str);

        /// \brief Set the table.
        ///
        /// Merge raw table from place with current table.
        ///
        /// \param place Pointer to source of the raw table to copy
        /// \param len sanity check limit of source
        void mergeHTable(void *place, size_t len);

    protected:
        /// \brief All pointers to hashtable.
        ///
        /// A union that helps by preventing casts during development as all of
        /// these variables points to the same buffer with contains the
        /// hashtable.
        union
        {
            void *hashtable;            ///! void pointer to hashtable
            unsigned char *chashtable;  ///! unsigned char pointer to hashtable
            const char *cchashtable;    ///! const char pointer to hashtable
            uint32_t *ihashtable;       ///! uint32_t pointer to hashtable
            uint16_t *dwhashtable;      ///! uint16_t pointer to hashtable
            uint8_t *shashtable;        ///! uint8_t pointer to hashtable
        };

        static uint32_t divRoundUp(uint64_t a, uint32_t b)
        {
            uint32_t r = a/b;
            if(a%b) r++;
            return r;
        }
        static uint8_t getWord(uint8_t *ptr, uint32_t index);
        static void from3WtoIndex(uint8_t *_3w, uint32_t *dbytes_shift,
            uint16_t *dbbits_shift);
        static void discoverHighLow(const char *fname, uint8_t *out);
};

#endif
