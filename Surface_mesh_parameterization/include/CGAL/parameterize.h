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

#ifndef CGAL_PARAMETERIZE_H
#define CGAL_PARAMETERIZE_H

#include <CGAL/license/Surface_mesh_parameterization.h>


#include <CGAL/Mean_value_coordinates_parameterizer_3.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <boost/function_output_iterator.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

/// \file parameterize.h

namespace CGAL {

namespace internal {

template <typename Set>
class Bool_property_map
{
  typedef Set S;
  typedef Bool_property_map<S> Self;

public:
  typedef typename Set::key_type key_type;
  typedef bool value_type;
  typedef bool reference;
  typedef boost::read_write_property_map_tag category;

  friend value_type get(const Self& pm, const key_type& k)
  {
    return pm.m_s->find(k) != pm.m_s->end();
  }

  friend void put(const Self& pm, key_type& k, const value_type& v)
  {
    if(v){
      pm.m_s->insert(k);
    } else {
      pm.m_s->erase(k);
    }
  }

  Bool_property_map() : m_s(0) { }
  Bool_property_map(S& s) : m_s(&s) { }

private:
  S* m_s;
};

} // namespace internal


namespace Parameterization {

template <typename Mesh, typename Map>
struct Vertices {

  Vertices(const Mesh& mesh, Map& map)
    : mesh(mesh), map(&map), index(0)
  { }

  void operator()(const typename boost::graph_traits<Mesh>::face_descriptor& fd)
  {
    BOOST_FOREACH(typename boost::graph_traits<Mesh>::vertex_descriptor vd, vertices_around_face(halfedge(fd,mesh),mesh)){
      typename Map::iterator it;
      bool new_element;
      boost::tie(it,new_element) = map->insert(std::make_pair(vd,1));
      if(new_element){
        it->second = index++;
      }
    }
  }

  const Mesh& mesh;
  mutable Map* map;
  int index;
};

} // namespace Parameterization


/// \ingroup  PkgSurfaceParameterizationMainFunction
///
/// Compute a one-to-one mapping from a 3D triangle surface `mesh` to a
/// simple 2D domain.
/// The mapping is piecewise linear on the triangle mesh.
/// The result is a pair (u,v) of parameter coordinates for each vertex of the input mesh.
///
/// A one-to-one mapping may be guaranteed or not, depending on
/// the chosen Parameterizer algorithm.
///
/// \tparam TriangleMesh must be a model of `FaceGraph`.
/// \tparam Parameterizer must be a model of `ParameterizerTraits_3`.
/// \tparam HD must be the halfedge_descriptor type corresponding to the graph
///         traits of TriangleMesh.
/// \tparam VertexUVmap must be a property map that associates a %Point_2
///         (type deduced by `Parameterized_traits_3`) to a `vertex_descriptor`
///         (type deduced by the graph traits of `TriangleMesh`).
///
/// \param mesh a triangulated surface.
/// \param parameterizer a parameterizer.
/// \param bhd an halfedge descriptor on the boundary of `mesh`.
/// \param uvm an instanciation of the class `VertexUVmap`.
///
/// \pre `mesh` must be a surface with one connected component.
/// \pre `mesh` must be a triangular mesh.
/// \pre The mesh border must be mapped onto a convex polygon
///   (for fixed border parameterizations).
template <class TriangleMesh, class Parameterizer, class HD, class VertexUVmap>
typename Parameterizer_traits_3<TriangleMesh>::Error_code
parameterize(TriangleMesh& mesh,
             Parameterizer parameterizer,
             HD bhd,
             VertexUVmap uvm)
{
  typedef typename boost::graph_traits<TriangleMesh>::vertex_descriptor vertex_descriptor;
  typedef boost::unordered_map<vertex_descriptor,int> Indices;
  Indices indices;
  boost::unordered_set<vertex_descriptor> vs;
  internal::Bool_property_map<boost::unordered_set<vertex_descriptor> > vpm(vs);
  CGAL::Polygon_mesh_processing::connected_component(face(opposite(bhd,mesh),mesh),
                                                     mesh,
                                                     boost::make_function_output_iterator(Parameterization::Vertices<TriangleMesh,Indices>(mesh,indices)));
  return parameterizer.parameterize(mesh, bhd, uvm, boost::make_assoc_property_map(indices), vpm);
}


/// \ingroup  PkgSurfaceParameterizationMainFunction
///
/// Compute a one-to-one mapping from a 3D triangle surface `mesh` to a
/// 2D circle, using Floater Mean Value Coordinates algorithm.
/// A one-to-one mapping is guaranteed.
///
/// The mapping is piecewise linear on the input mesh triangles.
/// The result is a (u,v) pair of parameter coordinates for each vertex of the input mesh.
///
/// \tparam TriangleMesh must be a model of `FaceGraph`.
/// \tparam HD must be the halfedge_descriptor type corresponding to the graph
///         traits of TriangleMesh.
/// \tparam VertexUVmap must be a property map that associates a %Point_2
///         (type deduced by `Parameterized_traits_3`) to a `vertex_descriptor`
///         (type deduced by the graph traits of `TriangleMesh`).
///
/// \param mesh a triangulated surface.
/// \param bhd an halfedge descriptor on the boundary of `mesh`.
/// \param uvm an instanciation of the class `VertexUVmap`.
///
/// \pre `mesh` must be a surface with one connected component.
/// \pre `mesh` must be a triangular mesh.
template <class TriangleMesh, class HD, class VertexUVmap>
typename Parameterizer_traits_3<TriangleMesh>::Error_code
parameterize(TriangleMesh& mesh,
             HD bhd,
             VertexUVmap uvm)
{
  typedef typename boost::graph_traits<TriangleMesh>::vertex_descriptor vertex_descriptor;
  typedef boost::unordered_map<vertex_descriptor,int> Indices;
  Indices indices;
  boost::unordered_set<vertex_descriptor> vs;
  internal::Bool_property_map< boost::unordered_set<vertex_descriptor> > vpm(vs);

  CGAL::Polygon_mesh_processing::connected_component(face(opposite(bhd,mesh),mesh),
                                                     mesh,
                                                     boost::make_function_output_iterator(Parameterization::Vertices<TriangleMesh,Indices>(mesh,indices)));

  Mean_value_coordinates_parameterizer_3<TriangleMesh> parameterizer;
  return parameterizer.parameterize(mesh, bhd, uvm, boost::make_assoc_property_map(indices), vpm);

  return Parameterizer_traits_3<TriangleMesh>::OK;
}


template <class TM, class SEM, class SVM>
class Seam_mesh;


template <class TM, class SEM, class SVM, class HD, class VertexUVmap>
typename Parameterizer_traits_3<Seam_mesh<TM,SEM,SVM> >::Error_code
parameterize(Seam_mesh<TM,SEM,SVM>& mesh,
             HD bhd,
             VertexUVmap uvm)
{
  typedef typename boost::graph_traits<Seam_mesh<TM,SEM,SVM> >::vertex_descriptor vertex_descriptor;
  boost::unordered_set<vertex_descriptor> vs;
  internal::Bool_property_map< boost::unordered_set<vertex_descriptor> > vpm(vs);

  typedef boost::unordered_map<vertex_descriptor,int> Vertex_index_map;
  Vertex_index_map vim;
  boost::associative_property_map<Vertex_index_map> vipm(vim);
  mesh.initialize_vertex_index_map(bhd,vipm);

  Mean_value_coordinates_parameterizer_3<Seam_mesh<TM,SEM,SVM> > parameterizer;
  return parameterizer.parameterize(mesh, bhd, uvm, vipm, vpm);
}


template <class TM, class SEM, class SVM, class Parameterizer, class HD, class VertexUVmap>
typename Parameterizer_traits_3<Seam_mesh<TM,SEM,SVM> >::Error_code
parameterize(Seam_mesh<TM,SEM,SVM>& mesh,
             Parameterizer parameterizer,
             HD bhd,
             VertexUVmap uvm)
{
  typedef typename boost::graph_traits<Seam_mesh<TM,SEM,SVM> >::vertex_descriptor vertex_descriptor;

  boost::unordered_set<vertex_descriptor> vs;
  internal::Bool_property_map< boost::unordered_set<vertex_descriptor> > vpm(vs);

  typedef boost::unordered_map<vertex_descriptor,int> Vertex_index_map;
  Vertex_index_map vim;
  boost::associative_property_map<Vertex_index_map> vipm(vim);
  mesh.initialize_vertex_index_map(bhd,vipm);

  return parameterizer.parameterize(mesh, bhd, uvm, vipm, vpm);
}

} // namespace CGAL

#endif // CGAL_PARAMETERIZE_H
