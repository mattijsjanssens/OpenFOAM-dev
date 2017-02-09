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
    Test-decomposedBlockData

Description

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "polyMesh.H"
#include "decomposedBlockData.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//  Main program:

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createPolyMesh.H"

    Info<< "End\n" << endl;

    IOobject io
    (
        "p",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    );

//    // Write
//    {
//        decomposedBlockData data(io);
//
//        OStringStream os;
//        os << "My String" << endl;
//        string s(os.str());
//
//        UList<char> slice(const_cast<char*>(s.data()), label(s.size()));
//        DebugVar(slice);
//
//        data.List<char>::operator=(slice);
//
//        data.write();
//    }

    // Read
    {
        io.readOpt() = IOobject::MUST_READ;
        decomposedBlockData data(io);

        DebugVar(data);
    }

    return 0;
}


// ************************************************************************* //
