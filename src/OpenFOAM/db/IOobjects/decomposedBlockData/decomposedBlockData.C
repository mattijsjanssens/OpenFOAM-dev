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

#include "decomposedBlockData.H"
#include "OPstream.H"
#include "IPstream.H"
#include "PstreamBuffers.H"
#include "OFstream.H"
#include "IFstream.H"
#include "IStringStream.H"
#include "dictionary.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(decomposedBlockData, 0);
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::decomposedBlockData::decomposedBlockData
(
    const IOobject& io,
    const UPstream::commsTypes commsType
)
:
    regIOobject(io),
    commsType_(commsType)
{
    // Temporary warning
    if (io.readOpt() == IOobject::MUST_READ_IF_MODIFIED)
    {
        WarningInFunction
            << "decomposedBlockData " << name()
            << " constructed with IOobject::MUST_READ_IF_MODIFIED"
            " but decomposedBlockData does not support automatic rereading."
            << endl;
    }
    if
    (
        (
            io.readOpt() == IOobject::MUST_READ
         || io.readOpt() == IOobject::MUST_READ_IF_MODIFIED
        )
     || (io.readOpt() == IOobject::READ_IF_PRESENT && headerOk())
    )
    {
        read();
    }
}


Foam::decomposedBlockData::decomposedBlockData
(
    const IOobject& io,
    const UList<char>& list,
    const UPstream::commsTypes commsType
)
:
    regIOobject(io),
    commsType_(commsType)
{
    // Temporary warning
    if (io.readOpt() == IOobject::MUST_READ_IF_MODIFIED)
    {
        WarningInFunction
            << "decomposedBlockData " << name()
            << " constructed with IOobject::MUST_READ_IF_MODIFIED"
            " but decomposedBlockData does not support automatic rereading."
            << endl;
    }

    if
    (
        (
            io.readOpt() == IOobject::MUST_READ
         || io.readOpt() == IOobject::MUST_READ_IF_MODIFIED
        )
     || (io.readOpt() == IOobject::READ_IF_PRESENT && headerOk())
    )
    {
        read();
    }
    else
    {
        List<char>::operator=(list);
    }
}


Foam::decomposedBlockData::decomposedBlockData
(
    const IOobject& io,
    const Xfer<List<char>>& list,
    const UPstream::commsTypes commsType
)
:
    regIOobject(io),
    commsType_(commsType)
{
    // Temporary warning
    if (io.readOpt() == IOobject::MUST_READ_IF_MODIFIED)
    {
        WarningInFunction
            << "decomposedBlockData " << name()
            << " constructed with IOobject::MUST_READ_IF_MODIFIED"
            " but decomposedBlockData does not support automatic rereading."
            << endl;
    }

    List<char>::transfer(list());

    if
    (
        (
            io.readOpt() == IOobject::MUST_READ
         || io.readOpt() == IOobject::MUST_READ_IF_MODIFIED
        )
     || (io.readOpt() == IOobject::READ_IF_PRESENT && headerOk())
    )
    {
        read();
    }
}


// * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * * //

Foam::decomposedBlockData::~decomposedBlockData()
{}


// * * * * * * * * * * * * * * * Members Functions * * * * * * * * * * * * * //

bool Foam::decomposedBlockData::readMasterHeader(IOobject& io, Istream& is)
{
    if (debug)
    {
        Pout<< "decomposedBlockData::readMasterHeader:"
            << " stream:" << is.name() << endl;
    }

    // Master-only reading of header
    is.fatalCheck("read(Istream&)");

    List<char> data(is);
    is.fatalCheck("read(Istream&) : reading entry");
    string buf(data.begin(), data.size());
    IStringStream str(is.name(), buf);

    return io.readHeader(str);
}


void Foam::decomposedBlockData::writeHeader
(
    Ostream& os,
    const IOstream::versionNumber version,
    const IOstream::streamFormat format,
    const word& type,
    const string& note,
    const fileName& location,
    const word& name
)
{
    IOobject::writeBanner(os)
        << "FoamFile\n{\n"
        << "    version     " << version << ";\n"
        << "    format      " << format << ";\n"
        << "    class       " << type << ";\n";
    if (note.size())
    {
        os  << "    note        " << note << ";\n";
    }

    if (location.size())
    {
        os  << "    location    " << location << ";\n";
    }

    os  << "    object      " << name << ";\n"
        << "}" << nl;

    IOobject::writeDivider(os) << nl;
}


void Foam::decomposedBlockData::readHeader
(
    Istream& is,
    IOstream::versionNumber& version,
    IOstream::streamFormat& format,
    word& type,
    string& note,
    fileName& location,
    word& name
)
{
    token firstToken(is);

    if
    (
        is.good()
     && firstToken.isWord()
     && firstToken.wordToken() == "FoamFile"
    )
    {
        dictionary headerDict(is);

        version = IOstream::versionNumber(headerDict.lookup("version"));
        format = IOstream::formatEnum(headerDict.lookup("format"));
        type = word(headerDict.lookup("class"));
        name = word(headerDict.lookup("object"));
        // The location entry is optional
        headerDict.readIfPresent("location", location);
        // The note entry is optional
        headerDict.readIfPresent("note", note);
    }
}


bool Foam::decomposedBlockData::readBlock
(
    const label blocki,
    Istream& is,
    List<char>& data
)
{
    if (debug)
    {
        Pout<< "decomposedBlockData::readMasterHeader:"
            << " stream:" << is.name() << " attempt to read block " << blocki
            << endl;
    }

    is.fatalCheck("read(Istream&)");

    if (blocki == 0)
    {
        is >> data;
        is.fatalCheck("read(Istream&) : reading entry");
    }
    else
    {
        // Read master for header
        IOstream::versionNumber version(IOstream::currentVersion);
        IOstream::streamFormat format;
        word type;
        string note;
        fileName location;
        word name;
        {
            List<char> data(is);
            is.fatalCheck("read(Istream&) : reading entry");
            string buf(data.begin(), data.size());
            IStringStream str(is.name(), buf);
            readHeader(str, version, format, type, note, location, name);
        }
        OStringStream os;
        writeHeader(os, version, format, type, note, location, name);
        string s(os.str());
        UList<char> slice(const_cast<char*>(s.data()), label(s.size()));


        for (label i = 1; i < blocki+1; i++)
        {
            // Read data, override old data
            is >> data;

            // Prepend the header
            label n = data.size();
            label sz = slice.size()+n;
            data.setSize(sz);
            for (label i = sz-1; i >= slice.size(); --i)
            {
                data[i] = data[i-slice.size()];
            }
            forAll(slice, i)
            {
                data[i] = slice[i];
            }

            is.fatalCheck("read(Istream&) : reading entry");
        }
    }
    return is.good();
}


bool Foam::decomposedBlockData::readBlocks
(
    autoPtr<ISstream>& isPtr,
    List<char>& data,
    const UPstream::commsTypes commsType
)
{
    if (debug)
    {
        Pout<< "decomposedBlockData::readBlocks:"
            << " stream:" << (isPtr.valid() ? isPtr().name() : "invalid")
            << " commsType:" << Pstream::commsTypeNames[commsType] << endl;
    }

    bool ok = false;

    if (commsType == UPstream::commsTypes::scheduled)
    {
        if (UPstream::master())
        {
            Istream& is = isPtr();
            is.fatalCheck("read(Istream&)");

            // Read master data
            {
                is >> data;
                is.fatalCheck("read(Istream&) : reading entry");
            }

            // Extract the header from the master data
            string headerBytes;
            {
                IOstream::versionNumber version(IOstream::currentVersion);
                IOstream::streamFormat format;
                word type;
                string note;
                fileName loc;
                word name;
                {
                    string buf(data.begin(), data.size());
                    IStringStream str(is.name(), buf);
                    readHeader(str, version, format, type, note, loc, name);
                }
                OStringStream os;
                writeHeader(os, version, format, type, note, loc, name);
                headerBytes = os.str();
            }
            UList<char> slice
            (
                const_cast<char*>(headerBytes.data()),
                label(headerBytes.size())
            );


            for
            (
                label proci = 1;
                proci < UPstream::nProcs();
                proci++
            )
            {
                List<char> elems(is);
                is.fatalCheck("read(Istream&) : reading entry");

                // Prepend the header
                label n = elems.size();
                label sz = slice.size()+n;
                elems.setSize(sz);
                for (label i = sz-1; i >= slice.size(); --i)
                {
                    elems[i] = elems[i-slice.size()];
                }
                forAll(slice, i)
                {
                    elems[i] = slice[i];
                }

                OPstream os(UPstream::commsTypes::scheduled, proci);
                os << elems;
            }

            ok = is.good();
        }
        else
        {
            IPstream is(UPstream::commsTypes::scheduled, UPstream::masterNo());
            is >> data;
        }
    }
    else
    {
        PstreamBuffers pBufs(UPstream::commsTypes::nonBlocking);

        if (UPstream::master())
        {
            Istream& is = isPtr();
            is.fatalCheck("read(Istream&)");

            // Read master data
            {
                is >> data;
                is.fatalCheck("read(Istream&) : reading entry");
            }

            // Extract the header from the master data
            string headerBytes;
            {
                IOstream::versionNumber version(IOstream::currentVersion);
                IOstream::streamFormat format;
                word type;
                string note;
                fileName loc;
                word name;
                {
                    string buf(data.begin(), data.size());
                    IStringStream str(is.name(), buf);
                    readHeader(str, version, format, type, note, loc, name);
                }
                OStringStream os;
                writeHeader(os, version, format, type, note, loc, name);
                headerBytes = os.str();
            }
            UList<char> slice
            (
                const_cast<char*>(headerBytes.data()),
                label(headerBytes.size())
            );

            for
            (
                label proci = 1;
                proci < UPstream::nProcs();
                proci++
            )
            {
                List<char> elems(is);
                is.fatalCheck("read(Istream&) : reading entry");

                // Prepend the header
                label n = elems.size();
                label sz = slice.size()+n;
                elems.setSize(sz);
                for (label i = sz-1; i >= slice.size(); --i)
                {
                    elems[i] = elems[i-slice.size()];
                }
                forAll(slice, i)
                {
                    elems[i] = slice[i];
                }

                UOPstream os(proci, pBufs);
                os << elems;
            }
        }

        labelList recvSizes;
        pBufs.finishedSends(recvSizes);

        if (!UPstream::master())
        {
            UIPstream is(UPstream::masterNo(), pBufs);
            is >> data;
        }
    }

    Pstream::scatter(ok);

    return ok;
}


bool Foam::decomposedBlockData::writeBlocks
(
    autoPtr<OSstream>& osPtr,
    List<std::streamoff>& start,
    const UList<char>& data,
    const UPstream::commsTypes commsType
)
{
    if (debug)
    {
        Pout<< "decomposedBlockData::writeBlocks:"
            << " stream:" << (osPtr.valid() ? osPtr().name() : "invalid")
            << " data:" << data.size()
            << " commsType:" << Pstream::commsTypeNames[commsType] << endl;
    }

    bool ok = false;

    if (commsType == UPstream::commsTypes::scheduled)
    {
        if (UPstream::master())
        {
            start.setSize(UPstream::nProcs());

            OSstream& os = osPtr();

            // Write master data
            {
                os << nl << "// Processor" << UPstream::masterNo() << nl;
                start[UPstream::masterNo()] = os.stdStream().tellp();
                os << data;
            }
            // Write slaves
            for (label proci = 1; proci < UPstream::nProcs(); proci++)
            {
                IPstream is(UPstream::commsTypes::scheduled, proci);
                List<char> elems(is);

                is.fatalCheck("write(Istream&) : reading entry");

                os << nl << "// Processor" << proci << nl;
                start[proci] = os.stdStream().tellp();
                os << elems;
            }

            ok = os.good();
        }
        else
        {
            OPstream os(UPstream::commsTypes::scheduled, UPstream::masterNo());
            os << data;
        }
    }
    else
    {
        PstreamBuffers pBufs(UPstream::commsTypes::nonBlocking);

        if (!UPstream::master())
        {
            UOPstream os(UPstream::masterNo(), pBufs);
            os << data;
        }

        labelList recvSizes;
        pBufs.finishedSends(recvSizes);

        if (UPstream::master())
        {
            start.setSize(UPstream::nProcs());

            OSstream& os = osPtr();

            // Write master data
            {
                os << nl << "// Processor" << UPstream::masterNo() << nl;
                start[UPstream::masterNo()] = os.stdStream().tellp();
                os << data;
            }

            // Write slaves
            for (label proci = 1; proci < UPstream::nProcs(); proci++)
            {
                UIPstream is(proci, pBufs);
                List<char> elems(is);

                is.fatalCheck("write(Istream&) : reading entry");

                os << nl << "// Processor" << proci << nl;
                start[proci] = os.stdStream().tellp();
                os << elems;
            }

            ok = os.good();
        }
    }

    Pstream::scatter(ok);

    return ok;
}


bool Foam::decomposedBlockData::read()
{
    autoPtr<ISstream> isPtr;
    if (UPstream::master())
    {
        isPtr.reset(new IFstream(objectPath()));
        IOobject::readHeader(isPtr());
    }
    return readBlocks(isPtr, *this, commsType_);
}


bool Foam::decomposedBlockData::writeObject
(
    IOstream::streamFormat fmt,
    IOstream::versionNumber ver,
    IOstream::compressionType cmp,
    const bool valid
) const
{
    autoPtr<OSstream> osPtr;
    if (UPstream::master())
    {
        // Note: always write binary. These are strings so readable
        //       anyway. They have already be tokenised on the sending side.
        osPtr.reset(new OFstream(objectPath(), IOstream::BINARY, ver, cmp));
        IOobject::writeHeader(osPtr());
    }
    List<std::streamoff> start;
    return writeBlocks(osPtr, start, *this, commsType_);
}


Foam::label Foam::decomposedBlockData::numBlocks(const fileName& fName)
{
    label nBlocks = 0;

    IFstream is(fName);
    is.fatalCheck("decomposedBlockData::numBlocks(const fileName&)");

    if (!is.good())
    {
        return nBlocks;
    }

    // Skip header
    token firstToken(is);

    if
    (
        is.good()
     && firstToken.isWord()
     && firstToken.wordToken() == "FoamFile"
    )
    {
        dictionary headerDict(is);
        is.version(headerDict.lookup("version"));
        is.format(headerDict.lookup("format"));
    }

    List<char> data;
    while (is.good())
    {
        token sizeToken(is);
        if (!sizeToken.isLabel())
        {
            return nBlocks;
        }
        is.putBack(sizeToken);

        is >> data;
        nBlocks++;
    }

    return nBlocks;
}


// ************************************************************************* //
