/*****************************************************************************\

ASDeform.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASDeform.h"
#include "ASContour.h"
#include "ASEdgeContour.h"
#include "ASSimpleGrid.h"
#include "DialsAndKnobs.h"
#include "Stats.h"

#include <QDebug>

static dkFloat k_alpha("Contours->Relaxation->Alpha", 0.001f);
static dkFloat k_beta("Contours->Relaxation->Beta", 0.001f);
static dkFloat k_tangentReg("Contours->Relaxation->Tangent reg", 0.0f);
static dkFloat k_lenghtStiffness("Contours->Relaxation->Length stiffness", 1.f);
static dkFloat k_step("Contours->Relaxation->Time step", 0.5f);
static dkInt   k_numIter("Contours->Relaxation->Iterations", 25);
static dkInt   k_resamplingFreq("Contours->Resampling->Frequency", 5);
static dkInt   k_maxResampling("Contours->Resampling->Max iter.", 100);

ASDeform::ASDeform() {
    _max_solver_iter = 100;
    _epsilon = 0.01;
}

ASDeform::~ASDeform() {
}

float gaussian(float sigma2, float x2)
{
    return 1.f / (sqrt(2.f * M_PI * sigma2)) * exp(-x2/(2.f*sigma2));
}

/************************************************************/
/*                    Right-hand side                       */
/************************************************************/

const float k_smallest_spring = 0.01;

void ASDeform::buildRhs(ASContour &c, DenseMatrixType &Vin, DenseMatrixType &Fext,
                      GQFloatImage &fext, float tangentReg, int contourSize)
{
    ASContour::ContourIterator it = c.iterator();
    int idx = 0;
    vec2 tangent;
    float f_tg = 0.0;

    c.computeLength();

    c.computeTangent();

    while(it.hasNext()){
        ASVertexContour* v = it.next();

        vec2 vPos = v->position();
        vec2 fspring(0.f,0.f);
        //Spring force for the interior points => preserve length and regular spacing
        vec2 dPos;
        float restLength;
        if(!v->isLast()){
            dPos = v->following()->position();
            restLength = v->edge()->restLength();
            vec2 dir = dPos-vPos;
            float distDV = len(dir);
            distDV = max(distDV, k_smallest_spring);
            fspring += float(k_lenghtStiffness * (1.0 - restLength/distDV)) * dir;
        } else if(!v->isFirst()) {
            dPos = v->previous()->position();
            restLength = v->previous()->edge()->restLength();
            vec2 dir = dPos-vPos;
            float distDV = len(dir);
            distDV = max(distDV, k_smallest_spring);
            fspring += float(k_lenghtStiffness * (1.0 - restLength/distDV)) * dir;
        }

        tangent = v->tangent();
        vec2 p = vPos;

        //Input positions
        Vin.coeffRef(idx,0) = p[0];
        Vin.coeffRef(idx,1) = p[1];

        // Tangent regularization force
        if(idx == 0){
            f_tg = 0.0;
        }else if(idx==contourSize-1){
            f_tg = 0.0;
        }else if(tangentReg > 0){

            //Delinguette and Montagnat 2000
            v->computeMetricParameters();
            // uniform vertex spacing
            float epsilon = v->metricParameter();
            f_tg = -(0.5 - epsilon)*v->r();
        }

        vec2 b(0,0);
        if (p[0]<fext.width() && p[1]<fext.height() && p[0]>=0 && p[1]>=0){
            b = vec2(fext.pixel(int(p[0]),int(p[1]),0),fext.pixel(int(p[0]),int(p[1]),1));
        }
        vec2 fx = (b DOT v->normal()) * v->normal();
        Fext.coeffRef(idx,0) = fx[0] + tangentReg*f_tg * tangent[0] + fspring[0];
        Fext.coeffRef(idx,1) = fx[1] + tangentReg*f_tg * tangent[1] + fspring[1];

        idx++;
    }
}

/************************************************************/
/*      		Internal Forces                     */
/************************************************************/

void ASDeform::buildMatrix(SparseMatrixType &A_dyn, ASContour &c, float alpha, float beta){

    int contourSize = c.nbVertices();

    int iplus1, iplus2, iminus1, iminus2;
    // Internal Forces
    for (int i=0; i<contourSize; ++i){

        iplus1 = i+1;
        iplus2 = i+2;
        iminus1 = i-1;
        iminus2 = i-2;

        if(c.isClosed())
        {
            if(iminus1 < 0){
                iminus1 += contourSize;
            }

            if(iminus2 < 0){
                iminus2 += contourSize;
            }

            if(iplus1 >= contourSize){
                iplus1 -= contourSize ;
            }

            if(iplus2 >= contourSize){
                iplus2 -= contourSize;
            }
        }else{
            if(iminus1 < 0){
                iminus1 = -iminus1-1;
            }

            if(iminus2 < 0){
                iminus2 = -iminus2-1;
            }

            if(iplus1 >= contourSize){
                iplus1 = 2*contourSize-1 - iplus1;
            }

            if(iplus2 >= contourSize){
                iplus2 = 2*contourSize-1 - iplus2;
            }
        }

        A_dyn.coeffRef(i,iminus2) +=                 beta;
        A_dyn.coeffRef(i,iminus1) +=   - alpha - 4.0*beta;
        A_dyn.coeffRef(i,i)       += 2.0*alpha + 6.0*beta;
        A_dyn.coeffRef(i,iplus1)  +=   - alpha - 4.0*beta;
        A_dyn.coeffRef(i,iplus2)  +=                 beta;
    }

    A_dyn *= k_step;

    for (int i=0; i<A_dyn.rows(); ++i)
        A_dyn.coeffRef(i,i) += 1.0;
}

inline float clamp(float value, float valMin, float valMax)
{
    return std::min(std::max(valMin,value),valMax);
}

/************************************************************/
/*              	CHOLMOD Solver                      */
/************************************************************/

void ASDeform::iterate(ASContour &c, GQFloatImage &fext)
{
    for(int iter=0; iter<k_resamplingFreq; iter++){

        int contourSize = c.nbVertices();

        // Input positions Vin
        DenseMatrixType Vin(contourSize,2);
        // External forces Fext
        DenseMatrixType Fext = DenseMatrixType(contourSize,2);

        SparseMatrixType A = SparseMatrixType(contourSize,contourSize);

        buildMatrix(A, c, k_alpha, k_beta);

        Eigen::SimplicialLDLT<SparseMatrixType> sparseLDLT(A);
        sparseLDLT.compute(A);

        for(int i=0; i<int(k_numIter/float(k_resamplingFreq)); i++){

            buildRhs(c,Vin,Fext,fext,k_tangentReg,contourSize);

            // Right-hand side and future out position
            DenseMatrixType Vout_x = DenseMatrixType( Vin.col(0) + k_step * Fext.col(0) );
            DenseMatrixType Vout_y = DenseMatrixType( Vin.col(1) + k_step * Fext.col(1) );

            Vout_x = sparseLDLT.solve(Vout_x);
            Vout_y = sparseLDLT.solve(Vout_y);

            // Update vertex positions
            ASContour::ContourIterator it = c.iterator();
            while(it.hasNext()){
                ASVertexContour* v = it.next();
                float x = clamp(Vout_x(v->index()),0,fext.width()-1);
                float y = clamp(Vout_y(v->index()),0,fext.height()-1);
                if(//(idx > 1) && (idx < contourSize-2) &&
                        !isnan(x) && !isnan(y)){
                    v->updatePosition(vec2(x,y));
                }
            }
            c.computeLength();
        }

        int nbIter = 0;
        while(c.resample() && nbIter < k_maxResampling) nbIter++;
    }
}
