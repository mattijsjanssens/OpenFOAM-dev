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
#include "uncollatedFileOperation.H"
#include "regIOobject.H"
#include "argList.H"
#include "HashSet.H"
#include "masterUncollatedFileOperation.H"
#include "objectRegistry.H"
#include "decomposedBlockData.H"
#include "polyMesh.H"

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

    word fileOperation::processorsDir = "processors";
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
        //IStringStream timeStream(dirEntries[i]);
        //token timeToken(timeStream);

        //if (timeToken.isNumber() && timeStream.eof())
        scalar timeValue;
        if (readScalar(dirEntries[i].c_str(), timeValue))
        {
            Times[nTimes].value() = timeValue;
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
    IOstream::compressionType cmp,
    const bool valid
) const
{
    if (valid)
    {
        fileName pathName(io.objectPath());

        mkDir(pathName.path());

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
    }
    return true;
}


Foam::fileName Foam::fileOperation::filePath(const fileName& fName) const
{
    fileName path;
    fileName local;
    label proci = fileOperations::masterUncollatedFileOperation::
    splitProcessorPath
    (
        fName,
        path,
        local
    );

    fileName procsName(path/processorsDir/local);

    // Give preference to processors variant
    if (proci != -1 && exists(procsName))
    {
        return procsName;
    }
    else if (exists(fName))
    {
        return fName;
    }
    else
    {
        return fileName::null;
    }
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


Foam::fileName Foam::fileOperation::getFile(const label watchIndex) const
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


Foam::instantList Foam::fileOperation::findTimes
(
    const fileName& directory,
    const word& constantName
) const
{
    if (debug)
    {
        Pout<< FUNCTION_NAME
            << " : Finding times in directory " << directory << endl;
    }

    // Read directory entries into a list
    fileNameList dirEntries
    (
        Foam::readDir
        (
            directory,
            fileName::DIRECTORY
        )
    );

    instantList times = sortTimes(dirEntries, constantName);

    // Check if directory is processorXXX
    fileName procsDir
    (
        fileOperations::masterUncollatedFileOperation::processorsPath
        (
            directory
        )
    );

    if (!procsDir.empty())
    {
        fileNameList extraEntries
        (
            Foam::readDir
            (
                procsDir,
                fileName::DIRECTORY
            )
        );

        instantList extraTimes = sortTimes(extraEntries, constantName);

        label sz = times.size();
        times.setSize(sz+extraTimes.size());
        forAll(extraTimes, i)
        {
            times[sz++] = extraTimes[i];
        }

        // Sort
        if (times.size() > 1)
        {
            label starti = 0;
            if (times[0].name() == constantName)
            {
                starti = 1;
            }
            std::sort(&times[starti], times.end(), instant::less());

            // Filter out duplicates
            label newi = starti+1;
            for (label i = newi; i < times.size(); i++)
            {
                if (times[i].value() != times[i-1].value())
                {
                    if (newi != i)
                    {
                        times[newi] = times[i];
                    }
                    newi++;
                }
            }

            times.setSize(newi);
        }
    }

    if (debug)
    {
        Pout<< FUNCTION_NAME
            << " : Found times:" << times << endl;
    }
    return times;
}


Foam::fileNameList Foam::fileOperation::readObjects
(
    const objectRegistry& db,
    const fileName& instance,
    const fileName& local,
    word& newInstance
) const
{
    if (debug)
    {
        Pout<< "fileOperation::readObjects :"
            << " db:" << db.objectPath()
            << " instance:" << instance << endl;
    }

    fileName timePath(db.path(instance));
    fileName path(db.path(instance, db.dbDir()/local));

    newInstance = word::null;
    fileNameList objectNames;
    if (Foam::isDir(timePath))
    {
        newInstance = instance;
        objectNames = Foam::readDir(path, fileName::FILE);
    }
    else
    {
        // Get processors equivalent of timePath

        fileName prefix;
        fileName postfix;
        label proci = fileOperations::masterUncollatedFileOperation::
        splitProcessorPath
        (
            timePath,
            prefix,
            postfix
        );
        fileName procsPath(prefix/processorsDir/postfix);

        if (proci != -1 && Foam::isDir(procsPath))
        {
            newInstance = instance;
            objectNames = Foam::readDir(procsPath/local, fileName::FILE);
        }
    }
    return objectNames;
}


Foam::label Foam::fileOperation::nProcs
(
    const fileName& dir,
    const fileName& local
) const
{
    if (Foam::isDir(dir/processorsDir))
    {
        fileName pointsFile
        (
            dir
           /processorsDir
           /"constant"
           /local
           /polyMesh::meshSubDir
           /"points"
        );

        if (Foam::isFile(pointsFile))
        {
            return decomposedBlockData::numBlocks(pointsFile);
        }
        else
        {
            WarningInFunction << "Cannot read file " << pointsFile
                << " to determine the number of decompositions."
                << " Falling back to looking for processor.*" << endl;
        }
    }

    label nProcs = 0;
    while
    (
        isDir
        (
            dir
           /(word("processor") + name(nProcs))
           /"constant"
           /local
           /polyMesh::meshSubDir
        )
    )
    {
        ++nProcs;
    }

    return nProcs;
}


const Foam::fileOperation& Foam::fileHandler()
{
    if (!fileOperation::fileHandlerPtr_.valid())
    {
        word handler(getEnv("FOAM_FILEHANDLER"));

        if (handler.size())
        {
            //if (Pstream::master())
            //{
            //    cout<< "fileHandler() : Inserting fileOperation of type "
            //        << handler << std::endl;
            //}
        }
        else
        {
            handler = fileOperations::uncollatedFileOperation::typeName;
        }

        fileOperation::fileHandlerPtr_ = fileOperation::New(handler);
    }
    return fileOperation::fileHandlerPtr_();
}


void Foam::fileHandler(autoPtr<fileOperation>& newHandlerPtr)
{
    if (fileOperation::fileHandlerPtr_.valid())
    {
        if
        (
            newHandlerPtr.valid()
         && newHandlerPtr->type() == fileOperation::fileHandlerPtr_->type()
        )
        {
            return;
        }
    }
    fileOperation::fileHandlerPtr_.clear();

    if (newHandlerPtr.valid())
    {
        fileOperation::fileHandlerPtr_ = newHandlerPtr;
    }
}


// ************************************************************************* //
