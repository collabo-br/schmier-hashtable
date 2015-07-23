#ifndef __HTB64_H__
#define __HTB64_H__

#include <stdio.h>
#include <stdint.h>

/// \brief Class to encode raw buffers to B64.
///
/// This class encodes and decodes raw buffer to/from B64.
class HT_B64 {
    public:
        HT_B64(void):
            decoding_table(NULL)
        { }
        ~HT_B64()
        {
            if (this->decoding_table)
                delete this->decoding_table;
        }

        /// \brief Encodes using B64.
        ///
        /// This function encodes a raw buffer using B64.
        ///
        /// \param data pointer to buffer to be encoded.
        /// \param input_length the size of the input buffer.
        /// \param output_length the size of the output buffer, 
        /// after the encoding this variable will contain the size of encoded 
        /// output.
        /// \param out_buff optional pointer to output buffer.
        /// 
        /// \return a new buffer with output, out_buff (if given) or NULL in 
        /// in case of erros.
        unsigned char *base64_encode(const unsigned char *data, 
            size_t input_length, size_t *output_length, unsigned char *out_buff=NULL);

        /// \brief Decodes using B64.
        ///
        /// This function decodes a raw buffer using B64.
        ///
        /// \param data pointer to buffer to be decoded.
        /// \param input_length the size of the input buffer.
        /// \param output_length the size of the output buffer, 
        /// after the decoding this variable will contain the size of decoded 
        /// output.
        /// \param out_buff optional pointer to output buffer.
        /// 
        /// \return a new buffer with output, out_buff (if given) or NULL in 
        /// in case of erros.
        unsigned char *base64_decode(const unsigned char *data,
            size_t input_length, size_t *output_length, unsigned char *out_buff=NULL);

    protected:
        static const unsigned char encoding_table[]; ///< Encode table.
        static uint8_t mod_table[];                  ///< Mod table.

        unsigned char *decoding_table;               ///< Dinamicaly created decode table

        /// Builds the decoding table
        void build_decoding_table(void);
};

#endif