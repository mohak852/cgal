#ifndef CGAL_ROOT_FOR_CIRCLES_2_2_H
#define CGAL_ROOT_FOR_CIRCLES_2_2_H

#include <iostream>

namespace CGAL {

  template < typename RT_ >
class Root_for_circles_2_2 {
    typedef RT_ RT;
    typedef typename Root_of_traits< RT >::RootOf_2         Root_of_2;
  private:
    Root_of_2 x_;
    Root_of_2 y_;
    
  public:
    Root_for_circles_2_2(){}
    
    Root_for_circles_2_2(const Root_of_2& r1, const Root_of_2& r2)
      : x_(r1), y_(r2)
    {}

    const Root_of_2& x() const {
      return x_;
    }
    
    const Root_of_2& y() const {
      return y_;
    }

  };
  
  template < typename RT >
    std::ostream &
    operator<<(std::ostream & os, const Root_for_circles_2_2<RT> &r)
    {
      
      return os << r.x() << " "
		<< r.y() << " ";
    }

}// CGAL  
  

#endif // CGAL_ROOT_FOR_CIRCLES_2_2_H
