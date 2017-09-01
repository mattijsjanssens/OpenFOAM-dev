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

#include "OFstreamCollator.H"
#include "OFstream.H"
#include "OSspecific.H"
#include "IOstreams.H"
#include "Pstream.H"
#include "decomposedBlockData.H"
#include "PstreamReduceOps.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(OFstreamCollator, 0);
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::OFstreamCollator::writeFile
(
    const label comm,
    const word& typeName,
    const fileName& fName,
    const string& s,
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp,
    const bool append
)
{
    if (debug)
    {
        Pout<< "OFstreamCollator : Writing " << s.size()
            << " bytes to " << fName
            << " using comm " << comm << endl;
    }

    //label oldWarn = UPstream::warnComm;
    //UPstream::warnComm = comm;

    autoPtr<OSstream> osPtr;
    if (UPstream::master(comm))
    {
        Foam::mkDir(fName.path());
        osPtr.reset
        (
            new OFstream
            (
                fName,
                fmt,
                ver,
                cmp,
                append
            )
        );

        // We don't have IOobject so cannot use IOobject::writeHeader
        OSstream& os = osPtr();
        decomposedBlockData::writeHeader
        (
            os,
            ver,
            fmt,
            typeName,
            "",
            fName,
            fName.name()
        );
    }

    UList<char> slice(const_cast<char*>(s.data()), label(s.size()));

    // Assuming threaded writing hides any slowness so we might
    // as well use scheduled communication to send the data to
    // the master processor in order.

    List<std::streamoff> start;
    decomposedBlockData::writeBlocks
    (
        comm,
        osPtr,
        start,
        slice,
        UPstream::commsTypes::scheduled,
        false       // do not reduce return state
    );

    if (osPtr.valid() && !osPtr().good())
    {
        FatalIOErrorInFunction(osPtr())
            << "Failed writing to " << fName << exit(FatalIOError);
    }

    if (debug)
    {
        Pout<< "OFstreamCollator : Finished writing " << s.size()
            << " bytes to " << fName
            << " using comm " << comm << endl;
    }

    //UPstream::warnComm = oldWarn;

    return true;
}


bool Foam::OFstreamCollator::writeAll(OFstreamCollator& handler)
{
    // Consume stack. Return true if normal exit (stack empty), false
    // if forcing exit.

    bool ok = true;

    while (true)
    {
        writeData* ptr = nullptr;

        lockMutex(handler.mutex_);
        if (handler.objects_.size())
        {
            ptr = handler.objects_.pop();
        }
        unlockMutex(handler.mutex_);

        if (!ptr)
        {
            break;
        }
        else if (ptr->pathName_.empty())
        {
            // An empty fileName is the signal to exit the thread
            if (debug)
            {
                Pout<< "OFstreamCollator : Detected signalling data" << endl;
            }

            ok = false;
            break;
        }
        else
        {
            bool ok = writeFile
            (
                handler.comm_,
                ptr->typeName_,
                ptr->pathName_,
                ptr->data_,
                ptr->format_,
                ptr->version_,
                ptr->compression_,
                ptr->append_
            );
            if (!ok)
            {
                FatalIOErrorInFunction(ptr->pathName_)
                    << "Failed writing " << ptr->pathName_
                    << exit(FatalIOError);
            }

            delete ptr;
        }
    }

    return ok;
}


void* Foam::OFstreamCollator::writeAll(void *threadarg)
{
    OFstreamCollator& handler = *static_cast<OFstreamCollator*>(threadarg);

    bool keepAlive = false;
    if (keepAlive)
    {
        while (true)
        {
            bool ok = writeAll(handler);

            if (!ok)
            {
                break;
            }

            sleep(1);
        }

    }
    else
    {
        writeAll(handler);
    }


    if (debug)
    {
        Pout<< "OFstreamCollator : Exiting write thread " << endl;
    }

    lockMutex(handler.mutex_);
    handler.threadRunning_ = false;
    unlockMutex(handler.mutex_);

    return nullptr;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::OFstreamCollator::OFstreamCollator(const off_t maxBufferSize)
:
    maxBufferSize_(maxBufferSize),
    mutex_
    (
        maxBufferSize_ > 0
      ? allocateMutex()
      : -1
    ),
    thread_
    (
        maxBufferSize_ > 0
      ? allocateThread()
      : -1
    ),
    threadRunning_(false),
    comm_
    (
        UPstream::allocateCommunicator
        (
            UPstream::worldComm,
            identity(UPstream::nProcs(UPstream::worldComm))
        )
    )
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::OFstreamCollator::~OFstreamCollator()
{
    if (threadRunning_)
    {
        bool keepAlive = false;
        if (keepAlive)
        {
            if (debug)
            {
                Pout<< "~OFstreamCollator : Push signalling data" << endl;
            }

            // Push special data onto write stack that signals the thread
            // to stop

            lockMutex(mutex_);
            objects_.push
            (
                new writeData
                (
                    typeName,
                    fileName::null,
                    string::null,
                    IOstream::BINARY,
                    IOstream::currentVersion,
                    IOstream::UNCOMPRESSED,
                    false
                )
            );
            unlockMutex(mutex_);
        }

        if (debug)
        {
            Pout<< "~OFstreamCollator : Waiting for write thread "
                << thread_ << endl;
        }

        joinThread(thread_);
    }
    if (thread_ != -1)
    {
        freeThread(thread_);
    }
    if (mutex_ != -1)
    {
        freeMutex(mutex_);
    }
    if (comm_ != -1)
    {
        UPstream::freeCommunicator(comm_);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::OFstreamCollator::write
(
    const word& typeName,
    const fileName& fName,
    const string& data,
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp,
    const bool append
)
{
    if (maxBufferSize_ > 0)
    {
        while (true)
        {
            // Count files to be written
            off_t totalSize = 0;
            lockMutex(mutex_);
            forAllConstIter(FIFOStack<writeData*>, objects_, iter)
            {
                totalSize += iter()->data_.size();
            }
            unlockMutex(mutex_);

            if
            (
                totalSize == 0
             || (totalSize+off_t(data.size()) < maxBufferSize_)
            )
            {
                break;
            }

            if (debug)
            {
                Pout<< "OFstreamCollator : Waiting for buffer space."
                    << " Currently in use:" << totalSize
                    << " limit:" << maxBufferSize_
                    << endl;
            }

            sleep(5);
        }

        if (debug)
        {
            Pout<< "OFstreamCollator : relaying write of " << fName
                << " to thread " << thread_ << endl;
        }

        // Push the data onto the buffer
        lockMutex(mutex_);
        objects_.push
        (
            new writeData(typeName, fName, data, fmt, ver, cmp, append)
        );
        unlockMutex(mutex_);

        // Start the thread if necessary
        lockMutex(mutex_);
        if (!threadRunning_)
        {
            createThread(thread_, writeAll, this);
            if (debug)
            {
                Pout<< "OFstreamCollator : Started write thread " << thread_
                    << endl;
            }
            threadRunning_ = true;
        }
        unlockMutex(mutex_);

        return true;
    }
    else
    {
        // Immediate writing
        return writeFile(comm_, typeName, fName, data, fmt, ver, cmp, append);
    }
}


// ************************************************************************* //
