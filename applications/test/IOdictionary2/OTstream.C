/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "token.H"
#include "OTstream.H"
#include "SubList.H"

namespace Foam
{
    defineCompoundTypeName(List<char>, charList);
    //addCompoundToRunTimeSelectionTable(List<char>, charList);
}

// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

Foam::Ostream& Foam::OTstream::write(const token& t)
{
    append(t);

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const char c)
{
    append(token(word(c)));

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const char* str)
{
    append(token(string(str)));

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const word& str)
{
    append(token(str));

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const string& str)
{
    append(token(str));

    return *this;
}


Foam::Ostream& Foam::OTstream::writeQuoted
(
    const std::string& str,
    const bool quoted
)
{
    append(token(Foam::string(str)));
//     if (quoted)
//     {
//         write(char(token::STRING));
//     }
//     else
//     {
//         write(char(token::WORD));
//     }
//
//     size_t len = str.size();
//     writeToBuffer(len);
//     writeToBuffer(str.c_str(), len + 1, 1);

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const int32_t val)
{
    append(token(val));

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const int64_t val)
{
    append(token(label(val)));

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const floatScalar val)
{
    append(token(val));

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const doubleScalar val)
{
    append(token(val));

    return *this;
}


Foam::Ostream& Foam::OTstream::write(const char* data, std::streamsize count)
{
    token t;
    UList<char> buf(const_cast<char*>(data), count);

    t = new token::Compound<List<char>>(buf);
    append(t);

    return *this;
}


void Foam::OTstream::print(Ostream& os) const
{
    os  << "Tokens:" << Foam::endl;

    const DynamicList<token>& lst = static_cast<DynamicList<token>>(*this);

    forAll(*this, i)
    {
        os  << "    " << lst[i] << Foam::endl;
    }
}


// ************************************************************************* //
