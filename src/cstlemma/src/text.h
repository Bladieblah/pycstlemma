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
#ifndef TEXT_H
#define TEXT_H

#include "defines.h"
#if defined PROGLEMMATISE

#include <stdio.h>
#include <string>

class Word;
class taggedWord;
class basefrm;
struct optionStruct;
enum class caseTp;

struct tallyStruct
{
    unsigned long int totcnt;
    unsigned long int totcntTypes;
    unsigned long int newcnt;
    unsigned long int newcntTypes;
    unsigned long int newhom;
    unsigned long int newhomTypes;
    tallyStruct()
    {
        totcnt = 0;
        totcntTypes = 0;
        newcnt = 0;
        newcntTypes = 0;
        newhom = 0;
        newhomTypes = 0;
    }
};

class field;

class text
{
private:
    basefrm **basefrmarrD;
    basefrm **basefrmarrL;

public:
    basefrm **ppD;
    basefrm **ppL;
    int cntD;
    int cntL;
    int newcnt;
    int newcntTypes;
    int aConflict;
    int aConflictTypes;

protected:
    Word **Root;
    const Word **tunsorted;
    unsigned long int *Lines;
    field *fields;
    size_t N;
    unsigned long int lineno;
    unsigned long int total;
    unsigned long int reducedtotal;
    bool InputHasTags;
    bool StartOfLine;

private:
    virtual const char *convert(const char *s, char *buf, const char *lastBufByte) = 0;

protected:
    bool atStartOfLine() const { return StartOfLine; }
    void insert(const char *w);
    void insert(const char *w, const char *tag);
    void AddField(field *fld);
    field *translateFormat(char *Iformat, field *&wordfield, field *&tagfield);

public:
    void incTotal()
    {
        ++total;
    }
    void incTotal(unsigned long int inc)
    {
        total += inc;
    }
    static bool setFormat(const char *format, const char *bformat, const char *Bformat, bool InputHasTags);
    void Lemmatise(FILE *fpo, const char *Sep, tallyStruct *tally, unsigned int SortOutput, int UseLemmaFreqForDisambiguation, bool nice, bool DictUnique, bool RulesUnique, enum caseTp baseformsAreLowercase, int listLemmas, bool mergeLemmas);
    std::string Lemmatise(const char *Sep, tallyStruct *tally, unsigned int SortOutput, int UseLemmaFreqForDisambiguation, bool nice, bool DictUnique, bool RulesUnique, enum caseTp baseformsAreLowercase, int listLemmas, bool mergeLemmas);

    text(bool InputHasTags, bool nice);
    virtual void DoYourWork(FILE *fpi, optionStruct &Option) = 0;
    virtual ~text();
    void createUnTaggedAlternatives(char *w);
    void createUnTagged(const char *w);
    void createTaggedAlternatives(char *w, const char *tag);
    void createTagged(const char *w, const char *tag);
    virtual void printUnsorted(FILE *fpo) = 0;
    virtual void writeUnsorted(std::string &str) = 0;
    void makeList();
};

extern char *globIformat;
extern int findSlashes(const char *buf);

#endif
#endif
