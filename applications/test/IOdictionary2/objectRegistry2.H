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
    Foam::objectRegistry2

Description
    Registry of regIOobject2s

SourceFiles
    objectRegistry2.C

\*---------------------------------------------------------------------------*/

#ifndef objectRegistry2_H
#define objectRegistry2_H

#include "HashTable.H"
#include "regIOobject2.H"
#include "wordReList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                       Class objectRegistry2 Declaration
\*---------------------------------------------------------------------------*/

class objectRegistry2
:
    public regIOobject2,
    public HashTable<regIOobject2*>
{
    // Private Data

        //- Master time objectRegistry2
        const Time& time_;

        //- Parent objectRegistry2
        const objectRegistry2& parent_;

        //- Local directory path of this objectRegistry2 relative to time
        fileName dbDir_;

        //- Current event
        mutable label event_;


    // Private Member Functions

        //- Is the objectRegistry2 parent_ different from time_
        //  Used to terminate searching within the ancestors
        bool parentNotTime() const;

        //- Disallow Copy constructor
        objectRegistry2(const objectRegistry2&);

        //- Disallow default bitwise copy construct and assignment
        void operator=(const objectRegistry2&);


public:

    //- Declare type name for this IOobject
    TypeName("objectRegistry2");


    // Constructors

        //- Construct the time objectRegistry2 given an initial estimate
        //  for the number of entries
        explicit objectRegistry2
        (
            const Time& db,
            const label nIoObjects = 128
        );

        //- Construct a sub-registry given an IObject to describe the registry
        //  and an initial estimate for the number of entries
        explicit objectRegistry2
        (
            const IOobject& io,
            const label nIoObjects = 128
        );


    //- Destructor
    virtual ~objectRegistry2();


    // Member functions

        // Access

            //- Return time
            const Time& time() const
            {
                return time_;
            }

            //- Return the parent objectRegistry2
            const objectRegistry2& parent() const
            {
                return parent_;
            }

            //- Local directory path of this objectRegistry2 relative to the time
            virtual const fileName& dbDir() const
            {
                return dbDir_;
            }

            //- Return the list of names of the IOobjects
            wordList names() const;

            //- Return the sorted list of names of the IOobjects
            wordList sortedNames() const;

            //- Return the list of names of IOobjects of given class name
            wordList names(const word& className) const;

            //- Return the sorted list of names of IOobjects of given class name
            wordList sortedNames(const word& className) const;

            //- Return the list of names of the IOobjects of given type
            template<class Type>
            wordList names() const;

            //- Return the list of objects whose name matches the input regExp
            template<class Type>
            wordList names(const wordRe& name) const;

            //- Return the list of objects whose name matches the input regExp
            template<class Type>
            wordList names(const wordReList& name) const;

            //- Lookup and return a const sub-objectRegistry2. Optionally create
            //  it if it does not exist.
            const objectRegistry2& subRegistry
            (
                const word& name,
                const bool forceCreate = false
            ) const;

            //- Lookup and return all objects of the given Type
            template<class Type>
            HashTable<const Type*> lookupClass(const bool strict = false) const;

            //- Lookup and return all objects of the given Type
            template<class Type>
            HashTable<Type*> lookupClass(const bool strict = false);

            //- Is the named Type found?
            template<class Type>
            bool foundObject(const word& name) const;

            //- Lookup and return the object of the given Type
            template<class Type>
            const Type& lookupObject(const word& name) const;

            //- Return new event number.
            label getEvent() const;


        // Edit

            //- Rename
            virtual void rename(const word& newName);

            //- Add an regIOobject2 to registry
            bool checkIn(regIOobject2&) const;

            //- Remove an regIOobject2 from registry
            bool checkOut(regIOobject2&) const;

        // Reading

            //- Return true if any of the object's files have been modified
            virtual bool modified() const;

            //- Read the objects that have been modified
            void readModifiedObjects();

            //- Read object if modified
            virtual bool readIfModified();


        // Writing

            //- writeData function required by regIOobject2 but not used
            //  for this class, write is used instead
            virtual bool writeData(Ostream&) const
            {
                NotImplemented;

                return false;
            }

            //- Write the objects
            virtual bool writeObject
            (
                IOstream::streamFormat fmt,
                IOstream::versionNumber ver,
                IOstream::compressionType cmp
            ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "objectRegistry2Templates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
