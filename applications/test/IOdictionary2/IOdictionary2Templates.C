/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016 OpenFOAM Foundation
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

#include "IOdictionary2.H"
#include "objectRegistry.H"
#include "Pstream.H"

// * * * * * * * * * * * * * * * Members Functions * * * * * * * * * * * * * //

// template<class Type>
// bool Foam::IOdictionary2::readFromParent()
// {
//     //Pout<< "IOdictionary2::readFromParent() :"
//     //    << " :" << objectPath()
//     //    << " ev:" << fileEventNo_ << endl;
// 
//     bool ok = false;
//     if
//     (
//        &db()
//      != dynamic_cast<const objectRegistry*>(&db().time())
//     )
//     {
//         // Pout<< "IOdictionary2::readFromParent() :"
//         //     << " : searching for:" << name()
//         //     << " in parent:" << db().parent().name()
//         //     << " with contents:" << db().parent().sortedToc()
//         //     << endl;
// 
// 
//         const Type* parentPtr =
//             lookupObjectPtr<Type>(db().parent(), name(), false);
//         if (parentPtr)
//         {
//             Type& parent = const_cast<Type&>(*parentPtr);
// 
//             //Pout<< "IOdictionary2::readFromParent() :"
//             //    << " : found parent object:" << parent.objectPath()
//             //    << " evmet:" << parent.fileEventNo_ << endl;
// 
//             ok = parent.readFromParent<Type>();
// 
//             //Pout<< "IOdictionary2::readFromParent() :"
//             //    << " : read parent " << parent.objectPath()
//             //    << "  ok:" << ok
//             //    << " parent event:" << parent.fileEventNo_
//             //    << " my event:" << fileEventNo_ << endl;
// 
//             //- Problem: event numbers are local to objectRegistry
//             //  so objects on different registries cannot be compared.
//             //  Instead store the event number from the event of the
//             //  level at which the file was read
//             if (parent.fileEventNo_ > fileEventNo_)
//             {
//                 //Pout<< "IOdictionary2::readFromParent() :"
//                 //    << " : get stream from parent " << parent.objectPath()
//                 //    << endl;
// 
//                 autoPtr<Istream> str(parent.readPart(*this));
//                 ok = readData(str());
//                 setUpToDate();
//                 fileEventNo_ = parent.fileEventNo_;
//                 //Pout<< "IOdictionary2::readFromParent() :"
//                 //    << " read myself:" << objectPath()
//                 //    << " event:" << fileEventNo_
//                 //    << " from parent:" << parent.objectPath()
//                 //    << " event:" << parent.fileEventNo_ << endl;
//             }
//             return ok;
//         }
//     }
//     else
//     {
//         // Highest registered. This is always based on file!
//         // It will be updated by readIfMotified
//         //ok = readData(readStream(typeName));
//         //close();
//     }
// 
//     return ok;
// }
//
// //- Write to parent (if it is registered)
// template<class Type>
// bool Foam::IOdictionary2::writeToParent()
// {
//     //Pout<< "IOdictionary2::writeToParent() :"
//     //    << " :" << objectPath()
//     //    << " ev:" << fileEventNo_ << endl;
//
//     bool ok = false;
//     if
//     (
//        &db()
//      != dynamic_cast<const objectRegistry*>(&db().time())
//     )
//     {
//         const Type* parentPtr =
//             lookupObjectPtr<Type>(db().parent(), name(), false);
//         if (parentPtr)
//         {
//             Type& parent = const_cast<Type&>(*parentPtr);
//
//             Pout<< "IOdictionary2::writeToParent() :"
//                 << " : found parent object:" << parent.objectPath()
//                 << " evmet:" << parent.fileEventNo_ << endl;
//
//             // Update parent from *this
//             {
//                 OStringStream os;
//                 writeData(os);
//                 IStringStream is(os.str());
//                 ok = parent.readPart(*this, is);
//             }
//
//             // Update parent
//             parent.writeToParent<Type>();
//
//             return ok;
//         }
//     }
//     else
//     {
//         // Highest registered. This is always based on file!
//         // It will be written itself (hopefully) through the
//         // objectRegistry::write()
//     }
//
//     return ok;
// }


// ************************************************************************* //
