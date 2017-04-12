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
    Foam::fileOperations::collatedFileOperation

Description
    Version of masterUncollatedFileOperation that collates regIOobjects
    into a container in the processors/ subdirectory.

See also
    masterUncollatedFileOperation

SourceFiles
    collatedFileOperation.C

\*---------------------------------------------------------------------------*/

#ifndef fileOperations_collatedFileOperation_H
#define fileOperations_collatedFileOperation_H

#include "OSspecific.H"
#include "masterUncollatedFileOperation.H"
#include "OFstreamWriter.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fileOperations
{

/*---------------------------------------------------------------------------*\
                Class collatedFileOperation Declaration
\*---------------------------------------------------------------------------*/

class collatedFileOperation
:
    public masterUncollatedFileOperation
{
    // Private data

        //- Thread controller
        mutable OFstreamWriter writeServer_;


public:

        //- Runtime type information
        TypeName("collated");


    // Static data

        //- Max size of thread buffer size. This is the overall size of
        //  all files to be written. Starts blocking if not enough size.
        //  Read as float to enable easy specificiation of large sizes.
        static float maxThreadBufferSize;


    // Constructors

        //- Construct null
        collatedFileOperation(const bool verbose);


    //- Destructor
    virtual ~collatedFileOperation();


    // Member Functions

        // (reg)IOobject functionality

            //- Generate disk file name for object. Opposite of filePath.
            virtual fileName objectPath
            (
                const IOobject& io,
                const word& typeName
            ) const;

//            //- Generate disk file name for object. Opposite of filePath.
//            //  Use IOobject version in preference
//            virtual fileName objectPath(const fileName&) const;

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
