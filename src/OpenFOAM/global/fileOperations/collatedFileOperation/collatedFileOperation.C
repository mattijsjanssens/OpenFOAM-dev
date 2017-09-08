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

#include "collatedFileOperation.H"
#include "addToRunTimeSelectionTable.H"
#include "Pstream.H"
#include "Time.H"
#include "threadedCollatedOFstream.H"
#include "decomposedBlockData.H"
#include "registerSwitch.H"
#include "masterOFstream.H"
#include "OFstream.H"
#include "polyMesh.H"

/* * * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * */

namespace Foam
{
namespace fileOperations
{
    defineTypeNameAndDebug(collatedFileOperation, 0);
    addToRunTimeSelectionTable
    (
        fileOperation,
        collatedFileOperation,
        word
    );

    float collatedFileOperation::maxThreadFileBufferSize
    (
        debug::floatOptimisationSwitch("maxThreadFileBufferSize", 1e9)
    );
    registerOptSwitch
    (
        "maxThreadFileBufferSize",
        float,
        collatedFileOperation::maxThreadFileBufferSize
    );

    word collatedFileOperation::processorsDir = "processors";
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::fileOperations::collatedFileOperation::appendObject
(
    const regIOobject& io,
    const fileName& pathName,
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp
) const
{
    // Append to processors/ file

    fileName prefix;
    fileName postfix;
    label proci = splitProcessorPath(io.objectPath(), prefix, postfix);

    if (debug)
    {
        Pout<< "writeObject:" << " : For local object : "
            << io.name()
            << " appending processor " << proci
            << " data to " << pathName << endl;
    }

    if (proci == -1)
    {
        FatalErrorInFunction
            << "Not a valid processor path " << pathName
            << exit(FatalError);
    }


    // Create string from all data to write
    string buf;
    {
        OStringStream os(fmt, ver);
        if (proci == 0)
        {
            if (!io.writeHeader(os))
            {
                return false;
            }
        }

        // Write the data to the Ostream
        if (!io.writeData(os))
        {
            return false;
        }

        if (proci == 0)
        {
            IOobject::writeEndDivider(os);
        }

        buf = os.str();
    }


    bool append = (proci > 0);

    // Note: cannot do append + compression. This is a limitation
    // of ogzstream (or rather most compressed formats)

    OFstream os
    (
        pathName,
        IOstream::BINARY,
        ver,
        IOstream::UNCOMPRESSED, // no compression
        append
    );

    if (!os.good())
    {
        FatalIOErrorInFunction(os)
            << "Cannot open for appending"
            << exit(FatalIOError);
    }

    if (proci == 0)
    {
        IOobject::writeBanner(os)
            << "FoamFile\n{\n"
            << "    version     " << os.version() << ";\n"
            << "    format      " << os.format() << ";\n"
            << "    class       " << decomposedBlockData::typeName
            << ";\n"
            << "    location    " << pathName << ";\n"
            << "    object      " << pathName.name() << ";\n"
            << "}" << nl;
        IOobject::writeDivider(os) << nl;
    }

    // Write data
    UList<char> slice
    (
        const_cast<char*>(buf.data()),
        label(buf.size())
    );
    os << nl << "// Processor" << proci << nl << slice << nl;

    return os.good();
}


Foam::word
Foam::fileOperations::collatedFileOperation::findInstancePath
(
    const instantList& timeDirs,
    const instant& t
)
{
    // Note:
    // - times will include constant (with value 0) as first element.
    //   For backwards compatibility make sure to find 0 in preference
    //   to constant.
    // - list is sorted so could use binary search

    forAllReverse(timeDirs, i)
    {
        if (t.equal(timeDirs[i].value()))
        {
            return timeDirs[i].name();
        }
    }

    return word::null;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fileOperations::collatedFileOperation::collatedFileOperation
(
    const bool verbose
)
:
    masterUncollatedFileOperation(false),
    writer_(maxThreadFileBufferSize)
{
    if (verbose)
    {
        Info<< "I/O    : " << typeName
            << " (maxThreadFileBufferSize " << maxThreadFileBufferSize
            << ')' << endl;

        if (maxThreadFileBufferSize == 0)
        {
            Info<< "         Threading not activated "
                   "since maxThreadFileBufferSize = 0." << nl
                << "         Writing may run slowly for large file sizes."
                << endl;
        }
        else
        {
            Info<< "         Threading activated "
                   "since maxThreadFileBufferSize > 0." << nl
                << "         Requires thread support enabled in MPI, "
                   "otherwise the simulation" << nl
                << "         may \"hang\".  If thread support cannot be "
                   "enabled, deactivate threading" << nl
                << "         by setting maxThreadFileBufferSize to 0 in "
                   "$FOAM_ETC/controlDict"
                << endl;
        }

        if
        (
            regIOobject::fileModificationChecking
         == regIOobject::inotifyMaster
        )
        {
            WarningInFunction
                << "Resetting fileModificationChecking to inotify" << endl;
        }

        if
        (
            regIOobject::fileModificationChecking
         == regIOobject::timeStampMaster
        )
        {
            WarningInFunction
                << "Resetting fileModificationChecking to timeStamp" << endl;
        }
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::fileOperations::collatedFileOperation::~collatedFileOperation()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

//XXXXXXX
Foam::fileName Foam::fileOperations::collatedFileOperation::filePathInfo
(
    const bool checkGlobal,
    const bool isFile,
    const IOobject& io,
    const HashPtrTable<instantList>& times,
    pathType& searchType,
    word& newInstancePath
)
{
    newInstancePath = word::null;

    if (io.instance().isAbsolute())
    {
        fileName objectPath = io.instance()/io.name();

        if (isFileOrDir(isFile, objectPath))
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
        if (io.time().processorCase())
        {
            fileName objectPath = processorsFilePath(isFile, io, io.instance());
            if (!objectPath.empty())
            {
                searchType = fileOperation::PROCESSORSOBJECT;
                return objectPath;
            }
        }
        {
            // 2. Check local
            fileName localObjectPath = io.objectPath();

            if (isFileOrDir(isFile, localObjectPath))
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

            if (isFileOrDir(isFile, parentObjectPath))
            {
                searchType = fileOperation::PARENTOBJECT;
                return parentObjectPath;
            }
        }

        // Check for approximately same time. E.g. if time = 1e-2 and
        // directory is 0.01 (due to different time formats)
        HashPtrTable<instantList>::const_iterator pathFnd
        (
            times.find
            (
                io.time().path()
            )
        );
        if (pathFnd != times.end())
        {
            newInstancePath = findInstancePath
            (
                *pathFnd(),
                instant(io.instance())
            );

            if (newInstancePath.size())
            {
                // 1. Try processors equivalent

                fileName fName
                (
                    processorsFilePath
                    (
                        isFile,
                        io,
                        newInstancePath
                    )
                );
                if (!fName.empty())
                {
                    searchType = fileOperation::PROCESSORSFINDINSTANCE;
                    return fName;
                }

                fName =
                    io.rootPath()/io.caseName()
                   /newInstancePath/io.db().dbDir()/io.local()/io.name();

                if (isFileOrDir(isFile, fName))
                {
                    searchType = fileOperation::FINDINSTANCE;
                    return fName;
                }
            }
        }

        searchType = fileOperation::NOTFOUND;
        return fileName::null;
    }
}


Foam::fileName
Foam::fileOperations::collatedFileOperation::processorsPath
(
    const IOobject& io,
    const word& instance,
    const word& processorsDir
)
{
    return
        io.rootPath()
       /io.time().globalCaseName()
       /processorsDir
       /instance
       /io.db().dbDir()
       /io.local();
}
Foam::fileName
Foam::fileOperations::collatedFileOperation::processorsPath
(
    const fileName& dir,
    const word& processorsDir
)
{
    // Check if directory is processorXXX
    word caseName(dir.name());

    std::string::size_type pos = caseName.find("processor");
    if (pos == 0)
    {
        return dir.path()/processorsDir;
    }
    else
    {
        return fileName::null;
    }
}
// Check if processorsXXX/fName exists. Rewrites 'processorDDD/' to
// - processors/
// - processorsNNN/
// and checks for it
Foam::fileName
Foam::fileOperations::collatedFileOperation::processorsFilePath
(
    const bool isFile,
    const fileName& fName
)
{
    fileName path;
    fileName local;
    label proci = splitProcessorPath(fName, path, local);

    // Give preference to processors variant
    if (proci != -1)
    {
        // Check processors/
        fileName procsName(path/processorsDir/local);
        if (isFileOrDir(isFile, procsName))
        {
            return procsName;
        }

        // Check for directory starting with processors/
        fileNameList dirs(Foam::readDir(path, fileName::DIRECTORY));
        forAll(dirs, i)
        {
            std::string::size_type pos = dirs[i].find(processorsDir);
            if (pos == 0)
            {
                procsName = path/dirs[i]/local;
                if (isFileOrDir(isFile, procsName))
                {
                    return procsName;
                }
            }
        }
    }

    return fileName::null;
}
Foam::fileName
Foam::fileOperations::collatedFileOperation::processorsFilePath
(
    const bool isFile,
    const IOobject& io,
    const word& instance
)
{
    // Check processors/ directory
    fileName objectPath =
        processorsPath(io, instance, processorsDir)/io.name();
    if (isFileOrDir(isFile, objectPath))
    {
        return objectPath;
    }


    // Check processorsDDD/ directory
    const fileName path(io.rootPath()/io.time().globalCaseName());
    fileNameList dirs(Foam::readDir(path, fileName::DIRECTORY));
    forAll(dirs, i)
    {
        std::string::size_type pos = dirs[i].find(processorsDir);
        if (pos == 0)
        {
            fileName objectPath =
                processorsPath(io, instance, dirs[i])/io.name();
            if (isFileOrDir(isFile, objectPath))
            {
                return objectPath;
            }
        }
    }
    return fileName::null;
}
Foam::autoPtr<Foam::ISstream>
Foam::fileOperations::collatedFileOperation::readProcStream
(
    IOobject& io,
    Istream& is
)
{
    // Analyse the objectpath to find out the processor we're trying
    // to access
    fileName path;
    fileName local;
    label proci = splitProcessorPath(io.objectPath(), path, local);

    if (proci == -1)
    {
        FatalIOErrorInFunction(is)
            << "Could not detect processor number"
            << " from objectPath:" << io.objectPath()
            << exit(FatalIOError);
    }

    return decomposedBlockData::readBlock(proci, is, io);
}
Foam::fileName Foam::fileOperations::collatedFileOperation::objectPath
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
            return processorsPath(io, io.instance(), processorsDir)/io.name();
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
            return processorsPath(io, instancePath, processorsDir)/io.name();
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
//XXXXXXX

Foam::label Foam::fileOperations::collatedFileOperation::numProcs
(
    const fileName& dir,
    const fileName& local
)
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

    if (Pstream::parRun())
    {
        if (Foam::isDir(dir/processorsDir+Foam::name(Pstream::nProcs())))
        {
            return Pstream::nProcs();
        }
    }


    // Look for processorsDDD
    {
        fileNameList dirs(Foam::readDir(dir, fileName::DIRECTORY));
        label nProcs = 0;
        forAll(dirs, i)
        {
            // Look for processorsDDD
            std::string::size_type pos = dirs[i].find(processorsDir);
            if (pos == 0 && dirs[i] != processorsDir)
            {
                string num(dirs[i].substr(pos, pos+sizeof(processorsDir)));
                char* endPtr;
                nProcs = strtol(num.c_str(), &endPtr, 10);
                if (*endPtr == '\0')
                {
                    return nProcs;
                }
            }

            // Look for max processorDDD
            pos = dirs[i].find("processor");
            if (pos == 0)
            {
                string num(dirs[i].substr(pos, pos+sizeof("processor")));

                char* endPtr;
                nProcs = strtol(num.c_str(), &endPtr, 10);
                if (*endPtr == '\0')
                {
                    return nProcs;
                }
            }
        }
    }

    return 0;
}


Foam::fileName Foam::fileOperations::collatedFileOperation::objectPath
(
    const IOobject& io,
    const word& typeName
) const
{
    // Replacement for objectPath
    if (io.time().processorCase())
    {
        return objectPath(io, fileOperation::PROCESSORSOBJECT, io.instance());
    }
    else
    {
        return objectPath(io, fileOperation::OBJECT, io.instance());
    }
}


bool Foam::fileOperations::collatedFileOperation::writeObject
(
    const regIOobject& io,
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp,
    const bool valid
) const
{
    const Time& tm = io.time();
    const fileName& inst = io.instance();

    if (inst.isAbsolute() || !tm.processorCase())
    {
        mkDir(io.path());
        fileName pathName(io.objectPath());

        if (debug)
        {
            Pout<< "writeObject:"
                << " : For object : " << io.name()
                << " falling back to master-only output to " << io.path()
                << endl;
        }

        masterOFstream os
        (
            pathName,
            fmt,
            ver,
            cmp,
            false,
            valid
        );

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
    else
    {
        // Construct the equivalent processors/ directory
        fileName path(processorsPath(io, inst, processorsDir));

        mkDir(path);
        fileName pathName(path/io.name());

        if (io.global())
        {
            if (debug)
            {
                Pout<< "writeObject:" << " : For global object : " << io.name()
                    << " falling back to master-only output to " << pathName
                    << endl;
            }

            masterOFstream os
            (
                pathName,
                fmt,
                ver,
                cmp,
                false,
                valid
            );

            // If any of these fail, return (leave error handling to Ostream
            // class)
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
        else if (!Pstream::parRun())
        {
            // Special path for e.g. decomposePar. Append to
            // processors/ file
            if (debug)
            {
                Pout<< "writeObject:"
                    << " : For object : " << io.name()
                    << " appending to " << pathName << endl;
            }

            return appendObject(io, pathName, fmt, ver, cmp);
        }
        else
        {
            if (debug)
            {
                Pout<< "writeObject:"
                    << " : For object : " << io.name()
                    << " starting collating output to " << pathName << endl;
            }

            threadedCollatedOFstream os(writer_, pathName, fmt, ver, cmp);

            // If any of these fail, return (leave error handling to Ostream
            // class)
            if (!os.good())
            {
                return false;
            }
            if (Pstream::master() && !io.writeHeader(os))
            {
                return false;
            }
            // Write the data to the Ostream
            if (!io.writeData(os))
            {
                return false;
            }
            if (Pstream::master())
            {
                IOobject::writeEndDivider(os);
            }

            return true;
        }
    }
}


// ************************************************************************* //
