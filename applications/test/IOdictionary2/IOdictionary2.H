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
#include "objectRegistry.H"
#include "IOstreams.H"

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

        //- eventNo (on database holding file) of last update
        label fileEventNo_;


    // Private Member Functions

        //- Read dictionary from file
        //  Parallel aware reading, using non-virtual type information
        //  (typeName instead of type()) because of use in constructor.
        void readFile(const bool);

        //- Read IOdictionary and register
        static IOdictionary2* readFileAndRegister(const IOobject& io);

public:

    TypeName("dictionary");


    // Constructors

        //- Construct given an IOobject
        IOdictionary2(const IOobject&);

        //- Construct given an IOobject and dictionary
        IOdictionary2(const IOobject&, const dictionary&);

        //- Construct given an IOobject and Istream
        IOdictionary2(const IOobject&, Istream&);

        //- Construct given an IOobject with parent-reading
        IOdictionary2(const IOobject&, const bool allowIndirect);


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



    // New

        // To be moved into objectRegistry

            //- Lookup object. Optionally recursive
            template<class Type>
            static const Type* lookupObjectPtr
            (
                const objectRegistry& obr,
                const word& name,
                const bool recurse
            )
            {
                objectRegistry::const_iterator iter = obr.find(name);
                if (iter != obr.end())
                {
                    return dynamic_cast<const Type*>(iter());
                }
                else if
                (
                    recurse
                 && &obr != dynamic_cast<const objectRegistry*>(&obr.time())
                )
                {
                    return lookupObjectPtr<Type>(obr.parent(), name, recurse);
                }
                else
                {
                    return NULL;
                }
            }

            //- Read from parent (if it is registered)
            //template<class Type>
            bool readFromParent();

            //- Write to parent (if it is registered)
            template<class Type>
            bool writeToParent();


        //- Return stream for obj to read itself
        virtual autoPtr<Istream> readPart(const IOobject& obj) const;

        //- Read part from Istream
        virtual bool readPart(const IOobject& child, Istream&);

        //- Read dictionary from parent or file
        virtual bool read();

        //- Write using given format, version and compression
        virtual bool writeObject
        (
            IOstream::streamFormat,
            IOstream::versionNumber,
            IOstream::compressionType
        ) const;

//XXXXX
        //- Override/set entry. Move into dictionary
        static void setScoped
        (
            dictionary& dict,
            const UList<word>& scope,
            const dictionary&
        );
        //- Construct key for use in dictionary scoped lookup
        static word scopedKey(const word&, const fileName&, const fileName&);

        //- Search up for file
        static autoPtr<IOobject> findFile(const IOobject& io);

        //- Read dictionary from parent or file
        virtual bool read2();

        //- Return true if the object's file (or files for objectRegistry)
        //  have been modified. (modified state is cached by Time)
        virtual bool modified() const;

        //- Read object if modified (as set by call to modified)
        virtual bool readIfModified();

        //- Write using given format, version and compression
        virtual bool writeObject2
        (
            IOstream::streamFormat,
            IOstream::versionNumber,
            IOstream::compressionType
        ) const;
//XXXXX

    // Member operators

        //- Assignment of other IOdictionary's entries to this IOdictionary2
        void operator=(const IOdictionary2&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "IOdictionary2Templates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
