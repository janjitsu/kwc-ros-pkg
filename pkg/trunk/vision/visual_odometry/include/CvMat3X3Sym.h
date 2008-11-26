#ifndef WGMAT3X3SYM_H_
#define WGMAT3X3SYM_H_

#include "CvMat3X3.h"

/**
 *  class template for 3x3 symmetric matrix
 */
template <typename DataT>
class CvMat3x3Sym: public CvMat3x3<DataT>
{
public:
	CvMat3x3Sym(){};
	virtual ~CvMat3x3Sym(){};
	/**
	 *  get the index of the matrix
	 */
	static inline int index(int i, int j) {
		return (i>=j)? i*3+j : j*3+i;
	}
	static inline DataT elem(DataT QQt[], int i, int j){
		return QQt[index(i,j)];
	}
	static inline DataT minor(DataT QQt[], int i, int j){
		int k0 = (i+1)%3;
		int k1 = (i+2)%3;
		int l0 = (j+1)%3;
		int l1 = (j+2)%3;
		return elem(QQt, k0, l0)*elem(QQt, k1, l1) - elem(QQt, k0, l1)*elem(QQt, k0, l1);
	}
	/**
	 * QQt = Q * Q^T
	 * Input is a 3x3 matrix
	 * Output is symmetric, as
	 *  [QQt[0], QQt[1], QQt[2],
	 *   QQt[1], QQt[3], QQt[4],
	 *   QQt[2], QQt[4], QQt[5]]
	 */
	static void QxQt(DataT Q[3*3], DataT QQt[]) {
		QQt[index(0,0)] = DOTPROD3QQt(Q, 0, 0);
		QQt[index(0,1)] = DOTPROD3QQt(Q, 0, 1);
		QQt[index(0,2)] = DOTPROD3QQt(Q, 0, 2);
		QQt[index(1,1)] = DOTPROD3QQt(Q, 1, 1);
		QQt[index(1,2)] = DOTPROD3QQt(Q, 1, 2);
		QQt[index(2,2)] = DOTPROD3QQt(Q, 2, 2);
	};
	static void QtxQ(DataT Q[3*3], DataT QtQ[]) {
		QtQ[index(0,0)] = DOTPROD3QtQ(Q, 0, 0);
		QtQ[index(0,1)] = DOTPROD3QtQ(Q, 0, 1);
		QtQ[index(0,2)] = DOTPROD3QtQ(Q, 0, 2);
		QtQ[index(1,1)] = DOTPROD3QtQ(Q, 1, 1);
		QtQ[index(1,2)] = DOTPROD3QtQ(Q, 1, 2);
		QtQ[index(2,2)] = DOTPROD3QtQ(Q, 2, 2);
	};
	/**
	 * compute the eigenvalue of a symmetric 3x3 matrix with rank 2
	 * QQt is given as [QQt[0], QQt[1], QQt[2],
	 *   				QQt[1], QQt[3], QQt[4],
	 *   				QQt[2], QQt[4], QQt[5]]
	 */
	static void eigValsSymmetricRank2(DataT QQt[], DataT ev[3]){
		// construct the characteristic equation as a x^3 + b x^2 + c x + d = 0
		// the char. equation is
		//  	[ a00 - x    a01        a02     ]
		// det( [ a01        a11 - x    a12     ] ) = 0
		//      [ a02        a12        a22 - x ]
		//
		// or
		//     a = -1
		//     b = a00 + a11 + a22
		//     c = -a11*a22 - a00*a22 - a00*a11 + a12*a12 + a01*a01 + a02*a02
		//     d = a00*a11*a22 + 2*a01*a12*a02 - a00*a12*a12 - a01*a01*a22 + a11*a02*a02
		// as the rank of QQt is 2, we know that d == 0
		DataT b, c;
//		a = -1.;
//		b = QQt[0] + QQt[3] + QQt[5];
		b = elem(QQt, 0, 0) + elem(QQt, 1, 1) + elem(QQt,2, 2);
//		c = -QQt[0]*QQt[3] - QQt[3]*QQt[5] - QQt[5]*QQt[0] + QQt[1]*QQt[1] + QQt[2]*QQt[2] + QQt[4]*QQt[4];
		c = - elem(QQt, 0, 0)*(elem(QQt, 1, 1) + elem(QQt, 2, 2)) - elem(QQt, 1, 1)*elem(QQt, 2, 2)
			+ elem(QQt, 0, 1)*elem(QQt, 0, 1) + elem(QQt, 0, 2)*elem(QQt, 0, 2) + elem(QQt, 1, 2)*elem(QQt, 1, 2);

		// now we are solving a quadratic equation
		DataT D = b*b+4.*c;  // D = b*b-4*a*c
		// D is expected to be positive
		assert(D>=0.0);
		DataT sqrtD = sqrt(D);
		DataT x0 = (b + sqrtD)/2.0;
		DataT x1 = (b - sqrtD)/2.0;
		// x2 = 0
		ev[0] = x0;
		ev[1] = x1;
		ev[2] = 0.0;
	};
	/**
	 * solve Ax = 0, where is A is symmetric
	 */
	static void solveSymmetricHomogeneous(DataT A[9], DataT x[3]){
		// choose the "best" among A[0,0], A[1,1] and A[2,2]
		int bestDiagIndex  = chooseGoodDiagElem(A);
		int bestMinorIndex = chooseGoodDiagMinor(A, bestDiagIndex);
		int k0, k1, k2;
		k2 = bestMinorIndex;
		k0 = (k2+1)%3;
		k1 = (k2+2)%3;
		DataT denominator = minor(A, k2, k2);
		DataT x0 = minor(A, k0, k2)/denominator;
		DataT x1 = minor(A, k1, k2)/denominator;
		// x2 = 1.0
		DataT norm = sqrt(x0*x0 + x1*x1 + 1);
		x[k0] = x0/norm;
		x[k1] = x1/norm;
		x[k2] = 1.0/norm;
	}
	static void eigVectorsSymmetric(DataT QQt[], DataT eigenVals[3], DataT eigenVectors[3*3]){
		DataT A[9];
		DataT x[3];
		DataT v[3];
		// make a copy
		for (int i=0; i<9; i++) A[i] = QQt[i];
		// solve for an eigen vectors of unit length

		// solve for eigenvalue equals to zero
		solveSymmetricHomogeneous(A, v);
		ELEM3(eigenVectors, 0, 0) = v[0];
		ELEM3(eigenVectors, 0, 1) = v[1];
		ELEM3(eigenVectors, 0, 2) = v[2];

		// re-assign the diagonal elements
		A[index(0,0)] = elem(QQt, 0, 0) - eigenVals[0];
		A[index(1,1)] = elem(QQt, 1, 1) - eigenVals[0];
		A[index(2,2)] = elem(QQt, 2, 2) - eigenVals[0];

		solveSymmetricHomogeneous(A, v);
		ELEM3(eigenVectors, 0, 0) = v[0];
		ELEM3(eigenVectors, 0, 1) = v[1];
		ELEM3(eigenVectors, 0, 2) = v[2];

		// re-assign the diagonal elements
		A[index(0,0)] = elem(QQt, 0, 0) - eigenVals[1];
		A[index(1,1)] = elem(QQt, 1, 1) - eigenVals[1];
		A[index(2,2)] = elem(QQt, 2, 2) - eigenVals[1];
		solveSymmetricHomogeneous(A, v);
		ELEM3(eigenVectors, 0, 0) = v[0];
		ELEM3(eigenVectors, 0, 1) = v[1];
		ELEM3(eigenVectors, 0, 2) = v[2];
	}
	static void rotMatrixFrom2Pair(DataT A[9], DataT B[9], DataT rot[3*3]){
		DataT Q[3*3];
		AxBt3x2(A, B, Q);
		DataT QQt[3*3];
		DataT QtQ[3*3];

		QxQt(Q, QQt);
		QtxQ(Q, QtQ);
		DataT ev[3];
		eigValsSymmetricRank2(QQt, ev);
		// it can be shown that the eigenvalue of QtQ is the same as QQt

		// now compute the eigenvectors of QQt and QtQ, given their eigenvalue ev[3]

	}

};

#endif /*WGMAT3X3SYM_H_*/
