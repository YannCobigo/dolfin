// Copyright (C) 2011-2012 Anders Logg and Garth N. Wells
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
// First added:  2011-02-11
// Last changed: 2012-08-22

#ifdef HAS_PETSC

#include <dolfin/log/dolfin_log.h>
#include "GenericVector.h"
#include "PETScVector.h"
#include "PETScBaseMatrix.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
PETScBaseMatrix::PETScBaseMatrix(Mat A) : _A(A)
{
  // Increase reference count
  if (_A)
    PetscObjectReference((PetscObject)_A);
}
//-----------------------------------------------------------------------------
PETScBaseMatrix::~PETScBaseMatrix()
{
  // Decrease reference count (PETSc will destroy object once
  // reference counts reached zero)
  if (_A)
    MatDestroy(&_A);
}
//-----------------------------------------------------------------------------
std::size_t PETScBaseMatrix::size(std::size_t dim) const
{
  if (dim > 1)
  {
    dolfin_error("PETScBaseMatrix.cpp",
                 "access size of PETSc matrix",
                 "Illegal axis (%d), must be 0 or 1", dim);
  }

  if (_A)
  {
    PetscInt m(0), n(0);
    PetscErrorCode ierr = MatGetSize(_A, &m, &n);
    if (ierr != 0) petsc_error(ierr, __FILE__, "MetGetSize");
    if (dim == 0)
      return m;
    else
      return n;
  }
  else
    return 0;
}
//-----------------------------------------------------------------------------
std::pair<std::size_t, std::size_t>
PETScBaseMatrix::local_range(std::size_t dim) const
{
  dolfin_assert(dim <= 1);
  if (dim == 1)
  {
    dolfin_error("PETScBaseMatrix.cpp",
                 "access local column range for PETSc matrix",
                 "Only local row range is available for PETSc matrices");
  }

  if (_A)
  {
    PetscInt m(0), n(0);
    PetscErrorCode ierr = MatGetOwnershipRange(_A, &m, &n);
    if (ierr != 0) petsc_error(ierr, __FILE__, "MatGetOwnershipRange");
    return std::make_pair(m, n);
  }
  else
    return std::make_pair(0, 0);
}
//-----------------------------------------------------------------------------
void PETScBaseMatrix::init_vector(GenericVector& z, std::size_t dim) const
{
  dolfin_assert(_A);

  PetscErrorCode ierr;

  // Downcast vector
  PETScVector& _z = as_type<PETScVector>(z);

  // Create new PETSc vector
  Vec x = NULL;
  if (dim == 0)
  {
    ierr = MatGetVecs(_A, PETSC_NULL, &x);
    if (ierr != 0) petsc_error(ierr, __FILE__, "MatGetVecs");
  }
  else if (dim == 1)
  {
    ierr = MatGetVecs(_A, &x, PETSC_NULL);
    if (ierr != 0) petsc_error(ierr, __FILE__, "MatGetVecs");
  }
  else
  {
    dolfin_error("PETScBaseMatrix.cpp",
                 "intialize PETSc vector to match PETSc matrix",
                 "Dimension must be 0 or 1, not %d", dim);
  }

  // Associate new PETSc vector with _z
  _z._x = x;
}
//-----------------------------------------------------------------------------

#endif
