// Copyright (C) 2012 Johannes Ring
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
// Modified by Joachim B Haga 2012
// Modified by Benjamin Kehlet 2012-2013
//
// First added:  2012-05-10
// Last changed: 2013-06-22

#include "CSGCGALMeshGenerator2D.h"
#include "CSGGeometry.h"
#include "CSGOperators.h"
#include "CSGPrimitives2D.h"
#include "CSGCGALDomain2D.h"

#include <dolfin/common/constants.h>
#include <dolfin/math/basic.h>
#include <dolfin/mesh/Mesh.h>
#include <dolfin/mesh/MeshEditor.h>
#include <dolfin/mesh/MeshFunction.h>
#include <dolfin/mesh/MeshDomains.h>
#include <dolfin/mesh/MeshValueCollection.h>
#include <dolfin/log/log.h>


#include <vector>
#include <cmath>
#include <limits>


#ifdef HAS_CGAL
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel Inexact_Kernel;

typedef CGAL::Triangulation_vertex_base_2<Inexact_Kernel>  Vertex_base;
typedef CGAL::Constrained_triangulation_face_base_2<Inexact_Kernel> Face_base;

template <class Gt, class Fb >
class Enriched_face_base_2 : public Fb 
{
 public:
  typedef Gt Geom_traits;
  typedef typename Fb::Vertex_handle Vertex_handle;
  typedef typename Fb::Face_handle Face_handle;

  template <typename TDS2>
  struct Rebind_TDS 
  {
    typedef typename Fb::template Rebind_TDS<TDS2>::Other Fb2;
    typedef Enriched_face_base_2<Gt,Fb2> Other;
  };

protected:
  int status;
  bool in_domain;

public:
  Enriched_face_base_2(): Fb(), status(-1) {}

  Enriched_face_base_2(Vertex_handle v0,
                       Vertex_handle v1,
                       Vertex_handle v2)
    : Fb(v0,v1,v2), status(-1), in_domain(true) {}

  Enriched_face_base_2(Vertex_handle v0,
                       Vertex_handle v1,
                       Vertex_handle v2,
                       Face_handle n0,
                       Face_handle n1,
                       Face_handle n2)
    : Fb(v0,v1,v2,n0,n1,n2), status(-1), in_domain(true) {}

  inline
  bool is_in_domain() const
  //{ return (status%2 == 1); }
  { return in_domain; }

  inline
  void set_in_domain(const bool b)
  //{ status = (b ? 1 : 0); }
  { in_domain = b; }

  inline
  void set_counter(int i)
  { status = i; }

  inline
  int counter() const
  { return status; }

  inline
  int& counter()
  { return status; }
};

typedef CGAL::Triangulation_vertex_base_2<Inexact_Kernel> Vb;
typedef CGAL::Triangulation_vertex_base_with_info_2<std::size_t, Inexact_Kernel, Vb> Vbb;
typedef Enriched_face_base_2<Inexact_Kernel, Face_base> Fb;
typedef CGAL::Triangulation_data_structure_2<Vbb, Fb> TDS;
typedef CGAL::Exact_predicates_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<Inexact_Kernel, TDS, Itag> CDT;
typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Mesh_criteria_2;
typedef CGAL::Delaunay_mesher_2<CDT, Mesh_criteria_2> CGAL_Mesher_2;

typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Face_handle Face_handle;
typedef CDT::All_faces_iterator All_faces_iterator;

typedef Inexact_Kernel::Point_2 Point_2;

using namespace dolfin;

//-----------------------------------------------------------------------------
CSGCGALMeshGenerator2D::CSGCGALMeshGenerator2D(const CSGGeometry& geometry)
: geometry(geometry)
{
  parameters = default_parameters();
  //subdomains.push_back(reference_to_no_delete_pointer(geometry));
}
//-----------------------------------------------------------------------------
CSGCGALMeshGenerator2D::~CSGCGALMeshGenerator2D() {}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void print_edge(CDT::Edge edge)
{
  const int i = edge.second;
  std::cout << "Edge: (" << edge.first->vertex( (i+1)%3 )->point()
            << "), (" << edge.first->vertex( (i+2)%3 )->point() << ")" << std::endl;
}
//-----------------------------------------------------------------------------
static void print_face(const CDT::Face_handle f)
{
    std::cout << "Face:" << std::endl;
    std::cout << "  " << f->vertex(0)->point() << std::endl;
    std::cout << "  " << f->vertex(1)->point() << std::endl;
    std::cout << "  " << f->vertex(2)->point() << std::endl;
    std::cout << "  " << (f->is_in_domain() ? "In domain" : "Not in domain") << std::endl;
}

//-----------------------------------------------------------------------------
// print the faces of a triangulation
static void print_triangulation(const CDT &cdt)
{
  for (  CDT::Finite_faces_iterator it = cdt.finite_faces_begin();
         it != cdt.finite_faces_end(); ++it)
  {
    print_face(it);
  }
}
//-----------------------------------------------------------------------------
static void print_constrained_edges(const CDT& cdt)
{
  std::cout << "-- Constrained edges" << std::endl;

  for (CDT::Finite_edges_iterator it = cdt.finite_edges_begin();
       it != cdt.finite_edges_end();
       it++)
  {
      if (cdt.is_constrained(*it))
        print_edge(*it);
  }

  std::cout << "--" << std::endl;
}
//-----------------------------------------------------------------------------
void explore_subdomain(CDT &ct,
                        CDT::Face_handle start, 
                        std::list<CDT::Face_handle>& other_domains)
{
  std::list<Face_handle> queue;
  queue.push_back(start);

  while (!queue.empty())
  {
    CDT::Face_handle face = queue.front();
    queue.pop_front();

    for(int i = 0; i < 3; i++) 
    {
      Face_handle n = face->neighbor(i);
      if (ct.is_infinite(n))
        continue;

      const CDT::Edge e(face,i);

      if (n->counter() == -1)
      {
        if (ct.is_constrained(e))
        {
          // Reached a border
          other_domains.push_back(n);
        } else
        {
          // Set neighbor interface to the same and push to queue
          n->set_counter(face->counter());
          n->set_in_domain(face->is_in_domain());
          queue.push_back(n);
        }
      }
    }
  }
}
//-----------------------------------------------------------------------------
// Set the member in_domain and counter for all faces in the cdt
void explore_subdomains(CDT& cdt, 
                        const CSGCGALDomain2D& total_domain,
                        const std::vector<std::pair<std::size_t, CSGCGALDomain2D> > &subdomain_geometries)
{
  std::cout << "--Exploring domains" << std::endl;

  // Set counter to -1 for all faces
  for (CDT::Finite_faces_iterator it = cdt.finite_faces_begin(); it != cdt.finite_faces_end(); ++it)
  {
    it->set_counter(-1);
    it->set_in_domain(false);
  }

  std::list<CDT::Face_handle> subdomains;
  subdomains.push_back(cdt.finite_faces_begin());

  //print_face(subdomains.front());
  //dolfin_assert(face_in_domain(subdomains.front(), total_domain));

  while (!subdomains.empty())
  {
    const CDT::Face_handle f = subdomains.front();
    subdomains.pop_front();

    if (f->counter() < 0)
    {
      // Set default marker (0)
      f->set_counter(0);

      const Point_2 p0 = f->vertex(0)->point();
      const Point_2 p1 = f->vertex(1)->point();
      const Point_2 p2 = f->vertex(2)->point();

      Point p( (p0[0] + p1[0] + p2[0]) / 3.0,
               (p0[1] + p1[1] + p2[1]) / 3.0 );
      std::cout << "Testing point: " << p << std::endl;
      // std::cout << "In domain: " << point_in_domain(p, total_domain) << std::endl;

      // Set the in_domain member (is face in the total domain)
      f->set_in_domain(total_domain.point_in_domain(p));

      for (int i = subdomain_geometries.size(); i > 0; --i)
      {
        if (subdomain_geometries[i-1].second.point_in_domain(p))
        {
          f->set_counter(subdomain_geometries[i-1].first);
          break;
        } 
      }
      
      explore_subdomain(cdt, f, subdomains);
    }
  }
}
//-----------------------------------------------------------------------------
  // Insert edges from polygon as constraints in the triangulation
void add_subdomain(CDT& cdt, const CSGCGALDomain2D& cgal_geometry, double threshold)
{

  // Insert the outer boundaries of the domain
  {
    std::list<std::vector<Point> > v;
    cgal_geometry.get_vertices(v, threshold);

    for (std::list<std::vector<Point> >::const_iterator pit = v.begin();
         pit != v.end(); ++pit)
    {
      std::vector<Point>::const_iterator it = pit->begin();
      Vertex_handle first = cdt.insert(Point_2(it->x(), it->y()));
      Vertex_handle prev = first;
      ++it;

      for(; it != pit->end(); ++it)
      {
        Vertex_handle current = cdt.insert(Point_2(it->x(), it->y()));
        cdt.insert_constraint(prev, current);
        prev = current;
      }

      cdt.insert_constraint(first, prev);
    }
  }

  // Insert holes
  {
    std::list<std::vector<Point> > holes;
    cgal_geometry.get_holes(holes, threshold);

    for (std::list<std::vector<Point> >::const_iterator hit = holes.begin();
         hit != holes.end(); ++hit)
    {

      std::vector<Point>::const_iterator pit = hit->begin();
      Vertex_handle first = cdt.insert(Point_2(pit->x(), pit->y()));
      Vertex_handle prev = first;
      ++pit;

      for(; pit != hit->end(); ++pit)
      {
        Vertex_handle current = cdt.insert(Point_2(pit->x(), pit->y()));
        cdt.insert_constraint(prev, current);
        prev = current;
      }

      cdt.insert_constraint(first, prev);
    }
  }
}
//-----------------------------------------------------------------------------
double shortest_constrained_edge(const CDT &cdt)
{
  double min_length = std::numeric_limits<double>::max();
  for (CDT::Finite_edges_iterator it = cdt.finite_edges_begin();
       it != cdt.finite_edges_end();
       it++)
  {
    if (!cdt.is_constrained(*it))
      continue;

    // An edge is an std::pair<Face_handle, int>
    // see CGAL/Triangulation_data_structure_2.h
    CDT::Face_handle f = it->first;
    const int i = it->second;

    CDT::Point p1 = f->vertex( (i+1)%3 )->point();
    CDT::Point p2 = f->vertex( (i+2)%3 )->point();

    min_length = std::min(CGAL::to_double((p1-p2).squared_length()), min_length);
  }

  return min_length;
}
//-----------------------------------------------------------------------------
void remove_short_edges(CDT& cdt, double threshold)
{
  cout << "Removing short edges" << endl;

  threshold *= threshold;
  std::cout << "Threshold: " << threshold << std::endl;


  bool removed = false;
  do
  {
    removed = false;

    for (CDT::Finite_edges_iterator it = cdt.finite_edges_begin();
         it != cdt.finite_edges_end();
         it++)
    {
      // Only care about constrained edges. Unconstrained shorted don't cause
      // problems here.
      if (!cdt.is_constrained(*it))
        continue;

      // An edge is an std::pair<Face_handle, int>
      // see CGAL/Triangulation_data_structure_2.h
      CDT::Face_handle f = it->first;
      const int i = it->second;
        
      CDT::Point p1 = f->vertex( (i+1)%3 )->point();
      CDT::Point p2 = f->vertex( (i+2)%3 )->point();
        
      if (CGAL::to_double((p1-p2).squared_length()) < threshold)
      {
        std::cout << "Found short edge: " << p1 << ", " << p2 << std::endl;

        // Remove the constraint as this edge will be removed
        cdt.remove_constraint(f, i);

        CDT::Vertex_handle to_be_removed = f->vertex( (i+1)%3 );
        CDT::Vertex_handle to_be_kept    = f->vertex( (i+2)%3 );
        std::cout << "To be removed: " << to_be_removed->point() << std::endl;
        std::cout << "To be kept: "    << to_be_kept->point() << std::endl;

        // save the vertices to which we should have constrained edges
        std::vector<Vertex_handle> constraints;
        CDT::Edge_circulator c = cdt.incident_edges(to_be_removed);
        CDT::Edge first = *c;
        do
        {
          if (cdt.is_constrained(*c))
          {
            std::cout << "Found constrained incident edge" << std::endl;
            print_edge(*c);
            cdt.remove_constraint(c->first, c->second);

            const CDT::Face_handle f = c->first;
            const int i = c->second;

            // Save the opposite vertex (we need to constrain this edge later)
            if (f->vertex( (i+1)%3) == to_be_removed)
              constraints.push_back(f->vertex( (i+2)%3 ));
            else
              constraints.push_back(f->vertex( (i+1)%3 ));
   
            std::cout << "Stored: " << constraints.back()->point() << std::endl;
         
          }
          ++c;
        } while (*c != first);

        cout << "Number of constrained edges removed: " << constraints.size() << endl;

        // Found short constrained edge to be remove
        cdt.remove(to_be_removed);

        // Recreate constraints
        for (std::vector<Vertex_handle>::iterator c_it = constraints.begin();
             c_it != constraints.end(); ++c_it)
        {
          std::cout << "Inserting constraint: " << to_be_kept->point() << ", " << (*c_it)->point() << std::endl;
          cdt.insert_constraint(to_be_kept, *c_it);
        }


        //removed = true;
        //break;
      }
    }
  } while (removed);
}
//-----------------------------------------------------------------------------
void CSGCGALMeshGenerator2D::generate(Mesh& mesh)
{
  // Create empty CGAL triangulation
  CDT cdt;

  // Convert the CSG geometry to a CGAL Polygon
  cout << "Converting geometry to polygon" << endl;
  CSGCGALDomain2D total_domain(&geometry);

  add_subdomain(cdt, total_domain, parameters["edge_minimum"]);

  // Empty polygon, will be populated when traversing the subdomains
  CSGCGALDomain2D overlaying;

  std::vector<std::pair<std::size_t, CSGCGALDomain2D> > 
    subdomain_geometries;

  // Add the subdomains to the CDT. Traverse in reverse order to get the latest
  // added subdomain on top
  std::list<std::pair<std::size_t, boost::shared_ptr<const CSGGeometry> > >::const_reverse_iterator it;

  for (it = geometry.subdomains.rbegin(); it != geometry.subdomains.rend(); ++it)
  {
    const std::size_t current_index = it->first;
    boost::shared_ptr<const CSGGeometry> current_subdomain = it->second;

    CSGCGALDomain2D cgal_geometry(current_subdomain.get());
    cgal_geometry.difference_inplace(overlaying);
    
    subdomain_geometries.push_back(std::make_pair(current_index, 
                                                  cgal_geometry));

    add_subdomain(cdt, cgal_geometry, parameters["edge_minimum"]);

    overlaying.join_inplace(cgal_geometry);
  }

  cout << "Shortest edge: " << shortest_constrained_edge(cdt) << endl;
  remove_short_edges(cdt, parameters["edge_minimum"]);
  cout << "Shortest edge: " << shortest_constrained_edge(cdt) << endl;
  
  explore_subdomains(cdt, total_domain, subdomain_geometries);

  // Create mesher
  CGAL_Mesher_2 mesher(cdt);

  // Add seeds for all faces in the domain
  std::list<Point_2> list_of_seeds;
  for(CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
      fit != cdt.finite_faces_end(); ++fit)
  {
    if (fit->is_in_domain())
    {
      // Calculate center of triangle and add to list of seeds
      Point_2 p0 = fit->vertex(0)->point();
      Point_2 p1 = fit->vertex(1)->point();
      Point_2 p2 = fit->vertex(2)->point();
      const double x = (p0[0] + p1[0] + p2[0]) / 3;
      const double y = (p0[1] + p1[1] + p2[1]) / 3;

      list_of_seeds.push_back(Point_2(x, y));
    }
  }

  mesher.set_seeds(list_of_seeds.begin(), list_of_seeds.end(), true);

  // Set shape and size criteria
  const int mesh_resolution = parameters["mesh_resolution"];
  if (mesh_resolution > 0)
  {
    const double min_radius = total_domain.compute_boundingcircle_radius();
    cout << "Min radius: " << min_radius << endl;
    const double cell_size = 2.0*min_radius/mesh_resolution;


    Mesh_criteria_2 criteria(parameters["triangle_shape_bound"],
                             cell_size);
    mesher.set_criteria(criteria);
  } 
  else
  {
    // Set shape and size criteria
    Mesh_criteria_2 criteria(parameters["triangle_shape_bound"],
                             parameters["cell_size"]);
    mesher.set_criteria(criteria);
  }

  // print_constrained_edges(cdt);

  // Refine CGAL mesh/triangulation
  mesher.refine_mesh();

  // print_constrained_edges(cdt);

  // Make sure triangulation is valid
  dolfin_assert(cdt.is_valid());

  // Mark the subdomains
  explore_subdomains(cdt, total_domain, subdomain_geometries);

  // Clear mesh
  mesh.clear();

  const std::size_t gdim = cdt.finite_vertices_begin()->point().dimension();
  const std::size_t tdim = cdt.dimension();
  const std::size_t num_vertices = cdt.number_of_vertices();

  // Count valid cells
  std::size_t num_cells = 0;
  CDT::Finite_faces_iterator cgal_cell;
  for (cgal_cell = cdt.finite_faces_begin(); 
       cgal_cell != cdt.finite_faces_end(); ++cgal_cell)
  {
    // Add cell if it is in the domain
    if (cgal_cell->is_in_domain())
    {
      num_cells++;
    }
  }

  // Create a MeshEditor and open
  dolfin::MeshEditor mesh_editor;
  mesh_editor.open(mesh, tdim, gdim);
  mesh_editor.init_vertices(num_vertices);
  mesh_editor.init_cells(num_cells);

  // Add vertices to mesh
  std::size_t vertex_index = 0;
  CDT::Finite_vertices_iterator cgal_vertex;
  for (cgal_vertex = cdt.finite_vertices_begin();
       cgal_vertex != cdt.finite_vertices_end(); ++cgal_vertex)
  {
    // Get vertex coordinates and add vertex to the mesh
    Point p;
    p[0] = cgal_vertex->point()[0];
    p[1] = cgal_vertex->point()[1];

    // Add mesh vertex
    mesh_editor.add_vertex(vertex_index, p);

    // Attach index to vertex and increment
    cgal_vertex->info() = vertex_index++;
  }

  dolfin_assert(vertex_index == num_vertices);

  // Add cells to mesh and build domain marker mesh function
  boost::shared_ptr<MeshValueCollection<std::size_t> > subdomain_marker = mesh.domains().markers(2);
  std::size_t cell_index = 0;
  for (cgal_cell = cdt.finite_faces_begin(); cgal_cell != cdt.finite_faces_end(); ++cgal_cell)
  {
    // Add cell if it is in the domain
    if (cgal_cell->is_in_domain())
    {
      mesh_editor.add_cell(cell_index,
                           cgal_cell->vertex(0)->info(),
                           cgal_cell->vertex(1)->info(),
                           cgal_cell->vertex(2)->info());

      subdomain_marker->set_value(cell_index, 0, cgal_cell->counter());
      ++cell_index;
    }
  }
  dolfin_assert(cell_index == num_cells);

  // Close mesh editor
  mesh_editor.close();
}

#else
namespace dolfin
{
  CSGCGALMeshGenerator2D::CSGCGALMeshGenerator2D(const CSGGeometry& geometry)
  {
    dolfin_error("CSGCGALMeshGenerator2D.cpp",
                 "Create mesh generator",
                 "Dolfin must be compiled with CGAL to use this feature.");
  }
  //-----------------------------------------------------------------------------
  CSGCGALMeshGenerator2D::~CSGCGALMeshGenerator2D()
  {
    // Do nothing
  }
  //-----------------------------------------------------------------------------
  void CSGCGALMeshGenerator2D::generate(Mesh& mesh)
  {
    // Do nothing
  }
}

#endif
//-----------------------------------------------------------------------------
