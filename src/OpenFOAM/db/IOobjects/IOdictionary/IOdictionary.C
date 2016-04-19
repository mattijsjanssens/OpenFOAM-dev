/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2016 OpenFOAM Foundation
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

#include "IOdictionary.H"
#include "objectRegistry.H"
#include "Pstream.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
defineTypeNameAndDebug(IOdictionary, 0);

bool IOdictionary::writeDictionaries
(
    debug::infoSwitch("writeDictionaries", 0)
);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::IOdictionary::IOdictionary(const IOobject& io)
:
    regIOobject(io)
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
                isHeaderOk = objectHeaderOk(*this); //headerOk();
            }
            Pstream::scatter(isHeaderOk);
        }
        else
        {
            isHeaderOk = objectHeaderOk(*this); //headerOk();
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
        readObjectOrFile(masterOnly);
    }

    dictionary::name() = IOobject::objectPath();
}


Foam::IOdictionary::IOdictionary(const IOobject& io, const dictionary& dict)
:
    regIOobject(io)
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
                isHeaderOk = objectHeaderOk(*this); //headerOk();
            }
            Pstream::scatter(isHeaderOk);
        }
        else
        {
            isHeaderOk = objectHeaderOk(*this); //headerOk();
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
        readObjectOrFile(masterOnly);
    }
    else
    {
        dictionary::operator=(dict);
    }

    dictionary::name() = IOobject::objectPath();
}


Foam::IOdictionary::IOdictionary(const IOobject& io, Istream& is)
:
    regIOobject(io)
{
    dictionary::name() = IOobject::objectPath();
    // Note that we do construct the dictionary null and read in afterwards
    // so that if there is some fancy massaging due to a functionEntry in
    // the dictionary at least the type information is already complete.
    is  >> *this;
}


// * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * * //

Foam::IOdictionary::~IOdictionary()
{}


// * * * * * * * * * * * * * * * Members Functions * * * * * * * * * * * * * //

const Foam::word& Foam::IOdictionary::name() const
{
    return regIOobject::name();
}


bool Foam::IOdictionary::objectHeaderOk(const IOobject& io)
{
    IOobject parentIO(io, io.db().parent());

    if (!io.db().isTime())
    {
        // 1. Check if there is a registered parent and if so check that

        const IOdictionary* parentPtr =
            parentIO.db().lookupObjectPtr<IOdictionary>
            (
                parentIO.name(),
                true
            );

        if (parentPtr)
        {
            const IOdictionary& parent = *parentPtr;

            word key
            (
                scopedName(io.db().dbDir(), parent.db().dbDir())
            );

            const entry* ePtr = parent.lookupScopedEntryPtr
            (
                key,
                false,
                true            // allow pattern match
            );
            if (ePtr && ePtr->isDict())
            {
                const dictionary& dict = ePtr->dict();
                // Check that has FoamFile entry. Move into IOobjectReadHeader.
                // Cannot yet since we don't have an IOdictionary
                // (headerOk not yet member function)
                if (dict.found("FoamFile"))
                {
                    // Make sure FoamFile contents is correct
                    const dictionary& headerDict = dict.subDict("FoamFile");
                    const word className(headerDict.lookup("class"));
                    const word headerObject(headerDict.lookup("object"));
                }

                return true;
            }
            else
            {
                return false;
            }
        }
    }


    // 2. Local check
    if (const_cast<IOobject&>(io).headerOk())
    {
        return true;
    }


    if (!io.db().isTime())
    {
        // 3. Search for parent file

        autoPtr<IOobject> fileIO = parentIO.findFile(false);
        if (fileIO.valid() && fileIO().headerOk())
        {
            return true;
        }
    }

    return false;
}


Foam::autoPtr<Foam::IOobject> Foam::IOdictionary::lookupClass
(
    const fileName& dbDir,
    const word& ClassName
) const
{
    word key(scopedName(dbDir, db().dbDir()));

    const entry* ePtr = lookupScopedEntryPtr
    (
        key,
        false,
        true            // allow pattern match
    );
    if (ePtr && ePtr->isDict())
    {
        const dictionary& dict = ePtr->dict();

        autoPtr<IOobject> ioPtr(new IOobject(*this));
        if (ioPtr().readHeader(dict))
        {
            if (ioPtr().headerClassName() == ClassName)
            {
                return ioPtr;
            }
        }
    }

    return autoPtr<IOobject>(NULL);
}


Foam::IOobjectList Foam::IOdictionary::lookupClass
(
    const word& ClassName,
    const IOobjectList& fileObjects,
    const PtrList<IOdictionary>& parentObjects,
    const fileName& dbDir
)
{
    IOobjectList objectsOfClass(fileObjects.size()+parentObjects.size());

    // 1. From file objects
    forAllConstIter(IOobjectList, fileObjects, iter)
    {
        if (iter()->headerClassName() == ClassName)
        {
            objectsOfClass.insert(iter.key(), new IOobject(*iter()));
        }
    }

    // 2. From parent objects
    forAll(parentObjects, i)
    {
        const IOdictionary& dict = parentObjects[i];

        autoPtr<IOobject> ioPtr(dict.lookupClass(dbDir, ClassName));
        if (ioPtr.valid())
        {
            IOobject* io = ioPtr.ptr();

            objectsOfClass.insert(io->name(), io);
        }
    }
    return objectsOfClass;
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void Foam::IOdictionary::operator=(const IOdictionary& rhs)
{
    dictionary::operator=(rhs);
}


// ************************************************************************* //
