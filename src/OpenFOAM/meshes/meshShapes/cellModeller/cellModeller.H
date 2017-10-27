/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2016 OpenFOAM Foundation
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
    Foam::cellModels

Description
    A static collection of cell models, and a means of looking them up.

SourceFiles
    cellModeller.C

\*---------------------------------------------------------------------------*/

#ifndef cellModeller_H
#define cellModeller_H

#include "cellModel.H"
#include "PtrList.H"
#include "HashTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                        Class cellModels Declaration
\*---------------------------------------------------------------------------*/

class cellModels
{
    // Private data

        //- PtrList of models
        PtrList<cellModel> models_;

        //- List of model pointers
        List<cellModel*> modelPtrs_;

        //- HashTable of model pointers
        HashTable<const cellModel*> modelDictionary_;


    // Private member functions

        //- Version of findEtcFile that uses fileHandler
        static fileName findEtcFile(const fileName& name);


public:

        //- Static singleton
        static autoPtr<cellModels> cellModellerPtr_;

    // Constructors

        //- Construct from central "cellModels" file
        cellModels();

    //- Destructor
    ~cellModels();


    // Member functions

        //- Look up a model by name and return a pointer to the model or nullptr
        const cellModel* lookup(const word&) const;

        //- Look up a model by index and return a pointer to the model or
        //  nullptr
        const cellModel* lookup(const label i) const
        {
            return modelPtrs_[i];
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Global declarations: defined in fileOperation.C

//- Get cellModels
const cellModels& cellModeller();


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
