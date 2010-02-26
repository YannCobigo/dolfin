// Copyright (C) 2010 Garth N. Wells
// Licensed under the GNU LGPL Version 2.1.
//
// First added:  2010-02-25
// Last changed:

#ifndef __DOFLIN_TRILINOS_PRECONDITIONER_H
#define __DOFLIN_TRILINOS_PRECONDITIONER_H

#ifdef HAS_TRILINOS

#include <string>

#include <petscpc.h>
#include <boost/shared_ptr.hpp>
#include <dolfin/common/types.h>
#include <dolfin/common/Variable.h>
#include <dolfin/la/PETScObject.h>
#include <dolfin/parameter/Parameters.h>

namespace dolfin
{

  // Forward declarations
  class EpetraKrylovSolver;


  /// This class is a wrapper for configuring Epetra preconditioners. It does
  /// not own a preconditioner. It can take a EpetraKrylovSolver and set the
  /// preconditioner type and parameters.

  class TrilinosPreconditioner : public Variable
  {
  public:

    /// Create Krylov solver for a particular method and preconditioner
    explicit TrilinosPreconditioner(std::string type = "default");

    /// Destructor
    virtual ~TrilinosPreconditioner();

    /// Set the precondtioner
    virtual void set(EpetraKrylovSolver& solver) const;

    /// Return preconditioner name
    std::string name() const;

    /// Return informal string representation (pretty-print)
    std::string str(bool verbose) const;

    /// Default parameter values
    static Parameters default_parameters();

  private:

    /// Setup the ML precondtioner
    void set_ml(AztecOO& solver) const;

    /// Named preconditioner
    std::string type;

    // Available named preconditioners
    static const std::map<std::string, int> methods;
  };

}

#endif

#endif