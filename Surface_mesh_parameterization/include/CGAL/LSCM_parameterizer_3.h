// Copyright (c) 2005  INRIA (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Laurent Saboret, Pierre Alliez, Bruno Levy

#ifndef CGAL_LSCM_PARAMETERIZER_3_H
#define CGAL_LSCM_PARAMETERIZER_3_H

#include <CGAL/license/Surface_mesh_parameterization.h>

#include <CGAL/internal/Surface_mesh_parameterization/Containers_filler.h>

#include <CGAL/Parameterizer_traits_3.h>
#include <CGAL/Two_vertices_parameterizer_3.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>

#include <CGAL/circulator.h>
#include <CGAL/Timer.h>

#ifdef CGAL_EIGEN3_ENABLED
#include <CGAL/Eigen_solver_traits.h>
#endif
#include <CGAL/OpenNL/linear_solver.h>

#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>

#include <iostream>
#include <vector>

/// \file LSCM_parameterizer_3.h

namespace CGAL {

// ------------------------------------------------------------------------------------
// Declaration
// ------------------------------------------------------------------------------------

/// \ingroup  PkgSurfaceParameterizationMethods
///
/// The class `LSCM_parameterizer_3` implements the
/// *Least Squares Conformal Maps (LSCM)* parameterization  \cgalCite{cgal:lprm-lscm-02}.
///
/// This is a conformal parameterization, i.e. it attempts to preserve angles.
///
/// This is a free border parameterization. There is no need to map the border
/// of the surface onto a convex polygon (only two pinned vertices are needed
/// to ensure a unique solution), but a one-to-one mapping is *not* guaranteed.
///
/// \cgalModels `ParameterizerTraits_3`
///
/// \tparam TriangleMesh                 a model of `FaceGraph`
/// \tparam BorderParameterizer_3        Strategy to parameterize the surface border. <br>
///         Note: The minimum is to parameterize two vertices.
/// \tparam SparseLinearAlgebraTraits_d  Traits class to solve a sparse linear system. <br>
///         Note: We may use a symmetric definite positive solver because LSCM
///         solves the system in the least squares sense.
///
/// \sa `CGAL::Parameterizer_traits_3<TriangleMesh>`
/// \sa `CGAL::Fixed_border_parameterizer_3<TriangleMesh, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::ARAP_parameterizer_3<TriangleMesh, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::Barycentric_mapping_parameterizer_3<TriangleMesh, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::Discrete_authalic_parameterizer_3<TriangleMesh, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::Discrete_conformal_map_parameterizer_3<TriangleMesh, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::Mean_value_coordinates_parameterizer_3<TriangleMesh, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`

template
<
  class TriangleMesh,
  class BorderParameterizer_3
    = Two_vertices_parameterizer_3<TriangleMesh>,
  class SparseLinearAlgebraTraits_d
#if defined(CGAL_EIGEN3_ENABLED) || defined(DOXYGEN_RUNNING)
    = Eigen_solver_traits<Eigen::SimplicialLDLT<Eigen_sparse_symmetric_matrix<double>::EigenType> >
#else
    = OpenNL::SymmetricLinearSolverTraits<typename TriangleMesh::NT>
#endif
>
class LSCM_parameterizer_3
  : public Parameterizer_traits_3<TriangleMesh>
{
// Private types
private:
  // Superclass
  typedef Parameterizer_traits_3<TriangleMesh>        Base;

// Public types
public:
  // We have to repeat the types exported by superclass
  /// @cond SKIP_IN_MANUAL
  typedef typename Base::Error_code       Error_code;
  /// @endcond

  /// Export BorderParameterizer_3 template parameter.
  typedef BorderParameterizer_3           Border_param;

// Private types
private:
  typedef SparseLinearAlgebraTraits_d     Sparse_LA;

// Private types
private:
  typedef typename boost::graph_traits<TriangleMesh>::vertex_descriptor vertex_descriptor;
  typedef typename boost::graph_traits<TriangleMesh>::halfedge_descriptor halfedge_descriptor;
  typedef typename boost::graph_traits<TriangleMesh>::face_descriptor face_descriptor;
  typedef typename boost::graph_traits<TriangleMesh>::face_iterator face_iterator;
  typedef typename boost::graph_traits<TriangleMesh>::vertex_iterator vertex_iterator;

  // Mesh_Adaptor_3 subtypes:
  typedef Parameterizer_traits_3<TriangleMesh>  Traits;
  typedef typename Traits::NT                   NT;
  typedef typename Traits::Point_2              Point_2;
  typedef typename Traits::Point_3              Point_3;
  typedef typename Traits::Vector_2             Vector_2;
  typedef typename Traits::Vector_3             Vector_3;

  // SparseLinearAlgebraTraits_d subtypes:
  typedef typename Sparse_LA::Vector      Vector;
  typedef typename Sparse_LA::Matrix      Matrix;

  typedef typename OpenNL::LinearSolver<Sparse_LA>     LeastSquaresSolver;

// Public operations
public:
  /// Constructor
  LSCM_parameterizer_3(Border_param border_param = Border_param(),
                       ///< %Object that maps the surface's border to 2D space
                       Sparse_LA sparse_la = Sparse_LA())
                       ///< Traits object to access a sparse linear system
    : m_borderParameterizer(border_param), m_linearAlgebra(sparse_la)
  { }

  // Default copy constructor and operator =() are fine

  /// Compute a one-to-one mapping from a triangular 3D surface mesh
  /// to a piece of the 2D space.
  /// The mapping is piecewise linear (linear in each triangle).
  /// The result is the (u,v) pair image of each vertex of the 3D surface.
  ///
  /// \tparam VertexUVmap must be a property map that associates a %Point_2
  ///         (type deduced by `Parameterized_traits_3`) to a `vertex_descriptor`
  ///         (type deduced by the graph traits of `TriangleMesh`).
  /// \tparam VertexIndexMap must be a property map that associates a unique integer index
  ///         to a `vertex_descriptor` (type deduced by the graph traits of `TriangleMesh`).
  /// \tparam VertexParameterizedMap must be a property map that associates a boolean
  ///         to a `vertex_descriptor` (type deduced by the graph traits of `TriangleMesh`).
  ///
  /// \param mesh a triangulated surface.
  /// \param bhd an halfedge descriptor on the boundary of `mesh`.
  /// \param uvmap an instanciation of the class `VertexUVmap`.
  /// \param vimap an instanciation of the class `VertexIndexMap`.
  /// \param vpmap an instanciation of the class `VertexParameterizedMap`.
  ///
  /// \pre `mesh` must be a surface with one connected component.
  /// \pre `mesh` must be a triangular mesh.
  /// \pre The vertices must be indexed (`vimap` must be initialized).
  ///
  template <typename VertexUVmap, typename VertexIndexMap, typename VertexParameterizedMap>
  Error_code parameterize(TriangleMesh& mesh,
                          halfedge_descriptor bhd,
                          VertexUVmap uvmap,
                          VertexIndexMap vimap,
                          VertexParameterizedMap vpmap)
  {
    // Count vertices
    int nbVertices = num_vertices(mesh);

    // Index vertices from 0 to nbVertices-1
    // TODO mesh.index_mesh_vertices();

    // Compute (u,v) for (at least two) border vertices
    // and mark them as "parameterized"
    Error_code status = get_border_parameterizer().parameterize_border(mesh,bhd,uvmap,vpmap);

    if(status != Base::OK)
      return status;

    // Create sparse linear system "A*X = B" of size 2*nbVertices x 2*nbVertices
    // (in fact, we need only 2 lines per triangle x 1 column per vertex)
    LeastSquaresSolver solver(2 * nbVertices);
    solver.set_least_squares(true) ;

    // Initialize the "A*X = B" linear system after
    // (at least two) border vertices parameterization
    initialize_system_from_mesh_border(solver, mesh, uvmap, vimap, vpmap);

    // Fill the matrix for the other vertices
    solver.begin_system();
    std::vector<face_descriptor> ccfaces;
    boost::unordered_set<vertex_descriptor> ccvertices;
    CGAL::internal::Surface_mesh_parameterization::Containers_filler<TriangleMesh>
                                                  fc(mesh, ccfaces, ccvertices);
    CGAL::Polygon_mesh_processing::connected_component(
                                      face(opposite(bhd,mesh),mesh),
                                      mesh,
                                      boost::make_function_output_iterator(fc));
    BOOST_FOREACH(face_descriptor fd, ccfaces){
      // Create two lines in the linear system per triangle (one for u, one for v)
      status = setup_triangle_relations(solver, mesh, fd, vimap);
      if (status != Base::OK)
        return status;
    }

    solver.end_system() ;

    // Solve the "A*X = B" linear system in the least squares sense
    if(! solver.solve())
      status = Base::ERROR_CANNOT_SOLVE_LINEAR_SYSTEM;

    if(status != Base::OK)
      return status;

    // Copy X coordinates into the (u,v) pair of each vertex
    //set_mesh_uv_from_system(mesh, solver, uvmap);

    BOOST_FOREACH(vertex_descriptor vd, ccvertices){
      int index = get(vimap,vd);
      NT u = solver.variable(2 * index).value();
      NT v = solver.variable(2 * index + 1).value();
      put(uvmap, vd, Point_2(u,v));
    }
    return status;
  }

// Private operations
private:
  /// Initialize "A*X = B" linear system after
  /// (at least two) border vertices are parameterized.
  ///
  /// \pre Vertices must be indexed.
  /// \pre X and B must be allocated and empty.
  /// \pre At least 2 border vertices must be parameterized.
  template <typename UVmap, typename VertexIndexMap, typename VertexParameterizedMap>
  void initialize_system_from_mesh_border(LeastSquaresSolver& solver,
                                          const TriangleMesh& mesh,
                                          UVmap uvmap,
                                          VertexIndexMap vimap,
                                          VertexParameterizedMap vpmap)
  {
    BOOST_FOREACH(vertex_descriptor v, vertices(mesh)){
      // Get vertex index in sparse linear system
      int index = get(vimap, v);

      // Get vertex (u,v) (meaningless if vertex is not parameterized)
      Point_2 uv = get(uvmap, v);
      // TODO: it is meaningless but must it be called for non-border vertices??
      // Write (u,v) in X (meaningless if vertex is not parameterized)
      // Note  : 2*index     --> u
      //         2*index + 1 --> v
      solver.variable(2 * index    ).set_value(uv.x());
      solver.variable(2 * index + 1).set_value(uv.y());

      // Copy (u,v) in B if vertex is parameterized
      if (get(vpmap,v)) {
        solver.variable(2 * index    ).lock();
        solver.variable(2 * index + 1).lock();
      }
    }
  }

  /// Utility for setup_triangle_relations():
  /// Computes the coordinates of the vertices of a triangle
  /// in a local 2D orthonormal basis of the triangle's plane.
  void project_triangle(const Point_3& p0, const Point_3& p1, const Point_3& p2,  // in
                        Point_2& z0, Point_2& z1, Point_2& z2);                   // out

  /// Create two lines in the linear system per triangle (one for u, one for v).
  ///
  /// \pre vertices must be indexed.
  template <typename VertexIndexMap >
  Error_code setup_triangle_relations(LeastSquaresSolver& solver,
                                      const TriangleMesh& mesh,
                                      face_descriptor facet,
                                      VertexIndexMap vimap);

// Private accessors
private:
  /// Get the object that maps the surface's border onto a 2D space.
  Border_param& get_border_parameterizer() { return m_borderParameterizer; }

  /// Get the sparse linear algebra (traits object to access the linear system).
  Sparse_LA& get_linear_algebra_traits() { return m_linearAlgebra; }

// Fields
private:
  /// %Object that maps (at least two) border vertices onto a 2D space
  Border_param m_borderParameterizer;

  /// Traits object to solve a sparse linear system
  Sparse_LA m_linearAlgebra;
};

// Utility for setup_triangle_relations():
// Computes the coordinates of the vertices of a triangle
// in a local 2D orthonormal basis of the triangle's plane.
template<class TriangleMesh, class Border_param, class Sparse_LA>
inline
void
LSCM_parameterizer_3<TriangleMesh, Border_param, Sparse_LA>::
project_triangle(const Point_3& p0, const Point_3& p1, const Point_3& p2,   // in
                 Point_2& z0, Point_2& z1, Point_2& z2)                     // out
{
  Vector_3 X = p1 - p0;
  NT X_norm = std::sqrt(X * X);
  if(X_norm != 0.0)
    X = X / X_norm;

  Vector_3 Z = CGAL::cross_product(X, p2 - p0);
  NT Z_norm = std::sqrt(Z * Z);
  if(Z_norm != 0.0)
    Z = Z / Z_norm;

  Vector_3 Y = CGAL::cross_product(Z, X);

  const Point_3& O = p0;

  NT x0 = 0;
  NT y0 = 0;
  NT x1 = std::sqrt( (p1 - O)*(p1 - O) );
  NT y1 = 0;
  NT x2 = (p2 - O) * X;
  NT y2 = (p2 - O) * Y;

  z0 = Point_2(x0, y0);
  z1 = Point_2(x1, y1);
  z2 = Point_2(x2, y2);
}

/// Create two lines in the linear system per triangle (one for u, one for v).
///
/// \pre vertices of `mesh` must be indexed.
// Implementation note: LSCM equation is:
//       (Z1 - Z0)(U2 - U0) = (Z2 - Z0)(U1 - U0)
// where Uk = uk + i.v_k is the complex number corresponding to (u,v) coords
//       Zk = xk + i.yk is the complex number corresponding to local (x,y) coords
// cool: no divide with this expression; makes it more numerically stable
// in presence of degenerate triangles
template<class TriangleMesh, class Border_param, class Sparse_LA>
  template <typename VertexIndexMap>
inline
typename LSCM_parameterizer_3<TriangleMesh, Border_param, Sparse_LA>::Error_code
LSCM_parameterizer_3<TriangleMesh, Border_param, Sparse_LA>::
setup_triangle_relations(LeastSquaresSolver& solver,
                         const TriangleMesh& mesh,
                         face_descriptor facet,
                         VertexIndexMap vimap)
{
  typedef typename boost::property_map<TriangleMesh, boost::vertex_point_t>::const_type PPmap;
  PPmap ppmap = get(vertex_point, mesh);

  // Get the 3 vertices of the triangle
  vertex_descriptor v0, v1, v2;
  halfedge_descriptor h0 = halfedge(facet, mesh);
  v0 = target(h0, mesh);
  halfedge_descriptor h1 = next(h0, mesh);
  v1 = target(h1, mesh);
  halfedge_descriptor h2 = next(h1, mesh);
  v2 = target(h2, mesh);

  // Get the vertices index
  int id0 = get(vimap, v0);
  int id1 = get(vimap, v1);
  int id2 = get(vimap, v2);

  // Get the vertices position
  const Point_3& p0 = get(ppmap, v0);
  const Point_3& p1 = get(ppmap, v1);
  const Point_3& p2 = get(ppmap, v2);

  // Computes the coordinates of the vertices of a triangle
  // in a local 2D orthonormal basis of the triangle's plane.
  Point_2 z0, z1, z2;
  project_triangle(p0, p1, p2, //in
                   z0, z1, z2); // out
  Vector_2 z01 = z1 - z0;
  Vector_2 z02 = z2 - z0;
  NT a = z01.x();
  NT b = z01.y();
  NT c = z02.x();
  NT d = z02.y();
  CGAL_assertion(b == 0.0);

  // Create two lines in the linear system per triangle (one for u, one for v)
  // LSCM equation is:
  //       (Z1 - Z0)(U2 - U0) = (Z2 - Z0)(U1 - U0)
  // where Uk = uk + i.v_k is the complex number corresponding to (u,v) coords
  //       Zk = xk + i.yk is the complex number corresponding to local (x,y) coords
  //
  // Note  : 2*index     --> u
  //         2*index + 1 --> v
  int u0_id = 2*id0    ;
  int v0_id = 2*id0 + 1;
  int u1_id = 2*id1    ;
  int v1_id = 2*id1 + 1;
  int u2_id = 2*id2    ;
  int v2_id = 2*id2 + 1;

  // Real part
  // Note: b = 0
  solver.begin_row();
  solver.add_coefficient(u0_id, -a+c);
  solver.add_coefficient(v0_id,  b-d);
  solver.add_coefficient(u1_id,   -c);
  solver.add_coefficient(v1_id,    d);
  solver.add_coefficient(u2_id,    a);
  solver.end_row();
  //
  // Imaginary part
  // Note: b = 0
  solver.begin_row();
  solver.add_coefficient(u0_id, -b+d);
  solver.add_coefficient(v0_id, -a+c);
  solver.add_coefficient(u1_id,   -d);
  solver.add_coefficient(v1_id,   -c);
  solver.add_coefficient(v2_id,    a);
  solver.end_row();

  return Base::OK;
}

} // namespace CGAL

#endif // CGAL_LSCM_PARAMETERIZER_3_H
