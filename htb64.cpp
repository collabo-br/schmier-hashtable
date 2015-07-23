#include "htb64.h"

const unsigned char HT_B64::encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                               'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                               'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                               'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                               'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                               'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                               'w', 'x', 'y', 'z', '0', '1', '2', '3',
                               '4', '5', '6', '7', '8', '9', '+', '/'};

uint8_t HT_B64::mod_table[] = {0, 2, 1};

void HT_B64::build_decoding_table(void)
{
    decoding_table = new unsigned char[256]();

    for (int i = 0; i < 64; i++)
        decoding_table[encoding_table[i]] = i;
}

unsigned char* HT_B64::base64_encode(const unsigned char *data,
                size_t input_length, size_t *output_length, unsigned char *out_buff) 
{

    size_t out_lenght = 4 * ((input_length + 2) / 3);

    unsigned char *encoded_data = out_buff;
    if(!encoded_data)
        encoded_data = new unsigned char[out_lenght]();
    else if (*output_length < out_lenght)
        return NULL;

    *output_length = out_lenght;

    for (unsigned int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (unsigned int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}

unsigned char* HT_B64::base64_decode(const unsigned char *data,
                size_t input_length, size_t *output_length, unsigned char *out_buff)
{
    if (!this->decoding_table) this->build_decoding_table();
    if (input_length % 4 != 0) return NULL;

    size_t out_lenght = input_length / 4 * 3;

    if (data[input_length - 1] == '=') (out_lenght)--;
    if (data[input_length - 2] == '=') (out_lenght)--;

    unsigned char *decoded_data = out_buff;
    if(!decoded_data)
        decoded_data = new unsigned char[out_lenght]();
    else if (*output_length < out_lenght)
        return NULL;

    *output_length = out_lenght;

    for (unsigned int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : this->decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : this->decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : this->decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : this->decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}