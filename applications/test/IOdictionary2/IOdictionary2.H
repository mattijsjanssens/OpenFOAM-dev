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
    Foam::IOdictionary2

Description
    IOdictionary2 is derived from dictionary and IOobject to give the dictionary
    automatic IO functionality via the objectRegistry.  To facilitate IO,
    IOdictioanry is provided with a constructor from IOobject and writeData and
    write functions.

SourceFiles
    IOdictionary2.C
    IOdictionary2IO.C

\*---------------------------------------------------------------------------*/

#ifndef IOdictionary2_H
#define IOdictionary2_H

#include "dictionary.H"
#include "regIOobject.H"
#include "IOobjectList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class IOdictionary2 Declaration
\*---------------------------------------------------------------------------*/

class IOdictionary2
:
    public regIOobject,
    public dictionary
{
    // Private data

        static bool writeDictionaries;


    // Private Member Functions

        //- Read dictionary from file
        //  Parallel aware reading, using non-virtual type information
        //  (typeName instead of type()) because of use in constructor.
        void readFile(const bool);

        //- Read dictionary from database or file. Parallel aware.
        bool readObjectOrFile(const bool masterOnly);

        //- Read header from FoamFile entry. Returns true if found.
        bool readHeader(const dictionary&);

        //- Return FoamFile header as dictionary
        dictionary headerDict(const word& type) const;


public:

    TypeName("dictionary");


    // Constructors

        //- Construct given an IOobject
        IOdictionary2(const IOobject&);

        //- Construct given an IOobject and dictionary
        IOdictionary2(const IOobject&, const dictionary&);

        //- Construct given an IOobject and Istream
        IOdictionary2(const IOobject&, Istream&);


    //- Destructor
    virtual ~IOdictionary2();


    // Member functions

        //- Name function is needed to disambiguate those inherited
        //  from regIOobject and dictionary
        const word& name() const;

        //- ReadData function required for regIOobject read operation
        virtual bool readData(Istream&);

        //- WriteData function required for regIOobject write operation
        virtual bool writeData(Ostream&) const;

        //- Read dictionary from parent or file
        virtual bool read();

        //- Return true if the object's file (or files for objectRegistry)
        //  have been modified. (modified state is cached by Time)
        virtual bool modified() const;

        //- Read object if modified (as set by call to modified)
        virtual bool readIfModified();

        //- Write using given format, version and compression
        virtual bool writeObject
        (
            IOstream::streamFormat,
            IOstream::versionNumber,
            IOstream::compressionType
        ) const;

        //- Helper: check that file (or parent IOdictionary) is present.
        //  - checks IOobject::headerOk
        //  - checks for registered IOdictionary with entry for IO present
        //  - checks for higher level file
        static bool objectHeaderOk(const IOobject&);

        //- Find for given scope if there is an object of class
        autoPtr<IOobject> lookupClass
        (
            const fileName&,
            const word& ClassName
        ) const;

        //- Helper: merge IOobjectList (from files) with that of database lookup
        static IOobjectList lookupClass
        (
            const word& ClassName,
            const IOobjectList& fileObjects,
            const PtrList<IOdictionary2>& parentObjects,
            const fileName& dbDir
        );


    // Member operators

        //- Assignment of other IOdictionary's entries to this IOdictionary2
        void operator=(const IOdictionary2&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
