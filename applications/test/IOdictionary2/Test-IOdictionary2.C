/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016 OpenFOAM Foundation
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

Description

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "wordRe.H"
#include "OSspecific.H"
#include "IOdictionary2.H"
#include "polyMesh.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//  Main program:

int main(int argc, char *argv[])
{
    #include "addRegionOption.H"
    #include "setRootCase.H"
    #include "createTime.H"
//     #include "createNamedPolyMesh.H"
// 
//     Pout<< "Mesh:" << mesh.name() << endl;
//     Pout<< "Mesh.db():" << mesh.dbDir() << endl;

{
    IOdictionary bla
    (
        IOobject
        (
            "bla",
            runTime.system(),
            runTime,
            IOobject::MUST_READ_IF_MODIFIED,
            IOobject::NO_WRITE
        )
    );
    Pout<< "bla:" << bla << endl;

    const entry* ePtr = bla.lookupScopedEntryPtr
    (
        ":level1.level2A.level3A",
        false,
        true            // allow pattern match
    );
    if (ePtr)
    {
        Pout<< "found entry : isDict:" << ePtr->isDict() << endl;
        ePtr->write(Pout);
    }
}


    objectRegistry level1
    (
        IOobject
        (
            "level1",
            runTime.system(),
            runTime
        )
    );


    // Path A
    // ~~~~~~

    objectRegistry level2A
    (
        IOobject
        (
            "level2A",
            runTime.system(),
            level1
        )
    );

    objectRegistry level3A
    (
        IOobject
        (
            "level3A",
            runTime.system(),
            level2A
        )
    );

    IOdictionary2 dictA
    (
        IOobject
        (
            "bla",
            runTime.system(),
            level3A,
            IOobject::MUST_READ_IF_MODIFIED,
            IOobject::NO_WRITE
        ),
        true
    );

    Pout<< "dictA:" << dictA << endl << endl;

    // Path B
    // ~~~~~~

    objectRegistry level2B
    (
        IOobject
        (
            "level2B",
            runTime.system(),
            level1
        )
    );

    objectRegistry level3B
    (
        IOobject
        (
            "level3B",
            runTime.system(),
            level2B
        )
    );

    IOdictionary2 dictB
    (
        IOobject
        (
            "bla",
            runTime.system(),
            level3B,
            IOobject::MUST_READ_IF_MODIFIED,
            IOobject::NO_WRITE
        ),
        true
    );

    Pout<< "dictB:" << dictB << endl << endl;


    // Do some writing
    // ~~~~~~~~~~~~~~~
    dictB.add("newEntry", 123);


    // Note that this does not actually write the file; it only updates
    // the levels; the actual file writing is done by the object registry
    dictB.writeObject2
    (
        dictB.time().writeFormat(),
        IOstream::currentVersion,
        dictB.time().writeCompression()
    );

    // After:
    Pout<< "TOP:" << runTime.lookupObject<IOdictionary2>("bla") << endl;

    // Manually enforce writing:
    //runTime.lookupObject<IOdictionary2>("bla").regIOobject::write();



    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
