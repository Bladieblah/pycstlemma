/*
CSTLEMMA - trainable lemmatiser

Copyright (C) 2002, 2014, 2009  Center for Sprogteknologi, University of Copenhagen

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

/*

XML
option X has been extended
word, POS, lemma and lemma class can all be found in attribute values.
Words can also be found in elements.

*/

#include "XMLtext.h"
#if defined PROGLEMMATISE

#include "wordReader.h"
#include "word.h"
#include "field.h"
#include "parsesgml.h"
#include "basefrm.h"
#include "option.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

#include <string>

using namespace std;

static void printXML(
    FILE *fpo

    ,
    const char *s)
{
    if (!s)
        return;
    REFER(fpo) // unused
    while (*s)
    {
        switch (*s)
        {
        case '<':
            fprintf(basefrm::m_fp, "&lt;");
            break;
        case '>':
            fprintf(basefrm::m_fp, "&gt;");
            break;
        case '&':
            /*if(strchr(s+1,';'))
                fputc('&',basefrm::m_fp);
            else*/
            fprintf(basefrm::m_fp, "&amp;");
            break;
        case '\'':
            fprintf(basefrm::m_fp, "&apos;");
            break;
        case '"':
            fprintf(basefrm::m_fp, "&quot;");
            break;
        default:
            fputc(*s, basefrm::m_fp);
        }
        ++s;
    }
}

static const char *
copy(FILE *fp, const char *o, const char *end)
{
    for (; o < end; ++o)
        fputc(*o, fp);
    return o;
}

class crumb
{
private:
    char *e;
    size_t L;
    crumb *next;

public:
    crumb(const char *s, size_t len, crumb *Crumbs) : L(len), next(Crumbs)
    {
        e = new char[len + 1];
        strncpy(e, s, len);
        e[len] = '\0';
    }
    ~crumb()
    {
        delete[] e;
    }
    const char *itsElement()
    {
        return e;
    }
    crumb *Pop(const char *until, size_t len)
    {
        crumb *nxt = next;
        if (L == len && !strncmp(e, until, len))
        {
            delete this;
            return nxt;
        }
        else
        {
            delete this;
            return nxt ? nxt->Pop(until, len) : NULL;
        }
    }
    void print()
    {
        if (next)
            next->print();
        printf("->%s", e);
    }
    bool is(const char *elm)
    {
        return !strcmp(e, elm);
    }
};

bool XMLtext::analyseThis()
{
    if (element)
    {
        return Crumbs && Crumbs->is(element);
    }
    else
    {
        if (ancestor)
        {
            return Crumbs != NULL;
        }
        else
        {
            return true;
        }
    }
}

bool XMLtext::segmentBreak()
{
    if (segment)
    {
        if (Crumbs && Crumbs->is(segment))
        {
            StartOfLine = true;
            return true;
        }
    }
    return false;
}

void XMLtext::CallBackStartElementName()
{
    startElement = ch;
}

void XMLtext::CallBackEndElementName()
{
    endElement = ch;
    if (ClosingTag)
    {
        if (Crumbs)
            Crumbs = Crumbs->Pop(startElement, size_t(endElement - startElement));
        ClosingTag = false;
    }
    else
    {
        if (Crumbs || !this->ancestor || (startElement + strlen(this->ancestor) == endElement && !strncmp(startElement, this->ancestor, size_t(endElement - startElement))))
        {
            Crumbs = new crumb(startElement, size_t(endElement - startElement), Crumbs);
            segmentBreak();
        }
    }
}

void XMLtext::CallBackStartAttributeName()
{
    if (analyseThis())
    {
        ClosingTag = false;
        startAttributeName = ch;
    }
}

void XMLtext::CallBackEndAttributeNameCounting()
{
    if (analyseThis())
    {
        endAttributeName = ch;
        if (this->wordAttributeLen == size_t(endAttributeName - startAttributeName) && !strncmp(startAttributeName, this->wordAttribute, size_t(wordAttributeLen)))
        {
            ++total;
        }
    }
}

void XMLtext::CallBackEndAttributeNameInserting()
{
    if (analyseThis())
    {
        endAttributeName = ch;

        WordPosComing = false;
        POSPosComing = false;
        LemmaPosComing = false;
        LemmaClassPosComing = false;

        if (this->wordAttributeLen == size_t(endAttributeName - startAttributeName) && !strncmp(startAttributeName, this->wordAttribute, wordAttributeLen))
        {
            WordPosComing = true;
        }
        else if (this->POSAttributeLen == size_t(endAttributeName - startAttributeName) && !strncmp(startAttributeName, this->POSAttribute, size_t(POSAttributeLen)))
        {
            POSPosComing = true;
        }
        else if (this->lemmaAttributeLen == size_t(endAttributeName - startAttributeName) && !strncmp(startAttributeName, this->lemmaAttribute, size_t(lemmaAttributeLen)))
        {
            LemmaPosComing = true;
        }
        else if (this->lemmaClassAttributeLen == size_t(endAttributeName - startAttributeName) && !strncmp(startAttributeName, this->lemmaClassAttribute, size_t(lemmaClassAttributeLen)))
        {
            LemmaClassPosComing = true;
        }
    }
}

void XMLtext::CallBackStartValue()
{
    if (analyseThis())
    {
        startValue = ch;
    }
}

void XMLtext::CallBackEndValue()
{
    if (analyseThis())
    {
        endValue = ch;
        if (WordPosComing)
        {
            this->Token[total].tokenWord.set(startValue, endValue);
        }
        else if (POSPosComing)
        {
            this->Token[total].tokenPOS.set(startValue, endValue);
        }
        else if (LemmaPosComing)
        {
            this->Token[total].tokenLemma.set(startValue, endValue);
        }
        else if (LemmaClassPosComing)
        {
            this->Token[total].tokenLemmaClass.set(startValue, endValue);
        }
    }
}

void XMLtext::CallBackNoMoreAttributes()
{
    if (analyseThis())
    {
        token *Tok = Token + total;
        if (Tok->tokenWord.getStart() != NULL && Tok->tokenWord.getEnd() != Tok->tokenWord.getStart())
        { // Word as attribute
            char *EW = Tok->tokenWord.getEnd();
            char savW = *EW;
            *EW = '\0';
            if (Tok->tokenPOS.getStart() != NULL)
            {
                char *EP = Tok->tokenPOS.getEnd();
                char savP = *EP;
                *EP = '\0';
                insert(Tok->tokenWord.getStart(), Tok->tokenPOS.getStart());
                *EP = savP;
            }
            else
            {
                insert(Tok->tokenWord.getStart());
            }
            *EW = savW;
        }
    }
}

void XMLtext::CallBackEndTag()
{
    ClosingTag = true;
}

void XMLtext::CallBackEmptyTag()
{
    if (Crumbs)
        Crumbs = Crumbs->Pop(startElement, size_t(endElement - startElement));
}

const char *XMLtext::convert(const char *s, char *buf, const char *lastBufByte)
{
    return wordReader::convert(s, buf, lastBufByte);
}

void XMLtext::printUnsorted(FILE *fpo)
{
    if (this->alltext)
    {
        unsigned int k;
        const char *o = this->alltext;
        for (k = 0; k < total; ++k)
        {
            if (tunsorted[k])
            {
                const char *start = Token[k].tokenWord.getStart();
                const char *startToken = Token[k].tokenToken.getStart();
                const char *endToken = Token[k].tokenToken.getEnd();
                const char *lemmastart = Token[k].tokenLemma.getStart();
                const char *lemmaend = Token[k].tokenLemma.getEnd();
                const char *lemmaclassstart = Token[k].tokenLemmaClass.getStart();
                const char *lemmaclassend = Token[k].tokenLemmaClass.getEnd();
                if (lemmastart == NULL)
                { // replace word with lemma or write lemma together with word
                    if (lemmaclassstart)
                    {
                        copy(fpo, o, lemmaclassstart);
                        tunsorted[k]->printLemmaClass();
                        o = lemmaclassend;
                    }
                    if (start)
                    {
                        copy(fpo, o, startToken);
                        o = endToken;
                    }
                    tunsorted[k]->print();
                }
                else
                {
                    if (lemmaclassstart == NULL)
                    {
                        copy(fpo, o, lemmastart);
                        o = lemmaend;
                        tunsorted[k]->print();
                    }
                    else if (lemmaclassstart < lemmastart)
                    {
                        copy(fpo, o, lemmaclassstart);
                        tunsorted[k]->printLemmaClass();
                        copy(fpo, lemmaclassend, lemmastart);
                        tunsorted[k]->print();
                        o = lemmaend;
                    }
                    else
                    {
                        copy(fpo, o, lemmastart);
                        tunsorted[k]->print();
                        copy(fpo, lemmaend, lemmaclassstart);
                        tunsorted[k]->printLemmaClass();
                        o = lemmaclassend;
                    }
                }
            }
        }
        o = copy(fpo, o, o + strlen(o));
    }
}

void XMLtext::writeUnsorted(string &str) {
    return;
}

token *XMLtext::getCurrentToken()
{
    return Token ? Token + total : NULL;
}

token *XMLtext::getJustMadeToken()
{
    return Token ? total > 0 ? Token + total - 1 : NULL : NULL;
}

void CallBackStartElementName(void *arg)
{
    ((XMLtext *)arg)->CallBackStartElementName();
}

void CallBackEndElementName(void *arg)
{
    ((XMLtext *)arg)->CallBackEndElementName();
}

void CallBackStartAttributeName(void *arg)
{
    ((XMLtext *)arg)->CallBackStartAttributeName();
}

static void (XMLtext::*CallBackEndAttributeNames)();

void CallBackEndAttributeName(void *arg)
{
    (((XMLtext *)arg)->*CallBackEndAttributeNames)();
}

void CallBackStartValue(void *arg)
{
    ((XMLtext *)arg)->CallBackStartValue();
}

void CallBackEndValue(void *arg)
{
    ((XMLtext *)arg)->CallBackEndValue();
}

void CallBackNoMoreAttributes(void *arg)
{
    ((XMLtext *)arg)->CallBackNoMoreAttributes();
}

void CallBackEndTag(void *arg)
{
    ((XMLtext *)arg)->CallBackEndTag();
}

void CallBackEmptyTag(void *arg)
{
    ((XMLtext *)arg)->CallBackEmptyTag();
}

void XMLtext::reachedTokenEnd(char *Ch)
{
    token *Tok = getJustMadeToken();
    if (Tok)
    {
        char *start = Tok->tokenToken.getStart();
        char *end = Tok->tokenToken.getEnd();
        if (start == end)
            Tok->tokenToken.set(start, Ch);
    }
}

XMLtext::XMLtext(FILE *fpi, optionStruct &Option)

    : text(Option.InputHasTags, Option.nice), Token(NULL), ancestor(Option.ancestor), element(Option.element), segment(Option.segment), POSAttribute(Option.POSAttribute), lemmaAttribute(Option.lemmaAttribute), lemmaClassAttribute(Option.lemmaClassAttribute), Crumbs(NULL), ClosingTag(false), WordPosComing(false), POSPosComing(false), LemmaPosComing(false), LemmaClassPosComing(false), alltext(NULL), wordAttribute(Option.wordAttribute)
{
}

void XMLtext::DoYourWork(FILE *fpi, optionStruct &Option)

{
    StartOfLine = true;

    wordAttributeLen = wordAttribute ? strlen(wordAttribute) : 0;
    POSAttributeLen = POSAttribute ? strlen(POSAttribute) : 0;
    lemmaAttributeLen = lemmaAttribute ? strlen(lemmaAttribute) : 0;
    lemmaClassAttributeLen = lemmaClassAttribute ? strlen(lemmaClassAttribute) : 0;
#ifdef COUNTOBJECTS
    ++COUNT;
#endif
    reducedtotal = 0;
    Root = 0;

    field *wordfield = 0;
    field *tagfield = 0;
    field *format = 0;
    if (Option.nice)
        LOG1LINE("counting words");
    lineno = 0;

    if (Option.XML && Option.Iformat)
    {
        fseek(fpi, 0, SEEK_END);
        int kar;
        long filesize = ftell(fpi);
        rewind(fpi);
        if (filesize > 0)
        {
            alltext = new char[(unsigned int)filesize + 1];
            char *p = alltext;
            while ((kar = getc(fpi)) != EOF)
                *p++ = (char)kar;
            *p = '\0';
            rewind(fpi);
        }
    }
    if (alltext)
    {
        parseAsXml();
        html_tag_class html_tag(this);
        CallBackEndAttributeNames = &XMLtext::CallBackEndAttributeNameCounting;
        if (element || ancestor)
        {
            html_tag.setCallBackStartElementName(::CallBackStartElementName);
            html_tag.setCallBackEndElementName(::CallBackEndElementName);
            html_tag.setCallBackEndTag(::CallBackEndTag);
            html_tag.setCallBackEmptyTag(::CallBackEmptyTag);
        }
        else
        {
            html_tag.setCallBackStartElementName(::dummyCallBack);
            html_tag.setCallBackEndElementName(::dummyCallBack);
            html_tag.setCallBackEndTag(::dummyCallBack);
            html_tag.setCallBackEmptyTag(::dummyCallBack);
        }
        if (wordAttribute || lemmaAttribute || POSAttribute || lemmaClassAttribute)
        {
            html_tag.setCallBackStartAttributeName(::CallBackStartAttributeName);
            html_tag.setCallBackEndAttributeName(::CallBackEndAttributeName);
            if (!Option.SortOutput)
                print = printXML;
        }
        else
        {
            html_tag.setCallBackStartAttributeName(::dummyCallBack);
            html_tag.setCallBackEndAttributeName(::dummyCallBack);
        }
        html_tag.setCallBackStartValue(::dummyCallBack);
        html_tag.setCallBackEndValue(::dummyCallBack);
        html_tag.setCallBackNoMoreAttributes(::dummyCallBack);
        ch = alltext;
        char *curr_pos = alltext;
        char *endpos = alltext;
        estate Seq = notag;
        bool (wordReader::*fnc)(int kar);
        int loop;
        fnc = &wordReader::countToken;
        if (Option.Iformat)
        {
            wordfield = 0;
            tagfield = 0;
            format = translateFormat(Option.Iformat, wordfield, tagfield);
            if (!wordfield)
            {
                fprintf(stderr, "Input format %s must specify '$w'.\n", Option.Iformat);

                exit(0);
            }
        }
        for (loop = 1; loop <= 2; ++loop)
        {
            wordReader WordReader(format, wordfield, tagfield, Option.treatSlashAsAlternativesSeparator, this);
            while (*ch)
            {
                while (*ch && ((Seq = (html_tag.*tagState)((unsigned char)*ch)) == tag || Seq == endoftag_startoftag))
                {
                    // TAG
                    ch++;
                }
                if (Seq == notag)
                { // Not an HTML tag. Backtrack.
                    endpos = ch;
                    ch = curr_pos;
                    // TEXT
                    while (ch < endpos)
                    {
                        WordReader.rawput(fnc, *ch);
                        // TEXT
                        ch++;
                    }
                }
                else if (Seq == endoftag)
                {
                    ++ch; // skip >
                }
                if (*ch)
                {
                    // TEXT START
                    while (*ch && isspace((unsigned char)*ch))
                    {
                        ++ch;
                    }
                    WordReader.initWord();
                    while (*ch && (Seq = (html_tag.*tagState)((unsigned char)*ch)) == notag)
                    {
                        if ((WordReader.*(WordReader.xput))(fnc, *ch))
                        {
                            // complete token seen. ch points at next token
                            reachedTokenEnd(ch);
                        }
                        // TEXT
                        ++ch;
                    }
                    WordReader.rawput(fnc, '\0');
                    // complete token seen. ch points at some tag or EOF
                    reachedTokenEnd(ch);
                    if (Seq == tag)
                    {
                        WordReader.rawput(fnc, '\0');
                        curr_pos = ch++; // skip <
                        // TEXT END
                    }
                }
            }
            if (Seq == tag)
            { // Not an SGML tag. Backtrack.
                endpos = ch;
                ch = curr_pos;
                // TEXT
                while (ch < endpos)
                {
                    WordReader.rawput(fnc, *ch);
                    // TEXT
                    ch++;
                }
            }
            if (loop == 1)
            {
                lineno = WordReader.getLineno();
                if (Option.nice)
                {
                    printf("... %lu words counted in %lu lines\n", total, lineno);

                    LOG1LINE("allocating array of pointers to words");
                }
                tunsorted = new const Word *[total];
                Token = new token[total + 1]; // 1 extra for the epilogue after the last token
                if (Option.nice)
                    LOG1LINE("allocating array of line offsets");
                Lines = new unsigned long int[lineno + 1];
                unsigned long int L = lineno + 1;
                do
                {
                    --L;
                    Lines[L] = 0;
                } while (L != 0);
                if (Option.nice)
                    LOG1LINE("...allocated array");

                total = 0;
                if (Option.nice)
                    LOG1LINE("reading words");
                lineno = 0;
                ch = alltext;
                curr_pos = alltext; // 20091106
                fnc = &wordReader::readToken;
                CallBackEndAttributeNames = &XMLtext::CallBackEndAttributeNameInserting;
                if (wordAttribute || POSAttribute || lemmaAttribute || lemmaClassAttribute)
                {
                    html_tag.setCallBackStartAttributeName(::CallBackStartAttributeName);
                    html_tag.setCallBackEndAttributeName(::CallBackEndAttributeName);
                    html_tag.setCallBackStartValue(::CallBackStartValue);
                    html_tag.setCallBackEndValue(::CallBackEndValue);
                    html_tag.setCallBackNoMoreAttributes(::CallBackNoMoreAttributes);
                }
            }
        }
    }
    makeList();
    if (Option.nice)
        LOG1LINE("...read words from XML file");
}

void XMLtext::wordDone()
{
    if (segment)
        StartOfLine = false;
}

#endif
