// Copyright (C) 2008 Martin Sandve Alnes, Kent-Andre Mardal and Johannes Ring
//
// This file is part of DOLFIN.
//
// DOLFIN is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DOLFIN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
//
// Modified by Anders Logg 2011
//
// First added:  2008-04-21
// Last changed: 2011-11-02

#ifdef HAS_TRILINOS

#include "dolfin/common/MPI.h"

#include <Epetra_MpiComm.h>
#include <Epetra_SerialComm.h>

#include "dolfin/common/SubSystemsManager.h"
#include "SparsityPattern.h"
#include "EpetraLUSolver.h"
#include "EpetraMatrix.h"
#include "EpetraVector.h"
#include "EpetraFactory.h"

#include "EpetraKrylovSolver.h"

using namespace dolfin;

// Singleton instance
EpetraFactory EpetraFactory::factory;

//-----------------------------------------------------------------------------
EpetraFactory::EpetraFactory()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
EpetraFactory::~EpetraFactory()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
std::shared_ptr<GenericMatrix> EpetraFactory::create_matrix() const
{
  std::shared_ptr<GenericMatrix> A(new EpetraMatrix);
  return A;
}
//-----------------------------------------------------------------------------
std::shared_ptr<GenericVector> EpetraFactory::create_vector() const
{
  std::shared_ptr<GenericVector> x(new EpetraVector);
  return x;
}
//-----------------------------------------------------------------------------
std::shared_ptr<TensorLayout> EpetraFactory::create_layout(std::size_t rank) const
{
  bool sparsity = false;
  if (rank > 1)
    sparsity = true;
  std::shared_ptr<TensorLayout> pattern(new TensorLayout(0, sparsity));
  return pattern;
}
//-----------------------------------------------------------------------------
std::shared_ptr<GenericLinearOperator> EpetraFactory::create_linear_operator() const
{
  std::shared_ptr<GenericLinearOperator> A(new NotImplementedLinearOperator);
  return A;
}
//-----------------------------------------------------------------------------
std::shared_ptr<GenericLUSolver> EpetraFactory::create_lu_solver(std::string method) const
{
  std::shared_ptr<GenericLUSolver> solver(new EpetraLUSolver(method));
  return solver;
}
//-----------------------------------------------------------------------------
std::shared_ptr<GenericLinearSolver> EpetraFactory::create_krylov_solver(std::string method,
                                              std::string preconditioner) const
{
  std::shared_ptr<GenericLinearSolver> solver(new EpetraKrylovSolver(method, preconditioner));
  return solver;
}
//-----------------------------------------------------------------------------
std::vector<std::pair<std::string, std::string> >
  EpetraFactory::lu_solver_methods() const
{
  return EpetraLUSolver::methods();
}
//-----------------------------------------------------------------------------
std::vector<std::pair<std::string, std::string> >
EpetraFactory::krylov_solver_methods() const
{
  return EpetraKrylovSolver::methods();
}
//-----------------------------------------------------------------------------
std::vector<std::pair<std::string, std::string> >
  EpetraFactory::krylov_solver_preconditioners() const
{
  return EpetraKrylovSolver::preconditioners();
}
//-----------------------------------------------------------------------------
#endif
