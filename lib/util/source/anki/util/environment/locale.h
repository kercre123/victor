//
//  locale.h
//
//  Created by Stuart Eichert on 3/24/2016
//  Copyright (c) 2016 Anki, Inc. All rights reserved.
//

#ifndef __util_environment_locale_H__
#define __util_environment_locale_H__

#include <string>

namespace Anki {
namespace Util {

class Locale {

public:
  enum class CountryISO2 {
    AD, // Andorra
    AE, // United Arab Emirates
    AF, // Afghanistan
    AG, // Antigua and Barbuda
    AI, // Anguilla
    AL, // Albania
    AM, // Armenia
    AO, // Angola
    AQ, // Antarctica
    AR, // Argentina
    AS, // American Samoa
    AT, // Austria
    AU, // Australia
    AW, // Aruba
    AX, // Åland Islands
    AZ, // Azerbaijan
    BA, // Bosnia and Herzegovina
    BB, // Barbados
    BD, // Bangladesh
    BE, // Belgium
    BF, // Burkina Faso
    BG, // Bulgaria
    BH, // Bahrain
    BI, // Burundi
    BJ, // Benin
    BL, // Saint Barthélemy
    BM, // Bermuda
    BN, // Brunei Darussalam
    BO, // Bolivia
    BQ, // Bonaire
    BR, // Brazil
    BS, // Bahamas
    BT, // Bhutan
    BV, // Bouvet Island
    BW, // Botswana
    BY, // Belarus
    BZ, // Belize
    CA, // Canada
    CC, // Cocos (Keeling) Islands
    CD, // Democratic Republic of the Congo
    CF, // Central African Republic
    CG, // Congo
    CH, // Switzerland
    CI, // Côte d'Ivoire
    CK, // Cook Islands
    CL, // Chile
    CM, // Cameroon
    CN, // China
    CO, // Colombia
    CR, // Costa Rica
    CU, // Cuba
    CV, // Cape Verde
    CW, // Curaçao
    CX, // Christmas Island
    CY, // Cyprus
    CZ, // Czech Republic
    DE, // Germany
    DJ, // Djibouti
    DK, // Denmark
    DM, // Dominica
    DO, // Dominican Republic
    DZ, // Algeria
    EC, // Ecuador
    EE, // Estonia
    EG, // Egypt
    EH, // Western Sahara
    ER, // Eritrea
    ES, // Spain
    ET, // Ethiopia
    FI, // Finland
    FJ, // Fiji
    FK, // Falkland Islands (Malvinas)
    FM, // Micronesia
    FO, // Faroe Islands
    FR, // France
    GA, // Gabon
    GB, // United Kingdom
    GD, // Grenada
    GE, // Georgia
    GF, // French Guiana
    GG, // Guernsey
    GH, // Ghana
    GI, // Gibraltar
    GL, // Greenland
    GM, // Gambia
    GN, // Guinea
    GP, // Guadeloupe
    GQ, // Equatorial Guinea
    GR, // Greece
    GS, // South Georgia and the South Sandwich Islands
    GT, // Guatemala
    GU, // Guam
    GW, // Guinea-Bissau
    GY, // Guyana
    HK, // Hong Kong
    HM, // Heard Island and McDonald Islands
    HN, // Honduras
    HR, // Croatia
    HT, // Haiti
    HU, // Hungary
    ID, // Indonesia
    IE, // Ireland
    IL, // Israel
    IM, // Isle of Man
    IN, // India
    IO, // British Indian Ocean Territory
    IQ, // Iraq
    IR, // Iran
    IS, // Iceland
    IT, // Italy
    JE, // Jersey
    JM, // Jamaica
    JO, // Jordan
    JP, // Japan
    KE, // Kenya
    KG, // Kyrgyzstan
    KH, // Cambodia
    KI, // Kiribati
    KM, // Comoros
    KN, // Saint Kitts and Nevis
    KP, // North Korea
    KR, // South Korea
    KW, // Kuwait
    KY, // Cayman Islands
    KZ, // Kazakhstan
    LA, // Lao People's Democratic Republic
    LB, // Lebanon
    LC, // Saint Lucia
    LI, // Liechtenstein
    LK, // Sri Lanka
    LR, // Liberia
    LS, // Lesotho
    LT, // Lithuania
    LU, // Luxembourg
    LV, // Latvia
    LY, // Libya
    MA, // Morocco
    MC, // Monaco
    MD, // Moldova
    ME, // Montenegro
    MF, // Saint Martin (French part)
    MG, // Madagascar
    MH, // Marshall Islands
    MK, // Macedonia
    ML, // Mali
    MM, // Myanmar
    MN, // Mongolia
    MO, // Macao
    MP, // Northern Mariana Islands
    MQ, // Martinique
    MR, // Mauritania
    MS, // Montserrat
    MT, // Malta
    MU, // Mauritius
    MV, // Maldives
    MW, // Malawi
    MX, // Mexico
    MY, // Malaysia
    MZ, // Mozambique
    NA, // Namibia
    NC, // New Caledonia
    NE, // Niger
    NF, // Norfolk Island
    NG, // Nigeria
    NI, // Nicaragua
    NL, // Netherlands
    NORWAY, // Norway - can't use NO because Obj-C++ complains
    NP, // Nepal
    NR, // Nauru
    NU, // Niue
    NZ, // New Zealand
    OM, // Oman
    PA, // Panama
    PE, // Peru
    PF, // French Polynesia
    PG, // Papua New Guinea
    PH, // Philippines
    PK, // Pakistan
    PL, // Poland
    PM, // Saint Pierre and Miquelon
    PN, // Pitcairn
    PR, // Puerto Rico
    PS, // Palestine
    PT, // Portugal
    PW, // Palau
    PY, // Paraguay
    QA, // Qatar
    RE, // Réunion
    RO, // Romania
    RS, // Serbia
    RU, // Russian Federation
    RW, // Rwanda
    SA, // Saudi Arabia
    SB, // Solomon Islands
    SC, // Seychelles
    SD, // Sudan
    SE, // Sweden
    SG, // Singapore
    SH, // Saint Helena
    SI, // Slovenia
    SJ, // Svalbard and Jan Mayen
    SK, // Slovakia
    SL, // Sierra Leone
    SM, // San Marino
    SN, // Senegal
    SO, // Somalia
    SR, // Suriname
    SS, // South Sudan
    ST, // Sao Tome and Principe
    SV, // El Salvador
    SX, // Sint Maarten (Dutch part)
    SY, // Syrian Arab Republic
    SZ, // Swaziland
    TC, // Turks and Caicos Islands
    TD, // Chad
    TF, // French Southern Territories
    TG, // Togo
    TH, // Thailand
    TJ, // Tajikistan
    TK, // Tokelau
    TL, // Timor-Leste
    TM, // Turkmenistan
    TN, // Tunisia
    TO, // Tonga
    TR, // Turkey
    TT, // Trinidad and Tobago
    TV, // Tuvalu
    TW, // Taiwan
    TZ, // Tanzania
    UA, // Ukraine
    UG, // Uganda
    UM, // United States Minor Outlying Islands
    US, // United States
    UY, // Uruguay
    UZ, // Uzbekistan
    VA, // Holy See (Vatican City State)
    VC, // Saint Vincent and the Grenadines
    VE, // Venezuela
    VG, // British Virgin Islands
    VI, // U.S. Virgin Islands
    VN, // Viet Nam
    VU, // Vanuatu
    WF, // Wallis and Futuna
    WS, // Samoa
    YE, // Yemen
    YT, // Mayotte
    ZA, // South Africa
    ZM, // Zambia
    ZW  // Zimbabwe
  };

  enum class Language {
    aa, // Afar
    ab, // Abkhazian
    ae, // Avestan
    af, // Afrikaans
    ak, // Akan
    am, // Amharic
    an, // Aragonese
    ar, // Arabic
    as, // Assamese
    av, // Avaric
    ay, // Aymara
    az, // Azerbaijani
    ba, // Bashkir
    be, // Belarusian
    bg, // Bulgarian
    bh, // Bihari languages
    bi, // Bislama
    bm, // Bambara
    bn, // Bengali
    bo, // Tibetan
    br, // Breton
    bs, // Bosnian
    ca, // Catalan; Valencian
    ce, // Chechen
    ch, // Chamorro
    co, // Corsican
    cr, // Cree
    cs, // Czech
    cu, // Church Slavic; Old Slavonic; Church Slavonic; Old Bulgarian; Old Church Slavonic
    cv, // Chuvash
    cy, // Welsh
    da, // Danish
    de, // German
    dv, // Divehi; Dhivehi; Maldivian
    dz, // Dzongkha
    ee, // Ewe
    el, // Greek
    en, // English
    eo, // Esperanto
    es, // Spanish; Castilian
    et, // Estonian
    eu, // Basque
    fa, // Persian
    ff, // Fulah
    fi, // Finnish
    fj, // Fijian
    fo, // Faroese
    fr, // French
    fy, // Western Frisian
    ga, // Irish
    gd, // Gaelic; Scottish Gaelic
    gl, // Galician
    gn, // Guarani
    gu, // Gujarati
    gv, // Manx
    ha, // Hausa
    he, // Hebrew
    hi, // Hindi
    ho, // Hiri Motu
    hr, // Croatian
    ht, // Haitian; Haitian Creole
    hu, // Hungarian
    hy, // Armenian
    hz, // Herero
    ia, // Interlingua (International Auxiliary Language Association)
    id, // Indonesian
    ie, // Interlingue; Occidental
    ig, // Igbo
    ii, // Sichuan Yi; Nuosu
    ik, // Inupiaq
    io, // Ido
    is, // Icelandic
    it, // Italian
    iu, // Inuktitut
    ja, // Japanese
    jv, // Javanese
    ka, // Georgian
    kg, // Kongo
    ki, // Kikuyu; Gikuyu
    kj, // Kuanyama; Kwanyama
    kk, // Kazakh
    kl, // Kalaallisut; Greenlandic
    km, // Central Khmer
    kn, // Kannada
    ko, // Korean
    kr, // Kanuri
    ks, // Kashmiri
    ku, // Kurdish
    kv, // Komi
    kw, // Cornish
    ky, // Kirghiz; Kyrgyz
    la, // Latin
    lb, // Luxembourgish; Letzeburgesch
    lg, // Ganda
    li, // Limburgan; Limburger; Limburgish
    ln, // Lingala
    lo, // Lao
    lt, // Lithuanian
    lu, // Luba-Katanga
    lv, // Latvian
    mg, // Malagasy
    mh, // Marshallese
    mi, // Maori
    mk, // Macedonian
    ml, // Malayalam
    mn, // Mongolian
    mr, // Marathi
    ms, // Malay
    mt, // Maltese
    my, // Burmese
    na, // Nauru
    nb, // Bokmål
    nd, // Ndebele
    ne, // Nepali
    ng, // Ndonga
    nl, // Dutch; Flemish
    nn, // Norwegian Nynorsk; Nynorsk
    no, // Norwegian
    nr, // Ndebele
    nv, // Navajo; Navaho
    ny, // Chichewa; Chewa; Nyanja
    oc, // Occitan (post 1500); Provençal
    oj, // Ojibwa
    om, // Oromo
    oriya, // Oriya - Can't use "or" because compiler complains
    os, // Ossetian; Ossetic
    pa, // Panjabi; Punjabi
    pi, // Pali
    pl, // Polish
    ps, // Pushto; Pashto
    pt, // Portuguese
    qu, // Quechua
    rm, // Romansh
    rn, // Rundi
    ro, // Romanian; Moldavian; Moldovan
    ru, // Russian
    rw, // Kinyarwanda
    sa, // Sanskrit
    sc, // Sardinian
    sd, // Sindhi
    se, // Northern Sami
    sg, // Sango
    si, // Sinhala; Sinhalese
    sk, // Slovak
    sl, // Slovenian
    sm, // Samoan
    sn, // Shona
    so, // Somali
    sq, // Albanian
    sr, // Serbian
    ss, // Swati
    st, // Sotho
    su, // Sundanese
    sv, // Swedish
    sw, // Swahili
    ta, // Tamil
    te, // Telugu
    tg, // Tajik
    th, // Thai
    ti, // Tigrinya
    tk, // Turkmen
    tl, // Tagalog
    tn, // Tswana
    to, // Tonga (Tonga Islands)
    tr, // Turkish
    ts, // Tsonga
    tt, // Tatar
    tw, // Twi
    ty, // Tahitian
    ug, // Uighur; Uyghur
    uk, // Ukrainian
    ur, // Urdu
    uz, // Uzbek
    ve, // Venda
    vi, // Vietnamese
    vo, // Volapük
    wa, // Walloon
    wo, // Wolof
    xh, // Xhosa
    yi, // Yiddish
    yo, // Yoruba
    za, // Zhuang; Chuang
    zh, // Chinese
    zu  // Zulu
  };
  static std::string CountryISO2ToString(const CountryISO2 countryCode);
  static CountryISO2 CountryISO2FromString(const std::string& countryString,
                                           bool* resultValid = nullptr);
  static std::string LanguageToString(const Language languageCode);
  static Language LanguageFromString(const std::string& languageString,
                                     bool* resultValid = nullptr);

  // this method will use native adapters to return the current device locale
  static Locale GetNativeLocale();
  static bool IsValidLocaleString(const std::string& localeString);
  static Locale LocaleFromString(const std::string& localeString);

  static const Language kDefaultLanguage = Language::en;
  static const CountryISO2 kDefaultCountry = CountryISO2::US;
  static const Locale kDefaultLocale;

  Locale() : Locale(kDefaultLanguage, kDefaultCountry) { }
  Locale(const std::string& language, const std::string& country)
    : Locale(LanguageFromString(language), CountryISO2FromString(country)) { }
  Locale(Language language, CountryISO2 country)
    : _language(language)
    , _country(country) { }

  Language GetLanguage() const { return _language; }
  CountryISO2 GetCountry() const { return _country; }
  std::string GetLanguageString() const { return LanguageToString(_language); }
  std::string GetCountryString() const { return CountryISO2ToString(_country); }
  std::string GetLocaleString() const; // ex: "en_US" or "de_DE"
  std::string GetLocaleStringLowerCase() const; // ex: "en_us" or "de_de"
  std::string GetUrlString() const; // ex: "en-us" or "de-de"
  std::string ToString() const; // ex: "en-US" or "de-DE"

  inline bool operator==(const Locale& rhs) const {
    return ((GetLanguage() == rhs.GetLanguage()) && (GetCountry() == rhs.GetCountry()));
  }
  inline bool operator!=(const Locale& rhs) const { return !(*this == rhs); }
  inline bool operator<(const Locale& rhs) const {
    return (ToString() < rhs.ToString());
  }

private:
  Language _language;
  CountryISO2 _country;
};

} // namespace Util
} // namespace Anki

#endif // __driveEngine_rushHourAdapters_locale_H__
