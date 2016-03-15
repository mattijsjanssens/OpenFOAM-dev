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

#include "objectRegistry2.H"
#include "Time.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
defineTypeNameAndDebug(objectRegistry2, 0);
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::objectRegistry2::parentNotTime() const
{
    return (&parent_ != dynamic_cast<const objectRegistry2*>(&time_));
}


// * * * * * * * * * * * * * * * * Constructors *  * * * * * * * * * * * * * //

Foam::objectRegistry2::objectRegistry2
(
    const Time& t,
    const label nIoObjects
)
:
    regIOobject2
    (
        IOobject
        (
            string::validate<word>(t.caseName()),
            "",
            t,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE,
            false
        ),
        true    // to flag that this is the top-level regIOobject2
    ),
    HashTable<regIOobject2*>(nIoObjects),
    time_(t),
    parent_(t),
    dbDir_(name()),
    event_(1)
{}


Foam::objectRegistry2::objectRegistry2
(
    const IOobject& io,
    const label nIoObjects
)
:
    regIOobject2(io),
    HashTable<regIOobject2*>(nIoObjects),
    time_(io.time()),
    parent_(io.db()),
    dbDir_(parent_.dbDir()/local()/name()),
    event_(1)
{
    writeOpt() = IOobject::AUTO_WRITE;
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::objectRegistry2::~objectRegistry2()
{
    List<regIOobject2*> myObjects(size());
    label nMyObjects = 0;

    for (iterator iter = begin(); iter != end(); ++iter)
    {
        if (iter()->ownedByRegistry())
        {
            myObjects[nMyObjects++] = iter();
        }
    }

    for (label i=0; i < nMyObjects; i++)
    {
        checkOut(*myObjects[i]);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::wordList Foam::objectRegistry2::names() const
{
    return HashTable<regIOobject2*>::toc();
}


Foam::wordList Foam::objectRegistry2::sortedNames() const
{
    return HashTable<regIOobject2*>::sortedToc();
}


Foam::wordList Foam::objectRegistry2::names(const word& ClassName) const
{
    wordList objectNames(size());

    label count=0;
    for (const_iterator iter = cbegin(); iter != cend(); ++iter)
    {
        if (iter()->type() == ClassName)
        {
            objectNames[count++] = iter.key();
        }
    }

    objectNames.setSize(count);

    return objectNames;
}


Foam::wordList Foam::objectRegistry2::sortedNames(const word& ClassName) const
{
    wordList sortedLst = names(ClassName);
    sort(sortedLst);

    return sortedLst;
}


const Foam::objectRegistry2& Foam::objectRegistry2::subRegistry
(
    const word& name,
    const bool forceCreate
) const
{
    if (forceCreate && !foundObject<objectRegistry2>(name))
    {
        objectRegistry2* fieldsCachePtr = new objectRegistry2
        (
            IOobject
            (
                name,
                time().constant(),
                *this,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            )
        );
        fieldsCachePtr->store();
    }
    return lookupObject<objectRegistry2>(name);
}


Foam::label Foam::objectRegistry2::getEvent() const
{
    label curEvent = event_++;

    if (event_ == labelMax)
    {
        if (objectRegistry2::debug)
        {
            WarningInFunction
                << "Event counter has overflowed. "
                << "Resetting counter on all dependent objects." << nl
                << "This might cause extra evaluations." << endl;
        }

        // Reset event counter
        curEvent = 1;
        event_ = 2;

        for (const_iterator iter = begin(); iter != end(); ++iter)
        {
            const regIOobject2& io = *iter();

            if (objectRegistry2::debug)
            {
                Pout<< "objectRegistry2::getEvent() : "
                    << "resetting count on " << iter.key() << endl;
            }

            if (io.eventNo() != 0)
            {
                const_cast<regIOobject2&>(io).eventNo() = curEvent;
            }
        }
    }

    return curEvent;
}


bool Foam::objectRegistry2::checkIn(regIOobject2& io) const
{
    if (objectRegistry2::debug)
    {
        Pout<< "objectRegistry2::checkIn(regIOobject2&) : "
            << name() << " : checking in " << io.name()
            << endl;
    }

    return const_cast<objectRegistry2&>(*this).insert(io.name(), &io);
}


bool Foam::objectRegistry2::checkOut(regIOobject2& io) const
{
    iterator iter = const_cast<objectRegistry2&>(*this).find(io.name());

    if (iter != end())
    {
        if (objectRegistry2::debug)
        {
            Pout<< "objectRegistry2::checkOut(regIOobject2&) : "
                << name() << " : checking out " << iter.key()
                << endl;
        }

        if (iter() != &io)
        {
            if (objectRegistry2::debug)
            {
                WarningInFunction
                    << name() << " : attempt to checkOut copy of "
                    << iter.key()
                    << endl;
            }

            return false;
        }
        else
        {
            regIOobject2* object = iter();

            bool hasErased = const_cast<objectRegistry2&>(*this).erase(iter);

            if (io.ownedByRegistry())
            {
                delete object;
            }

            return hasErased;
        }
    }
    else
    {
        if (objectRegistry2::debug)
        {
            Pout<< "objectRegistry2::checkOut(regIOobject2&) : "
                << name() << " : could not find " << io.name()
                << " in registry " << name()
                << endl;
        }
    }

    return false;
}


void Foam::objectRegistry2::rename(const word& newName)
{
    regIOobject2::rename(newName);

    // adjust dbDir_ as well
    string::size_type i = dbDir_.rfind('/');

    if (i == string::npos)
    {
        dbDir_ = newName;
    }
    else
    {
        dbDir_.replace(i+1, string::npos, newName);
    }
}


bool Foam::objectRegistry2::modified() const
{
    forAllConstIter(HashTable<regIOobject2*>, *this, iter)
    {
        if (iter()->modified())
        {
            return true;
        }
    }

    return false;
}


void Foam::objectRegistry2::readModifiedObjects()
{
    for (iterator iter = begin(); iter != end(); ++iter)
    {
        if (objectRegistry2::debug)
        {
            Pout<< "objectRegistry2::readModifiedObjects() : "
                << name() << " : Considering reading object "
                << iter.key() << endl;
        }

        iter()->readIfModified();
    }
}


bool Foam::objectRegistry2::readIfModified()
{
    readModifiedObjects();
    return true;
}


bool Foam::objectRegistry2::writeObject
(
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp
) const
{
    bool ok = true;

    forAllConstIter(HashTable<regIOobject2*>, *this, iter)
    {
        if (objectRegistry2::debug)
        {
            Pout<< "objectRegistry2::write() : "
                << name() << " : Considering writing object "
                << iter.key()
                << " with writeOpt " << iter()->writeOpt()
                << " to file " << iter()->objectPath()
                << endl;
        }

        if (iter()->writeOpt() != NO_WRITE)
        {
            ok = iter()->writeObject(fmt, ver, cmp) && ok;
        }
    }

    return ok;
}


// ************************************************************************* //
