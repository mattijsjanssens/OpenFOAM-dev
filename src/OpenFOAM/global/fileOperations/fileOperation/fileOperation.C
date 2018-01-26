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


void Foam::fileOperation::mergeTimes
(
    const instantList& extraTimes,
    const word& constantName,
    instantList& times
)
{
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


bool Foam::fileOperation::isFileOrDir(const bool isFile, const fileName& f)
{
    const fileName::Type fTyp = Foam::type(f);

    return
        (isFile && fTyp == fileName::FILE)
     || (!isFile && fTyp == fileName::DIRECTORY);
}


//void Foam::fileOperation::cacheProcessorsPath(const fileName& fName) const
//{
//    fileName path;
//    fileName local;
//    label gStart;
//    label gSz;
//    label numProcs;
//    label proci = splitProcessorPath(fName, path, local, gStart, gSz, numProcs);
//
//    if (proci != -1)
//    {
//        // So we have a processor case. Read the directory to find out
//        // - name (if any) of collated processors directory
//        // - number of processors
//
//        if (procsDir_.valid())
//        {
//            // Already done detection
//            return;
//        }
//
//        if (debug)
//        {
//            Pout<< "fileOperation::cacheProcessorsPath :" << " fName:" << fName
//                << endl;
//        }
//
//        // Get actual output directory name
//        const word procDir(processorsDir(fName));
//
//        // Processor-local file name
//        if (isDir(path/procDir))
//        {
//            procsDir_.reset(new fileName(procDir));
//        }
//        else
//        {
//            // Try processors
//            if
//            (
//                processorsBaseDir != procDir
//             && isDir(path/processorsBaseDir)
//            )
//            {
//                procsDir_.reset(new fileName(processorsBaseDir));
//            }
//        }
//
//        // Try processorsDDD so we need to know the number of processors
//        // for this. In parallel this is the number of ranks in the
//        // communicator, for serial this has to be done by searching
//
//        label n = Pstream::nProcs();    //comm_);
//        if (!Pstream::parRun())
//        {
//            // E.g. checkMesh -case processor0
//            // Can be slow since only called at startup?
//            n = nProcs(path, local);
//
//            // Set number of processors
//            if (n > 0)
//            {
//                const_cast<fileOperation&>(*this).setNProcs(n);
//            }
//        }
//
//        word collatedProcDir(processorsBaseDir+Foam::name(n));
//        if
//        (
//           !procsDir_.valid()
//         && collatedProcDir != procDir
//         && isDir(path/collatedProcDir)
//        )
//        {
//            procsDir_.reset(new fileName(collatedProcDir));
//        }
//
//
//        // Nothing detected. Leave for next time round.
//        //if (!procsDir_.valid())
//        //{
//        //    procsDir_.reset(new fileName(""));
//        //}
//
//        if (debug)
//        {
//            if (procsDir_.valid())
//            {
//                Pout<< "fileOperation::cacheProcessorsPath : Detected:"
//                    << procsDir_() << endl;
//            }
//            else
//            {
//                Pout<< "fileOperation::cacheProcessorsPath :"
//                    << " Did not detect any processors dir for fName:"
//                    << fName << endl;
//            }
//        }
//    }
//}
void Foam::fileOperation::cacheProcessorsPath(const fileName& fName) const
{
    fileName path;
    fileName local;
    label gStart;
    label gSz;
    label numProcs;
    label proci = splitProcessorPath(fName, path, local, gStart, gSz, numProcs);

    if (proci != -1)
    {
        // So we have a processor case. Read the directory to find out
        // - name (if any) of collated processors directory
        // - number of processors

        if (procsDirs_.found(path))
        {
            // Already done detection
            return;
        }

        if (debug)
        {
            Pout<< "fileOperation::cacheProcessorsPath :" << " fName:" << fName
                << " detecting any processors in:" << path
                << endl;
        }


        // Try processorsDDD so we need to know the number of processors
        // for this. In parallel this is the number of ranks in the
        // communicator, for serial this has to be done by searching

        label n = Pstream::nProcs();    //comm_);
        if (!Pstream::parRun())
        {
            // E.g. checkMesh -case processor0
            // Can be slow since only called at startup?
            n = nProcs(path, local);

            // Set number of processors
            if (n > 0)
            {
                const_cast<fileOperation&>(*this).setNProcs(n);
            }
        }

        // Get actual output directory name
        const word procDir(processorsDir(fName));

Pout<< "** 1trying " << path/procDir << endl;

        // Processor-local file name
        if (isDir(path/procDir))
        {
            procsDirs_.insert(path, procDir);
        }
        else
        {
            // Try processors
Pout<< "** 2trying " << path/processorsBaseDir << endl;

            if
            (
                processorsBaseDir != procDir
             && isDir(path/processorsBaseDir)
            )
            {
                procsDirs_.insert(path, processorsBaseDir);
            }
        }


        word collatedProcDir(processorsBaseDir+Foam::name(n));
Pout<< "** 3trying " << path/collatedProcDir << endl;
        if
        (
           //!procsDir_.valid()
            collatedProcDir != procDir
         && isDir(path/collatedProcDir)
        )
        {
            //procsDir_.reset(new fileName(collatedProcDir));
            procsDirs_.insert(path, collatedProcDir);
        }


        // Nothing detected. Leave for next time round.
        //if (!procsDir_.valid())
        //{
        //    procsDir_.reset(new fileName(""));
        //}

        if (debug)
        {
            if (procsDirs_.found(path))
            {
                Pout<< "fileOperation::cacheProcessorsPath : Detected:"
                    << procsDirs_[path] << endl;
            }
            else
            {
                Pout<< "fileOperation::cacheProcessorsPath :"
                    << " Did not detect any processors dir for fName:"
                    << fName << endl;
            }
        }
    }
}


Foam::fileName Foam::fileOperation::lookupProcessorsPath
(
    const fileName& fName
) const
{
    fileName path;
    fileName local;
    label gStart;
    label gSz;
    label numProcs;
    label proci = splitProcessorPath(fName, path, local, gStart, gSz, numProcs);

    fileName procDir;
    if (proci != -1)
    {
        HashTable<fileName>::const_iterator iter = procsDirs_.find(path);

        if (iter != procsDirs_.end())
        {
            procDir = iter();
        }
    }
    return procDir;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fileOperation::fileOperation(label comm)
:
    comm_(comm)
    //procsDir_(nullptr)
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
    if (debug)
    {
        Pout<< "fileOperation::filePath :" << " fName:" << fName << endl;
    }

    fileName path;
    fileName local;
    label gStart;
    label gSz;
    label numProcs;
    label proci = splitProcessorPath(fName, path, local, gStart, gSz, numProcs);

    if (numProcs != -1)
    {
        WarningInFunction << "Filename is already adapted:" << fName << endl;
    }

    // Give preference to processors variant
    if (proci != -1)
    {
        // Read directories to see actual name of processor directory
        cacheProcessorsPath(fName);

//         label n = Pstream::nProcs();
//         if (!Pstream::parRun())
//         {
//             // E.g. checkMesh -case processor0
//             // Can be slow since only called at startup?
//             n = nProcs(path, local);
// 
//             // Set number of processors
//             if (n > 0)
//             {
//                 const_cast<fileOperation&>(*this).setNProcs(n);
//             }
//         }
// 
//         // Get actual output directory name
//         const word procDir(processorsDir(fName));
// 
//         // Processor-local file name
//         fileName namedProcsName(path/procDir/local);
// 
//         if (exists(namedProcsName))
//         {
//             if (debug)
//             {
//                 Pout<< "fileOperation::filePath : " << namedProcsName
//                     << endl;
//             }
//             return namedProcsName;
//         }
//         // Try processors
//         if (processorsBaseDir != procDir)
//         {
//             fileName procsName(path/processorsBaseDir/local);
//             if (exists(procsName))
//             {
//                 if (debug)
//                 {
//                     Pout<< "fileOperation::filePath :" << procsName << endl;
//                 }
//                 return procsName;
//             }
//         }
//         // Try processorsDDD
//         word collatedProcDir(processorsBaseDir+Foam::name(n));
//         if (collatedProcDir != procDir)
//         {
//             fileName procsName(path/collatedProcDir/local);
//             if (exists(procsName))
//             {
//                 if (debug)
//                 {
//                     Pout<< "fileOperation::filePath :" << procsName << endl;
//                 }
//                 return procsName;
//             }
//         }

        // Try (cached) collated file name
        fileName procDir(lookupProcessorsPath(fName));
        if (!procDir.empty())
        {
            fileName collatedName(path/procDir/local);
            if (exists(collatedName))
            {
                if (debug)
                {
                    Pout<< "fileOperation::filePath : " << collatedName << endl;
                }
                return collatedName;
            }
        }
    }

    if (exists(fName))
    {
        if (debug)
        {
            Pout<< "fileOperation::filePath : " << fName << endl;
        }
        return fName;
    }
    else
    {
        if (debug)
        {
            Pout<< "fileOperation::filePath : Not found" << endl;
        }
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
        Pout<< "fileOperation::findTimes : Finding times in directory "
            << directory << endl;
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


    // Check for any collated processors directories
    cacheProcessorsPath(directory);

    fileName procDir(lookupProcessorsPath(directory));
    if (!procDir.empty())
    {
        fileName collDir(processorsPath(directory, procDir));
        if (!collDir.empty() && collDir != directory)
        {
            fileNameList extraEntries
            (
                Foam::readDir
                (
                    collDir,
                    fileName::DIRECTORY
                )
            );
            mergeTimes
            (
                sortTimes(extraEntries, constantName),
                constantName,
                times
            );
        }
    }

//     // Check if directory is processorsDDD
//     fileName namedProcsDir
//      (processorsPath(directory, processorsDir(directory)));
// 
//     if (!namedProcsDir.empty() && namedProcsDir != directory)
//     {
//         fileNameList extraEntries
//         (
//             Foam::readDir
//             (
//                 namedProcsDir,
//                 fileName::DIRECTORY
//             )
//         );
//         mergeTimes
//         (sortTimes(extraEntries, constantName), constantName, times);
//     }
// 
//     // Check if directory is processors
//     fileName unnamedProcsDir(processorsPath(directory, processorsBaseDir));
// 
//     if (!unnamedProcsDir.empty() && namedProcsDir != unnamedProcsDir)
//     {
//         fileNameList extraEntries
//         (
//             Foam::readDir
//             (
//                 unnamedProcsDir,
//                 fileName::DIRECTORY
//             )
//         );
//         mergeTimes
//         (sortTimes(extraEntries, constantName), constantName, times);
//     }
// 
//     // Check if directory is processorsDDD
//     label n = Pstream::nProcs();
//     if (!Pstream::parRun() && detectProcessorPath(directory) != -1)
//     {
//         // E.g. checkMesh -case processor0
//         // Can be slow since only called at startup?
//         n = nProcs(directory, "");
// 
//         // Set number of processors
//         if (n > 0)
//         {
//             const_cast<fileOperation&>(*this).setNProcs(n);
//         }
//     }
// 
//     fileName collatedProcsDir
//     (
//         processorsPath
//         (
//             directory,
//             processorsBaseDir+Foam::name(n)
//         )
//     );
// 
//     if (!collatedProcsDir.empty() && collatedProcsDir != namedProcsDir)
//     {
//         fileNameList extraEntries
//         (
//             Foam::readDir
//             (
//                 collatedProcsDir,
//                 fileName::DIRECTORY
//             )
//         );
//         mergeTimes
//         (sortTimes(extraEntries, constantName), constantName, times);
//     }

    if (debug)
    {
        Pout<< "fileOperation::findTimes : Found times:" << times << endl;
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
    if (Pstream::master(comm_))
    {
        fileNameList dirNames(Foam::readDir(dir, fileName::Type::DIRECTORY));

        // Detect any processorsDDD or processorDDD
        label maxProc = -1;
        forAll(dirNames, i)
        {
            fileName path, local;
            label start, size, n;
            maxProc = max
            (
                maxProc,
                splitProcessorPath(dirNames[i], path, local, start, size, n)
            );
            if (n != -1)
            {
                // Direct detection of processorsDDD
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
    Pstream::scatter(nProcs, Pstream::msgType(), comm_);
    return nProcs;
}


Foam::fileName Foam::fileOperation::processorsCasePath
(
    const IOobject& io,
    const word& procsDir
) const
{
    return io.rootPath()/io.time().globalCaseName()/procsDir;
}


Foam::fileName Foam::fileOperation::processorsPath
(
    const IOobject& io,
    const word& instance,
    const word& procsDir
) const
{
    return
        processorsCasePath(io, procsDir)
       /instance
       /io.db().dbDir()
       /io.local();
}


Foam::fileName Foam::fileOperation::processorsPath
(
    const fileName& dir,
    const word& procsDir
) const
{
    // Check if directory is processorDDD
    word caseName(dir.name());

    std::string::size_type pos = caseName.find("processor");
    if (pos == 0)
    {
        if (caseName.size() <= 9 || caseName[9] == 's')
        {
            WarningInFunction << "Directory " << dir
                << " does not end in old-style processorDDD" << endl;
        }

        return dir.path()/procsDir;
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

    label& groupStart,
    label& groupSize,

    label& nProcs
)
{
    path.clear();
    local.clear();

    // Potentially detected start of number of processors in local group
    groupStart = -1;
    groupSize = 0;

    // Potentially detected number of processors
    nProcs = -1;

    // Search for processor at start of line or /processor
    std::string::size_type pos = objectPath.find("processor");
    if (pos == string::npos)
    {
        return -1;
    }

    // "processorDDD"
    // "processorsNNN"
    // "processorsNNN_AA-BB"


    if (pos > 0 && objectPath[pos-1] != '/')
    {
        // Directory not starting with "processor" e.g. "somenamewithprocessor"
        return -1;
    }

    // Strip leading directory
    if (pos > 0)
    {
        path = objectPath.substr(0, pos-1);
    }
    fileName f(objectPath.substr(pos+9));

    // Strip trailing local directory
    pos = f.find('/');
    if (pos != string::npos)
    {
        local = f.substr(pos+1);
        f = f.substr(0, pos);
    }

    if (f.size() && f[0] == 's')
    {
        // "processsorsNNN"

        f = f.substr(1);

        // Detect "processorsNNN_AA-BB"
        {
            std::string::size_type fromStart = f.find("_");
            std::string::size_type toStart = f.find("-");
            if (fromStart != string::npos && toStart != string::npos)
            {
                string nProcsName(f.substr(0, fromStart));
                string fromName(f.substr(fromStart+1, toStart-(fromStart+1)));
                string toName(f.substr(toStart+1));

                label groupEnd = -1;
                if
                (
                    Foam::read(fromName.c_str(), groupStart)
                 && Foam::read(toName.c_str(), groupEnd)
                 && Foam::read(nProcsName.c_str(), nProcs)
                )
                {
                    groupSize = groupEnd-groupStart+1;
                    return -1;
                }
            }
        }

        // Detect "processorsN"
        label n;
        if (Foam::read(f.c_str(), n))
        {
            nProcs = n;
        }
        return -1;
    }
    else
    {
        // Detect "processorN"
        label proci;
        if (Foam::read(f.c_str(), proci))
        {
            return proci;
        }
        else
        {
            return -1;
        }
    }
}


Foam::label Foam::fileOperation::detectProcessorPath(const fileName& fName)
{
    fileName path, local;
    label start, size, nProcs;
    return splitProcessorPath(fName, path, local, start, size, nProcs);
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
