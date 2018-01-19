/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2017-2018 OpenFOAM Foundation
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
    Foam::fileOperations::multiCollatedFileOperation

Description
    Version of collatedFileOperation with multiple writers, one per hostname

See also
    collatedFileOperation

SourceFiles
    multiCollatedFileOperation.C

\*---------------------------------------------------------------------------*/

#ifndef fileOperations_multiCollatedFileOperation_H
#define fileOperations_multiCollatedFileOperation_H

#include "collatedFileOperation.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fileOperations
{

/*---------------------------------------------------------------------------*\
                    Class multiCollatedFileOperation Declaration
\*---------------------------------------------------------------------------*/

class multiCollatedFileOperation
:
    public collatedFileOperation    //masterUncollatedFileOperation
{
   // Private Member Functions

        //- Get the ranks that are writers (through command line argument;
        //  non-parallel only)
        static labelList writeRanks();

        //- Get the list of processors part of this set
        static labelList subRanks(const label n);


public:

        //- Runtime type information
        TypeName("multiCollated");


    // Constructors

        //- Construct null
        multiCollatedFileOperation(const bool verbose);


    //- Destructor
    virtual ~multiCollatedFileOperation();
};


/*---------------------------------------------------------------------------*\
            Class multiCollatedFileOperationInitialise Declaration
\*---------------------------------------------------------------------------*/

class multiCollatedFileOperationInitialise
:
    public collatedFileOperationInitialise
{
public:

    // Constructors

        //- Construct from components
        multiCollatedFileOperationInitialise(int& argc, char**& argv);


    //- Destructor
    virtual ~multiCollatedFileOperationInitialise()
    {}
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fileOperations
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
