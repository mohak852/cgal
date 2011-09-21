// Copyright (c) 2011  INRIA Bordeaux Sud-Ouest (France), All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
// See the file LICENSE.LGPL distributed with CGAL.
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
// Author(s)     : Gael Guennebaud

#ifndef CGAL_EIGEN_SVD_H
#define CGAL_EIGEN_SVD_H

#include <CGAL/Eigen_matrix.h>
#include <CGAL/Eigen_vector.h>
#include <Eigen/SVD>



namespace CGAL {

class Eigen_svd{
public:
  typedef double FT;
  typedef Eigen_vector<FT> Vector;
  typedef Eigen_matrix<FT> Matrix;
  //solve MX=B using SVD and return the condition number of M
  //The solution is stored in B
  static FT solve(const Matrix& M, Vector& B){
    Eigen::JacobiSVD<typename Matrix::EigenType> jacobiSvd(M.eigen_object(),::Eigen::ComputeThinU | ::Eigen::ComputeThinV);
    B.eigen_object()=jacobiSvd.solve(Vector::EigenType(B.eigen_object()));
    return jacobiSvd.singularValues().array().abs().maxCoeff()/
           jacobiSvd.singularValues().array().abs().minCoeff();
  }
};

}//namespace CGAL
#endif // CGAL_EIGEN_SVD_H
