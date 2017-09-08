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

    Uses threading if maxThreadFileBufferSize > 0.

See also
    masterUncollatedFileOperation

SourceFiles
    collatedFileOperation.C

\*---------------------------------------------------------------------------*/

#ifndef fileOperations_collatedFileOperation_H
#define fileOperations_collatedFileOperation_H

#include "masterUncollatedFileOperation.H"
#include "OFstreamCollator.H"

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

        //- Threaded writer
        mutable OFstreamCollator writer_;


   // Private Member Functions

        //- Append to processors/ file
        bool appendObject
        (
            const regIOobject& io,
            const fileName& pathName,
            IOstream::streamFormat fmt,
            IOstream::versionNumber ver,
            IOstream::compressionType cmp
        ) const;

        static word findInstancePath
        (
            const instantList& timeDirs,
            const instant& t
        );


public:

        //- Runtime type information
        TypeName("collated");


    // Static data

        //- Return the processors directory name (usually "processors")
        static word processorsDir;

        //- Max size of thread buffer size. This is the overall size of
        //  all files to be written. Starts blocking if not enough size.
        //  Read as float to enable easy specificiation of large sizes.
        static float maxThreadFileBufferSize;


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


        // Helpers

            //- Like io.path with provided instance and any 'processorXXX'
            //  replaced by 3rd argument (usually 'processsors')
            static fileName processorsPath
            (
                const IOobject&,
                const word&,
                const word& processorsDir
            );

            //- Operating on fileName: replace processorXXX with processorsDir
            static fileName processorsPath
            (
                const fileName&,
                const word& processorsDir
            );

            //- Generate equivalent file under processors/ and check for
            //  existence
            static fileName processorsFilePath
            (
                const bool isFile,
                const fileName& fName
            );

            //- Generate equivalent file under processors/ and check for
            //  existence
            static fileName processorsFilePath
            (
                const bool isFile,
                const IOobject& io,
                const word& instance
            );

            //- Search for object; return info on how it was found
            //    checkGlobal : also check undecomposed case
            //    isFile      : true:check for file  false:check for directory
            static fileName filePathInfo
            (
                const bool checkGlobal,
                const bool isFile,
                const IOobject&,
                const HashPtrTable<instantList>& times,
                pathType&,
                word&
            );

            //- Construct path on local processor
            static fileName objectPath
            (
                const IOobject& io,
                const pathType& searchType,
                const word& instancePath
            );

            //- Read header info from is and extract processor data as
            //  stream. Assumes io.objectPath contains processorDDD/
            static autoPtr<ISstream> readProcStream
            (
                IOobject& io,
                Istream& is
            );

            //- Helper to find number of processors
            static label numProcs
            (
                const fileName& dir,
                const fileName& local
            );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fileOperations
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
