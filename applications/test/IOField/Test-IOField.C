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

Application
    Test-IOField

Description
    Test the processor-local reading of IOField (used in the lagrangian libs)

\*---------------------------------------------------------------------------*/

#include "IOField.H"
#include "argList.H"
#include "polyMesh.H"
#include "Time.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void write(const IOobject& io, const label sz)
{
    IOField<label> fld(io, sz);
    forAll(fld, i)
    {
        fld[i] = i+1000;
    }
    Pout<< "writing:" << fld << endl;
    fld.write(sz > 0);
}

void read(const IOobject& io, const label sz)
{
    bool valid = (sz > 0);
    Pout<< "valid:" << valid << endl;
    IOField<label> fld(io, valid);
    Pout<< "fld:" << fld << endl;
}


void readAndWrite
(
    IOobject& io,
    const label sz,
    const word& readType,
    const word& writeType
)
{
    Pout<< "** Reading:" << readType
        << " Writing:" << writeType << endl;

    autoPtr<fileOperation> writeHandler(fileOperation::New(writeType));
    fileHandler(writeHandler);

    // Delete
    rm(fileHandler().filePath(io.objectPath()));

    // Write
    io.readOpt() = IOobject::NO_READ;
    write(io, sz);

    autoPtr<fileOperation> readHandler(fileOperation::New(readType));
    fileHandler(readHandler);

    // Read
    io.readOpt() = IOobject::MUST_READ;
    read(io, sz);

    Pout<< "** Done reading:" << readType
        << " Writing:" << writeType << endl << endl << endl;
}


// Main program:

int main(int argc, char *argv[])
{
    #include "addTimeOptions.H"
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createPolyMesh.H"

    label sz = 0;
    if (Pstream::myProcNo() % 2)
    {
        sz = 1;
    }

    IOobject io
    (
        "bla",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    );

    readAndWrite
    (
        io,
        sz,
        "localFileOperation",
        "localFileOperation"
    );

    readAndWrite
    (
        io,
        sz,
        "localFileOperation",
        "masterFileOperation"
    );


    Pout<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
