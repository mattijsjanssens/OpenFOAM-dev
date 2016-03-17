/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2015 OpenFOAM Foundation
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

#include "IOdictionary2.H"
#include "objectRegistry.H"
#include "Pstream.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
defineTypeNameAndDebug(IOdictionary2, 0);

bool IOdictionary2::writeDictionaries
(
    debug::infoSwitch("writeDictionaries", 0)
);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::IOdictionary2::IOdictionary2(const IOobject& io)
:
    regIOobject(io),
    fileEventNo_(labelMin)
{
    // Temporary warning
    if (debug && io.readOpt() == IOobject::MUST_READ)
    {
        WarningInFunction
            << "Dictionary " << name()
            << " constructed with IOobject::MUST_READ"
            " instead of IOobject::MUST_READ_IF_MODIFIED." << nl
            << "Use MUST_READ_IF_MODIFIED if you need automatic rereading."
            << endl;
    }

    // Everyone check or just master
    bool masterOnly =
        regIOobject::fileModificationChecking == timeStampMaster
     || regIOobject::fileModificationChecking == inotifyMaster;


    // Check if header is ok for READ_IF_PRESENT
    bool isHeaderOk = false;
    if (io.readOpt() == IOobject::READ_IF_PRESENT)
    {
        if (masterOnly)
        {
            if (Pstream::master())
            {
                isHeaderOk = headerOk();
            }
            Pstream::scatter(isHeaderOk);
        }
        else
        {
            isHeaderOk = headerOk();
        }
    }


    if
    (
        (
            io.readOpt() == IOobject::MUST_READ
         || io.readOpt() == IOobject::MUST_READ_IF_MODIFIED
        )
     || isHeaderOk
    )
    {
        readFile(masterOnly);
        setUpToDate();
        fileEventNo_ = eventNo();
    }

    dictionary::name() = IOobject::objectPath();
}


Foam::IOdictionary2::IOdictionary2(const IOobject& io, const bool allowIndirect)
:
    regIOobject(io),
    fileEventNo_(labelMin)
{
    Pout<< "IOdictionary2::IOdictionary2 :" << objectPath() << endl;
    read2();
}



Foam::IOdictionary2::IOdictionary2(const IOobject& io, const dictionary& dict)
:
    regIOobject(io),
    fileEventNo_(labelMin)
{
    // Temporary warning
    if (debug && io.readOpt() == IOobject::MUST_READ)
    {
        WarningInFunction
            << "Dictionary " << name()
            << " constructed with IOobject::MUST_READ"
            " instead of IOobject::MUST_READ_IF_MODIFIED." << nl
            << "Use MUST_READ_IF_MODIFIED if you need automatic rereading."
            << endl;
    }

    // Everyone check or just master
    bool masterOnly =
        regIOobject::fileModificationChecking == timeStampMaster
     || regIOobject::fileModificationChecking == inotifyMaster;


    // Check if header is ok for READ_IF_PRESENT
    bool isHeaderOk = false;
    if (io.readOpt() == IOobject::READ_IF_PRESENT)
    {
        if (masterOnly)
        {
            if (Pstream::master())
            {
                isHeaderOk = headerOk();
            }
            Pstream::scatter(isHeaderOk);
        }
        else
        {
            isHeaderOk = headerOk();
        }
    }


    if
    (
        (
            io.readOpt() == IOobject::MUST_READ
         || io.readOpt() == IOobject::MUST_READ_IF_MODIFIED
        )
     || isHeaderOk
    )
    {
        readFile(masterOnly);
    }
    else
    {
        dictionary::operator=(dict);
    }

    setUpToDate();
    fileEventNo_ = eventNo();
    dictionary::name() = IOobject::objectPath();
}


Foam::IOdictionary2::IOdictionary2(const IOobject& io, Istream& is)
:
    regIOobject(io),
    fileEventNo_(labelMin)
{
    dictionary::name() = IOobject::objectPath();
    // Note that we do construct the dictionary null and read in afterwards
    // so that if there is some fancy massaging due to a functionEntry in
    // the dictionary at least the type information is already complete.
    is  >> *this;
    setUpToDate();
    fileEventNo_ = eventNo();
}


// * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * * //

Foam::IOdictionary2::~IOdictionary2()
{}


// * * * * * * * * * * * * * * * Members Functions * * * * * * * * * * * * * //

const Foam::word& Foam::IOdictionary2::name() const
{
    return regIOobject::name();
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void Foam::IOdictionary2::operator=(const IOdictionary2& rhs)
{
    dictionary::operator=(rhs);
}


// ************************************************************************* //
