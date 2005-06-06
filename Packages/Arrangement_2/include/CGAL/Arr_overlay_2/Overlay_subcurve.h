// Copyright (c) 1997  Tel-Aviv University (Israel).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $Source$
// $Revision$ $Date$
// $Name$
//
// Author(s)     : Baruch Zukerman <baruchzu@post.tau.ac.il>

#include <CGAL/Sweep_line_2/Arr_sweep_line_curve.h>

CGAL_BEGIN_NAMESPACE

template<class _Traits, class HalfedgeHandle>
class Overlay_subcurve : public Arr_sweep_line_curve<_Traits, Halfedge_handle>
{
public:
  typedef _Traits                                          Traits;
  typedef typename Traits::Point_2                         Point_2;
  typedef typename Traits::X_monotone_curve_2              X_monotone_curve_2;
  typedef Arr_sweep_line_curve<Traits, Halfedge_handle>    Base;
  typedef Overlay_subcurve<Traits, HalfedgeHandle>         Self;

  typedef Status_line_curve_less_functor<Traits, Self>    StatusLineCurveLess;
  typedef Red_black_tree<Self*, StatusLineCurveLess, CGAL_ALLOCATOR(int)> 
                                                          StatusLine;
  typedef typename StatusLine::iterator                   StatusLineIter;

  typedef Arr_insert_info<HalfedgeHandle>                 ArrInsertInfo;
  typedef Arr_sweep_line_event<Traits, Self>              Event;

  typedef typename Traits::Color                          Color;

  using Base::m_lastCurve;

protected:

  /*! A Subcure from a different color that lies above */
  Self*        m_above;
  
  /*! */
  StatusLineIter m_hint2;

public:

  Overlay_subcurve() : Base(),
                       m_above(NULL)
  {}

  Overlay_subcurve(const X_monotone_curve_2 &curve) : Base(curve),
                                                      m_above(NULL)
  {}

   void init(const X_monotone_curve_2 &curve)
  {
    Base::init(curve);
  }


  void set_hint(StatusLineIter hint) 
  {
    m_hint2 = hint;
  }

  StatusLineIter get_hint() const 
  {
    return m_hint2;
  }

  void set_above(Self* sc)
  {
    m_above = sc;
  }

  Self* get_above() const
  {
    return m_above;
  }

  Color get_color() const
  {
    return m_lastCurve.get_color();
  }

  bool has_same_color(const Self* sc) const
  {
    return (m_lastCurve.get_color() == sc.get_color());
  }

  Halfedge_handle get_halfedge_handle() const
  {
    return m_lastCurve.get_halfedge_handle();
  }

};
  
CGAL_END_NAMESPACE

#endif