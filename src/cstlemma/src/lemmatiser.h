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
#include "defines.h"
#if defined PROGLEMMATISE
#include "dictionary.h"
#endif

#include <string>

#if defined PROGLEMMATISE
class tagpairs;
#endif

struct optionStruct;
struct tallyStruct;

class Lemmatiser
{
private:
        Lemmatiser &operator=(const Lemmatiser &f)
        {
                REFER(f) // unused
                return *this;
        }
#ifdef COUNTOBJECTS
public:
        static int COUNT;
#endif
private:
        static int instance;
#if (defined PROGLEMMATISE)
        dictionary dict;
        static tagpairs *TextToDictTags;
#endif
        int listLemmas;
        int status;
        bool SortInput; // derived from other options
        bool changed;

public:
        optionStruct &Option;
        bool nice;
        int getStatus()
        {
                return status;
        }
        Lemmatiser(optionStruct &Option);
        ~Lemmatiser();
#if defined PROGLEMMATISE
        static const char *translate(const char *tag);
        int setFormats();
        int openFiles();
        void showSwitches();
        void LemmatiseText(FILE *fpin, FILE *fpout, tallyStruct *tally);

        int LemmatiseFile();
        int LemmatiseInit();
        std::string LemmatiseString(std::string str);
        void LemmatiseEnd();
#endif
#if defined PROGMAKEDICT
        int MakeDict();
#endif
#if defined PROGMAKESUFFIXFLEX
        int MakeFlexPatterns();
#endif
};
