/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2017 OpenFOAM Foundation
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

#include "fileOperation.H"
#include "localFileOperation.H"
#include "regIOobject.H"
#include "argList.H"
#include "HashSet.H"

/* * * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * */

namespace Foam
{
    autoPtr<fileOperation> fileOperation::fileHandlerPtr_;

    defineTypeNameAndDebug(fileOperation, 0);
    defineRunTimeSelectionTable(fileOperation, word);

    class addArgsOptions
    {
        public:
        addArgsOptions()
        {
            argList::addOption
            (
                "fileHandler",
                "handler",
                "override the fileHandler"
            );
        }
    };

    addArgsOptions intObj;
}

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::fileMonitor& Foam::fileOperation::monitor() const
{
    if (!monitorPtr_.valid())
    {
        monitorPtr_.reset
        (
            new fileMonitor
            (
                regIOobject::fileModificationChecking == IOobject::inotify
             || regIOobject::fileModificationChecking == IOobject::inotifyMaster
            )
        );
    }
    return monitorPtr_();
}


Foam::instantList Foam::fileOperation::sortTimes
(
    const fileNameList& dirEntries,
    const word& constantName
)
{
    // Initialise instant list
    instantList Times(dirEntries.size() + 1);
    label nTimes = 0;

    // Check for "constant"
    bool haveConstant = false;
    forAll(dirEntries, i)
    {
        if (dirEntries[i] == constantName)
        {
            Times[nTimes].value() = 0;
            Times[nTimes].name() = dirEntries[i];
            nTimes++;
            haveConstant = true;
            break;
        }
    }

    // Read and parse all the entries in the directory
    forAll(dirEntries, i)
    {
        IStringStream timeStream(dirEntries[i]);
        token timeToken(timeStream);

        if (timeToken.isNumber() && timeStream.eof())
        {
            Times[nTimes].value() = timeToken.number();
            Times[nTimes].name() = dirEntries[i];
            nTimes++;
        }
    }

    // Reset the length of the times list
    Times.setSize(nTimes);

    if (haveConstant)
    {
        if (nTimes > 2)
        {
            std::sort(&Times[1], Times.end(), instant::less());
        }
    }
    else if (nTimes > 1)
    {
        std::sort(&Times[0], Times.end(), instant::less());
    }

    return Times;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fileOperation::fileOperation()
{}


Foam::autoPtr<Foam::fileOperation> Foam::fileOperation::New(const word& type)
{
    if (debug)
    {
        InfoInFunction << "Constructing fileOperation" << endl;
    }

    wordConstructorTable::iterator cstrIter =
        wordConstructorTablePtr_->find(type);

    if (cstrIter == wordConstructorTablePtr_->end())
    {
        FatalErrorInFunction
            << "Unknown fileOperation type "
            << type << nl << nl
            << "Valid fileOperation types are" << endl
            << wordConstructorTablePtr_->sortedToc()
            << abort(FatalError);
    }

    return autoPtr<fileOperation>(cstrIter()());
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::fileOperation::~fileOperation()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::fileName Foam::fileOperation::objectPath(const IOobject& io) const
{
    return io.objectPath();
}


bool Foam::fileOperation::writeObject
(
    const regIOobject& io,
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp
) const
{
    mkDir(io.path());

    fileName pathName(io.objectPath());

    autoPtr<Ostream> osPtr
    (
        NewOFstream
        (
            pathName,
            fmt,
            ver,
            cmp
        )
    );

    if (!osPtr.valid())
    {
        return false;
    }

    Ostream& os = osPtr();

    // If any of these fail, return (leave error handling to Ostream class)
    if (!os.good())
    {
        return false;
    }

    if (!io.writeHeader(os))
    {
        return false;
    }

    // Write the data to the Ostream
    if (!io.writeData(os))
    {
        return false;
    }

    IOobject::writeEndDivider(os);

    return true;
}


Foam::label Foam::fileOperation::addWatch(const fileName& fName) const
{
    return monitor().addWatch(fName);
}


bool Foam::fileOperation::removeWatch(const label watchIndex) const
{
    return monitor().removeWatch(watchIndex);
}


Foam::label Foam::fileOperation::findWatch
(
    const labelList& watchIndices,
    const fileName& fName
) const
{
    forAll(watchIndices, i)
    {
        if (getFile(watchIndices[i]) == fName)
        {
            return i;
        }
    }
    return -1;
}


void Foam::fileOperation::addWatches
(
    regIOobject& rio,
    const fileNameList& files
) const
{
    const labelList& watchIndices = rio.watchIndices();

    DynamicList<label> newWatchIndices;
    labelHashSet removedWatches(watchIndices);

    forAll(files, i)
    {
        const fileName& f = files[i];
        label index = findWatch(watchIndices, f);

        if (index == -1)
        {
            newWatchIndices.append(addWatch(f));
        }
        else
        {
            // Existing watch
            newWatchIndices.append(watchIndices[index]);
            removedWatches.erase(index);
        }
    }

    // Remove any unused watches
    forAllConstIter(labelHashSet, removedWatches, iter)
    {
        removeWatch(watchIndices[iter.key()]);
    }

    rio.watchIndices() = newWatchIndices;
}


const Foam::fileName& Foam::fileOperation::getFile(const label watchIndex) const
{
    return monitor().getFile(watchIndex);
}


void Foam::fileOperation::updateStates
(
    const bool masterOnly,
    const bool syncPar
) const
{
    monitor().updateStates(masterOnly, Pstream::parRun());
}


Foam::fileMonitor::fileState Foam::fileOperation::getState
(
    const label watchFd
) const
{
    return monitor().getState(watchFd);
}


void Foam::fileOperation::setUnmodified(const label watchFd) const
{
    monitor().setUnmodified(watchFd);
}


const Foam::fileOperation& Foam::fileHandler()
{
    if (!fileOperation::fileHandlerPtr_.valid())
    {
        word handler(getEnv("FOAM_FILEHANDLER"));
        if (!handler.size())
        {
            handler = fileOperations::localFileOperation::typeName;
        }

        if (Pstream::master())
        {
            cout<< "fileHandler() : Inserting fileOperation of type "
                << handler << std::endl;
        }
        fileOperation::fileHandlerPtr_ = fileOperation::New(handler);
    }
    return fileOperation::fileHandlerPtr_();
}


void Foam::fileHandler(autoPtr<fileOperation>& newHandlerPtr)
{
    if (fileOperation::fileHandlerPtr_.valid())
    {
        if (Pstream::master())
        {
            cout<< "fileHandler() : Deleting fileOperation of type "
                << fileOperation::fileHandlerPtr_().type() << std::endl;
        }
    }
    fileOperation::fileHandlerPtr_.clear();

    if (newHandlerPtr.valid())
    {
        if (Pstream::master())
        {
            cout<< "fileHandler() : Inserting fileOperation of type "
                << newHandlerPtr().type() << std::endl;
        }
        fileOperation::fileHandlerPtr_ = newHandlerPtr;
    }
}


// ************************************************************************* //
