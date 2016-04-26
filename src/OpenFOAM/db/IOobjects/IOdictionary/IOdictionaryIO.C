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
#include "Pstream.H"
#include "objectRegistry.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::IOdictionary::readFile(const bool masterOnly)
{
    if (Pstream::master() || !masterOnly)
    {
        if (debug)
        {
            Pout<< "IOdictionary : Reading " << objectPath()
                << " from file " << endl;
        }

        // Set flag for e.g. codeStream
        bool oldFlag = regIOobject::masterOnlyReading;
        regIOobject::masterOnlyReading = masterOnly;

        // Read file
        readStream(typeName) >> *this;
        close();

        regIOobject::masterOnlyReading = oldFlag;

        if (writeDictionaries && Pstream::master())
        {
            Sout<< nl
                << "--- IOdictionary " << name()
                << ' ' << objectPath() << ":" << nl;
            writeHeader(Sout);
            writeData(Sout);
            Sout<< "--- End of IOdictionary " << name() << nl << endl;
        }
    }

    if (masterOnly && Pstream::parRun())
    {
        // Scatter master data using communication scheme

        const List<Pstream::commsStruct>& comms =
        (
            (Pstream::nProcs() < Pstream::nProcsSimpleSum)
          ? Pstream::linearCommunication()
          : Pstream::treeCommunication()
        );

        // Master reads headerclassname from file. Make sure this gets
        // transfered as well as contents.
        Pstream::scatter
        (
            comms,
            const_cast<word&>(headerClassName()),
            Pstream::msgType(),
            Pstream::worldComm
        );
        Pstream::scatter(comms, note(), Pstream::msgType(), Pstream::worldComm);

        // Get my communication order
        const Pstream::commsStruct& myComm = comms[Pstream::myProcNo()];

        // Reveive from up
        if (myComm.above() != -1)
        {
            if (debug)
            {
                Pout<< "IOdictionary : Reading " << objectPath()
                    << " from processor " << myComm.above() << endl;
            }

            // Note: use ASCII for now - binary IO of dictionaries is
            // not currently supported or rather the primitiveEntries of
            // the dictionary think they are in binary form whereas they are
            // not. Could reset all the ITstreams to ascii?
            IPstream fromAbove
            (
                Pstream::scheduled,
                myComm.above(),
                0,
                Pstream::msgType(),
                Pstream::worldComm,
                IOstream::ASCII
            );
            IOdictionary::readData(fromAbove);
        }

        // Send to my downstairs neighbours
        forAll(myComm.below(), belowI)
        {
            if (debug)
            {
                Pout<< "IOdictionary : Sending " << objectPath()
                    << " to processor " << myComm.below()[belowI] << endl;
            }
            OPstream toBelow
            (
                Pstream::scheduled,
                myComm.below()[belowI],
                0,
                Pstream::msgType(),
                Pstream::worldComm,
                IOstream::ASCII
            );
            IOdictionary::writeData(toBelow);
        }
    }
}

bool Foam::IOdictionary::readObjectOrFile(const bool masterOnly)
{
    // 1. Check if there is a registered parent and if so read from that

    if (!db().isTime())
    {
        IOobject parentIO(*this, db().parent());

        const IOdictionary* parentPtr =
            parentIO.db().lookupObjectPtr<IOdictionary>
            (
                parentIO.name(),
                true                // Recurse to find at any level
            );

        if (parentPtr)
        {
            const IOdictionary& parent = *parentPtr;

            word key(dictionary::scopedName(db().dbDir(), parent.db().dbDir()));

            if (key.empty() || key == ":")
            {
                readHeader(parent);
                dictionary::operator=(parent);
                return true;
            }

            const entry* ePtr = parent.lookupScopedEntryPtr
            (
                key,
                false,
                true            // allow pattern match
            );
            if (ePtr && ePtr->isDict())
            {
                const dictionary& dict = ePtr->dict();
                readHeader(dict);
                dictionary::operator=(dict);
                return true;
            }

            // TBD: ?do we want to fall back to
            //      - reading the top-level: dictionary::operator=parent
            //      - read the original file
            //      - fatal error and force user to provide region entry.
            // With reading-the-original-file we have the problem of keeping
            // any region entries. Also we then write as a subdictionary
            // and these get copied.


            WarningInFunction
                << "Did not find entry " << key
                << " in parent dictionary " << parent.name()
                << endl;//    << exit(FatalIOError);

            // 3. Normal, local reading
            //dictionary::operator=(parent);
            readFile(masterOnly);
            return true;
        }


        // 2. Search for parent file
        autoPtr<IOobject> fileIO(parentIO.findFile(masterOnly));

        if (fileIO.valid())
        {
            // Read from file
            fileIO().registerObject() = true;
            IOdictionary* dictPtr = new IOdictionary(fileIO);
            dictPtr->store();

            // Recurse to find the newly stored object
            return readObjectOrFile(masterOnly);
        }
    }


    // 3. Normal, local reading
    readFile(masterOnly);

    return true;
}


// * * * * * * * * * * * * * * * Members Functions * * * * * * * * * * * * * //

bool Foam::IOdictionary::readData(Istream& is)
{
    is >> *this;

    if (writeDictionaries && Pstream::master() && !is.bad())
    {
        Sout<< nl
            << "--- IOdictionary " << name()
            << ' ' << objectPath() << ":" << nl;
        writeHeader(Sout);
        writeData(Sout);
        Sout<< "--- End of IOdictionary " << name() << nl << endl;
    }

    return !is.bad();
}


bool Foam::IOdictionary::writeData(Ostream& os) const
{
    dictionary::write(os, false);
    return os.good();
}


bool Foam::IOdictionary::read()
{
    // 1. Check if there is a registered parent and if so read from that

    if (!db().isTime())
    {
        IOobject parentIO(*this, db().parent());

        const IOdictionary* parentPtr =
            parentIO.db().lookupObjectPtr<IOdictionary>
            (
                parentIO.name(),
                true
            );

        if (parentPtr)
        {
            const IOdictionary& parent = *parentPtr;

            word key(scopedName(db().dbDir(), parent.db().dbDir()));

            const entry* ePtr = parent.lookupScopedEntryPtr
            (
                key,
                false,
                true            // allow pattern match
            );
            if (!ePtr)
            {
                FatalIOErrorInFunction(parent)
                    << "Did not find entry " << key
                    << " in parent dictionary " << name()
                    << exit(FatalIOError);
            }

            const dictionary& dict = ePtr->dict();
            readHeader(dict);
            dictionary::operator=(dict);
            return true;
        }


        // 2. Search for parent file

        bool masterOnly =
            regIOobject::fileModificationChecking == timeStampMaster
         || regIOobject::fileModificationChecking == inotifyMaster;

        autoPtr<IOobject> fileIO = parentIO.findFile(masterOnly);
        if (fileIO.valid())
        {
            // Read from file
            fileIO().registerObject() = true;
            IOdictionary* dictPtr = new IOdictionary(fileIO());
            dictPtr->store();

            // Recurse to find the newly stored object
            return read();
        }
    }


    // 3. Normal, local reading
    bool ok = readData(readStream(typeName));
    close();
    setUpToDate();

    return ok;
}


bool Foam::IOdictionary::modified() const
{
    // Check if there is a registered parent and if so check that one

    if (!db().isTime())
    {
        IOobject parentIO(*this, db().parent());

        const IOdictionary* parentPtr =
            parentIO.db().lookupObjectPtr<IOdictionary>
            (
                parentIO.name(),
                true
            );

        if (parentPtr)
        {
            return parentPtr->modified();
        }
    }
    return regIOobject::modified();
}


bool Foam::IOdictionary::readIfModified()
{
    // Check if there is a registered parent and if so check that one

    if (!db().isTime())
    {
        IOobject parentIO(*this, db().parent());

        const IOdictionary* parentPtr =
            parentIO.db().lookupObjectPtr<IOdictionary>
            (
                parentIO.name(),
                true
            );

        if (parentPtr)
        {
            IOdictionary& parent = const_cast<IOdictionary&>(*parentPtr);
            bool haveRead = parent.readIfModified();

            if (haveRead)
            {
                // Read myself from parent
                word key(scopedName(db().dbDir(), parent.db().dbDir()));

                const entry* ePtr = parent.lookupScopedEntryPtr
                (
                    key,
                    false,
                    true            // allow pattern match
                );
                if (!ePtr)
                {
                    FatalIOErrorInFunction(parent)
                        << "Did not find entry " << key
                        << " in parent dictionary " << name()
                        << exit(FatalIOError);
                }

                const dictionary& dict = ePtr->dict();
                readHeader(dict);
                dictionary::operator=(dict);
                return true;
            }
        }
    }

    return regIOobject::readIfModified();
}


bool Foam::IOdictionary::writeObject
(
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp
) const
{
    // 1. Check if there is a registered parent and if so write to it

    if (!db().isTime())
    {
        IOobject parentIO(*this, db().parent());

        const IOdictionary* parentPtr =
            parentIO.db().lookupObjectPtr<IOdictionary>
            (
                parentIO.name(),
                true
            );

        if (parentPtr)
        {
            IOdictionary& parent = const_cast<IOdictionary&>(*parentPtr);

            word scope
            (
                dictionary::scopedName
                (
                    db().dbDir(),
                    parent.db().dbDir()
                )
            );

            // Add dictionary
            parent.setScoped(scope, *this);

            // Add the header
            {
                dictionary dict(headerDict(type()));

                const entry* entPtr = parent.lookupScopedEntryPtr
                (
                    scope,
                    false,          //recursive
                    false
                );

                const_cast<dictionary&>(entPtr->dict()).set("FoamFile", dict);
            }

            return true;
        }
    }

    return regIOobject::writeObject(fmt, ver, cmp);
}


// ************************************************************************* //
