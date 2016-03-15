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

#include "regIOobject2.H"
#include "Time.H"
#include "polyMesh.H"
#include "registerSwitch.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(regIOobject2, 0);

    template<>
    const char* NamedEnum
    <
        regIOobject2::fileCheckTypes,
        4
    >::names[] =
    {
        "timeStamp",
        "timeStampMaster",
        "inotify",
        "inotifyMaster"
    };
}

int Foam::regIOobject2::fileModificationSkew
(
    Foam::debug::optimisationSwitch("fileModificationSkew", 30)
);
registerOptSwitch
(
    "fileModificationSkew",
    int,
    Foam::regIOobject2::fileModificationSkew
);


const Foam::NamedEnum<Foam::regIOobject2::fileCheckTypes, 4>
    Foam::regIOobject2::fileCheckTypesNames;

Foam::regIOobject2::fileCheckTypes Foam::regIOobject2::fileModificationChecking
(
    fileCheckTypesNames.read
    (
        debug::optimisationSwitches().lookup
        (
            "fileModificationChecking"
        )
    )
);

namespace Foam
{
    // Register re-reader
    class addfileModificationCheckingToOpt
    :
        public ::Foam::simpleRegIOobject
    {
    public:

        addfileModificationCheckingToOpt(const char* name)
        :
            ::Foam::simpleRegIOobject(Foam::debug::addOptimisationObject, name)
        {}

        virtual ~addfileModificationCheckingToOpt()
        {}

        virtual void readData(Foam::Istream& is)
        {
            regIOobject2::fileModificationChecking =
                regIOobject2::fileCheckTypesNames.read(is);
        }

        virtual void writeData(Foam::Ostream& os) const
        {
            os <<  regIOobject2::fileCheckTypesNames
                [regIOobject2::fileModificationChecking];
        }
    };

    addfileModificationCheckingToOpt addfileModificationCheckingToOpt_
    (
        "fileModificationChecking"
    );
}


bool Foam::regIOobject2::masterOnlyReading = false;


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::regIOobject2::regIOobject2(const IOobject& io, const bool isTime)
:
    IOobject(io),
    registered_(false),
    ownedByRegistry_(false),
    watchIndex_(-1),
    eventNo_                // Do not get event for top level Time database
    (
        isTime
      ? 0
      : db().getEvent()
    ),
    isPtr_(NULL)
{
    // Register with objectRegistry2 if requested
    if (registerObject())
    {
        checkIn();
    }
}


Foam::regIOobject2::regIOobject2(const regIOobject2& rio)
:
    IOobject(rio),
    registered_(false),
    ownedByRegistry_(false),
    watchIndex_(rio.watchIndex_),
    eventNo_(db().getEvent()),
    isPtr_(NULL)
{
    // Do not register copy with objectRegistry2
}


Foam::regIOobject2::regIOobject2(const regIOobject2& rio, bool registerCopy)
:
    IOobject(rio),
    registered_(false),
    ownedByRegistry_(false),
    watchIndex_(-1),
    eventNo_(db().getEvent()),
    isPtr_(NULL)
{
    if (registerCopy && rio.registered_)
    {
        const_cast<regIOobject2&>(rio).checkOut();
        checkIn();
    }
}


Foam::regIOobject2::regIOobject2
(
    const word& newName,
    const regIOobject2& rio,
    bool registerCopy
)
:
    IOobject(newName, rio.instance(), rio.local(), rio.db()),
    registered_(false),
    ownedByRegistry_(false),
    watchIndex_(-1),
    eventNo_(db().getEvent()),
    isPtr_(NULL)
{
    if (registerCopy)
    {
        checkIn();
    }
}


Foam::regIOobject2::regIOobject2
(
    const IOobject& io,
    const regIOobject2& rio
)
:
    IOobject(io),
    registered_(false),
    ownedByRegistry_(false),
    watchIndex_(-1),
    eventNo_(db().getEvent()),
    isPtr_(NULL)
{
    if (registerObject())
    {
        checkIn();
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::regIOobject2::~regIOobject2()
{
    if (objectRegistry2::debug)
    {
        Info<< "Destroying regIOobject2 called " << name()
            << " of type " << type()
            << " in directory " << path()
            << endl;
    }

    if (isPtr_)
    {
        delete isPtr_;
        isPtr_ = NULL;
    }

    // Check out of objectRegistry2 if not owned by the registry

    if (!ownedByRegistry_)
    {
        checkOut();
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::regIOobject2::checkIn()
{
    if (!registered_)
    {
        // multiple checkin of same object is disallowed - this would mess up
        // any mapping
        registered_ = db().checkIn(*this);

        if
        (
            registered_
         && readOpt() == MUST_READ_IF_MODIFIED
         && time().runTimeModifiable()
        )
        {
            if (watchIndex_ != -1)
            {
                FatalErrorInFunction
                    << "Object " << objectPath()
                    << " already watched with index " << watchIndex_
                    << abort(FatalError);
            }

            fileName f = filePath();
            if (!f.size())
            {
                // We don't have this file but would like to re-read it.
                // Possibly if master-only reading mode.
                f = objectPath();
            }
            watchIndex_ = time().addWatch(f);
        }

        // check-in on defaultRegion is allowed to fail, since subsetted meshes
        // are created with the same name as their originating mesh
        if (!registered_ && debug && name() != polyMesh::defaultRegion)
        {
            if (debug == 2)
            {
                // for ease of finding where attempted duplicate check-in
                // originated
                FatalErrorInFunction
                    << "failed to register object " << objectPath()
                    << " the name already exists in the objectRegistry2" << endl
                    << "Contents:" << db().sortedToc()
                    << abort(FatalError);
            }
            else
            {
                WarningInFunction
                    << "failed to register object " << objectPath()
                    << " the name already exists in the objectRegistry2"
                    << endl;
            }
        }
    }

    return registered_;
}


bool Foam::regIOobject2::checkOut()
{
    if (registered_)
    {
        registered_ = false;

        if (watchIndex_ != -1)
        {
            time().removeWatch(watchIndex_);
            watchIndex_ = -1;
        }
        return db().checkOut(*this);
    }

    return false;
}


bool Foam::regIOobject2::upToDate(const regIOobject2& a) const
{
    if (a.eventNo() >= eventNo_)
    {
        return false;
    }
    else
    {
        return true;
    }
}


bool Foam::regIOobject2::upToDate
(
    const regIOobject2& a,
    const regIOobject2& b
) const
{
    if
    (
        a.eventNo() >= eventNo_
     || b.eventNo() >= eventNo_
    )
    {
        return false;
    }
    else
    {
        return true;
    }
}


bool Foam::regIOobject2::upToDate
(
    const regIOobject2& a,
    const regIOobject2& b,
    const regIOobject2& c
) const
{
    if
    (
        a.eventNo() >= eventNo_
     || b.eventNo() >= eventNo_
     || c.eventNo() >= eventNo_
    )
    {
        return false;
    }
    else
    {
        return true;
    }
}


bool Foam::regIOobject2::upToDate
(
    const regIOobject2& a,
    const regIOobject2& b,
    const regIOobject2& c,
    const regIOobject2& d
) const
{
    if
    (
        a.eventNo() >= eventNo_
     || b.eventNo() >= eventNo_
     || c.eventNo() >= eventNo_
     || d.eventNo() >= eventNo_
    )
    {
        return false;
    }
    else
    {
        return true;
    }
}


void Foam::regIOobject2::setUpToDate()
{
    eventNo_ = db().getEvent();
}


void Foam::regIOobject2::rename(const word& newName)
{
    // Check out of objectRegistry2
    checkOut();

    IOobject::rename(newName);

    if (registerObject())
    {
        // Re-register object with objectRegistry2
        checkIn();
    }
}


void Foam::regIOobject2::operator=(const IOobject& io)
{
    if (isPtr_)
    {
        delete isPtr_;
        isPtr_ = NULL;
    }

    // Check out of objectRegistry2
    checkOut();

    IOobject::operator=(io);

    if (registerObject())
    {
        // Re-register object with objectRegistry2
        checkIn();
    }
}


// ************************************************************************* //
