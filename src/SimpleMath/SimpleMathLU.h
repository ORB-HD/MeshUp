#ifndef _SIMPLE_MATH_LU_H
#define _SIMPLE_MATH_LU_H

#include <iostream>
#include <limits>

#include "SimpleMathFixed.h"
#include "SimpleMathDynamic.h"
#include "SimpleMathBlock.h"

namespace SimpleMath {

	template <typename matrix_type>
	class PartialPivLU {
		public:
			typedef typename matrix_type::value_type value_type;	

		private:
			PartialPivLU() {}

			typedef Dynamic::Matrix<value_type> MatrixXXd;
			typedef Dynamic::Matrix<value_type> VectorXd;
			
			bool mIsFactorized;
			unsigned int *mPermutations;
			MatrixXXd mU;

		public:
			PartialPivLU(const matrix_type &matrix) :
				mIsFactorized(false),
				mU(matrix) {
					// We can only solve quadratic systems
					assert (mU.rows() == mU.cols());

					mPermutations = new unsigned int [matrix.cols()];
					for (unsigned int i = 0; i < matrix.cols(); i++) {
						mPermutations[i] = i;
					}
					compute();
			}
			PartialPivLU compute() {
				unsigned int n = mU.rows();
				unsigned int pi;

				unsigned int i,j,k;

				for (j = 0; j < n; j++) {
					double pv = fabs (mU(j,mPermutations[j]));

					// LOG << "j = " << j << " pv = " << pv << std::endl;
					// find the pivot
					for (k = j; k < n; k++) {
						double pt = fabs (mU(j,mPermutations[k]));
						if (pt > pv) {
							pv = pt;
							pi = k;
							unsigned int p_swap = mPermutations[j];
							mPermutations[j] = mPermutations[pi];
							mPermutations[pi] = p_swap;
							//	LOG << "swap " << j << " with " << pi << std::endl;
							//	LOG << "j = " << j << " pv = " << pv << std::endl;
						}
					}

					for (i = j + 1; i < n; i++) {
						if (fabs(mU(j,mPermutations[j])) <= std::numeric_limits<double>::epsilon()) {
							std::cerr << "Error: pivoting failed for matrix U = " << std::endl;
							std::cerr << "U = " << std::endl << A << std::endl;
						}
						//		assert (fabs(A(j,mPermutations[j])) > std::numeric_limits<double>::epsilon());
						double d = mU(i,mPermutations[j])/mU(j,mPermutations[j]);

						b[i] -= b[j] * d;

						for (k = j; k < n; k++) {
							mU(i,mPermutations[k]) -= mU(j,mPermutations[k]) * d;
						}
					}
				}

				mIsFactorized = true;

				return *this;
			}
			Dynamic::Matrix<value_type> solve (
					const Dynamic::Matrix<value_type> &rhs
					) const {
				assert (mIsFactorized);

				// temporary result vector which contains the pivoted result
				VectorXd px = rhs;

				for (int i = mR.cols() - 1; i >= 0; --i) {
					for (j = i + 1; j < n; j++) {
						px[i] += A(i, pivot[j]) * px[j];
					}
					px[i] = (b[i] - px[i]) / A (i, pivot[i]);
				}

				return x;
			}
	};

}

/* _SIMPLE_MATH_LU_H */
#endif
