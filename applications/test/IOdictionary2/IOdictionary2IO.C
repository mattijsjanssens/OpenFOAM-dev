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

#include "IOdictionary2.H"
#include "Pstream.H"
#include "Time.H"
#include "ITstream.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::IOdictionary2::readFile(const bool masterOnly)
{
    if (Pstream::master() || !masterOnly)
    {
        if (debug)
        {
            Pout<< "IOdictionary2 : Reading " << objectPath()
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
                << "--- IOdictionary2 " << name()
                << ' ' << objectPath() << ":" << nl;
            writeHeader(Sout);
            writeData(Sout);
            Sout<< "--- End of IOdictionary2 " << name() << nl << endl;
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
                Pout<< "IOdictionary2 : Reading " << objectPath()
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
            IOdictionary2::readData(fromAbove);
        }

        // Send to my downstairs neighbours
        forAll(myComm.below(), belowI)
        {
            if (debug)
            {
                Pout<< "IOdictionary2 : Sending " << objectPath()
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
            IOdictionary2::writeData(toBelow);
        }
    }
}


// * * * * * * * * * * * * * * * Members Functions * * * * * * * * * * * * * //

bool Foam::IOdictionary2::readData(Istream& is)
{
    is >> *this;

    if (writeDictionaries && Pstream::master() && !is.bad())
    {
        Sout<< nl
            << "--- IOdictionary2 " << name()
            << ' ' << objectPath() << ":" << nl;
        writeHeader(Sout);
        writeData(Sout);
        Sout<< "--- End of IOdictionary2 " << name() << nl << endl;
    }

    return !is.bad();
}


bool Foam::IOdictionary2::writeData(Ostream& os) const
{
    dictionary::write(os, false);
    return os.good();
}


Foam::IOdictionary2* Foam::IOdictionary2::readFileAndRegister
(
    const IOobject& io
)
{
    //Pout<< "IOdictionary2::readFileAndRegister() :"
    //    << " :" << io.objectPath() << endl;

    const IOdictionary2* dictPtr =
        lookupObjectPtr<IOdictionary2>(io.db(), io.name(), false);
    if (dictPtr)
    {
        return const_cast<IOdictionary2*>(dictPtr);
    }


    fileName fName(io.filePath());

    //Pout<< "IOdictionary2::readFileAndRegister() :"
    //    << " fName:" << fName << endl;


    if (isFile(fName))
    {
        InfoInFunction
            << nl
            << "    Reading parent object " << io.name()
            << " from file " << fName << endl;

        // Read from file
        IOdictionary2* dictPtr = new IOdictionary2(io);

        dictPtr->store();

        //Pout<< "IOdictionary2::readFileAndRegister() :"
        //    << " read dictionary:" << *dictPtr
        //    << " event:" << dictPtr->fileEventNo_ << endl;

        return dictPtr;
    }
    else if
    (
       &io.db()
     != dynamic_cast<const objectRegistry*>(&io.db().time())
    )
    {
        //IOobject parentIO(io, io.db().parent());
        IOobject parentIO
        (
            io.name(),
            io.instance(),
            io.local(),
            io.db().parent(),
            io.readOpt(),
            io.writeOpt(),
            io.registerObject()
        );

        IOdictionary2* parentPtr = readFileAndRegister(parentIO);

        if (parentPtr)
        {
            // Store dictionary at this level, read from top
            autoPtr<Istream> isPtr(parentPtr->readPart(io));
            IOdictionary2* dictPtr = new IOdictionary2(io, isPtr());

            dictPtr->store();

            // Override fileEvent with that of parent
            dictPtr->fileEventNo_ = parentPtr->fileEventNo_;

            //Pout<< "IOdictionary2::readFileAndRegister() :"
            ///    << " created Istream dictionary:" << dictPtr->objectPath()
            //    << " event:" << dictPtr->fileEventNo_ << endl;

            return dictPtr;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}


Foam::autoPtr<Foam::Istream> Foam::IOdictionary2::readPart
(
    const IOobject& child
) const
{
    //Pout<< "IOdictionary2::readPart() :"
    //    << " :" << objectPath()
    //    << " child:" << child.objectPath() << endl;

    const entry* ePtr = lookupEntryPtr(child.db().name(), false, true);
    if (ePtr)
    {
        OStringStream os;
        os << ePtr->dict() << endl;

        return autoPtr<Istream>(new IStringStream(os.str()));
    }
    else
    {
        FatalIOErrorInFunction
        (
            *this
        )   << "keyword " << child.db().name() << " is undefined in dictionary "
            << objectPath()
            << exit(FatalIOError);

        return autoPtr<Istream>(NULL);
    }
}


// // If the object is already registered: read from parent
// bool Foam::IOdictionary2::readFromParent()
// {
//     //Pout<< "IOdictionary2::readFromParent() :"
//     //    << " :" << objectPath()
//     //    << " ev:" << fileEventNo_ << endl;
// 
//     bool ok = false;
//     if
//     (
//        &db()
//      != dynamic_cast<const objectRegistry*>(&db().time())
//     )
//     {
//         // Pout<< "IOdictionary2::readFromParent() :"
//         //     << " : searching for:" << name()
//         //     << " in parent:" << db().parent().name()
//         //     << " with contents:" << db().parent().sortedToc()
//         //     << endl;
// 
// 
//         const IOdictionary2* parentPtr =
//             lookupObjectPtr<IOdictionary2>(db().parent(), name(), false);
//         if (parentPtr)
//         {
//             IOdictionary2& parent = const_cast<IOdictionary2&>(*parentPtr);
// 
//             //Pout<< "IOdictionary2::readFromParent() :"
//             //    << " : found parent object:" << parent.objectPath()
//             //    << " evmet:" << parent.fileEventNo_ << endl;
// 
//             ok = parent.readFromParent();
// 
//             //Pout<< "IOdictionary2::readFromParent() :"
//             //    << " : read parent " << parent.objectPath()
//             //    << "  ok:" << ok
//             //    << " parent event:" << parent.fileEventNo_
//             //    << " my event:" << fileEventNo_ << endl;
// 
//             //- Problem: event numbers are local to objectRegistry
//             //  so objects on different registries cannot be compared.
//             //  Instead store the event number from the event of the
//             //  level at which the file was read
//             if (parent.fileEventNo_ > fileEventNo_)
//             {
//                 //Pout<< "IOdictionary2::readFromParent() :"
//                 //    << " : get stream from parent " << parent.objectPath()
//                 //    << endl;
// 
//                 autoPtr<Istream> str(parent.readPart(*this));
//                 ok = readData(str());
//                 setUpToDate();
//                 fileEventNo_ = parent.fileEventNo_;
//                 //Pout<< "IOdictionary2::readFromParent() :"
//                 //    << " read myself:" << objectPath()
//                 //    << " event:" << fileEventNo_
//                 //    << " from parent:" << parent.objectPath()
//                 //    << " event:" << parent.fileEventNo_ << endl;
//             }
//             return ok;
//         }
//     }
//     else
//     {
//         // Highest registered. This is always based on file!
//         // It will be updated by readIfMotified
//         //ok = readData(readStream(typeName));
//         //close();
//     }
// 
//     return ok;
// }


bool Foam::IOdictionary2::read()
{
    //Pout<< "IOdictionary2::read()" << endl;

    // Check if there is a registered parent and if so read from that
    bool ok = readFromParent<IOdictionary2>();
    if (ok)
    {
        //Pout << "IOdictionary2::read() : read from parent" << endl;
        return ok;
    }

    if
    (
       &db()
     != dynamic_cast<const objectRegistry*>(&db().time())
    )
    {
        // See if can read from file anywhere along the path up. Construct
        // and register intermediate IOdictionaries as we go along.

        //IOobject parentIO(*this, db().parent());
        IOobject parentIO
        (
            name(),
            instance(),
            local(),
            db().parent(),
            readOpt(),
            writeOpt(),
            registerObject()
        );

        //Pout<< "IOdictionary2::read() :"
        //    << " : parent object:" << parentIO.objectPath()
        //    << " : this event:" << fileEventNo_ << endl;

        IOdictionary2* parentPtr = readFileAndRegister(parentIO);

        if (parentPtr)
        {
            //Pout << "IOdictionary2::read() : read parent file" << endl;
            // Read contents by trying again - this should trigger the
            // readFromParent()
            return read();
        }
    }

    // Read normal
    ok = readData(readStream(typeName));
    close();
    setUpToDate();
    fileEventNo_ = eventNo();
    //Pout<< "IOdictionary2::read() :"
    //    << " read " << objectPath() << " eve:" << fileEventNo_ << endl;

    return ok;
}

//XXXXXXXXXX
bool Foam::IOdictionary2::readPart
(
    const IOobject& child,
    Istream& is
)
{
    dictionary dict(is);
    set(child.db().name(), dict);
    return true;
}
// // If the object is already registered: write to parent
// bool Foam::IOdictionary2::writeToParent()
// {
//     //Pout<< "IOdictionary2::writeToParent() :"
//     //    << " :" << objectPath()
//     //    << " ev:" << fileEventNo_ << endl;
// 
//     bool ok = false;
//     if
//     (
//        &db()
//      != dynamic_cast<const objectRegistry*>(&db().time())
//     )
//     {
//         const IOdictionary2* parentPtr =
//             lookupObjectPtr<IOdictionary2>(db().parent(), name(), false);
//         if (parentPtr)
//         {
//             IOdictionary2& parent = const_cast<IOdictionary2&>(*parentPtr);
// 
//             Pout<< "IOdictionary2::writeToParent() :"
//                 << " : found parent object:" << parent.objectPath()
//                 << " evmet:" << parent.fileEventNo_ << endl;
// 
//             // Update parent from *this
//             {
//                 OStringStream os;
//                 writeData(os);
//                 IStringStream is(os.str());
//                 ok = parent.readPart(*this, is);
//             }
// 
//             // Update parent
//             parent.writeToParent();
// 
//             return ok;
//         }
//     }
//     else
//     {
//         // Highest registered. This is always based on file!
//         // It will be written itself (hopefully) through the
//         // objectRegistry::write()
//     }
// 
//     return ok;
// }
bool Foam::IOdictionary2::writeObject
(
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp
) const
{
    bool ok = const_cast<IOdictionary2&>(*this).writeToParent<IOdictionary2>();
    if (ok)
    {
        return ok;
    }
    return regIOobject::writeObject(fmt, ver, cmp);
}


// ************************************************************************* //
