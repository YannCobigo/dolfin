// Copyright (C) 2010 Garth N. Wells
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
// along with DOLFIN. If not, see <http://www.nu.org/licenses/>.
// 
// Modified by Cobigo Logg 2013-
// 
// First added:  2013-09-09
// Last changed: 

#ifdef HAS_VIENNACL

#include "ViennaCLSolver.h"
#include "ViennaCLPreconditioner.h"


namespace dolfin
{

  //-----------------------------------------------------------------------------
  ViennaCLSolverIteratif::ViennaCLSolverIteratif( std::string preconditioner )
  {
    // Set preconditioner
    if ( preconditioner == "none" || preconditioner == "" )
      _preconditioner.reset( new ViennaCLPreconditionerNoPreconditioning(preconditioner) );
    else if ( preconditioner == "ilut" || preconditioner == "default" )
      _preconditioner.reset( new ViennaCLPreconditionerILUT(preconditioner) );
    else if ( preconditioner == "ilu0")
      _preconditioner.reset( new ViennaCLPreconditionerILU0(preconditioner) );
    else if ( preconditioner == "block_ilu")
      _preconditioner.reset( new ViennaCLPreconditionerBlockILU(preconditioner) );
    else if ( preconditioner == "block_ilu")
      _preconditioner.reset( new ViennaCLPreconditionerBlockILU(preconditioner) );
    else
      dolfin_error("ViennaCLSolver.cpp",
		   "create ViennaCL preconditioner",
		   "Unknown preconditioner type (\"%s\")", preconditioner.c_str());
  };
  //-----------------------------------------------------------------------------

  ViennaCLSolverIterCG::ViennaCLSolverIterCG( std::string preconditioner ): 
    ViennaCLSolverIteratif(preconditioner),
    _custom_tag(new viennacl::linalg::cg_tag( static_cast<double>(parameters("krylov_solver")["relative_tolerance"]),
					      static_cast<int>(parameters("krylov_solver")["maximum_iterations"]) ))
  {};
  //-----------------------------------------------------------------------------

  ublas_vector   
  ViennaCLSolverIterCG::solve( const ublas_sparse_matrix& A, const ublas_vector& b)
  {
    return _preconditioner->solve(A, b, *this);
  };
  //-----------------------------------------------------------------------------

  ViennaCLSolverIterBiCGStab::ViennaCLSolverIterBiCGStab( std::string preconditioner ): 
    ViennaCLSolverIteratif(preconditioner),
    _custom_tag(new viennacl::linalg::bicgstab_tag( static_cast<double>(parameters("krylov_solver")["relative_tolerance"]),
						    static_cast<int>(parameters("krylov_solver")["maximum_iterations"]),
						    static_cast<int>(parameters("krylov_solver")["restart"]) ))
  {};
  //-----------------------------------------------------------------------------

  ublas_vector   
  ViennaCLSolverIterBiCGStab::solve( const ublas_sparse_matrix& A, const ublas_vector& b)
  {
    return _preconditioner->solve(A, b, *this);
  };
  //-----------------------------------------------------------------------------

  ViennaCLSolverIterGMRES::ViennaCLSolverIterGMRES( std::string preconditioner ): 
    ViennaCLSolverIteratif(preconditioner),
    _custom_tag(new viennacl::linalg::gmres_tag(/* static_cast<double>(parameters("krylov_solver")["relative_tolerance"]),
						 static_cast<int>(parameters("krylov_solver")["maximum_iterations"]),
						 static_cast<int>(parameters("krylov_solver")["restart"]) */))

  {};
 //-----------------------------------------------------------------------------

  ublas_vector   
  ViennaCLSolverIterGMRES::solve( const ublas_sparse_matrix& A, const ublas_vector& b)
  {
    return _preconditioner->solve(A, b, *this);
  };
  //-----------------------------------------------------------------------------
}
#endif
