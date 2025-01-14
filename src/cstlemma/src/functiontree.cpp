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
#include "functiontree.h"
#if defined PROGLEMMATISE
#include "functio.h"
#include "comparison.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <string>

using namespace std;

functionTree::functionTree() : m_fnc(NULL), next(NULL), child(NULL), m_comp(comparison::eany), m_nmbr(-1), Hidden(false)
{
}

functionTree::~functionTree()
{
    delete m_fnc;
    delete next;
    delete child;
}

void functionTree::printIt(const OutputClass *outputObj) const
{
    if (!Hidden)
    {
        if (!skip(outputObj))
        {
            if (m_fnc)
                m_fnc->doIt(outputObj);
            if (child)
            {
                child->printIt(outputObj);
            }
        }
    }
    if (next)
        next->printIt(outputObj);
}

void functionTree::writeIt(const OutputClass *outputObj, string &str) const
{
    if (!Hidden)
    {
        if (!skip(outputObj))
        {
            if (m_fnc)
            {
                m_fnc->toString(outputObj, str);
            }
            if (child)
            {
                child->writeIt(outputObj, str);
            }
        }
    }
    if (next)
        next->writeIt(outputObj, str);
}

int functionTree::count(const OutputClass *outputObj) const
{
    int ret = -1;
    if (m_fnc)
        ret = m_fnc->count(outputObj);
    if (ret == -1 && child)
        ret = child->count(outputObj);
    if (ret == -1 && next)
        ret = next->count(outputObj);
    return ret;
}

int functionTree::count2(const OutputClass *outputObj) const
{
    int ret = -1;
    if (m_comp != comparison::eless && m_comp != comparison::eequal && m_comp != comparison::emore && m_comp != comparison::enotequal)
    {
        if (m_fnc)
            ret = m_fnc->count(outputObj);
        if (ret == -1 && child)
        {
            ret = child->count2(outputObj);
        }
    }
    if (ret == -1 && next)
        ret = next->count2(outputObj);
    return ret;
}

bool functionTree::OK(const OutputClass *outputObj) const
{
    int cnt = -1;
    switch (m_comp)
    {
    case comparison::eless:
    case comparison::eequal:
    case comparison::enotequal:
    case comparison::emore:
        if (child)
        {
            assert(child);
            cnt = child->count(outputObj);
        }
        else // $b2
            cnt = count(outputObj);
        if (cnt < 0)
        {
            fprintf(stderr, "Something wrong in field specification.\n"
                            "The number-of-values specification %d is only valid if there is a field\n"
                            "with a variable number of values.\n",
                    m_nmbr);

            exit(0);
        }
    default:;
    }
    switch (m_comp)
    {
    case comparison::eless:
        return cnt < m_nmbr;
    case comparison::eequal:
        return cnt == m_nmbr;
    case comparison::enotequal:
        return cnt != m_nmbr;
    case comparison::emore:
        return cnt > m_nmbr;
    default:
        return true;
    }
}

bool functionTree::passTest(const OutputClass *outputObj) const
{
    if (OK(outputObj))
    {
        if (m_comp != comparison::etest && child && !child->passTest(outputObj))
            return false;
        if (next)
            return next->passTest(outputObj);
        return true;
    }
    else
        return false;
}

bool functionTree::skip(const OutputClass *outputObj) const
{
    if (m_comp == comparison::etest) // bare [...] (no comparisons)
    // [...] tests for two situations:
    // 1) whether nested [...] have conditions that are not met and
    // 2) whether anything countable that is not between [...]0 or [...]<1 returns zero
    // If either of these situations occur, the actions between [ and ] are skipped.
    // Notice that a [...] always succeeds, as does [...]* and ... (an expression without [ and ])
    {
        assert(!m_fnc);
        assert(child);
        return !child->passTest(outputObj) || child->count2(outputObj) == 0;
    }
    else
        return !OK(outputObj);
}
#endif
