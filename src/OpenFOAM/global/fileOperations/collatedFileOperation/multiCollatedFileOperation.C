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

\*---------------------------------------------------------------------------*/

#include "multiCollatedFileOperation.H"
#include "addToRunTimeSelectionTable.H"
#include "PackedBoolList.H"

/* * * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * */

namespace Foam
{
namespace fileOperations
{
    defineTypeNameAndDebug(multiCollatedFileOperation, 0);
    addToRunTimeSelectionTable
    (
        fileOperation,
        multiCollatedFileOperation,
        word
    );

    // Mark as needing threaded mpi
    addNamedToRunTimeSelectionTable
    (
        fileOperationInitialise,
        multiCollatedFileOperationInitialise,
        word,
        multiCollated
    );
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::labelList Foam::fileOperations::multiCollatedFileOperation::ioRanks()
{
    labelList ioRanks;
    if (!Pstream::parRun())
    {
        string ioRanksString(getEnv("FOAM_IORANKS"));
        if (!ioRanksString.empty())
        {
            IStringStream is(ioRanksString);
            is >> ioRanks;
        }
    }
    return ioRanks;
}


Foam::labelList Foam::fileOperations::multiCollatedFileOperation::subRanks
(
    const label n
)
{
    DynamicList<label> subRanks(64);

    string ioRanksString(getEnv("FOAM_IORANKS"));
    if (!ioRanksString.empty())
    {
        IStringStream is(ioRanksString);
        labelList ioRanks(is);

        if (findIndex(ioRanks, 0) == -1)
        {
            FatalErrorInFunction
                << "Rank 0 (master) should be in the IO ranks. Currently "
                << ioRanks << exit(FatalError);
        }

        // The lowest numbered rank is the IO rank
        PackedBoolList isIOrank(n);
        isIOrank.set(ioRanks);

        for (label proci = Pstream::myProcNo(); proci >= 0; --proci)
        {
            if (isIOrank[proci])
            {
                // Found my master. Collect all processors with same master
                subRanks.append(proci);
                for
                (
                    label rank = proci+1;
                    rank < n && !isIOrank[rank];
                    ++rank
                )
                {
                    subRanks.append(rank);
                }
                break;
            }
        }
    }
    else
    {
        // Normal operation: one lowest rank per hostname is the writer
        const string myHostName(hostName());

        stringList hosts(Pstream::nProcs());
        hosts[Pstream::myProcNo()] = myHostName;
        Pstream::gatherList(hosts);
        Pstream::scatterList(hosts);

        // Collect procs with same hostname
        forAll(hosts, proci)
        {
            if (hosts[proci] == myHostName)
            {
                subRanks.append(proci);
            }
        }
    }
    return subRanks;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fileOperations::multiCollatedFileOperation::multiCollatedFileOperation
(
    const bool verbose
)
:
    collatedFileOperation
    (
        UPstream::allocateCommunicator
        (
            UPstream::worldComm,
            subRanks(Pstream::nProcs())
        ),
        ioRanks(),  // For serial operation: know processor directories
        typeName,
        verbose
    )
{
    if (verbose)
    {
        // Print a bit of information
        stringList ioRanks(Pstream::nProcs());
        if (Pstream::master(comm_))
        {
            ioRanks[Pstream::myProcNo()] = hostName()+"."+name(pid());
        }
        Pstream::gatherList(ioRanks);

        Info<< "         IO nodes:" << endl;
        forAll(ioRanks, proci)
        {
            if (!ioRanks[proci].empty())
            {
                Info<< "             " << ioRanks[proci] << endl;
            }
        }
    }
}


Foam::fileOperations::multiCollatedFileOperationInitialise::
multiCollatedFileOperationInitialise(int& argc, char**& argv)
:
    collatedFileOperationInitialise(argc, argv)
{
    // Filter out any of my arguments
    const string s("-ioRanks");

    int index = -1;
    for (int i=1; i<argc-1; i++)
    {
        if (argv[i] == s)
        {
            index = i;
            setEnv("FOAM_IORANKS", argv[i+1], true);
            break;
        }
    }

    if (index != -1)
    {
        for (int i=index+2; i<argc; i++)
        {
            argv[i-2] = argv[i];
        }
        argc -= 2;
    }


    const string ioString("-ioRoots");
    index = -1;
    for (int i=1; i<argc-1; i++)
    {
        if (argv[i] == ioString)
        {
            index = i;
            setEnv("FOAM_ROOTS", argv[i+1], true);
            break;
        }
    }

    if (index != -1)
    {
        for (int i=index+2; i<argc; i++)
        {
            argv[i-2] = argv[i];
        }
        argc -= 2;
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::fileOperations::multiCollatedFileOperation::~multiCollatedFileOperation()
{
    if (comm_ != -1)
    {
        UPstream::freeCommunicator(comm_);
    }
}


// ************************************************************************* //
