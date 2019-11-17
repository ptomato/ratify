/* Copyright 2009, 2019 P. F. Chimento
This file is part of Osxcart.

Osxcart is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Osxcart is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with Osxcart.  If not, see <http://www.gnu.org/licenses/>. */

#include "config.h"

#include <glib.h>

#include "rtf-langcode.h"

/* rtf-langcode.c - One long list of ISO and Windows language codes, and
functions to convert between the two */

typedef struct {
    int wincode;
    const char *isocode;
} IsoLangCode;

/* These are from the RTF spec. The ISO codes are the best I could figure out
from the Wikipedia page. */
IsoLangCode isolangcodes[] = {
    { 0x0400, "zxx" }, /* None */
    { 0x0401, "ar-sa" }, /* Arabic (Saudi Arabia) */
    { 0x0402, "bg" }, /* Bulgarian */
    { 0x0403, "ca" }, /* Catalan */
    { 0x0404, "zh-tw" }, /* Chinese (Taiwan) */
    { 0x0405, "cs" }, /* Czech */
    { 0x0406, "da" }, /* Danish */
    { 0x0407, "de-de" }, /* German (Germany) */
    { 0x0408, "el" }, /* Greek */
    { 0x0409, "en-us" }, /* English (United States) */
    { 0x040A, "es-es" }, /* Spanish (Spain, Traditional Sort)? */
    { 0x040B, "fi" }, /* Finnish */
    { 0x040C, "fr-fr" }, /* French (France) */
    { 0x040D, "he" }, /* Hebrew */
    { 0x040E, "hu" }, /* Hungarian */
    { 0x040F, "is" }, /* Icelandic */
    { 0x0410, "it-it" }, /* Italian (Italy) */
    { 0x0411, "ja" }, /* Japanese */
    { 0x0412, "ko" }, /* Korean */
    { 0x0413, "nl-nl" }, /* Dutch (Netherlands) */
    { 0x0414, "nb" }, /* Norwegian, Bokmal */
    { 0x0415, "pl" }, /* Polish */
    { 0x0416, "pt-br" }, /* Portuguese (Brazil) */
    { 0x0417, "rm" }, /* Romansh */
    { 0x0418, "ro-ro" }, /* Romanian (Romania) */
    { 0x0419, "ru-ru" }, /* Russian (Russia) */
    { 0x041A, "hr-hr" }, /* Croatian (Croatia) */
    { 0x041B, "sk" }, /* Slovak */
    { 0x041C, "sq" }, /* Albanian */
    { 0x041D, "sv-se" }, /* Swedish (Sweden) */
    { 0x041E, "th" }, /* Thai */
    { 0x041F, "tr" }, /* Turkish */
    { 0x0420, "ur-pk" }, /* Urdu (Islamic Republic of Pakistan) */
    { 0x0421, "id" }, /* Indonesian */
    { 0x0422, "uk" }, /* Ukrainian */
    { 0x0423, "be" }, /* Belarussian */
    { 0x0424, "sl" }, /* Slovenian */
    { 0x0425, "et" }, /* Estonian */
    { 0x0426, "lv" }, /* Latvian */
    { 0x0427, "lt" }, /* Lithuanian */
    { 0x0428, "tg" }, /* Tajik */
    { 0x0429, "fa" }, /* Persian */
    { 0x042A, "vi" }, /* Vietnamese */
    { 0x042B, "hy" }, /* Armenian */
    { 0x042C, "az" }, /* Azerbaijani (Latin) */
    { 0x042D, "eu" }, /* Basque */
    { 0x042E, "hsb" }, /* Upper Sorbian */
    { 0x042F, "mk" }, /* Macedonian */
/*  { 0x0430, Sutu (South Africa) -- has no code? */
    { 0x0431, "ts" }, /* Tsonga */
    { 0x0432, "tn" }, /* Setswana */
    { 0x0433, "ve" }, /* Venda */
    { 0x0434, "xh" }, /* isiXhosa */
    { 0x0435, "zu" }, /* isiZulu */
    { 0x0436, "af" }, /* Afrikaans */
    { 0x0437, "ka" }, /* Georgian */
    { 0x0438, "fo" }, /* Faroese */
    { 0x0439, "hi" }, /* Hindi */
    { 0x043A, "mt" }, /* Maltese */
    { 0x043B, "se-no" }, /* Northern Sami (Norway) */
    { 0x043C, "gd" }, /* Gaelic (Scotland) */
    { 0x043D, "yi" }, /* Yiddish */
    { 0x043E, "ms-my" }, /* Malay (Malaysia) */
    { 0x043F, "kk" }, /* Kazakh */
    { 0x0440, "ky" }, /* Kyrgyz */
    { 0x0441, "sw" }, /* Kiswahili */
    { 0x0442, "tk" }, /* Turkmen */
    { 0x0443, "uz" }, /* Uzbek (Latin) */
    { 0x0444, "tt" }, /* Tatar */
    { 0x0445, "bn-in" }, /* Bengali (India) */
    { 0x0446, "pa-in" }, /* Punjabi (India) */
    { 0x0447, "gu" }, /* Gujarati */
    { 0x0448, "or" }, /* Oriya */
    { 0x0449, "ta" }, /* Tamil */
    { 0x044A, "te" }, /* Telugu */
    { 0x044B, "kn" }, /* Kannada */
    { 0x044C, "ml" }, /* Malayalam */
    { 0x044D, "as" }, /* Assamese */
    { 0x044E, "mr" }, /* Marathi */
    { 0x044F, "sa" }, /* Sanskrit */
    { 0x0450, "mn-mn" }, /* Mongolian (Cyrillic, Mongolia) */
    { 0x0451, "bo" }, /* Tibetan */
    { 0x0452, "cy" }, /* Welsh */
    { 0x0453, "km" }, /* Khmer */
    { 0x0454, "lo" }, /* Lao */
    { 0x0455, "my" }, /* Burmese */
    { 0x0456, "gl" }, /* Galician */
    { 0x0457, "kok" }, /* Konkani */
    { 0x0458, "mni" }, /* Manipuri */
    { 0x0459, "sd" }, /* Sindhi (Devanagari) */
    { 0x045A, "syr" }, /* Syriac */
    { 0x045B, "si" }, /* Sinhala */
    { 0x045C, "chr" }, /* Cherokee */
    { 0x045D, "iu" }, /* Inuktitut (Syllabics) */
    { 0x045E, "am" }, /* Amharic */
    { 0x045F, "tia-ma" }, /* Tamazight (Arabic, Morocco) */
    { 0x0460, "ks" }, /* Kashmiri (Arabic) */
    { 0x0461, "ne-np" }, /* Nepali (Nepal) */
    { 0x0462, "fy" }, /* Frisian */
    { 0x0463, "ps" }, /* Pashto */
    { 0x0464, "fil" }, /* Filipino */
    { 0x0465, "dv" }, /* Divehi */
    { 0x0466, "bin" }, /* Edo */
    { 0x0467, "ff" }, /* Fulfulde */
    { 0x0468, "ha" }, /* Hausa */
    { 0x0469, "ibb" }, /* Ibibio */
    { 0x046A, "yo" }, /* Yoruba */
    { 0x046B, "qu-bo" }, /* Quechua (Bolivia) */
    { 0x046C, "nso" }, /* Sotho sa Leboa */
    { 0x046D, "ba" }, /* Bashkir */
    { 0x046E, "lb" }, /* Luxembourgish */
    { 0x046F, "kl" }, /* Greenlandic */
    { 0x0470, "ig" }, /* Igbo */
    { 0x0471, "kr" }, /* Kanuri */
    { 0x0472, "om" }, /* Oromo */
    { 0x0473, "ti-et" }, /* Tigrinya (Ethiopia) */
    { 0x0474, "gn" }, /* Guarani */
    { 0x0475, "haw" }, /* Hawaiian */
    { 0x0476, "la" }, /* Latin */
    { 0x0477, "so" }, /* Somali */
    { 0x0478, "iii" }, /* Yi */
    { 0x0479, "pap" }, /* Papiamentu */
    { 0x047A, "arn" }, /* Mapudungun */
    { 0x047C, "moh" }, /* Mohawk */
    { 0x047E, "br" }, /* Breton */
    { 0x0480, "ug" }, /* Uighur */
    { 0x0481, "mi" }, /* Maori */
    { 0x0482, "oc" }, /* Occitan */
    { 0x0483, "co" }, /* Corsican */
    { 0x0484, "gsw" }, /* Alsatian? */
    { 0x0485, "sah" }, /* Yakut */
    { 0x0486, "quc" }, /* K'iche' */
    { 0x0487, "rw" }, /* Kinyarwanda */
    { 0x0488, "wo" }, /* Wolof */
    { 0x048C, "prs" }, /* Dari? */
    { 0x0801, "ar-iq" }, /* Arabic (Iraq) */
    { 0x0804, "zh-cn" }, /* Chinese (P.R.C.) */
    { 0x0807, "de-ch" }, /* German (Switzerland) */
    { 0x0809, "en-gb" }, /* English (United Kingdom) */
    { 0x080A, "es-mx" }, /* Spanish (Mexico) */
    { 0x080C, "fr-be" }, /* French (Belgium) */
    { 0x0810, "it-ch" }, /* Italian (Switzerland) */
    { 0x0813, "nl-be" }, /* Dutch (Belgium) */
    { 0x0814, "nn" }, /* Norwegian, Nynorsk */
    { 0x0816, "pt-pt" }, /* Portuguese (Portugal) */
    { 0x0818, "ro-md" }, /* Romanian (Moldova) */
    { 0x0819, "ru-md" }, /* Russian (Moldova) */
    { 0x081A, "sr-rs" }, /* Serbian (Latin, Serbia) */
    { 0x081D, "sv-fi" }, /* Swedish (Finland) */
    { 0x0820, "ur-in" }, /* Urdu (India) */
    { 0x0827, "lt" }, /* Lithuanian Traditional? */
    { 0x082C, "az" }, /* Azerbaijani (Cyrillic) */
    { 0x082E, "dsb" }, /* Lower Sorbian */
    { 0x083B, "se-se" }, /* Northern Sami (Sweden) */
    { 0x083C, "ga" }, /* Gaelic (Ireland) */
    { 0x083E, "ms-bn" }, /* Malay (Brunei Darussalam) */
    { 0x0843, "uz" }, /* Uzbek (Cyrillic) */
    { 0x0845, "bn-bd" }, /* Bengali (Bangladesh) */
    { 0x0846, "pa-pk" }, /* Punjabi (Pakistan) */
    { 0x0850, "mn-cn" }, /* Mongolian (Traditional Mongolian, P.R.C.) */
    { 0x0851, "dz" }, /* Dzongkha */
    { 0x0859, "sd" }, /* Sindhi (Arabic) */
    { 0x085D, "iu" }, /* Inuktitut (Latin) */
    { 0x085F, "tia-dz" }, /* Tamazight (Latin, Algeria) */
    { 0x0860, "ks" }, /* Kashmiri */
    { 0x0861, "ne-in" }, /* Nepali (India) */
    { 0x086B, "qu-ec" }, /* Quechua (Ecuador) */
    { 0x0873, "ti-er" }, /* Tigrinya (Eritrea) */
    { 0x0C00, "und" }, /* "Custom Current"? */
    { 0x0C01, "ar-eg" }, /* Arabic (Egypt) */
    { 0x0C04, "zh-hk" }, /* Chinese (Hong Kong S.A.R.) */
    { 0x0C07, "de-at" }, /* German (Austria) */
    { 0x0C09, "en-au" }, /* English (Australia) */
    { 0x0C0A, "es-es" }, /* Spanish (Spain, International Sort)? */
    { 0x0C0C, "fr-ca" }, /* French (Canada) */
    { 0x0C1A, "sr-rs" }, /* Serbian (Cyrillic, Serbia) */
    { 0x0C3B, "se-fi" }, /* Northern Sami (Finland) */
    { 0x0C6B, "qu-pe" }, /* Quechua (Peru) */
    { 0x1001, "ar-ly" }, /* Arabic (Libya) */
    { 0x1004, "zh-sg" }, /* Chinese (Singapore) */
    { 0x1007, "de-lu" }, /* German (Luxembourg) */
    { 0x1009, "en-ca" }, /* English (Canada) */
    { 0x100A, "es-gt" }, /* Spanish (Guatemala) */
    { 0x100C, "fr-ch" }, /* French (Switzerland) */
    { 0x101A, "hr-ba" }, /* Croatian (Bosnia and Herzegovina) */
    { 0x103B, "smj-no" }, /* Lule Sami (Norway) */
    { 0x1401, "ar-dz" }, /* Arabic (Algeria) */
    { 0x1404, "zh-mo" }, /* Chinese (Macao S.A.R.) */
    { 0x1407, "de-li" }, /* German (Liechtenstein) */
    { 0x1409, "en-nz" }, /* English (New Zealand) */
    { 0x140A, "es-cr" }, /* Spanish (Costa Rica) */
    { 0x140C, "fr-lu" }, /* French (Luxembourg) */
    { 0x141A, "bs" }, /* Bosnian (Latin) */
    { 0x143B, "smj-se" }, /* Lule Sami (Sweden) */
    { 0x1801, "ar-ma" }, /* Arabic (Morocco) */
    { 0x1809, "en-ie" }, /* English (Ireland) */
    { 0x180A, "es-pa" }, /* Spanish (Panama) */
    { 0x180C, "fr-mc" }, /* French (Monaco) */
    { 0x181A, "sr-ba" }, /* Serbian (Latin, Bosnia and Herzegovina) */
    { 0x183B, "sma-no" }, /* Southern Sami (Norway) */
    { 0x1C01, "ar-tn" }, /* Arabic (Tunisia) */
    { 0x1C09, "en-za" }, /* English (South Africa) */
    { 0x1C0A, "es-do" }, /* Spanish (Dominican Republic) */
    { 0x1C0C, "fr" }, /* French (West Indies)? */
    { 0x1C1A, "sr-ba" }, /* Serbian (Cyrillic, Bosnia and Herzegovina) */
    { 0x1C3B, "sma-se" }, /* Southern Sami (Sweden) */
    { 0x2001, "ar-om" }, /* Arabic (Oman) */
    { 0x2009, "en-jm" }, /* English (Jamaica) */
    { 0x200A, "es-ve" }, /* Spanish (Venezuela) */
    { 0x200C, "fr-re" }, /* French (Reunion) */
    { 0x201A, "bs" }, /* Bosnian (Cyrillic) */
    { 0x203B, "sms" }, /* Skolt Sami */
    { 0x2401, "ar-ye" }, /* Arabic (Yemen) */
    { 0x2409, "en" }, /* English (Caribbean)? */
    { 0x240A, "es-co" }, /* Spanish (Colombia) */
    { 0x240C, "fr-cd" }, /* French (Democratic Republic of Congo) */
    { 0x243B, "smn" }, /* Inari Sami */
    { 0x2801, "ar-sy" }, /* Arabic (Syria) */
    { 0x2809, "en-bz" }, /* English (Belize) */
    { 0x280A, "es-pe" }, /* Spanish (Peru) */
    { 0x280C, "fr-sn" }, /* French (Senegal) */
    { 0x2C01, "ar-jo" }, /* Arabic (Jordan) */
    { 0x2C09, "en-tt" }, /* English (Trinidad and Tobago) */
    { 0x2C0A, "es-ar" }, /* Spanish (Argentina) */
    { 0x2C0C, "fr-cm" }, /* French (Cameroon) */
    { 0x3001, "ar-lb" }, /* Arabic (Lebanon) */
    { 0x3009, "en-zw" }, /* English (Zimbabwe) */
    { 0x300A, "es-ec" }, /* Spanish (Ecuador) */
    { 0x300C, "fr-ci" }, /* French (Cote d'Ivoire) */
    { 0x3401, "ar-kw" }, /* Arabic (Kuwait) */
    { 0x3409, "en-ph" }, /* English (Philippines) */
    { 0x340A, "es-cl" }, /* Spanish (Chile) */
    { 0x340C, "fr-ml" }, /* French (Mali) */
    { 0x3801, "ar-ae" }, /* Arabic (U.A.E.) */
    { 0x3809, "en-id" }, /* English (Indonesia) */
    { 0x380A, "es-uy" }, /* Spanish (Uruguay) */
    { 0x380C, "fr-ma" }, /* French (Morocco) */
    { 0x3C01, "ar-bh" }, /* Arabic (Bahrain) */
    { 0x3C09, "en-hk" }, /* English (Hong Kong S.A.R.) */
    { 0x3C0A, "es-py" }, /* Spanish (Paraguay) */
    { 0x3C0C, "fr-ht" }, /* French (Haiti) */
    { 0x4001, "ar-qa" }, /* Arabic (Qatar) */
    { 0x4009, "en-in" }, /* English (India) */
    { 0x400A, "es-bo" }, /* Spanish (Bolivia) */
    { 0x4409, "en-my" }, /* English (Malaysia) */
    { 0x440A, "es-sv" }, /* Spanish (El Salvador) */
    { 0x4809, "en-sg" }, /* English (Singapore) */
    { 0x480A, "es-hn" }, /* Spanish (Honduras) */
    { 0x4C0A, "es-ni" }, /* Spanish (Nicaragua) */
    { 0x500A, "es-pr" }, /* Spanish (Puerto Rico) */
    { 0x540A, "es-us" }, /* Spanish (United States) */
    /* Aliases */
    { 0x0000, "zxx" }, /* None */
    { 0x0009, "en" }, /* English? */
    { 0x0013, "nl" }, /* "Dutch Preferred"? */
    { 0x0409, "c" }, /* C/POSIX locale = English? */
    { 0, NULL }
};

/* Do not free return value */
const char *
language_to_iso(int wincode)
{
    IsoLangCode *lang;
    for(lang = isolangcodes; lang->isocode != NULL; lang++)
        if(wincode == lang->wincode)
            return lang->isocode;
    return "zxx"; /* No language */
}

int
language_to_wincode(const char *isocode)
{
    IsoLangCode *lang;
    for(lang = isolangcodes; lang->isocode != NULL; lang++)
        if(g_ascii_strcasecmp(isocode, lang->isocode) == 0)
            return lang->wincode;
    return 1024; /* No language */
}
