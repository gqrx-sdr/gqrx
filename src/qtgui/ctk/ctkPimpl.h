/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

/**
\page CorePimpl CTK Pimpl Macros

\brief Utility macros for handling private implementations. It is in addition
       to QtGlobal: Q_DECLARE_PRIVATE, Q_DECLARE_PUBLIC,
       Q_D and Q_Q.

Application code generally doesn't have to be concerned about hiding its
implementation details, but when writing library code it is important to
maintain a constant interface, both source and binary. Maintaining a constant
source interface is easy enough, but keeping the binary interface constant
means moving implementation details into a private class. The PIMPL, or
d-pointer, idiom is a common method of implementing this separation. ctkPimpl
offers a convenient way to connect the public and private sides of your class.

\section start Getting Started
Before you declare the public class, you need to make a forward declaration
of the private class. The private class must have the same name as the public
class, followed by the word Private.

\subsection pub The Public Class
Generally, you shouldn't keep any data members in the public class without a
good reason. Functions that are part of the public interface should be declared
in the public class, and functions that need to be available to subclasses (for
calling or overriding) should be in the protected section of the public class.
To connect the private class to the public class, include the
Q_DECLARE_PRIVATE macro in the private section of the public class.

Additionally, you must construct the d_ptr pointer in the constructor of the
 public class.

\subsection priv The Private Class
As mentioned above, data members should usually be kept in the private class.
This allows the memory layout of the private class to change without breaking
binary compatibility for the public class. Functions that exist only as
implementation details, or functions that need access to private data members,
should be implemented here.

To define the private class, nothing special needs to be done, except if you
want the private class to have access to the public class. Then use
Q_DECLARE_PUBLIC and create a public class pointer member. The constructor of
the private class should take a public class reference as a parameter.

\section cross Accessing Private Members
Use the Q_D() macros from functions in
the public class to access the private class. Similarly, functions in the
private class can invoke functions in the public class by using the Q_Q()
macro.
\section example Example
Header file (ctkFooObject.h):
\code
// Qt includes
#include <QObject>

// CTK includes
#include "ctkCoreExport.h"
class ctkFooObjectPrivate;

class CTK_CORE_EXPORT ctkFooObject: public QObject
{
public:
  ctkFooObject(QObject* parent = 0);
  virtual ~ctkFooObject();

  void setProperty(double property);
  double property()const;

protected:
  QScopedPointer<ctkFooObjectPrivate> d_ptr;
 
private:
  Q_DECLARE_PRIVATE(ctkFooObject);
  Q_DISABLE_COPY(ctkFooObject);
};
\endcode
Implementation file (ctkFooObject.cpp):
\code
// CTK includes
#include "ctkFooObject.h"

class ctkFooObjectPrivate
{
public:
  void processSomething();

  double MyProperty;
};

ctkFooObject::ctkFooObject(QObject* parentObject)
  : d_ptr(new ctkFooObjectPrivate)
{
  Q_D(ctkFooObject);
  d->MyProperty = 10.;
}

void ctkFooObject::setProperty(double newProperty)
{
  Q_D(ctkFooObject);
  d->MyProperty = newProperty;
}

double ctkFooObject::property()const
{
  Q_D(const ctkFooObject);
  return d->MyProperty;
}
\endcode
*/

#ifndef __ctkPimpl_h
#define __ctkPimpl_h

// Qt includes
#include <QtGlobal>

/*!
 * \ingroup Core
 * @{
 */

/*!
 * Define a public class constructor with no argument
 *
 * Also make sure the Pimpl is initalized
 * \see \ref CorePimpl
 */
#define CTK_CONSTRUCTOR_NO_ARG_CPP(PUB)  \
  PUB::PUB(): d_ptr(new PUB##Private)    \
    {                                    \
    }

/*!
 * Define a public class constructor with one argument
 *
 * Also make sure the Pimpl is initalized
 * \see \ref CorePimpl
 */
#define CTK_CONSTRUCTOR_1_ARG_CPP(PUB, _ARG1)   \
  PUB::PUB(_ARG1 _parent)                       \
    : Superclass( _parent )                     \
    , d_ptr(new PUB##Private)                   \
    {                                           \
    }
    
/*!
 * Define the setter in the public class.
 *
 * This should be put in the .cxx file of the public class. The parameter are
 * the name of the public class (PUB), the type of the argument to return (_TYPE),
 * the name of the getter(_NAME) and the name of the variable in the Private class(_VARNAME).
 * \see \ref CorePimpl
 */
#define CTK_SET_CPP(PUB, _TYPE, _NAME, _VARNAME)    \
  void PUB::_NAME(_TYPE var)                        \
  {                                                 \
    Q_D(PUB);                                       \
    d->_VARNAME =  var;                             \
  }

/*!
 * Define the setter in the public class.
 *
 * This should be put in the .cxx file of the public class. The parameter are
 * the name of the public class (PUB), the type of the argument to return (_TYPE),
 * the name of the setter(_NAME) and the name of the variable in the Private class(_VARNAME).
 * \see \ref CorePimpl
 */
#define CTK_GET_CPP(PUB, _TYPE, _NAME, _VARNAME)   \
  _TYPE PUB::_NAME()const                          \
  {                                                \
    Q_D(const PUB);                                \
    return d->_VARNAME;                            \
  }

/**@}*/

#endif
