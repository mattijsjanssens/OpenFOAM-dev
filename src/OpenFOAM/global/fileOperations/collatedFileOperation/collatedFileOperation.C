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
#include "OFstreamWriter.H"
#include "threadedOFstream.H"
#include "masterOFstream.H"
#include "masterCollatingOFstream.H"
#include "decomposedBlockData.H"
#include "OFstream.H"
#include "registerSwitch.H"

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

    float collatedFileOperation::maxThreadBufferSize
    (
        debug::floatOptimisationSwitch
        (
            "maxThreadBufferSize",
            masterUncollatedFileOperation::maxBufferSize
        )
    );
    registerOptSwitch
    (
        "maxThreadBufferSize",
        float,
        collatedFileOperation::maxThreadBufferSize
    );
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fileOperations::collatedFileOperation::collatedFileOperation
(
    const bool verbose
)
:
    masterUncollatedFileOperation(false),
    writeServer_(off_t(maxThreadBufferSize))
{
    if (verbose)
    {
        Info<< "I/O    : " << typeName
            << " (maxBufferSize " << maxBufferSize
            << "  maxThreadBufferSize " << maxThreadBufferSize
            << ')' << endl;
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

Foam::fileName Foam::fileOperations::collatedFileOperation::objectPath
(
    const IOobject& io
) const
{
    // Replacement for objectPath
    if (io.time().processorCase())
    {
        return masterUncollatedFileOperation::objectPath
        (
            io,
            fileOperation::PROCESSORSOBJECT,
            io.instance()
        );
    }
    else
    {
        return masterUncollatedFileOperation::objectPath
        (
            io,
            fileOperation::OBJECT,
            io.instance()
        );
    }
}


Foam::fileName Foam::fileOperations::collatedFileOperation::objectPath
(
    const fileName& fName
) const
{
    fileName path;
    fileName local;
    label proci = splitProcessorPath(fName, path, local);

    if (proci != -1)
    {
        return path/processorsDir/local;
    }
    else
    {
        return fName;
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

    // Write header (suppressed for masterCollatingOFstream slaves)
    bool writeHeader = true;

    autoPtr<Ostream> osPtr;
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
        osPtr.reset
        (
            new masterOFstream
            (
                (
                    off_t(maxThreadBufferSize) > 0
                 ? &writeServer_
                 : nullptr
                ),
                pathName,
                fmt,
                ver,
                cmp,
                false,
                valid
            )
        );
    }
    else
    {
        // Construct the equivalent processors/ directory
        fileName path(processorsPath(io, inst));

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
            osPtr.reset
            (
                new masterOFstream
                (
                    nullptr,    // Avoid thread writing
                    pathName,
                    fmt,
                    ver,
                    cmp,
                    false,
                    valid
                )
            );
        }
        else
        {
            if (!Pstream::parRun())
            {
                // Special path for e.g. decomposePar. Append to
                // processors/ file

                fileName prefix;
                fileName postfix;
                label proci = splitProcessorPath
                (
                    io.objectPath(),
                    prefix,
                    postfix
                );

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


                // Note: cannot do append + compression. This is a limitation
                // of ogzstream (or rather most compressed formats)
                bool append = (proci > 0);

                autoPtr<OSstream> osPtr;
                if (off_t(maxThreadBufferSize) > 0)
                {
                    osPtr.reset
                    (
                        new threadedOFstream
                        (
                            writeServer_,
                            pathName,
                            IOstream::BINARY,
                            ver,
                            IOstream::UNCOMPRESSED, // no compression
                            append
                        )
                    );
                }
                else
                {
                    osPtr.reset
                    (
                        new OFstream
                        (
                            pathName,
                            IOstream::BINARY,
                            ver,
                            IOstream::UNCOMPRESSED, // no compression
                            append
                        )
                    );
                }
                OSstream& os = osPtr();

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
            else
            {
                if (debug)
                {
                    Pout<< "writeObject:"
                        << " : For object : " << io.name()
                        << " starting collating output to " << path << endl;
                }

                osPtr.reset
                (
                    new masterCollatingOFstream
                    (
                        (
                            off_t(maxThreadBufferSize) > 0
                         ? &writeServer_
                         : nullptr
                        ),
                        pathName,
                        fmt,
                        ver,
                        cmp
                    )
                );

                // Suppress header on slaves
                writeHeader = Pstream::master();
            }
        }
    }
    Ostream& os = osPtr();

    // If any of these fail, return (leave error handling to Ostream class)
    if (!os.good())
    {
        return false;
    }

    if (writeHeader && !io.writeHeader(os))
    {
        return false;
    }

    // Write the data to the Ostream
    if (!io.writeData(os))
    {
        return false;
    }

    if (writeHeader)
    {
        IOobject::writeEndDivider(os);
    }

    return true;
}


// ************************************************************************* //
