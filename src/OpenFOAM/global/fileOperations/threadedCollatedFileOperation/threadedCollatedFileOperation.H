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

Class
    Foam::fileOperations::threadedCollatedFileOperation

Description
    Version of masterUnthreadedCollatedFileOperation that collates regIOobjects
    into a container in the processors/ subdirectory.

See also
    masterUnthreadedCollatedFileOperation

SourceFiles
    threadedCollatedFileOperation.C

\*---------------------------------------------------------------------------*/

#ifndef fileOperations_threadedCollatedFileOperation_H
#define fileOperations_threadedCollatedFileOperation_H

#include "collatedFileOperation.H"
#include "OFstreamCollator.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fileOperations
{

/*---------------------------------------------------------------------------*\
                Class threadedCollatedFileOperation Declaration
\*---------------------------------------------------------------------------*/

class threadedCollatedFileOperation
:
    public collatedFileOperation
{
    // Private data

        //- Threaded writer
        mutable OFstreamCollator writer_;


public:

        //- Runtime type information
        TypeName("threadedCollated");


    // Constructors

        //- Construct null
        threadedCollatedFileOperation(const bool verbose);


    //- Destructor
    virtual ~threadedCollatedFileOperation();


    // Member Functions

        // (reg)IOobject functionality

            //- Writes a regIOobject (so header, contents and divider).
            //  Returns success state.
            virtual bool writeObject
            (
                const regIOobject&,
                IOstream::streamFormat format=IOstream::ASCII,
                IOstream::versionNumber version=IOstream::currentVersion,
                IOstream::compressionType compression=IOstream::UNCOMPRESSED,
                const bool valid = true
            ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fileOperations
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
