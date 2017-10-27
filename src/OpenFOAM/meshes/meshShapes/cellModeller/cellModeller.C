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

Description
    Constructor of cellModeller: just sets the cellModeller's params.

\*---------------------------------------------------------------------------*/

#include "cellModeller.H"
#include "IFstream.H"
#include "fileOperation.H"
#include "OSspecific.H"
#include "foamVersion.H"

/* * * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * */

namespace Foam
{
    autoPtr<cellModels> cellModels::cellModellerPtr_;
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::fileName Foam::cellModels::findEtcFile(const fileName& name)
{
    // Search for user files in
    // * ~/.OpenFOAM/VERSION
    // * ~/.OpenFOAM
    //
    fileName searchDir = home()/".OpenFOAM";
    if (fileHandler().isDir(searchDir))
    {
        fileName fullName = searchDir/FOAMversion/name;
        if (fileHandler().isFile(fullName))
        {
            return fullName;
        }

        fullName = searchDir/name;
        if (fileHandler().isFile(fullName))
        {
            return fullName;
        }
    }

    // Search for group (site) files in
    // * $WM_PROJECT_SITE/VERSION
    // * $WM_PROJECT_SITE
    //
    searchDir = getEnv("WM_PROJECT_SITE");
    if (searchDir.size())
    {
        if (fileHandler().isDir(searchDir))
        {
            fileName fullName = searchDir/FOAMversion/name;
            if (fileHandler().isFile(fullName))
            {
                return fullName;
            }

            fullName = searchDir/name;
            if (fileHandler().isFile(fullName))
            {
                return fullName;
            }
        }
    }
    else
    {
        // Or search for group (site) files in
        // * $WM_PROJECT_INST_DIR/site/VERSION
        // * $WM_PROJECT_INST_DIR/site
        //
        searchDir = getEnv("WM_PROJECT_INST_DIR");
        if (fileHandler().isDir(searchDir))
        {
            fileName fullName = searchDir/"site"/FOAMversion/name;
            if (fileHandler().isFile(fullName))
            {
                return fullName;
            }

            fullName = searchDir/"site"/name;
            if (fileHandler().isFile(fullName))
            {
                return fullName;
            }
        }
    }

    // Search for other (shipped) files in
    // * $WM_PROJECT_DIR/etc
    //
    searchDir = getEnv("WM_PROJECT_DIR");
    if (fileHandler().isDir(searchDir))
    {
        fileName fullName = searchDir/"etc"/name;
        if (fileHandler().isFile(fullName))
        {
            return fullName;
        }
    }
    std::cerr
        << "--> FOAM FATAL ERROR in Foam::findEtcFiles() :"
           " could not find mandatory file\n    '"
        << name.c_str() << "'\n\n" << std::endl;
    ::exit(1);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::cellModels::cellModels()
:
    models_(fileHandler().NewIFstream(findEtcFile("cellModels"))()())
{
    if (modelPtrs_.size())
    {
        FatalErrorInFunction
            << "attempt to re-construct cellModeller when it already exists"
            << exit(FatalError);
    }

    label maxIndex = 0;
    forAll(models_, i)
    {
        if (models_[i].index() > maxIndex) maxIndex = models_[i].index();
    }

    modelPtrs_.setSize(maxIndex + 1);
    modelPtrs_ = nullptr;

    // For all the words in the wordlist, set the details of the model
    // to those specified by the word name and the other parameters
    // given. This should result in an automatic 'read' of the model
    // from its File (see cellModel class).
    forAll(models_, i)
    {
        if (modelPtrs_[models_[i].index()])
        {
            FatalErrorInFunction
                << "more than one model share the index "
                << models_[i].index()
                << exit(FatalError);
        }

        modelPtrs_[models_[i].index()] = &models_[i];

        if (modelDictionary_.found(models_[i].name()))
        {
            FatalErrorInFunction
                << "more than one model share the name "
                << models_[i].name()
                << exit(FatalError);
        }

        modelDictionary_.insert(models_[i].name(), &models_[i]);
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::cellModels::~cellModels()
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

const Foam::cellModel* Foam::cellModels::lookup(const word& name) const
{
    HashTable<const cellModel*>::const_iterator iter =
        modelDictionary_.find(name);

    if (iter != modelDictionary_.end())
    {
        return iter();
    }
    else
    {
        return nullptr;
    }
}


const Foam::cellModels& Foam::cellModeller()
{
    if (!cellModels::cellModellerPtr_.valid())
    {
        cellModels::cellModellerPtr_.reset(new cellModels());
    }
    return cellModels::cellModellerPtr_();
}


// ************************************************************************* //
