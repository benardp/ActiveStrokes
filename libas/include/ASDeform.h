/*****************************************************************************\

ASDeform.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef DEFORM_H_
#define DEFORM_H_

#ifdef Success
#undef Success
#endif

#include <Eigen/Sparse>
#include <Eigen/Core>
#include <Eigen/SparseCholesky>

#include "GQImage.h"

class ASContour;
class ASBrushPath;
class ASSimpleGrid;

class ASDeform {
public:

    typedef Eigen::SparseMatrix<float> SparseMatrixType;
    typedef Eigen::MatrixXf DenseMatrixType;

    ASDeform();
    virtual ~ASDeform();

    void iterate(ASContour& c, GQFloatImage &fext);

protected:
    void buildRhs(ASContour &c, DenseMatrixType &Vin, DenseMatrixType &Fext, GQFloatImage &fext, float tangentReg, int contourSize);
    void buildMatrix(SparseMatrixType &A_dyn, ASContour &c, float alpha, float beta);

private:

    int _max_solver_iter;
    float _epsilon;
};

#endif /* DEFORM_H_ */
