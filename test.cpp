#include <stdio.h>

#include <gtest/gtest.h>
#include <gtest/gtest-all.cc>
#include <gtest/gtest_main.cc>

#include <execinfo.h>
#include <signal.h>

#include "htb64.h"
#include "ht_file_versioning.h"
#include "one_at_time.hpp"

//  ____       _              ____  _                   _ 
// / ___|  ___| |_ _   _ _ __/ ___|(_) __ _ _ __   __ _| |
// \___ \ / _ \ __| | | | '_ \___ \| |/ _` | '_ \ / _` | |
//  ___) |  __/ |_| |_| | |_) |__) | | (_| | | | | (_| | |
// |____/ \___|\__|\__,_| .__/____/|_|\__, |_| |_|\__,_|_|
//                      |_|           |___/               
void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

class ExecBeforeMain {
public:
    ExecBeforeMain(void) {
        signal(SIGSEGV, handler);
    }
};
ExecBeforeMain bm;

//  _____         _   _   _           _     _             
// |_   _|__  ___| |_| | | | __ _ ___| |__ (_)_ __   __ _ 
//   | |/ _ \/ __| __| |_| |/ _` / __| '_ \| | '_ \ / _` |
//   | |  __/\__ \ |_|  _  | (_| \__ \ | | | | | | | (_| |
//   |_|\___||___/\__|_| |_|\__,_|___/_| |_|_|_| |_|\__, |
//                                                  |___/ 
TEST(TESTOneAtTimeHash, same_result_passing_by_param) {
    const char *t1 = "AAAAAAAAAAAAAAAAAAAA";
    Buffer b1(t1, strlen(t1));
    Hash h1_(2);
    Hash *h1 = BuckedOneAtTimeHash::hash(b1, &h1_);
    ASSERT_EQ(h1, &h1_);
    ASSERT_EQ(h1->hash_size, h1_.hash_size);
    ASSERT_EQ(h1->hash, h1_.hash);
    ASSERT_EQ( h1->chash, h1->hash );
    ASSERT_EQ( h1_.chash, h1_.hash );
    ASSERT_EQ( strncmp(h1->cchash, h1_.cchash, h1->hash_size), 0 );
}

TEST(TESTOneAtTimeHash, same_result_different_runs) {
    const char *t[] = {
        "AAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAABA",
        "AAAAAAAABAAAAAAAAAAA",
        "BAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAA",
        "BBBBBBBBBBBBBBBBBBBB"
    };

    for (int a=0; a<(sizeof(t)/sizeof(const char*)); a++) {
        Buffer b(t[a], strlen(t[a]));

        Hash *h1, *h2, h3(2), h4(2);

        h1 = BuckedOneAtTimeHash::hash(b, 2);
        h2 = BuckedOneAtTimeHash::hash(b, 2);
        BuckedOneAtTimeHash::hash(b, &h3);
        BuckedOneAtTimeHash::hash(b, &h4);

        ASSERT_EQ(h1->hash_size, h2->hash_size);
        ASSERT_EQ(h2->hash_size, h3.hash_size);
        ASSERT_EQ(h4.hash_size, h4.hash_size);

        ASSERT_NE(h1->hash, h2->hash);
        ASSERT_NE(h2->hash, h3.hash);
        ASSERT_NE(h3.hash, h4.hash);

        ASSERT_EQ( strncmp(h1->cchash, h2->cchash, h1->hash_size), 0 );
        ASSERT_EQ( strncmp(h2->cchash, h3.cchash, h1->hash_size), 0 );
        ASSERT_EQ( strncmp(h3.cchash, h4.cchash, h1->hash_size), 0 );

        delete h1;
        delete h2;
    }
}

TEST(TESTOneAtTimeHash, assert_hashing) {
    const char *htable[][2] = {
        {"AAAAAAAAAAAAAAAAAAAA", "2F63F282"},
        {"AAAAAAAAAAAAAAAAAABA", "6BEE0A2B"},
        {"AAAAAAAABAAAAAAAAAAA", "ADC7ADE7"},
        {"BAAAAAAAAAAAAAAAAAAA", "A08D3712"},
        {"AAAAAAAAAAAAAAAAAAA", "E31DEB5C"},
        {"AAAAAAAAAAAAAAAAAA", "838B87AD"},
        {"BBBBBBBBBBBBBBBBBBBB", "18343D70"},
        {"BAHSBBHABBHABSBHBHAHBSBHAHHBDBHAHBHBABDH", "FAD7E941"},
        {"BAHSBBHABB", "1CA7109E"},
        {"bahsbbhabb", "A9CD7318"},
        {"", "FEA28CF9"},
        {" ", "8D91D5A5"},
        {"I know you're out there. I can feel you now."
        " I know that you're afraid. You're afraid of"
        " us. You're afraid of change. I don't know t"
        "he future. I didn't come here to tell you ho"
        "w this is going to end. I came here to tell "
        "you how it's going to begin.", "B4CA45E0"},
        {"I know you're out there. I can feel you now."
        " I know that you're afraid. You're afraid of"
        " us. You're afraid of change. I don't know t"
        "he future. I didn't come here to tell you ho"
        "w this is going to end. I came here to tell "
        "you how it's going to begin", "DB99CFCE"},
        {"I know you're out there. I can feel you now."
        " I know that your afraid. You're afraid of" //<<NOOOO
        " us. You're afraid of change. I don't know t"
        "he future. I didn't come here to tell you ho"
        "w this is going to end. I came here to tell "
        "you how it's going to begin.", "E72A4FCF"}
    };

    Hash tmp(1);

    for (
       int a=0;
       a<(sizeof(htable)/sizeof(htable[0]));
       a++
    ) {
        char *pos = (char*)(htable[a][1]);
        union {
            char cval[4];
            uint32_t ival;
        };

        for(int count=sizeof(ival)-1; count>=0; count--, pos+=2) {
            unsigned int tempi=0;
            sscanf(pos, "%02X", &tempi);
            cval[count] = tempi;
        }

        Buffer b(htable[a][0], strlen(htable[a][0]));
        BuckedOneAtTimeHash::hash(b, &tmp);
        ASSERT_EQ(strncmp(tmp.cchash, cval, tmp.hash_size), 0);
    }
}

//  _____         _   _____                     _ _             
// |_   _|__  ___| |_| ____|_ __   ___ ___   __| (_)_ __   __ _ 
//   | |/ _ \/ __| __|  _| | '_ \ / __/ _ \ / _` | | '_ \ / _` |
//   | |  __/\__ \ |_| |___| | | | (_| (_) | (_| | | | | | (_| |
//   |_|\___||___/\__|_____|_| |_|\___\___/ \__,_|_|_| |_|\__, |
//                                                        |___/ 
TEST(TESTHT_B64, assert_format) {
    const char *htable[][2] = {
        {"AAAAAAAAAAAAAAAAAAAA", "QUFBQUFBQUFBQUFBQUFBQUFBQUE="},
        {"AAAAAAAAAAAAAAAAAABA", "QUFBQUFBQUFBQUFBQUFBQUFBQkE="},
        {"AAAAAAAABAAAAAAAAAAA", "QUFBQUFBQUFCQUFBQUFBQUFBQUE="},
        {"BAAAAAAAAAAAAAAAAAAA", "QkFBQUFBQUFBQUFBQUFBQUFBQUE="},
        {"AAAAAAAAAAAAAAAAAAA", "QUFBQUFBQUFBQUFBQUFBQUFBQQ=="},
        {"AAAAAAAAAAAAAAAAAA", "QUFBQUFBQUFBQUFBQUFBQUFB"},
        {"BBBBBBBBBBBBBBBBBBBB", "QkJCQkJCQkJCQkJCQkJCQkJCQkI="},
        {
            "BAHSBBHABBHABSBHBHAHBSBHAHHBDBHAHBHBABDH",
            "QkFIU0JCSEFCQkhBQlNCSEJIQUhCU0JIQUhIQkRCSEFIQkhCQUJESA=="
        },
        {"BAHSBBHABB", "QkFIU0JCSEFCQg=="},
        {"bahsbbhabb", "YmFoc2JiaGFiYg=="},
        {"", ""},
        {" ", "IA=="},
        {
            "I know you're out there. I can feel you now. I know that you're afr"
            "aid. You're afraid of us. You're afraid of change. I don't know the"
            " future. I didn't come here to tell you how this is going to end. I"
            " came here to tell you how it's going to begin.",
            "SSBrbm93IHlvdSdyZSBvdXQgdGhlcmUuIEkgY2FuIGZlZWwgeW91IG5vdy4gSSBrbm9"
            "3IHRoYXQgeW91J3JlIGFmcmFpZC4gWW91J3JlIGFmcmFpZCBvZiB1cy4gWW91J3JlIG"
            "FmcmFpZCBvZiBjaGFuZ2UuIEkgZG9uJ3Qga25vdyB0aGUgZnV0dXJlLiBJIGRpZG4nd"
            "CBjb21lIGhlcmUgdG8gdGVsbCB5b3UgaG93IHRoaXMgaXMgZ29pbmcgdG8gZW5kLiBJ"
            "IGNhbWUgaGVyZSB0byB0ZWxsIHlvdSBob3cgaXQncyBnb2luZyB0byBiZWdpbi4="
        },
        {
            " I'm going to hang up this phone, and then I'm going to show these "
            "people what you don't want them to see. I'm going to show them a wo"
            "rld without you, a world without rules and controls, without border"
            "s or boundaries, a world where anything is possible. Where we go fr"
            "om there, is a choice I leave to you.",
            "IEknbSBnb2luZyB0byBoYW5nIHVwIHRoaXMgcGhvbmUsIGFuZCB0aGVuIEknbSBnb2l"
            "uZyB0byBzaG93IHRoZXNlIHBlb3BsZSB3aGF0IHlvdSBkb24ndCB3YW50IHRoZW0gdG"
            "8gc2VlLiBJJ20gZ29pbmcgdG8gc2hvdyB0aGVtIGEgd29ybGQgd2l0aG91dCB5b3UsI"
            "GEgd29ybGQgd2l0aG91dCBydWxlcyBhbmQgY29udHJvbHMsIHdpdGhvdXQgYm9yZGVy"
            "cyBvciBib3VuZGFyaWVzLCBhIHdvcmxkIHdoZXJlIGFueXRoaW5nIGlzIHBvc3NpYmx"
            "lLiBXaGVyZSB3ZSBnbyBmcm9tIHRoZXJlLCBpcyBhIGNob2ljZSBJIGxlYXZlIHRvIH"
            "lvdS4="
        }
    };

    HT_B64 b64;

    for (
       int a=0;
       a<(sizeof(htable)/sizeof(htable[0]));
       a++
    ) {
        size_t out_len1 = 0;
        size_t out_len2 = 0;

        unsigned char *tmp1 = b64.base64_encode(
            (const unsigned char*)(htable[a][0]),
            strlen(htable[a][0]), &out_len1
        );
        ASSERT_TRUE(tmp1 != NULL);

        unsigned char *tmp2 = b64.base64_decode(tmp1, out_len1, &out_len2);
        ASSERT_TRUE(tmp2 != NULL);

        ASSERT_EQ(strlen(htable[a][0]), out_len2);
        ASSERT_EQ(strlen(htable[a][1]), out_len1);

        ASSERT_EQ(strncmp((const char*)(tmp2), htable[a][0], out_len2), 0);
        ASSERT_EQ(strncmp((const char*)(tmp1), htable[a][1], out_len1), 0);

        delete tmp1;
        delete tmp2;
    }
}

//  _____         _    ____                                        
// |_   _|__  ___| |_ / ___|___  _ __ ___  _ __  _ __ ___  ___ ___ 
//   | |/ _ \/ __| __| |   / _ \| '_ ` _ \| '_ \| '__/ _ \/ __/ __|
//   | |  __/\__ \ |_| |__| (_) | | | | | | |_) | | |  __/\__ \__ \
//   |_|\___||___/\__|\____\___/|_| |_| |_| .__/|_|  \___||___/___/
//                                        |_|                      
TEST(TESTHTDataCompress, assert_symetric) {
    const char *htable[] = {
        "AAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAABA",
        "AAAAAAAABAAAAAAAAAAA",
        "BAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAA",
        "BBBBBBBBBBBBBBBBBBBB",
        "BAHSBBHABBHABSBHBHAHBSBHAHHBDBHAHBHBABDH",
        "BAHSBBHABB",
        "bahsbbhabb",
        "I know you're out there. I can feel you now. I know that you're afr"
        "aid. You're afraid of us. You're afraid of change. I don't know the"
        " future. I didn't come here to tell you how this is going to end. I"
        " came here to tell you how it's going to begin.",       
        " I'm going to hang up this phone, and then I'm going to show these "
        "people what you don't want them to see. I'm going to show them a wo"
        "rld without you, a world without rules and controls, without border"
        "s or boundaries, a world where anything is possible. Where we go fr"
        "om there, is a choice I leave to you.",
    };

    for (
       int a=0;
       a<(sizeof(htable)/sizeof(htable[0]));
       a++
    ) {
        size_t il = strlen(htable[a]);

        size_t ill = il;
        size_t ol = il*2;
        uint8_t *o = NULL;
        uint8_t *i = new uint8_t[ill]();

        HTDataCompress::compress((uint8_t*)(htable[a]), il, &o, &ol);
        ASSERT_TRUE(o != NULL);
        ASSERT_TRUE(ol > 0);
        HTDataCompress::decompress(o, ol, i, ill);
        ASSERT_TRUE(ill > 0);

        ASSERT_EQ(ill, il);
        ASSERT_EQ(strncmp((const char*)(i), htable[a], ill), 0);

        delete i;
        delete o;
    }
}

TEST(TESTHTDataCompress, assert_stable) {
    const char *htable[][2] = {
        {"AAAAAAAAAAAAAAAAAAAA", "CUEABhQ4cCA="},
        {"AAAAAAAAAAAAAAAAAABA", "CUEABhQ4MKCQIA=="},
        {"AAAAAAAABAAAAAAAAAAA", "CUEABgQoRGBBgA=="},
        {"BAAAAAAAAAAAAAAAAAAA", "CUKCBBQ4kOBA"},
        {"AAAAAAAAAAAAAAAAAAA", "CUEABhQ4UCA="},
        {"AAAAAAAAAAAAAAAAAA", "CUEABhQ4MCA="},
        {"BBBBBBBBBBBBBBBBBBBB", "CUIABhQ4cCA="},
        {
            "BAHSBBHABBHABSBHBHAHBSBHAHHBDBHAHBHBABDH",
            "CUKCIJkiRAiSIAQNChmIpGBAhQ2RMCQCsSBAIUSQAA=="
        },
        {"BAHSBBHABB", "CUKCIJkiRAiSIAQB"},
        {"bahsbbhabb", "CWLCoJkjRgyaMAQB"},
        {
            "I know you're out there. I can feel you now. I know that you're afr"
            "aid. You're afraid of us. You're afraid of change. I don't know the"
            " future. I didn't come here to tell you how this is going to end. I"
            " came here to tell you how it's going to begin.",
            "CUlArHHz5g6IPG/qnJBTBgRCOiDooCmz0AUIgGPCuAFhpkwZNgYRghh4pyJAgQQhogn"
            "z8GDChSDCmJETJg2ZilkQKmQYc2bNhmZA1JlzM+dLnjTJ/AQxRqWbM2VKgiDzxs2Jhy"
            "cLRmRopg6dOhQtSq1Z9eGYN20YSnxJ5w1Ejx9bgkCDMmKaOSDsgjjzJo1TiG3LuLEZF"
            "iNauRMZsnXLBm7IuQXT0Dlxd2/fM39BiClzpq8L"
        },
        {
            " I'm going to hang up this phone, and then I'm going to show these "
            "people what you don't want them to see. I'm going to show them a wo"
            "rld without you, a world without rules and controls, without border"
            "s or boundaries, a world where anything is possible. Where we go fr"
            "om there, is a choice I leave to you.",
            "CSCSnGgD4sybNG7OgKDzBgSaMAhB1IGjEE2aOSDgoHnjpgwLEA/JUCzjBqBAggYhLgQ"
            "xR+MdkXPKYCzzBg6bmHcc0gGR500dEGQ2ntB556FOOmjKDFQJs4yLkgMLHky4tKXIgW"
            "FA3Hkjh03IO2mO9tTJs45HrFq5egWrsY5OOXVsXgQJYsxGOnLesJnj8WvYtiDEbCVTR"
            "s7FrYB7uiETRk6aMns/Zt3aNStSOTEf5jkqFYRFjG/mzEkjxqbTK5ZvxiwIwgxepag9"
            "esY6RmOaMTGTgLAZxk5MlWRdAA=="
        }
    };

    HT_B64 b64;

    for (
       int a=0;
       a<(sizeof(htable)/sizeof(htable[0]));
       a++
    ) {
        size_t il = strlen(htable[a][0]);
        size_t ol = il*2;
        uint8_t *o = NULL;

        HTDataCompress::compress((uint8_t*)(htable[a][0]), il, &o, &ol);
        ASSERT_TRUE(o != NULL);
        ASSERT_TRUE(ol > 0);

        size_t el=0;
        unsigned char *e = b64.base64_encode(o, ol, &el); 
        ASSERT_TRUE(e != NULL);
        ASSERT_TRUE(el > 0);

        ASSERT_EQ(strncmp((const char*)(e), htable[a][1], el), 0);

        delete o;
        delete e;
    }
}

//  _____         _   _______     __        
// |_   _|__  ___| |_|  ___\ \   / /__ _ __ 
//   | |/ _ \/ __| __| |_   \ \ / / _ \ '__|
//   | |  __/\__ \ |_|  _|   \ V /  __/ |   
//   |_|\___||___/\__|_|      \_/ \___|_|   
//
TEST(TESTHTFileVersioning, assert_table_works) {
    const char *esta_la[] = {
        "AAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAABA",
        "AAAAAAAABAAAAAAAAAAA",
        "BAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAA",
        "BBBBBBBBBBBBBBBBBBBB",
        "BAHSBBHABBHABSBHBHAHBSBHAHHBDBHAHBHBABDH",
        "BAHSBBHABB",
        "bahsbbhabb",
        "",
        " ",
        "AUNNSDOIAHSDH",
        "AUNNSDOIAHSDh",
        "%%#@#(*$&@#($*&(@#%%*@#",
        "%%#@#(*$&@#($*&(@#%%*@",
        "I know you're out there. I can feel you now. I know that you're afr"
        "aid. You're afraid of us. You're afraid of change. I don't know the"
        " future. I didn't come here to tell you how this is going to end. I"
        " came here to tell you how it's going to begin.",       
        " I'm going to hang up this phone, and then I'm going to show these "
        "people what you don't want them to see. I'm going to show them a wo"
        "rld without you, a world without rules and controls, without border"
        "s or boundaries, a world where anything is possible. Where we go fr"
        "om there, is a choice I leave to you.",
    };

    const char *nao_esta_la[] = {
        "AAAAAAAAAA",
        "AAAAAAAAAAAAAAAAABA",
        "AAAAAAABAAAAAAAAAAA",
        "BAAAAAAAAAAAAAAAAAA",
        "CCAAAAAAAAAAAAAAAAA",
        "BBBBBBBXBBBBBBBBBBBB",
        "BAH",
        "BAHS",
        "bahs",
        "-",
        " -",
        "%%#@#(**$&@#($*&(@#%%*@#",
        "%%#@#(*$$&@#($*&(@#%%*@"
    };

    HTFileVersioning fv;

    for (
       int a=0;
       a<(sizeof(esta_la)/sizeof(esta_la[0]));
       a++
    ) {
        fv.addFile(esta_la[a]);
    }

    for (
       int a=0;
       a<(sizeof(esta_la)/sizeof(esta_la[0]));
       a++
    ) {
        ASSERT_TRUE(fv.checkFile(esta_la[a]));
    }

    unsigned errors = 0;
    unsigned total = sizeof(nao_esta_la)/sizeof(nao_esta_la[0]);
    for (
       unsigned a=0;
       a<total;
       a++
    ) {
        //THIS IS NOT A ERROR CONDITION, AS IT IS EXPECTED TO HAPPEN,
        //BUT TOO MUCH FAILS HERE *MAY*  INDICATE SOMETHING IS WRONG
        if (fv.checkFile(nao_esta_la[a]))
            errors++;
    }
    EXPECT_TRUE(errors == 0);
    ASSERT_TRUE(errors < total);
}

TEST(TESTHTFileVersioning, assert_export_works) {
    const char *monte[] = {
        "AAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAABA",
        "AAAAAAAABAAAAAAAAAAA",
        "BAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAA",
        "BBBBBBBBBBBBBBBBBBBB",
        "BAHSBBHABBHABSBHBHAHBSBHAHHBDBHAHBHBABDH",
        "BAHSBBHABB",
        "bahsbbhabb",
        "",
        " ",
        "AUNNSDOIAHSDH",
        "AUNNSDOIAHSDh",
        "%%#@#(*$&@#($*&(@#%%*@#",
        "%%#@#(*$&@#($*&(@#%%*@",
        "I know you're out there. I can feel you now. I know that you're afr"
        "aid. You're afraid of us. You're afraid of change. I don't know the"
        " future. I didn't come here to tell you how this is going to end. I"
        " came here to tell you how it's going to begin.",       
        " I'm going to hang up this phone, and then I'm going to show these "
        "people what you don't want them to see. I'm going to show them a wo"
        "rld without you, a world without rules and controls, without border"
        "s or boundaries, a world where anything is possible. Where we go fr"
        "om there, is a choice I leave to you.",
    };

    const char *nao_esta_la[] = {
        "AAAAAAAAAA",
        "AAAAAAAAAAAAAAAAABA",
        "AAAAAAABAAAAAAAAAAA",
        "BAAAAAAAAAAAAAAAAAA",
        "CCAAAAAAAAAAAAAAAAA",
        "BBBBBBBXBBBBBBBBBBBB",
        "BAH",
        "BAHS",
        "bahs",
        "-",
        " -",
        "%%#@#(**$&@#($*&(@#%%*@#",
        "%%#@#(*$$&@#($*&(@#%%*@"
    };

    unsigned total = sizeof(monte)/sizeof(monte[0]);
    HTFileVersioning fv1, fv2;

    for(unsigned a=0; a < total; a++) {
        fv1.addFile(monte[a]);
    }

    std::string tabela1 = fv1.getHTable();
    fv2.setHTable(tabela1);
    std::string tabela2 = fv2.getHTable();

    ASSERT_EQ(tabela1, tabela2);

    for (int a=0; a<total; a++) {
        ASSERT_TRUE(fv2.checkFile(monte[a]));
    }

    unsigned errors = 0;
    unsigned total2 = sizeof(nao_esta_la)/sizeof(nao_esta_la[0]);
    for (unsigned a=0; a<total2; a++) {
        //THIS IS NOT A ERROR CONDITION, AS IT IS EXPECTED TO HAPPEN,
        //BUT TOO MUCH FAILS HERE *MAY*  INDICATE SOMETHING IS WRONG
        if (fv2.checkFile(nao_esta_la[a]))
            errors++;
    }
    EXPECT_TRUE(errors == 0);
    ASSERT_TRUE(errors < total2);
}

TEST(TESTHTFileVersioning, test_merge_works) {
    const char *t1[] = {
        "AAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAABA",
        "AAAAAAAABAAAAAAAAAAA",
        "BAAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAAA",
        "BBBBBBBBBBBBBBBBBBBB",
        "BAHSBBHABBHABSBHBHAHBSBHAHHBDBHAHBHBABDH",
        "BAHSBBHABB",
        "bahsbbhabb",
        "",
        " ",
        "AUNNSDOIAHSDH",
        "AUNNSDOIAHSDh",
        "%%#@#(*$&@#($*&(@#%%*@#",
        "%%#@#(*$&@#($*&(@#%%*@",
        "I know you're out there. I can feel you now. I know that you're afr"
        "aid. You're afraid of us. You're afraid of change. I don't know the"
        " future. I didn't come here to tell you how this is going to end. I"
        " came here to tell you how it's going to begin.",       
        " I'm going to hang up this phone, and then I'm going to show these "
        "people what you don't want them to see. I'm going to show them a wo"
        "rld without you, a world without rules and controls, without border"
        "s or boundaries, a world where anything is possible. Where we go fr"
        "om there, is a choice I leave to you.",
    };

    const char *t2[] = {
        "CCCCCCCCCCCCCCCCCCCCCCCCCCC",
        "CCCCACCCCCCCCCCCCCCCCCCCCCC",
        "CCCCCCCCCCCCCCCCCCCACCCCCCC",
        "CCCCCCCCCCCCCCCCCCCCCCCCCC",
        "CCCCCCCCCCCCCCCCCCCCCCCCCCCC",
        "BAHSBBHABB",
        "bahsbbhabb",
        "ANSDOP VCUVA AOISHD029u34nvga ",
        "ANSDOP VCUVA AOISHD029u34nvgB ",
        "09an2w30)U&A)UA@#$#N0 9uasndf a0sf+",
        "Why, Mr. Anderson, why? Why, why do you do it? Why, why get up? Why"
        " keep fighting? Do you believe you're fighting for something, for m"
        "ore than your survival? Can you tell me what it is, do you even kno"
        "w? Is it freedom or truth, perhaps peace - could it be for love?",
        "Illusions, Mr. Anderson, vagaries of perception. Temporary construc"
        "ts of a feeble human intellect trying desperately to justify an exi"
        "stence that is without meaning or purpose. And all of them as artif"
        "icial as the Matrix itself."
    };

    const char *nao_esta_la[] = {
        "BAH",
        "BAHS",
        "bahs",
        "-",
        " -",
        "%%#@#(**$&@#($*&(@#%%*@#",
        "%%#@#(*$$&@#($*&(@#%%*@"
    };

    HTFileVersioning fv1, fv2, fvf;

    unsigned t1t = sizeof(t1)/sizeof(t1[0]);
    unsigned t2t = sizeof(t1)/sizeof(t1[0]);
    unsigned nelt = sizeof(nao_esta_la)/sizeof(nao_esta_la[0]);

    for(unsigned a=0; a<t1t; a++) {
        fv1.addFile(t1[a]);
    }

    for(unsigned a=0; a<t2t; a++) {
        fv2.addFile(t2[a]);
    }

    fvf.mergeHTable(fv1.getHTable());
    fvf.mergeHTable(fv2.getHTable());

    uint8_t *fv1tal = new uint8_t[HTFileVersioning::getHTableBytesLen()]();
    uint8_t *fv2tal = new uint8_t[HTFileVersioning::getHTableBytesLen()]();
    uint8_t *fvftal = new uint8_t[HTFileVersioning::getHTableBytesLen()]();

    fv1.getRawHTable(fv1tal, HTFileVersioning::getHTableBytesLen());
    fv2.getRawHTable(fv2tal, HTFileVersioning::getHTableBytesLen());
    fvf.getRawHTable(fvftal, HTFileVersioning::getHTableBytesLen());

    for(unsigned a=0; a<HTFileVersioning::getHTableBytesLen(); a++) {
        ASSERT_EQ(fvftal[a], fv1tal[a]|fv2tal[a]);
    }

    delete fv1tal;
    delete fv2tal;
    delete fvftal;

    for(unsigned a=0; a<t1t; a++) {
        ASSERT_TRUE(fvf.checkFile(t1[a]));
    }
    for(unsigned a=0; a<t2t; a++) {
        ASSERT_TRUE(fvf.checkFile(t2[a]));
    }

    unsigned error = 0;
    for(unsigned a=0; a<nelt; a++) {
        if(fvf.checkFile(nao_esta_la[a])) {
            error++;
        }
    }
    EXPECT_EQ(error, 0);
    ASSERT_LT(error, nelt);
}