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
#include "pointMesh.H"
#include "pointFields.H"
#include "scalarList.H"
#include "vectorList.H"
#include "OTstream.H"
#include "zeroGradientPointPatchFields.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//  Main program:

int main(int argc, char *argv[])
{
    #include "addRegionOption.H"
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createNamedPolyMesh.H"
//
// {
//     IOdictionary2 displacement
//     (
//         IOobject
//         (
//             "pointDisplacement",
//             mesh.time().timeName(),
//             mesh,
//             IOobject::READ_IF_PRESENT,
//             IOobject::AUTO_WRITE
//         )
//     );
//
//     DebugVar(IOdictionary2::objectHeaderOk(displacement));
//
//     displacement.set("someEntry", "someValue");
//
//     runTime++;
//     displacement.instance() = displacement.time().timeName();
// //     displacement.writeObject
// //     (
// //         runTime.writeFormat(),
// //         IOstream::currentVersion,
// //         runTime.writeCompression()
// //     );
//
//     runTime.write();
//
//
//
// return 0;

//     const word key(":level1.level2A.level3A");
//
//     // Lookup
//     {
//         const entry* ePtr = displacement.lookupScopedEntryPtr
//         (
//             key,
//             false,
//             true            // allow pattern match
//         );
//         if (ePtr)
//         {
//             Pout<< "found entry : isDict:" << ePtr->isDict() << endl;
//             ePtr->write(Pout);
//         }
//     }
//     // Change
//     {
//         dictionary subDict;
//         subDict.set("internalField", 123);
//         displacement.dictionary::setScoped(key, subDict);
//     }
//     // Lookup
//     {
//         const entry* ePtr = displacement.lookupScopedEntryPtr
//         (
//             key,
//             false,
//             true            // allow pattern match
//         );
//         if (ePtr)
//         {
//             Pout<< "found entry : isDict:" << ePtr->isDict() << endl;
//             ePtr->write(Pout);
//         }
//     }
//
//     DebugVar(displacement);
//     return 0;
// }

//     {
//         OTstream os("bla", 100);
//         os << "some string" << endl;
//         os << 123 << endl;
//         os << 66.88 << endl;
//         os << word("ABC") << endl;
//
//
//         List<char> buf(3);
//         buf[0] = 'a';
//         buf[1] = 'b';
//         buf[2] = 'c';
//         os.write(buf.begin(), buf.size()*sizeof(char));
//
//         os.print(Pout);
//     }
//     {
//         OTstream os("bla", 100);
//
//         dictionary d;
//         d.add("one", label(1));
//         d.add("two", 2.0);
//         d.write(os);
//
//         os.print(Pout);
//     }



// {
//     IOdictionary bla
//     (
//         IOobject
//         (
//             "bla",
//             runTime.system(),
//             runTime,
//             IOobject::MUST_READ_IF_MODIFIED,
//             IOobject::NO_WRITE
//         )
//     );
//     Pout<< "bla:" << bla << endl;
//
//     const entry* ePtr = bla.lookupScopedEntryPtr
//     (
//         ":level1.level2A.level3A",
//         false,
//         true            // allow pattern match
//     );
//     if (ePtr)
//     {
//         Pout<< "found entry : isDict:" << ePtr->isDict() << endl;
//         ePtr->write(Pout);
//     }
// }





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

//     IOdictionary2 dictA
//     (
//         IOobject
//         (
//             "bla",
//             runTime.system(),
//             level3A,
//             IOobject::MUST_READ_IF_MODIFIED,
//             IOobject::NO_WRITE
//         ),
//         true
//     );
//
//     Pout<< "dictA:" << dictA << endl << endl;


// {
//     IOobject io
//     (
//         "pointDisplacement_field",
//         runTime.timeName(),
//         mesh,
//         IOobject::MUST_READ,
//         IOobject::AUTO_WRITE
//     );
//     DebugVar(io.objectPath());
//
//     IOobject parentIO
//     (
//         io.name(),
//         io.instance(),
//         io.local(),
//         io.db().parent(),
//         io.readOpt(),
//         io.writeOpt(),
//         io.registerObject()
//     );
//     DebugVar(parentIO.objectPath());
//
//
//     const pointMesh& pMesh = pointMesh::New(mesh);
//     pointVectorField displacement
//     (
//         IOobject
//         (
//             "pointDisplacement_field",
//             runTime.timeName(),
//             mesh,
//             IOobject::MUST_READ,
//             IOobject::AUTO_WRITE
//         ),
//         pMesh
//     );
//     DebugVar(displacement.objectPath());
//     DebugVar(displacement);
//     Pout<< "TOP:" << runTime.lookupObject<IOdictionary2>("pointDisplacement")
//         << endl;
// }
{
    const pointMesh& pMesh = pointMesh::New(mesh);
    pointVectorField displacement
    (
        IOobject
        (
            "pointDisplacement",
            runTime.timeName(),
            level3A,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        pMesh
        //dimensionedVector("zero", dimLength, vector::zero)
    );
    DebugVar(displacement.objectPath());
    DebugVar(displacement);

    displacement.internalField() = vector::one;
    displacement.correctBoundaryConditions();

    //displacement.write();
    // After:
    runTime++;
    displacement.instance() = displacement.time().timeName();
    runTime.write();
    Pout<< "TOP:" << runTime.lookupObject<IOdictionary>("pointDisplacement")
        << endl;

    // Postprocess

    PtrList<IOdictionary2> parentDicts;
    {
        // Search for list of runTime objects for this time
        IOobjectList objects(runTime, runTime.timeName());

        wordList masterNames
        (
            objects.sortedNames
            (
                IOdictionary::typeName
            )
        );
        Pstream::scatter(masterNames);

        parentDicts.setSize(masterNames.size());
        forAll(masterNames, i)
        {
            Info<< "Loading dictionary " << masterNames[i] << endl;
            const IOobject& io = *objects[masterNames[i]];
            parentDicts.set(i, new IOdictionary2(io));
        }
    }


    // Search for list of objects for this time
    IOobjectList fileObjects(level3A, runTime.timeName());
    DebugVar(fileObjects.sortedNames());

    // Add parent objects
    IOobjectList objects
    (
        IOdictionary2::lookupClass
        (
            pointVectorField::typeName,
            fileObjects,
            parentDicts,
            level3A.dbDir()
        )
    );
    DebugVar(objects.sortedNames());
    return 0;
}

//
//     // Path B
//     // ~~~~~~
//
//     objectRegistry level2B
//     (
//         IOobject
//         (
//             "level2B",
//             runTime.system(),
//             level1
//         )
//     );
//
//     objectRegistry level3B
//     (
//         IOobject
//         (
//             "level3B",
//             runTime.system(),
//             level2B
//         )
//     );
//
//     IOdictionary2 dictB
//     (
//         IOobject
//         (
//             "bla",
//             runTime.system(),
//             level3B,
//             IOobject::MUST_READ_IF_MODIFIED,
//             IOobject::NO_WRITE
//         ),
//         true
//     );
//
//     Pout<< "dictB:" << dictB << endl << endl;
//
//
//     // Do some writing
//     // ~~~~~~~~~~~~~~~
//     dictB.add("newEntry", 123);
//
//
//     // Note that this does not actually write the file; it only updates
//     // the levels; the actual file writing is done by the object registry
//     dictB.writeObject2
//     (
//         dictB.time().writeFormat(),
//         IOstream::currentVersion,
//         dictB.time().writeCompression()
//     );
//
//     // After:
//     Pout<< "TOP:" << runTime.lookupObject<IOdictionary2>("bla") << endl;

    // Manually enforce writing:
    //runTime.lookupObject<IOdictionary2>("bla").regIOobject::write();

//     objectRegistry::debug = 1;
//     runTime.write();
//     objectRegistry::debug = 0;





    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
