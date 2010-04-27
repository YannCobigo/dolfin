// Copyright (C) 2010-03-17  André Massing.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by André Massing, 2010
//
// First added:  2010-03-17
// Last changed: 2010-04-09
//
// Remark: This is an adapted version of Brian Mirtichs original C code for
// computing polyhedral mass properties, which can be found at 
// http://www.cs.berkeley.edu/~jfc/mirtich/massProps.html

#ifdef HAS_CGAL

#include "BarycenterQuadrature.h"
#include <math.h>

const int X = 0;
const int Y = 1;
const int Z = 2;

#define SQR(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))

using std::sqrt;
using namespace dolfin;

typedef Nef_polyhedron_3::Plane_3 Plane_3;
typedef Nef_polyhedron_3::Point_3 Point_3;

//-----------------------------------------------------------------------------
BarycenterQuadrature::BarycenterQuadrature(const Nef_polyhedron_3 & polyhedron)
{
  compute_quadrature(polyhedron);
}

void BarycenterQuadrature::compute_quadrature(const Nef_polyhedron_3 & polyhedron)
{
  //coordinates of the bary center
  double bary_center [3] = {0,0,0};

  double _weight = 0;

  //determine if polyhedron has a non vanishing 3d measure.
  //This requires to have volumes marked with one.
  bool has_volume = false;
  Nef_polyhedron_3::Volume_const_iterator vi;
  CGAL_forall_volumes(vi,polyhedron)
  {
    if(vi->mark())
    {
      has_volume = true;
      break;
    }
  }

  //variables needing during the computation
  //components of surface normal
  double nx, ny, nz;
  double normal [3] = {0,0,0};

  //start and end point of the edge
  double source[3] = {0,0,0};
  double target[3] = {0,0,0};

  //permutation of the coordinates X,Y,Z
  int A,B,C;

  for(Nef_polyhedron_3::Halffacet_const_iterator f = polyhedron.halffacets_begin (),
	end = polyhedron.halffacets_end();
      f != end; ++f)
  {
    if (f->incident_volume()->mark() || (!has_volume && f->is_twin()))
    {
      //compute best projection direction.
      //get normal of the surface and the offset
      //FIXME: check right orientation
      Plane_3 face_plane  = f->plane();
      nx = CGAL::to_double(face_plane.a());
      ny = CGAL::to_double(face_plane.b());
      nz = CGAL::to_double(face_plane.c());

      double normal_length = sqrt(nx*nx + ny*ny + nz*nz);
      normal[0] = nx/normal_length;
      normal[1] = ny/normal_length;
      normal[2] = nz/normal_length;

      nx = fabs(nx);
      ny = fabs(ny);
      nz = fabs(nz);

      //Choose  permutation to get best conditioned projection direction.
      if (nx > ny && nx > nz)
	C = X;
      else
	C = (ny > nz) ? Y : Z;
      A = (C + 1) % 3;
      B = (A + 1) % 3;

      //begin compute the face integrals
      //begin compute the projection integrals
      double a0, a1, da;
      double b0, b1, db;
      double a0_2, b0_2;
      double a0_3, b0_3;
      double a0_4, b0_4;
      double a1_2, b1_2;
      double C1, Ca, Caa, Cb, Cbb, Cab;
      double Kab;
      double P1, Pa, Pb, Paa, Pbb, Pab;
      P1 = Pa = Pb = Paa = Pab = Pbb = 0.0;
      double Fa, Fb, Fc, Faa, Fbb, Fcc;

      for(Nef_polyhedron_3::Halffacet_cycle_const_iterator fc = f->facet_cycles_begin();
	  fc != f->facet_cycles_end(); ++fc)
      {
	//exclude a SHalfloops  (whatever this is)
	if ( fc.is_shalfedge() )
	{
	  Nef_polyhedron_3::SHalfedge_const_handle h = fc;
	  Nef_polyhedron_3::SHalfedge_around_facet_const_circulator hc(h);
	  Nef_polyhedron_3::SHalfedge_around_facet_const_circulator he(hc);
	  CGAL_For_all(hc,he)
	  {
	    Nef_polyhedron_3::SVertex_const_handle v = hc->source();

	    const Point_3 & source_point(v->source()->point());
	    const Point_3 & target_point(v->target()->point());

	    source[0] = CGAL::to_double(source_point.x());
	    source[1] = CGAL::to_double(source_point.y());
	    source[2] = CGAL::to_double(source_point.z());

	    target[0] = CGAL::to_double(target_point.x());
	    target[1] = CGAL::to_double(target_point.y());
	    target[2] = CGAL::to_double(target_point.z());

	    a0 = source[A];
	    b0 = source[B];
	    a1 = target[A];
	    b1 = target[B];

	    da = a1 - a0;
	    db = b1 - b0;

	    a0_2 = a0 * a0;
	    a0_3 = a0_2 * a0;
	    a0_4 = a0_3 * a0;
	    a1_2 = a1 * a1;
	    b0_2 = b0 * b0;
	    b0_3 = b0_2 * b0;
	    b0_4 = b0_3 * b0;
	    b1_2 = b1 * b1;

	    C1 = a1 + a0;
	    Ca = a1*C1 + a0_2;
	    Caa = a1*Ca + a0_3;
	    Cb = b1*(b1 + b0) + b0_2;
	    Cbb = b1*Cb + b0_3;
	    Cab = 3*a1_2 + 2*a1*a0 + a0_2;
	    Kab = a1_2 + 2*a1*a0 + 3*a0_2;

	    P1 += db*C1;
	    Pa += db*Ca;
	    Paa += db*Caa;
	    Pb += da*Cb;
	    Pbb += da*Cbb;
	    Pab += db*(b1*Cab + b0*Kab);
	  }
	}
      }

      P1 /= 2.0;
      Pa /= 6.0;
      Paa /= 12.0;
      Pb /= -6.0;
      Pbb /= -12.0;
      Pab /= 24.0;

      //end compute the projection integrals

      double k1, k2, k3;
      k1 = 1 / normal[C]; k2 = k1 * k1; k3 = k2 * k1;
      double w =  CGAL::to_double(face_plane.d())/normal_length ;

      Fa = k1 * Pa;
      Fb = k1 * Pb;
      Fc = -k2 * (normal[A]*Pa + normal[B]*Pb + w*P1);

      Faa = k1 * Paa;
      Fbb = k1 * Pbb;
      Fcc = k3 * (SQR(normal[A])*Paa + 2*normal[A]*normal[B]*Pab + SQR(normal[B])*Pbb
		  + w*(2*(normal[A]*Pa + normal[B]*Pb) + w*P1));

      //end computation face integrals

      if (has_volume)
      {
	_weight += normal[X] * ((A == X) ? Fa : ((B == X) ? Fb : Fc));
	bary_center[A] += normal[A] * Faa;
	bary_center[B] += normal[B] * Fbb;
	bary_center[C] += normal[C] * Fcc;
      }
      else
      {
	_weight += -P1/normal[C];
	bary_center[A] += -Fa;
	bary_center[B] += -Fb;
	bary_center[C] += -Fc;
      }
    }
  }

  if (has_volume)
  {
    bary_center[X] /= (2*_weight);
    bary_center[Y] /= (2*_weight);
    bary_center[Z] /= (2*_weight);
  }
  else
  {
    bary_center[X] /= _weight;
    bary_center[Y] /= _weight;
    bary_center[Z] /= _weight;
  }

  _weights.push_back(_weight);
  _points.push_back(Point(3,bary_center));

}
//-----------------------------------------------------------------------------
#endif