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

#include "masterFileOperation.H"
#include "addToRunTimeSelectionTable.H"
#include "Pstream.H"
#include "Time.H"
#include "instant.H"
#include "IFstream.H"
#include "masterOFstream.H"
#include "decomposedBlockData.H"

/* * * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * */

namespace Foam
{
namespace fileOperations
{
    defineTypeNameAndDebug(masterFileOperation, 0);
    addToRunTimeSelectionTable(fileOperation, masterFileOperation, word);
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::fileName Foam::fileOperations::masterFileOperation::filePath
(
    const bool checkGlobal,
    const IOobject& io,
    pathType& searchType,
    word& newInstancePath
)
{
    newInstancePath = word::null;

    if (io.instance().isAbsolute())
    {
        fileName objectPath = io.instance()/io.name();
        if (Foam::isFile(objectPath))
        {
            searchType = fileOperation::ABSOLUTE;
            return objectPath;
        }
        else
        {
            searchType = fileOperation::NOTFOUND;
            return fileName::null;
        }
    }
    else
    {
        // 1. Check processors/
        fileName path = processorsPath(io, io.instance());
        fileName objectPath = path/io.name();
        if (Foam::isFile(objectPath))
        {
            searchType = fileOperation::PROCESSORSOBJECT;
            return objectPath;
        }
        else
        {
            // 2. Check local
            fileName localObjectPath = io.path()/io.name();

            if (Foam::isFile(localObjectPath))
            {
                searchType = fileOperation::OBJECT;
                return localObjectPath;
            }
        }



        // Any global checks
        if
        (
            checkGlobal
         && io.time().processorCase()
         && (
                io.instance() == io.time().system()
             || io.instance() == io.time().constant()
            )
        )
        {
            fileName parentObjectPath =
                io.rootPath()/io.time().globalCaseName()
               /io.instance()/io.db().dbDir()/io.local()/io.name();

            if (Foam::isFile(parentObjectPath))
            {
                searchType = fileOperation::PARENTOBJECT;
                return parentObjectPath;
            }
        }

        return fileName::null;
    }
}


Foam::fileName Foam::fileOperations::masterFileOperation::processorsCasePath
(
    const IOobject& io
)
{
    return
        io.rootPath()
       /io.time().globalCaseName()
       /"processors";
}


Foam::fileName Foam::fileOperations::masterFileOperation::processorsPath
(
    const IOobject& io,
    const word& instance
)
{
    return
        processorsCasePath(io)
       /instance
       /io.db().dbDir()
       /io.local();
}


Foam::fileName Foam::fileOperations::masterFileOperation::processorsPath
(
    const fileName& dir
)
{
    // Check if directory is processorXXX
    word caseName(dir.name());

    std::string::size_type pos = caseName.find("processor");
    if (pos == 0)
    {
        return dir.path()/"processors";
    }
    else
    {
        return fileName::null;
    }
}


Foam::label Foam::fileOperations::masterFileOperation::splitProcessorPath
(
    const fileName& objectPath,
    fileName& path,
    fileName& local
)
{
    std::string::size_type pos = objectPath.find("/processor");
    if (pos == string::npos)
    {
        return -1;
    }

    path = objectPath.substr(0, pos);

    local = objectPath.substr(pos+10);

    pos = local.find('/');

    if (pos == string::npos)
    {
        return -1;
    }
    string procName(local.substr(0, pos));
    label proci;
    if (read(procName.c_str(), proci))
    {
        local = local.substr(pos+1);
        return proci;
    }
    return -1;
}


Foam::fileName Foam::fileOperations::masterFileOperation::objectPath
(
    const IOobject& io,
    const pathType& searchType,
    const word& instancePath
)
{
    // Replacement for IOobject::objectPath()

    switch (searchType)
    {
        case fileOperation::ABSOLUTE:
        {
            return io.instance()/io.name();
        }
        break;

        case fileOperation::OBJECT:
        {
            return io.path()/io.name();
        }
        break;

        case fileOperation::PROCESSORSOBJECT:
        {
            return processorsPath(io, io.instance())/io.name();
        }
        break;

        case fileOperation::PARENTOBJECT:
        {
            return
                io.rootPath()/io.time().globalCaseName()
               /io.instance()/io.db().dbDir()/io.local()/io.name();
        }
        break;

        case fileOperation::FINDINSTANCE:
        {
            return
                io.rootPath()/io.caseName()
               /instancePath/io.db().dbDir()/io.local()/io.name();
        }
        break;

        case fileOperation::PROCESSORSFINDINSTANCE:
        {
            return processorsPath(io, instancePath)/io.name();
        }
        break;

        case fileOperation::NOTFOUND:
        {
            return fileName::null;
        }
        break;

        default:
        {
            NotImplemented;
            return fileName::null;
        }
    }
}


bool Foam::fileOperations::masterFileOperation::uniformFile
(
    const fileNameList& filePaths
)
{
    const fileName& object0 = filePaths[0];

    for (label i = 1; i < filePaths.size(); i++)
    {
        if (filePaths[i] != object0)
        {
            return false;
        }
    }
    return true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fileOperations::masterFileOperation::masterFileOperation()
{
    if (regIOobject::fileModificationChecking == regIOobject::timeStampMaster)
    {
        WarningInFunction
            << "Resetting fileModificationChecking to timeStamp" << endl;
        regIOobject::fileModificationChecking = regIOobject::timeStamp;
    }
    else if
    (
        regIOobject::fileModificationChecking
     == regIOobject::inotifyMaster
    )
    {
        WarningInFunction
            << "Resetting fileModificationChecking to inotifyMaster" << endl;
        regIOobject::fileModificationChecking = regIOobject::inotifyMaster;
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::fileOperations::masterFileOperation::~masterFileOperation()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::fileOperations::masterFileOperation::mkDir
(
    const fileName& dir,
    mode_t mode
) const
{
    return masterOp<mode_t, mkDirOp>(dir, mkDirOp(mode));
}


bool Foam::fileOperations::masterFileOperation::chMod
(
    const fileName& fName,
    mode_t mode
) const
{
    return masterOp<mode_t, chModOp>(fName, chModOp(mode));
}


mode_t Foam::fileOperations::masterFileOperation::mode
(
    const fileName& fName
) const
{
    return masterOp<mode_t, modeOp>(fName, modeOp());
}


Foam::fileName::Type Foam::fileOperations::masterFileOperation::type
(
    const fileName& fName
) const
{
    return fileName::Type(masterOp<label, typeOp>(fName, typeOp()));
}


bool Foam::fileOperations::masterFileOperation::exists
(
    const fileName& fName,
    const bool checkGzip
) const
{
    return masterOp<bool, existsOp>(fName, existsOp(checkGzip));
}


bool Foam::fileOperations::masterFileOperation::isDir
(
    const fileName& fName
) const
{
    return masterOp<bool, isDirOp>(fName, isDirOp());
}


bool Foam::fileOperations::masterFileOperation::isFile
(
    const fileName& fName,
    const bool checkGzip
) const
{
    return masterOp<bool, isFileOp>(fName, isFileOp());
}


off_t Foam::fileOperations::masterFileOperation::fileSize
(
    const fileName& fName
) const
{
    return masterOp<off_t, fileSizeOp>(fName, fileSizeOp());
}


time_t Foam::fileOperations::masterFileOperation::lastModified
(
    const fileName& fName
) const
{
    return masterOp<time_t, lastModifiedOp>(fName, lastModifiedOp());
}


double Foam::fileOperations::masterFileOperation::highResLastModified
(
    const fileName& fName
) const
{
    return masterOp<double, lastModifiedHROp>
    (
        fName,
        lastModifiedHROp()
    );
}


bool Foam::fileOperations::masterFileOperation::mvBak
(
    const fileName& fName,
    const std::string& ext
) const
{
    return masterOp<bool, mvBakOp>(fName, mvBakOp(ext));
}


bool Foam::fileOperations::masterFileOperation::rm(const fileName& fName) const
{
    return masterOp<bool, rmOp>(fName, rmOp());
}


bool Foam::fileOperations::masterFileOperation::rmDir
(
    const fileName& dir
) const
{
    return masterOp<bool, rmDirOp>(dir, rmDirOp());
}


Foam::fileNameList Foam::fileOperations::masterFileOperation::readDir
(
    const fileName& dir,
    const fileName::Type type,
    const bool filtergz
) const
{
    return masterOp<fileNameList, readDirOp>
    (
        dir,
        readDirOp(type, filtergz)
    );
}


bool Foam::fileOperations::masterFileOperation::cp
(
    const fileName& src,
    const fileName& dst
) const
{
    return masterOp<bool, cpOp>(src, dst, cpOp());
}


bool Foam::fileOperations::masterFileOperation::ln
(
    const fileName& src,
    const fileName& dst
) const
{
    return masterOp<bool, lnOp>(src, dst, lnOp());
}


bool Foam::fileOperations::masterFileOperation::mv
(
    const fileName& src,
    const fileName& dst
) const
{
    return masterOp<bool, mvOp>(src, dst, mvOp());
}


Foam::fileName Foam::fileOperations::masterFileOperation::filePath
(
    const bool checkGlobal,
    const IOobject& io
) const
{
    if (debug)
    {
        Pout<< "masterFileOperation::filePath :"
            << " objectPath:" << io.objectPath()
            << " checkGlobal:" << checkGlobal << endl;
    }

    fileName objPath;
    pathType searchType = fileOperation::NOTFOUND;
    word newInstancePath;
    if (Pstream::master())
    {
        objPath = filePath(checkGlobal, io, searchType, newInstancePath);
    }
    label masterType(searchType);
    Pstream::scatter(masterType);
    searchType = pathType(masterType);

    if
    (
        searchType == fileOperation::FINDINSTANCE
     || searchType == fileOperation::PROCESSORSFINDINSTANCE
    )
    {
        Pstream::scatter(newInstancePath);
    }

    if (!Pstream::master())
    {
        objPath = objectPath(io, searchType, newInstancePath);
    }

    if (debug)
    {
        Pout<< "masterFileOperation::filePath :"
            << " Returning from file searching:" << endl
            << "    objectPath:" << io.objectPath() << endl
            << "    filePath  :" << objPath << endl << endl;
    }
    return objPath;
}


Foam::fileNameList Foam::fileOperations::masterFileOperation::readObjects
(
    const objectRegistry& db,
    const fileName& instance,
    const fileName& local,
    word& newInstance
) const
{
    if (debug)
    {
        Pout<< "masterFileOperation::readObjects :"
            << " db:" << db.objectPath()
            << " instance:" << instance << endl;
    }

    fileNameList objectNames;
    newInstance = word::null;

    if (Pstream::master())
    {
        //- Use non-time searching version
        objectNames = fileOperation::readObjects
        (
            db,
            instance,
            local,
            newInstance
        );

        if (newInstance.empty())
        {
            // Find similar time

            // Copy of Time::findInstancePath. We want to avoid the
            // parallel call to findTimes. Alternative is to have
            // version of findInstancePath that takes instantList ...
            const instantList timeDirs
            (
                fileOperation::findTimes
                (
                    db.time().path(),
                    db.time().constant()
                )
            );

            const instant t(instance);
            forAllReverse(timeDirs, i)
            {
                if (t.equal(timeDirs[i].value()))
                {
                    objectNames = fileOperation::readObjects
                    (
                        db,
                        timeDirs[i].name(),     // newly found time
                        local,
                        newInstance
                    );
                    break;
                }
            }
        }
    }

    Pstream::scatter(newInstance);
    Pstream::scatter(objectNames);

    if (debug)
    {
        Pout<< "masterFileOperation::readObjects :"
            << " newInstance:" << newInstance
            << " objectNames:" << objectNames << endl;
    }

    return objectNames;
}


bool Foam::fileOperations::masterFileOperation::readHeader
(
    IOobject& io,
    const fileName& fName
) const
{
    bool ok = false;

    if (debug)
    {
        Pout<< "masterFileOperation::readHeader:" << " fName:" << fName << endl;
    }

    if (Pstream::master())
    {
        if (!fName.empty() && Foam::isFile(fName))
        {
            IFstream is(fName);

            if (is.good())
            {
                ok = io.readHeader(is);

                if (io.headerClassName() == decomposedBlockData::typeName)
                {
                    // Read the header inside the container (master data)
                    ok = decomposedBlockData::readMasterHeader(io, is);
                }
            }
        }
    }
    Pstream::scatter(ok);
    Pstream::scatter(io.headerClassName());
    Pstream::scatter(io.note());

    if (debug)
    {
        Pout<< "masterFileOperation::readHeader:" << " ok:" << ok
            << " class:" << io.headerClassName() << endl;
    }
    return ok;
}


Foam::autoPtr<Foam::Istream>
Foam::fileOperations::masterFileOperation::readStream
(
    regIOobject& io,
    const fileName& fName
) const
{
    if (!fName.size())
    {
        FatalErrorInFunction
            << "empty file name" << exit(FatalError);
    }

    autoPtr<Istream> isPtr;
    bool isCollated = false;
    if (UPstream::master())
    {
        isPtr.reset(new IFstream(fName));

        // Read header data (on copy)
        IOobject headerIO(io);
        headerIO.readHeader(isPtr());
        if (headerIO.headerClassName() == decomposedBlockData::typeName)
        {
            isCollated = true;
        }
        else
        {
            // Close file. Reopened below.
            isPtr.clear();
        }
    }

    Pstream::scatter(isCollated);

    if (isCollated)
    {
        if (debug)
        {
            Pout<< "masterFileOperation::readHeader:"
                << " for object : " << io.name()
                << " starting collating input from " << fName << endl;
        }

        List<char> data;
        if (!Pstream::parRun())
        {
            // Analyse the objectpath to find out the processor we're trying
            // to access
            fileName path;
            fileName local;
            label proci =
                fileOperations::masterFileOperation::splitProcessorPath
                (
                    io.objectPath(),
                    path,
                    local
                );

            if (proci == -1)
            {
                FatalIOErrorInFunction(isPtr())
                    << "Could not detect processor number"
                    << " from objectPath:" << io.objectPath()
                    << exit(FatalIOError);
            }
            decomposedBlockData::readBlock(proci, isPtr(), data);
        }
        else
        {
            // Read my data
            decomposedBlockData::readBlocks(isPtr, data, UPstream::scheduled);
        }

        // TBD: remove extra copying
        string buf(data.begin(), data.size());
        autoPtr<Istream> realIsPtr(new IStringStream(buf));

        // Read header
        if (!io.readHeader(realIsPtr()))
        {
            FatalIOErrorInFunction(realIsPtr())
                << "problem while reading header for object " << io.name()
                << exit(FatalIOError);
        }

        return realIsPtr;
    }
    else
    {
        if (debug)
        {
            Pout<< "masterFileOperation::readHeader:"
                << " for object : " << io.name()
                << " starting separated input from " << fName << endl;
        }

        fileNameList filePaths(Pstream::nProcs());
        filePaths[Pstream::myProcNo()] = fName;
        Pstream::gatherList(filePaths);

        PstreamBuffers pBufs(Pstream::nonBlocking);

        if (Pstream::master())
        {
            //const bool uniform = uniformFile(filePaths);

            autoPtr<IFstream> ifsPtr(new IFstream(fName));
            IFstream& is = ifsPtr();

            // Read header
            if (!io.readHeader(is))
            {
                FatalIOErrorInFunction(is)
                    << "problem while reading header for object " << io.name()
                    << exit(FatalIOError);
            }

            // Open master (steal from ifsPtr)
            isPtr.reset(ifsPtr.ptr());

            // Read slave files
            for (label proci = 1; proci < Pstream::nProcs(); proci++)
            {
                if (debug)
                {
                    Pout<< "masterFileOperation::readHeader:"
                        << " For processor " << proci
                        << " opening " << filePaths[proci] << endl;
                }

                std::ifstream is(filePaths[proci]);
                // Get length of file
                is.seekg(0, ios_base::end);
                std::streamoff count = is.tellg();
                is.seekg(0, ios_base::beg);

                if (debug)
                {
                    Pout<< "masterFileOperation::readHeader:"
                        << " From " << filePaths[proci]
                        <<  " reading " << label(count) << " bytes" << endl;
                }
                List<char> buf(static_cast<label>(count));
                is.read(buf.begin(), count);

                UOPstream os(proci, pBufs);
                os.write(buf.begin(), count);
            }
        }

        labelList recvSizes;
        pBufs.finishedSends(recvSizes);

        // isPtr will be valid on master. Else the information is in the
        // PstreamBuffers

        if (!isPtr.valid())
        {
            UIPstream is(Pstream::masterNo(), pBufs);
            string buf(recvSizes[Pstream::masterNo()], '\0');
            is.read(&buf[0], recvSizes[Pstream::masterNo()]);

            if (debug)
            {
                Pout<< "masterFileOperation::readHeader:"
                    << " Done reading " << buf.size() << " bytes" << endl;
            }
            isPtr.reset(new IStringStream(buf));

            if (!io.readHeader(isPtr()))
            {
                FatalIOErrorInFunction(isPtr())
                    << "problem while reading header for object " << io.name()
                    << exit(FatalIOError);
            }
        }
        return isPtr;
    }
}


bool Foam::fileOperations::masterFileOperation::writeObject
(
    const regIOobject& io,
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp,
    const bool valid
) const
{
    fileName pathName(io.objectPath());

    if (debug)
    {
        Pout<< "masterFileOperation::readHeader:" << " io:" << pathName
            << " valid:" << valid << endl;
    }

    autoPtr<Ostream> osPtr
    (
        NewOFstream
        (
            pathName,
            fmt,
            ver,
            cmp,
            valid
        )
    );
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


Foam::instantList Foam::fileOperations::masterFileOperation::findTimes
(
    const fileName& directory,
    const word& constantName
) const
{
    if (debug)
    {
        Pout<< "masterFileOperation::readHeader:"
            << " Finding times in directory " << directory << endl;
    }

    instantList times;

    if (Pstream::master())
    {
        times = fileOperation::findTimes(directory, constantName);
    }
    Pstream::scatter(times);

    if (debug)
    {
        Pout<< "masterFileOperation::readHeader:"
            << " Found times:" << times << endl;
    }
    return times;
}


Foam::autoPtr<Foam::Istream>
Foam::fileOperations::masterFileOperation::NewIFstream
(
    const fileName& filePath
) const
{
    if (Pstream::parRun())
    {
        // Insert logic of filePath. We assume that if a file is absolute
        // on the master it is absolute also on the slaves etc.

        fileNameList filePaths(Pstream::nProcs());
        filePaths[Pstream::myProcNo()] = filePath;
        Pstream::gatherList(filePaths);

        PstreamBuffers pBufs(Pstream::nonBlocking);

        if (Pstream::master())
        {
            const bool uniform = uniformFile(filePaths);

            if (uniform)
            {
                if (debug)
                {
                    Pout<< "masterFileOperation::readHeader:"
                        << " Opening global file " << filePath << endl;
                }

                // get length of file:
                off_t count(Foam::fileSize(filePath));

                std::ifstream is(filePath);

                if (debug)
                {
                    Pout<< "masterFileOperation::readHeader:"
                        << " From " << filePath
                        <<  " reading " << label(count) << " bytes" << endl;
                }
                List<char> buf(static_cast<label>(count));
                is.read(buf.begin(), count);

                for (label proci = 1; proci < Pstream::nProcs(); proci++)
                {
                    UOPstream os(proci, pBufs);
                    os.write(buf.begin(), count);
                }
            }
            else
            {
                for (label proci = 1; proci < Pstream::nProcs(); proci++)
                {
                    if (debug)
                    {
                        Pout<< "masterFileOperation::readHeader:"
                            << " For processor " << proci
                            << " opening " << filePaths[proci] << endl;
                    }

                    off_t count(Foam::fileSize(filePaths[proci]));

                    std::ifstream is(filePaths[proci]);

                    if (debug)
                    {
                        Pout<< "masterFileOperation::readHeader:"
                            << " From " << filePaths[proci]
                            <<  " reading " << label(count) << " bytes" << endl;
                    }
                    List<char> buf(static_cast<label>(count));
                    is.read(buf.begin(), count);

                    UOPstream os(proci, pBufs);
                    os.write(buf.begin(), count);
                }
            }
        }


        labelList recvSizes;
        pBufs.finishedSends(recvSizes);

        if (Pstream::master())
        {
            // Read myself
            return autoPtr<Istream>
            (
                new IFstream(filePaths[Pstream::masterNo()])
            );
        }
        else
        {
            if (debug)
            {
                Pout<< "masterFileOperation::readHeader:"
                    << " Reading " << filePath
                    << " from processor " << Pstream::masterNo() << endl;
            }

            UIPstream is(Pstream::masterNo(), pBufs);
            string buf(recvSizes[Pstream::masterNo()], '\0');
            is.read(&buf[0], recvSizes[Pstream::masterNo()]);

            if (debug)
            {
                Pout<< "masterFileOperation::readHeader:"
                    << " Done reading " << buf.size() << " bytes" << endl;
            }

            // Note: IPstream is not an IStream so use a IStringStream to
            //       convert the buffer. Note that we construct with a string
            //       so it holds a copy of the buffer.
            return autoPtr<Istream>(new IStringStream(buf));
        }
    }
    else
    {
        // Read myself
        return autoPtr<Istream>(new IFstream(filePath));
    }
}


Foam::autoPtr<Foam::Ostream>
Foam::fileOperations::masterFileOperation::NewOFstream
(
    const fileName& pathName,
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp,
    const bool valid
) const
{
    return autoPtr<Ostream>(new masterOFstream(pathName, fmt, ver, cmp, valid));
}


Foam::label Foam::fileOperations::masterFileOperation::addWatch
(
    const fileName& fName
) const
{
    label watchFd;
    if (Pstream::master())
    {
        watchFd = monitor().addWatch(fName);
    }
    Pstream::scatter(watchFd);
    return watchFd;
}


bool Foam::fileOperations::masterFileOperation::removeWatch
(
    const label watchIndex
) const
{
    bool ok;
    if (Pstream::master())
    {
        ok = monitor().removeWatch(watchIndex);
    }
    Pstream::scatter(ok);
    return ok;
}


Foam::label Foam::fileOperations::masterFileOperation::findWatch
(
    const labelList& watchIndices,
    const fileName& fName
) const
{
    label index = -1;

    if (Pstream::master())
    {
        forAll(watchIndices, i)
        {
            if (monitor().getFile(watchIndices[i]) == fName)
            {
                index = i;
                break;
            }
        }
    }
    Pstream::scatter(index);
    return index;
}


void Foam::fileOperations::masterFileOperation::addWatches
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


const Foam::fileName Foam::fileOperations::masterFileOperation::getFile
(
    const label watchIndex
) const
{
    fileName fName;
    if (Pstream::master())
    {
        fName = monitor().getFile(watchIndex);
    }
    Pstream::scatter(fName);
    return fName;
}


void Foam::fileOperations::masterFileOperation::updateStates
(
    const bool masterOnly,
    const bool syncPar
) const
{
    if (Pstream::master())
    {
        monitor().updateStates(true, false);
    }
}


Foam::fileMonitor::fileState Foam::fileOperations::masterFileOperation::getState
(
    const label watchFd
) const
{
    unsigned int state = fileMonitor::UNMODIFIED;
    if (Pstream::master())
    {
        state = monitor().getState(watchFd);
    }
    Pstream::scatter(state);
    return fileMonitor::fileState(state);
}


void Foam::fileOperations::masterFileOperation::setUnmodified
(
    const label watchFd
) const
{
    if (Pstream::master())
    {
        monitor().setUnmodified(watchFd);
    }
}


// ************************************************************************* //
