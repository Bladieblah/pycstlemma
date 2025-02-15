/*
CSTLEMMA - trainable lemmatiser

Copyright (C) 2002, 2014  Center for Sprogteknologi, University of Copenhagen

This file is part of CSTLEMMA.

CSTLEMMA is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

CSTLEMMA is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CSTLEMMA; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "lemmatiser.h"
#include "applyrules.h"
#include "tags.h"
#include "caseconv.h"
#include "option.h"
#include "makedict.h"

#include "flex.h"
#include "basefrm.h"
#include "XMLtext.h"
#include "flattext.h"
#include "lemmtags.h"
#ifdef _MSC_VER
#include <io.h>
#endif
#include <stdarg.h>
#include <string>
#include <time.h>
#include <limits.h>

using namespace std;

int Lemmatiser::instance = 0;

static bool DoInfo = true;
tagpairs *Lemmatiser::TextToDictTags = 0;

static int info(const char *fmt, ...)
{
    if (DoInfo)
    {
        int ret;
        va_list ap;
        va_start(ap, fmt);
        ret = vprintf(fmt, ap);
        va_end(ap);
        LOG1LINE("");
        return ret;
    }
    return 0;
}

static void showtime(clock_t t0)
{
    clock_t t1;
    unsigned long span, sec, msec;
    t1 = clock();
    span = t1 - t0;
    sec = span / CLOCKS_PER_SEC;
    span -= sec * CLOCKS_PER_SEC;
    span *= 1000;
    msec = span / CLOCKS_PER_SEC;
    info("\nTime: %ld.%.3ld", sec, msec);
}

const char *Lemmatiser::translate(const char *tag)
{
    return TextToDictTags ? TextToDictTags->translate(tag) : tag; // tag as found in the text
}

Lemmatiser::Lemmatiser(optionStruct &a_Option) : listLemmas(0), SortInput(false), Option(a_Option), changed(true)
{
    nice = Option.nice;
    instance++;
    if (instance == 1)
    {
#if defined PROGLEMMATISE
        if (!Option.argo)
            DoInfo = false; // suppress messages when using stdout
#endif
        switch (Option.whattodo)
        {
        case whattodoTp::MAKEDICT:
        {
#if defined PROGMAKEDICT
            status = MakeDict();
#endif
            break;
        }
        case whattodoTp::MAKEFLEXPATTERNS:
        {
#if defined PROGMAKESUFFIXFLEX
            status = MakeFlexPatterns();
#endif
            break;
        }
        default:
        {
#if defined PROGLEMMATISE
            status = LemmatiseInit();
#endif
            break;
        }
        }
    }
    else
        status = -3; // Only one instance of Lemmatiser allowed.
#ifdef COUNTOBJECTS
    ++COUNT;
#endif
}

Lemmatiser::~Lemmatiser()
{
    instance--;
    switch (Option.whattodo)
    {
    case whattodoTp::MAKEDICT:
    {
        break;
    }
    case whattodoTp::MAKEFLEXPATTERNS:
    {
        break;
    }
    default:
    {
#if defined PROGLEMMATISE
        LemmatiseEnd();
#endif
    }
    }
#ifdef COUNTOBJECTS
    --COUNT;
#endif
}

#if (defined PROGLEMMATISE) || (defined PROGMAKEDICT)
static void cannotOpenFile(const char *s1, const char *name, const char *s2)
{
    fprintf(stderr, "%s \"%s\" %s\n", s1, name, s2);
}
#endif

#if defined PROGMAKEDICT
int Lemmatiser::MakeDict()
{
    FILE *fpin;
    FILE *fpout;
    FILE *ffreq = 0;
    if (!Option.cformat)
    {
        LOG1LINE("You need to specify a column-order with the -c option");
        return -1;
    }
    if (Option.argi)
    {
        fpin = fopen(Option.argi, "r");
        if (!fpin)
        {
            cannotOpenFile("Cannot open input file", Option.argi, "for reading");
            return -1;
        }
    }
    else
        fpin = stdin;

    if (Option.argo)
    {
        fpout = fopen(Option.argo, "wb");
        if (!fpout)
        {
            cannotOpenFile("Cannot open binary dictionary", Option.argo, "for writing");
            return -1;
        }
    }
    else
        fpout = stdout;
    int ret = makedict(fpin, fpout, nice, Option.cformat, Option.freq, Option.CollapseHomographs);
    if (fpin != stdin)
        fclose(fpin);
    if (fpout != stdout)
        fclose(fpout);
    if (ffreq)
        fclose(ffreq);
    return ret;
}
#endif

#if defined PROGMAKESUFFIXFLEX
int Lemmatiser::MakeFlexPatterns()
{
    FILE *fpdict;
    FILE *fpflex;
    if (!Option.cformat)
    {
        LOG1LINE("You need to specify a column-order with the -c option");
        return -1;
    }

    if (Option.argi)
    {
        fpdict = fopen(Option.argi, "r");
        if (!fpdict)
            return -1;
    }
    else
        fpdict = stdin;

    if (Option.flx)
    {
        fpflex = fopen(Option.flx, "rb");
        if (fpflex)
        {
            Flex.readFromFile(fpflex);
            fclose(fpflex);
        }
    }
    if (Option.argo)
    {
        fpflex = fopen(Option.argo, "wb");
        if (!fpflex)
            return -1;
    }
    else
        fpflex = stdout;

    int failed;
    Flex.makeFlexRules(fpdict, fpflex, nice, Option.cformat, failed, Option.CutoffRefcount, Option.showRefcount, Option.argo);
    if (fpdict != stdin)
        fclose(fpdict);

    if (fpflex != stdout)
        fclose(fpflex);
    return 0;
}
#endif

#if defined PROGLEMMATISE
int Lemmatiser::setFormats()
{
    info("\nFormats:");

    listLemmas = 0;
    if (Option.Iformat)
    {
        info("-I\t%s\tInput format.", Option.Iformat);
    }
    if (!Option.Bformat && !Option.bformat && !Option.cformat && !Option.Wformat)
    {
        Option.setBformat(optionStruct::Default_B_format);
        if (Option.dictfile)
        {
            Option.setbformat(optionStruct::Default_b_format);
            Option.setcformat(
                Option.XML            ? optionStruct::DefaultCFormatXML
                : Option.InputHasTags ? optionStruct::DefaultCFormat
                                      : optionStruct::DefaultCFormat_NoTags);
        }
        else
        {
            Option.setcformat(
                Option.XML            ? optionStruct::DefaultCFormatXML_NoDict
                : Option.InputHasTags ? optionStruct::DefaultCFormat_NoDict
                                      : optionStruct::DefaultCFormat_NoTags_NoDict);
        }
    }
    if (Option.Defaultbformat() && !Option.DefaultBformat())
    {
        delete[] Option.bformat;
        Option.bformat = 0;
    }
    else if (Option.DefaultBformat() && !Option.Defaultbformat())
    {
        delete[] Option.Bformat;
        Option.Bformat = 0;
    }
    else if (Option.DefaultBformat() && Option.Defaultbformat() && Option.Wformat)
    {
        LOG1LINE("You need to specify -b or -B formats if you specify the -W format");
        return -1;
    }

    if (!Option.DefaultCformat() && Option.Wformat)
    {
        LOG1LINE("You cannot specify both -c and -W format options");
        return -1;
    }
    else if (Option.cformat && !Option.Wformat)
    {
        info("-c\t%s\tOutput format", Option.cformat);

        if (Option.bformat)
            info("-b\t%s\tDictionary base form output format.", Option.bformat);

        if (Option.Bformat)
            info("-B\t%s\tComputed base form output format.", Option.Bformat);

        listLemmas = 0;
    }
    else
    {
        if (Option.bformat)
        {
            info("-b\t%s\tOutput format for data pertaining to the base form, according to the dictionary", Option.bformat);

            listLemmas |= 1;
        }
        if (Option.Bformat)
        {
            listLemmas |= 2;
            info("-B\t%s\tOutput format for data pertaining to the base form, as predicted by flex pattern rules.", Option.Bformat);
        }
        if (!listLemmas)
        {
            LOG1LINE("You must specify at least one of -b and -B if you do not specify -c.");
            return -1;
        }
        if (Option.Wformat)
            info("-W\t%s\tOutput format for data pertaining to full forms.", Option.Wformat);
    }
    if (listLemmas)
    {
        SortInput = basefrm::setFormat(Option.Wformat, Option.bformat, Option.Bformat, Option.InputHasTags);
        if (SortInput)
            info("Input is sorted before processing (due to $f field in -W<format> argument)");
    }
    else
    {
        SortInput = text::setFormat(Option.cformat, Option.bformat, Option.Bformat, Option.InputHasTags);
        if (SortInput)
            info("Input is sorted before processing (due to $f field in -c<format> argument)");
    }
    if (!SortInput)
    {
        if (listLemmas || Option.UseLemmaFreqForDisambiguation < 2)
            SortInput = true; // performance
    }
    if (!Option.XML)
        info("-X-\tNot XML input.");

    else
    {
        if (Option.ancestor || Option.element || Option.segment || Option.wordAttribute || Option.POSAttribute || Option.lemmaAttribute || Option.lemmaClassAttribute)
        {
            info("\tXML-aware scanning of input");

            if (Option.ancestor)
                info("-Xa%s\tOnly analyse elements with ancestor %s", Option.ancestor, Option.ancestor);

            if (Option.element)
                info("-Xe%s\tOnly analyse elements %s", Option.element, Option.element);

            if (Option.segment)
                info("-Xe%s\tBegin new segment after the tag %s", Option.segment, Option.segment);

            if (Option.wordAttribute)
            {
                info("-Xw%s\tLook for word in attribute %s", Option.wordAttribute, Option.wordAttribute);

                if (!Option.lemmaAttribute)
                    info("\tStore lemma in attribute %s", Option.wordAttribute);
            }
            if (Option.POSAttribute)
            {
                if (Option.InputHasTags)
                    info("-Xp%s\tLook for Part of Speech in attribute %s", Option.POSAttribute, Option.POSAttribute);

                else
                    info("-Xp%s\tLook for Part of Speech in attribute %s (ignored because of option -t-)", Option.POSAttribute, Option.POSAttribute);
            }
            if (Option.lemmaAttribute)
                info("-Xl%s\tStore lemma in attribute %s", Option.lemmaAttribute, Option.lemmaAttribute);

            if (Option.lemmaClassAttribute)
                info("-Xc%s\tStore lemma class in attribute %s", Option.lemmaClassAttribute, Option.lemmaClassAttribute);
        }
        else
        {
            info("-X\tXML-aware scanning of input");
        }
    }
    changed = false;
    return 0;
}

int Lemmatiser::openFiles()
{
    FILE *fpflex;
    FILE *fpdict = 0;
    FILE *fpv = 0;
    FILE *fpx = 0;
    FILE *fpz = 0;
    if (Option.flx)
    {
        fpflex = fopen(Option.flx, "rb");
        if (fpflex)
        {
            info("-f\t%s\tFile with flex patterns.", Option.flx);
        }
        else
        {
            cannotOpenFile("-f\t", Option.flx, "\t(Flexrules): Cannot open file.");
        }
    }
    else
    {
        LOG1LINE("-f  Flexpatterns: File not specified.");
        return -1;
    }

    if (Option.dictfile)
    {
        fpdict = fopen(Option.dictfile, "rb");
        if (fpdict)
        {
            info("-d\t%s\tFile with dict patterns.", Option.dictfile);
        }
        else
        {
            cannotOpenFile("-d\t", Option.dictfile, "\t(Dictionary): Cannot open file.");
            return -1;
        }
    }
    else
    {
        info("-d\tDictionary: File not specified.");
    }

    if (Option.InputHasTags)
    {
        if (Option.v)
        {
            fpv = fopen(Option.v, "rb"); /* "r" -> "rb" 20130122 */
            if (!fpv)
            {
                cannotOpenFile("-v\t", Option.v, "\t(Tag friends file): Cannot open file.");
                return -1;
            }
            else
                info("-v\t%-20s\tTag friends file", Option.v);
        }
        else
            info("-v\tTag friends file: File not specified.");

        if (Option.x)
        {
            fpx = fopen(Option.x, "rb"); /* "r" -> "rb" 20130122 */
            if (!fpx)
            {
                cannotOpenFile("-x\t", Option.x, "\t(Lexical type translation table): Cannot open file.");
                return -1;
            }
            else
                info("-x\t%-20s\tLexical type translation table", Option.x);
        }
        else
            info("-x\tLexical type translation table: File not specified.");

        if (Option.z)
        {
            fpz = fopen(Option.z, "rb");
            if (!fpz)
            {
                cannotOpenFile("-z\t", Option.z, "\t(Full form - Lemma type conversion table): Cannot open file.");
                return -1;
            }
            else
                info("-z\t%-20s\tFull form - Lemma type conversion table", Option.z);
        }
        else
            info("-z\tFull form - Lemma type conversion table: File not specified.");
    }
    else
    {
        if (Option.z)
        {
            fpz = fopen(Option.z, "rb"); /* "r" -> "rb" 20130122 */
            if (!fpz)
            {
                cannotOpenFile("-z\t", Option.z, "\t(Full form - Lemma type conversion table): Cannot open file.");
                return -1;
            }
            else
                info("-z\t%-20s\tFull form - Lemma type conversion table", Option.z);
        }
    }

    if (fpflex)
    {
        Flex.readFromFile(fpflex, Option.InputHasTags ? Option.flx : 0);
        // fpflex contains rules for untagged text. These can be used if tag-specific rules do not exist.
        if (Option.RulesUnique)
            Flex.removeAmbiguous();
        if (nice)
        {
            LOG1LINE("");
            Flex.print();
        }
        fclose(fpflex);
    }
    else if (!readRules(Option.flx))
    {
        if (Option.InputHasTags)
            ; // Rules will be read as necessary, depending on which tags occur in the text.
        else
            return -1;
    }

    if (fpv)
    {
        if (TagFriends)
            delete TagFriends;
        TagFriends = new tagpairs(fpv, nice);
        /*                if(!readTags(fpx,nice))
                    {
                    fclose(fpx);
                    return -1;
                    }*/
        fclose(fpv);
    }

    if (fpx)
    {
        if (TextToDictTags)
            delete TextToDictTags;
        TextToDictTags = new tagpairs(fpx, nice);
        /*                if(!readTags(fpx,nice))
                    {
                    fclose(fpx);
                    return -1;
                    }*/
        fclose(fpx);
    }

    if (fpz)
    {
        if (!readLemmaTags(fpz, nice))
        {
            fclose(fpz);
            return -1;
        }
        fclose(fpz);
    }

    if (nice && fpdict)
        printf("\nreading dictionary \"%s\"\n", Option.dictfile);

    dict.initdict(fpdict);
    if (fpdict)
        fclose(fpdict);
    return 0;
}

void Lemmatiser::showSwitches()
{
    info("\nSwitches:");

    if (Option.InputHasTags)
    {
        info("-t\tInput has tags.");
    }
    else
    {
        info("-t-\tInput has no tags.");

        if (Option.POSAttribute)
        {
            info("\tThe PoS-attrribute '%s' (-Xp%s) is ignored.", Option.POSAttribute, Option.POSAttribute);

            Option.POSAttribute = NULL;
        }
        if (!Option.Iformat)
        {
            if (Option.keepPunctuation == 1)
                info("-p\tKeep punctuation.");

            else if (Option.keepPunctuation == 2)
                info("-p+\tTreat punctuation as separate tokens.");

            else
                info("-p-\tIgnore punctuation.");
        }
    }

    if (!Option.Wformat)
    {
        if (Option.SortOutput)
        {
            SortInput = true;
            info("-q\tSort output.");

            info("Input is sorted before processing (due to option -q)\n");

            if (SortFreq(Option.SortOutput))
            {
                info("-q#\tSort output by frequence.");
            }
        }
        else
        {
            info("-q-\tDo not sort output.(default)");
        }
    }

    if (!SortInput)
        info("Input is not sorted before processing (no option -q and no $f field in -c<format> or -W<format> argument)");

    if (!strcmp(Option.Sep, "\t"))
        info("-s\tAmbiguous output is tab-separated");

    else if (!strcmp(Option.Sep, " "))
        info("-s" commandlineQuote " " commandlineQuote "\tAmbiguous output  is blank-separated");

    else if (!strcmp(Option.Sep, Option.DefaultSep))
        info("-s%s\tAmbiguous output is " commandlineQuote "%s" commandlineQuote "-separated (default)", Option.Sep, Option.Sep);

    else
        info("-s%s\tAmbiguous output is " commandlineQuote "%s" commandlineQuote "-separated", Option.Sep, Option.Sep);

    if (Option.RulesUnique)
        info("-U\tenforce unique flex rules");

    else
        info("-U-\tallow ambiguous flex rules (default)");

    if (Option.DictUnique)
        info("-u\tenforce unique dictionary look-up");

    else
        info("-u-\tallow ambiguous dictionary look-up (default)");

    switch (Option.UseLemmaFreqForDisambiguation)
    {
    case 0:
        info("-H0\tuse lemma frequencies for disambigation");

        basefrm::hasW = true;
        break;
    case 1:
        info("-H1\tuse lemma frequencies for disambigation, show pruned lemmas between <<>>");

        basefrm::hasW = true;
        break;
    case 2:
        info("-H2\tdon't use lemma frequencies for disambigation (default)");

        break;
    }
    if (Option.baseformsAreLowercase == caseTp::elower)
        info("-l\tlemmas are forced to lowercase (default)");

    else if (Option.baseformsAreLowercase == caseTp::emimicked)
        info("-l-\tlemmas are same case as full form");

    else
        info("-l-\tcase of lemmas is decided by rules or dictionary");

    if (Option.size < ULONG_MAX)
        info("-m%lu\tReading max %lu words from input", Option.size, Option.size);

    else
        info("-m0\tReading unlimited number of words from input (default).");

    if (Option.arge)
    {
        if ('0' < *Option.arge && *Option.arge <= '9')
            info("-e%s\tUse ISO8859-%s Character encoding for case conversion.", Option.arge, Option.arge);

        else
            info("-e%s\tUse Unicode Character encoding for case conversion.", Option.arge);
    }
    else
        info("-e-\tDon't use case conversion.");

    if (nice)
        LOG1LINE("reading text\n");
}

int Lemmatiser::LemmatiseInit()
{
    changed = true;
    int ret = setFormats();
    if (ret)
        return ret;

    ret = openFiles();
    if (ret)
        return ret;

    showSwitches();
    return 0;
}

void Lemmatiser::LemmatiseText(FILE *fpin, FILE *fpout, tallyStruct *tally)
{
    if (changed)
        setFormats();
    text *Text;
    if (Option.XML)
    {
        Text = new XMLtext(fpin, Option);
        Text->DoYourWork(fpin, Option);
    }
    else
    {
        Text = new flattext(fpin, Option.InputHasTags, Option.Iformat, Option.keepPunctuation, nice, Option.size, Option.treatSlashAsAlternativesSeparator);
    }
    if (nice)
        LOG1LINE("processing");
    Text->Lemmatise(fpout, Option.Sep, tally, Option.SortOutput, Option.UseLemmaFreqForDisambiguation, nice, Option.DictUnique, Option.RulesUnique, Option.baseformsAreLowercase, listLemmas, Option.Wformat != NULL                         // list lemmas with all word forms
                                                                                                                                                                                                         && ((listLemmas & 3) == 3)                 // both of -b and -B are specified
                                                                                                                                                                                                         && !strcmp(Option.Bformat, Option.bformat) // -b and -B options are the same format
                                                                                                                                                                                                                                                    // true: outputs must be merged
    );
    delete Text;
}

int Lemmatiser::LemmatiseFile()
{
    if (Option.XML && !Option.Iformat)
    {
        fprintf(stderr, "file block 1\n");
        // 20140224
        // Set default for input format
        if (Option.InputHasTags && !Option.POSAttribute)
            Option.Iformat = dupl("$w/$t\\s");
        else
            Option.Iformat = dupl("$w\\s");
    }
    if (Option.XML && (Option.keepPunctuation != 1))
    {
        fprintf(stderr, "Automatic tokenization (-p option) is not supported for XML input.\n");

        return -1;
    }
    else if ((Option.Iformat != 0) && (Option.keepPunctuation != 1))
    {
        fprintf(stderr, "You can specify -I or -p, not both.\n");

        return -1;
    }
    if (changed)
        setFormats();
    clock_t t0;
    t0 = clock();
    FILE *fpout;

    if (Option.argo)
    {
        fpout = fopen(Option.argo, "wb");

        if (!fpout)
        {
            cannotOpenFile("-o\t", Option.argo, "\t(Output text): Cannot open file.");
            return -1;
        }
        else
            info("-o\t%-20s\tOutput text", Option.argo);
    }
    else
    {
        DoInfo = false;
        info("-o\tOutput text: Using standard output.");

        fpout = stdout;
    }
    switch (Option.keepPunctuation)
    {
    case 0:
        info("-p-\tignore punctuation (only together with -t- and no -W format)\n");

        break;
    case 1:
        info("-p\tkeep punctuation (default)\n");

        break;
    case 2:
        info("-p+\ttreat punctuation as tokens (only together with -t- and no -W format)\n");

        break;
    default:
        info("-p:\tUnknown argument.\n");
    }

    tallyStruct tally;

    if (Option.argi)
    {
        FILE *fpin;

        fpin = fopen(Option.argi, "rb");
        if (!fpin)

        {
            cannotOpenFile("-i\t", Option.argi, "\t(Input text): Cannot open file.");
            return -1;
        }
        else
        {
            info("-i\t%-20s\tInput text", Option.argi);
        }
        LemmatiseText(fpin, fpout, &tally);
        showtime(t0);
        fclose(fpin);
    }
    else
    {
        info("-i\tInput text: Not specified.");
        LOG1LINE(
            "No input file specified. (Option -i). If you want to use standard input,\n"
            "recompile with #define STREAM 1 in defines.h\n"
            "But notice: CSTlemma works best with complete texts, due to heuristics for\n"
            "disambiguation. When reading from standard input, each line of text is\n"
            "lemmatized independently of the rest of the text, reducing the possibility\n"
            "to guess the right lemma in case of ambiguity.");
    }

    if (fpout != stdout)
        fclose(fpout);

    info("\nall words      %10.lu\n"
         "unknown words  %10.lu (%lu%%)\n"
         "conflicting    %10.lu (%lu%%)\n",
         tally.totcnt, tally.newcnt, tally.totcnt ? (tally.newcnt * 200 + 1) / (2 * tally.totcnt) : 100, tally.newhom, tally.totcnt ? (tally.newhom * 200 + 1) / (2 * tally.totcnt) : 100);

    if (SortInput)
        info("\nall types      %10.lu\n"
             "unknown types  %10.lu (%lu%%)\n"
             "conflicting    %10.lu (%lu%%)",
             tally.totcntTypes, tally.newcntTypes, tally.totcntTypes ? (tally.newcntTypes * 200 + 1) / (2 * tally.totcntTypes) : 100UL, tally.newhomTypes, tally.totcntTypes ? (tally.newhomTypes * 200 + 1) / (2 * tally.totcntTypes) : 100UL);

    return 0;
}

string Lemmatiser::LemmatiseString(string str)
{
    string result;

    tallyStruct tally;

    text *Text;

    Text = new flattext(str, 1, nice, ULONG_MAX, false);
    
    if (nice)
        LOG1LINE("processing");
    
    result = Text->Lemmatise("|", &tally, 0, 2, nice, false, false, caseTp::easis, listLemmas, false);
    delete Text;

    return result;
}

void Lemmatiser::LemmatiseEnd()
{
    delete TextToDictTags;
    delete TagFriends;
}

#endif
