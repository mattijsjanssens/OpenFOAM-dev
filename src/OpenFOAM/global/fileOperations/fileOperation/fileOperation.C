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
#include "objectRegistry.H"
#include "decomposedBlockData.H"
#include "polyMesh.H"
#include "registerSwitch.H"
#include "Time.H"

/* * * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * */

namespace Foam
{
    autoPtr<fileOperation> fileOperation::fileHandlerPtr_;

    defineTypeNameAndDebug(fileOperation, 0);
    defineRunTimeSelectionTable(fileOperation, word);

    word fileOperation::defaultFileHandler
    (
        debug::optimisationSwitches().lookupOrAddDefault
        (
            "fileHandler",
            //Foam::fileOperations::uncollatedFileOperation::typeName,
            word("uncollated"),
            false,
            false
        )
    );

    word fileOperation::processorsBaseDir = "processors";
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


bool Foam::fileOperation::isFileOrDir(const bool isFile, const fileName& f)
{
    const fileName::Type fTyp = Foam::type(f);

    return
        (isFile && fTyp == fileName::FILE)
     || (!isFile && fTyp == fileName::DIRECTORY);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fileOperation::fileOperation()
:
    collatedDir_("UNSET"),
    haveCollatedDir_(Switch::INVALID),
    processorsDir_(processorsBaseDir)
{}


Foam::autoPtr<Foam::fileOperation> Foam::fileOperation::New
(
    const word& type,
    const bool verbose
)
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

    return autoPtr<fileOperation>(cstrIter()(verbose));
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::fileOperation::~fileOperation()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::fileName Foam::fileOperation::objectPath
(
    const IOobject& io,
    const word& typeName
) const
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
    label nProcs;
    label proci = splitProcessorPath(fName, path, local, nProcs);

    if (nProcs != -1)
    {
        WarningInFunction << "Filename is already adapted:" << fName
            << endl;
    }

    // Processor-local file name
    fileName procsName(path/processorsDir_/local);

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
    fileName procsDir(processorsPath(directory));

    if (!procsDir.empty() && procsDir != directory)
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

        if (extraTimes.size())
        {
            bool haveConstant =
            (
                times.size() > 0
             && times[0].name() == constantName
            );

            bool haveExtraConstant =
            (
                extraTimes.size() > 0
             && extraTimes[0].name() == constantName
            );

            // Combine times
            instantList combinedTimes(times.size()+extraTimes.size());
            label sz = 0;
            label extrai = 0;
            if (haveExtraConstant)
            {
                extrai = 1;
                if (!haveConstant)
                {
                    combinedTimes[sz++] = extraTimes[0];    // constant
                }
            }
            forAll(times, i)
            {
                combinedTimes[sz++] = times[i];
            }
            for (; extrai < extraTimes.size(); extrai++)
            {
                combinedTimes[sz++] = extraTimes[extrai];
            }
            combinedTimes.setSize(sz);
            times.transfer(combinedTimes);

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

    fileName path(db.path(instance, db.dbDir()/local));

    newInstance = word::null;
    fileNameList objectNames;

    if (Foam::isDir(path))
    {
        newInstance = instance;
        objectNames = Foam::readDir(path, fileName::FILE);
    }
    else
    {
        // Get processors equivalent of path
        fileName procsPath(filePath(path));

        if (!procsPath.empty())
        {
            newInstance = instance;
            objectNames = Foam::readDir(procsPath, fileName::FILE);
        }
    }
    return objectNames;
}


void Foam::fileOperation::setNProcs(const label nProcs)
{}


Foam::label Foam::fileOperation::nProcs
(
    const fileName& dir,
    const fileName& local
) const
{
    label nProcs = 0;
    if (Pstream::master())
    {
        fileNameList dirNames(Foam::readDir(dir, fileName::Type::DIRECTORY));

        // Detect any processorsXXX or processorXXX
        label maxProc = -1;
        forAll(dirNames, i)
        {
            fileName path, local;
            label n;
            maxProc = max
            (
                maxProc,
                splitProcessorPath(dirNames[i], path, local, n)
            );
            if (n != -1)
            {
                // Direct detection of processorsXXX
                maxProc = n-1;
                break;
            }
        }
        nProcs = maxProc+1;


        if (nProcs == 0 && Foam::isDir(dir/processorsBaseDir))
        {
            fileName pointsFile
            (
                dir
               /processorsBaseDir
               /"constant"
               /local
               /polyMesh::meshSubDir
               /"points"
            );

            if (Foam::isFile(pointsFile))
            {
                nProcs = decomposedBlockData::numBlocks(pointsFile);
            }
            else
            {
                WarningInFunction << "Cannot read file " << pointsFile
                    << " to determine the number of decompositions."
                    << " Returning 1" << endl;
            }
        }
    }
    Pstream::scatter(nProcs);
    return nProcs;
}


Foam::fileName Foam::fileOperation::processorsCasePath(const IOobject& io) const
{
    if (haveCollatedDir_)
    {
        return
            io.rootPath()
           /io.time().globalCaseName()
           /collatedDir_.name();
    }
    else
    {
        return
            io.rootPath()
           /io.time().globalCaseName()
           /processorsDir_;
    }
}


Foam::fileName Foam::fileOperation::processorsPath
(
    const IOobject& io,
    const word& instance
) const
{
    return
        processorsCasePath(io)
       /instance
       /io.db().dbDir()
       /io.local();
}


Foam::fileName Foam::fileOperation::processorsPath(const fileName& dir) const
{
    // Check if directory is processorXXX
    word caseName(dir.name());

    std::string::size_type pos = caseName.find("processor");
    if (pos == 0)
    {
        if (caseName.size() <= 9 || caseName[9] == 's')
        {
            WarningInFunction << "Directory " << dir
                << " does not end in old-style processorDDD" << endl;
        }

        if (haveCollatedDir_)
        {
            return dir.path()/collatedDir_.name();
        }
        else
        {
            return dir.path()/processorsDir_;
        }
    }
    else
    {
        return fileName::null;
    }
}


Foam::label Foam::fileOperation::splitProcessorPath
(
    const fileName& objectPath,
    fileName& path,
    fileName& local,
    label& nProcs
)
{
    // Potentially detected number of processors
    nProcs = -1;

    // Search for processor at start of line or /processor
    std::string::size_type pos = objectPath.find("processor");
    if (pos == string::npos)
    {
        return -1;
    }

    if (pos == 0)
    {
        path = "";
        local = objectPath.substr(pos+9);
    }
    else if (objectPath[pos-1] != '/')
    {
        return -1;
    }
    else
    {
        path = objectPath.substr(0, pos-1);
        local = objectPath.substr(pos+9);
    }

    if (local.size() && local[0] == 's')
    {
        // "processsorsXXX"

        local = local.substr(pos+1);

        if (local.empty())
        {
            return -1;
        }

        label n;
        pos = local.find('/');
        if (pos == string::npos)
        {
            // processorsXXX without local
            if (Foam::read(local.c_str(), n))
            {
                local.clear();
                nProcs = n;
            }
        }
        else
        {
            string procName(local.substr(0, pos));
            if (Foam::read(procName.c_str(), n))
            {
                nProcs = n;
            }
            local = local.substr(pos+1);
        }
        return -1;
    }

    pos = local.find('/');
    if (pos == string::npos)
    {
        // processorXXX without local
        label proci;
        if (Foam::read(local.c_str(), proci))
        {
            local.clear();
            return proci;
        }
        return -1;
    }
    string procName(local.substr(0, pos));
    label proci;
    if (Foam::read(procName.c_str(), proci))
    {
        local = local.substr(pos+1);
        return proci;
    }
    return -1;
}


const Foam::fileOperation& Foam::fileHandler()
{
    if (!fileOperation::fileHandlerPtr_.valid())
    {
        word handler(getEnv("FOAM_FILEHANDLER"));

        if (!handler.size())
        {
            handler = fileOperation::defaultFileHandler;
        }

        fileOperation::fileHandlerPtr_ = fileOperation::New(handler, true);
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
