/********************************************************************************
 * HOMMEXX 1.0: Copyright of Sandia Corporation
 * This software is released under the BSD license
 * See the file 'COPYRIGHT' in the HOMMEXX/src/share/cxx directory
 *******************************************************************************/

#include "ElementsGeometry.hpp"
#include "utilities/SubviewUtils.hpp"
#include "utilities/SyncUtils.hpp"
#include "utilities/TestUtils.hpp"
#include "HybridVCoord.hpp"

#include <limits>
#include <random>
#include <assert.h>

namespace Homme {

void ElementsGeometry::init(const int num_elems, const bool consthv) {

  m_num_elems = num_elems;
  m_consthv   = consthv;

  m_fcor = ExecViewManaged<Real * [NP][NP]>("FCOR", m_num_elems);
  m_spheremp = ExecViewManaged<Real * [NP][NP]>("SPHEREMP", m_num_elems);
  m_rspheremp = ExecViewManaged<Real * [NP][NP]>("RSPHEREMP", m_num_elems);
  m_metinv = ExecViewManaged<Real * [2][2][NP][NP]>("METINV", m_num_elems);
  m_metdet = ExecViewManaged<Real * [NP][NP]>("METDET", m_num_elems);

  if(!consthv){
    m_tensorvisc = ExecViewManaged<Real * [2][2][NP][NP]>("TENSORVISC", m_num_elems);
    m_vec_sph2cart = ExecViewManaged<Real * [2][3][NP][NP]>("VEC_SPH2CART", m_num_elems);
  }

  m_phis = ExecViewManaged<Real * [NP][NP]>("PHIS", m_num_elems);

  //matrix D and its derivatives 
  m_d =
      ExecViewManaged<Real * [2][2][NP][NP]>("matrix D", m_num_elems);
  m_dinv = ExecViewManaged<Real * [2][2][NP][NP]>(
      "DInv - inverse of matrix D", m_num_elems);
}

void ElementsGeometry::init (const int ie, CF90Ptr& D, CF90Ptr& Dinv, CF90Ptr& fcor,
                             CF90Ptr& spheremp, CF90Ptr& rspheremp,
                             CF90Ptr& metdet, CF90Ptr& metinv, CF90Ptr& phis,
                             CF90Ptr& tensorvisc, CF90Ptr& vec_sph2cart, const bool consthv) {

  using ScalarView   = ExecViewUnmanaged<Real [NP][NP]>;
  using TensorView   = ExecViewUnmanaged<Real [2][2][NP][NP]>;
  using Tensor23View = ExecViewUnmanaged<Real [2][3][NP][NP]>;

  using ScalarViewF90   = HostViewUnmanaged<const Real [NP][NP]>;
  using TensorViewF90   = HostViewUnmanaged<const Real [2][2][NP][NP]>;
  using Tensor23ViewF90 = HostViewUnmanaged<const Real [2][3][NP][NP]>;

  ScalarView::HostMirror h_fcor      = Kokkos::create_mirror_view(Homme::subview(m_fcor,ie));
  ScalarView::HostMirror h_metdet    = Kokkos::create_mirror_view(Homme::subview(m_metdet,ie));
  ScalarView::HostMirror h_spheremp  = Kokkos::create_mirror_view(Homme::subview(m_spheremp,ie));
  ScalarView::HostMirror h_rspheremp = Kokkos::create_mirror_view(Homme::subview(m_rspheremp,ie));
  ScalarView::HostMirror h_phis      = Kokkos::create_mirror_view(Homme::subview(m_phis,ie));
  TensorView::HostMirror h_metinv    = Kokkos::create_mirror_view(Homme::subview(m_metinv,ie));
  TensorView::HostMirror h_d         = Kokkos::create_mirror_view(Homme::subview(m_d,ie));
  TensorView::HostMirror h_dinv      = Kokkos::create_mirror_view(Homme::subview(m_dinv,ie));

  TensorView::HostMirror h_tensorvisc;
  Tensor23View::HostMirror h_vec_sph2cart;
  if( !consthv ){
    h_tensorvisc   = Kokkos::create_mirror_view(Homme::subview(m_tensorvisc,ie));
    h_vec_sph2cart = Kokkos::create_mirror_view(Homme::subview(m_vec_sph2cart,ie));
  }

  ScalarViewF90 h_fcor_f90         (fcor);
  ScalarViewF90 h_metdet_f90       (metdet);
  ScalarViewF90 h_spheremp_f90     (spheremp);
  ScalarViewF90 h_rspheremp_f90    (rspheremp);
  ScalarViewF90 h_phis_f90         (phis);
  TensorViewF90 h_metinv_f90       (metinv);
  TensorViewF90 h_d_f90            (D);
  TensorViewF90 h_dinv_f90         (Dinv);
  TensorViewF90 h_tensorvisc_f90   (tensorvisc);
  Tensor23ViewF90 h_vec_sph2cart_f90 (vec_sph2cart);
  
  // 2d scalars
  for (int igp = 0; igp < NP; ++igp) {
    for (int jgp = 0; jgp < NP; ++jgp) {
      h_fcor      (igp, jgp) = h_fcor_f90      (igp,jgp);
      h_spheremp  (igp, jgp) = h_spheremp_f90  (igp,jgp);
      h_rspheremp (igp, jgp) = h_rspheremp_f90 (igp,jgp);
      h_metdet    (igp, jgp) = h_metdet_f90    (igp,jgp);
      h_phis      (igp, jgp) = h_phis_f90      (igp,jgp);
    }
  }

  // 2d tensors
  for (int idim = 0; idim < 2; ++idim) {
    for (int jdim = 0; jdim < 2; ++jdim) {
      for (int igp = 0; igp < NP; ++igp) {
        for (int jgp = 0; jgp < NP; ++jgp) {
          h_d      (idim,jdim,igp,jgp) = h_d_f90      (idim,jdim,igp,jgp);
          h_dinv   (idim,jdim,igp,jgp) = h_dinv_f90   (idim,jdim,igp,jgp);
          h_metinv (idim,jdim,igp,jgp) = h_metinv_f90 (idim,jdim,igp,jgp);
        }
      }
    }
  }
  
  if(!consthv) {
    for (int idim = 0; idim < 2; ++idim) {
      for (int jdim = 0; jdim < 2; ++jdim) {
        for (int igp = 0; igp < NP; ++igp) {
          for (int jgp = 0; jgp < NP; ++jgp) {
            h_tensorvisc   (idim,jdim,igp,jgp) = h_tensorvisc_f90   (idim,jdim,igp,jgp);
          }
        }
      }
    }
    for (int idim = 0; idim < 2; ++idim) {
      for (int jdim = 0; jdim < 3; ++jdim) {
        for (int igp = 0; igp < NP; ++igp) {
          for (int jgp = 0; jgp < NP; ++jgp) {
            h_vec_sph2cart (idim,jdim,igp,jgp) = h_vec_sph2cart_f90 (idim,jdim,igp,jgp);
          }
        }
      }
    }
  }//end if consthv

  Kokkos::deep_copy(Homme::subview(m_fcor,ie), h_fcor);
  Kokkos::deep_copy(Homme::subview(m_metinv,ie), h_metinv);
  Kokkos::deep_copy(Homme::subview(m_metdet,ie), h_metdet);
  Kokkos::deep_copy(Homme::subview(m_spheremp,ie), h_spheremp);
  Kokkos::deep_copy(Homme::subview(m_rspheremp,ie), h_rspheremp);
  Kokkos::deep_copy(Homme::subview(m_phis,ie), h_phis);
  Kokkos::deep_copy(Homme::subview(m_d,ie), h_d);
  Kokkos::deep_copy(Homme::subview(m_dinv,ie), h_dinv);
  if( !consthv ) {
    Kokkos::deep_copy(Homme::subview(m_tensorvisc,ie), h_tensorvisc);
    Kokkos::deep_copy(Homme::subview(m_vec_sph2cart,ie), h_vec_sph2cart);
  }
}

//test for tensor hv is needed
void ElementsGeometry::random_init(int num_elems) {
  // For testing, we enable tensor viscosity (since we may need it in the test)
  init (num_elems, true);

  // Arbitrary minimum value to generate and minimum determinant allowed
  constexpr const Real min_value = 0.015625;
  std::random_device rd;
  std::mt19937_64 engine(rd());
  std::uniform_real_distribution<Real> random_dist(min_value, 1.0 / min_value);

  genRandArray(m_fcor,         engine, random_dist);

  genRandArray(m_spheremp,     engine, random_dist);
  genRandArray(m_rspheremp,    engine, random_dist);
  genRandArray(m_metdet,       engine, random_dist);
  genRandArray(m_metinv,       engine, random_dist);
  genRandArray(m_tensorvisc,   engine, random_dist);
  genRandArray(m_vec_sph2cart, engine, random_dist);

  genRandArray(m_phis,         engine, random_dist);

  // Lambdas used to constrain the metric tensor and its inverse
  const auto compute_det = [](HostViewUnmanaged<Real[2][2]> mtx) {
    return mtx(0, 0) * mtx(1, 1) - mtx(0, 1) * mtx(1, 0);
  };

  // 2d tensors
  // Generating lots of matrices with reasonable determinants can be difficult
  // So instead of generating them all at once and verifying they're correct,
  // generate them one at a time, verifying them individually
  HostViewManaged<Real[2][2]> h_matrix("single host metric matrix");

  auto h_d    = Kokkos::create_mirror_view(m_d);
  auto h_dinv = Kokkos::create_mirror_view(m_dinv);

  for (int ie = 0; ie < m_num_elems; ++ie) {
    // Because this constraint is difficult to satisfy for all of the tensors,
    // incrementally generate the view
    for (int igp = 0; igp < NP; ++igp) {
      for (int jgp = 0; jgp < NP; ++jgp) {
        do {
          genRandArray(h_matrix, engine, random_dist);
        } while (compute_det(h_matrix)<=0.0);

        for (int i = 0; i < 2; ++i) {
          for (int j = 0; j < 2; ++j) {
            h_d(ie, i, j, igp, jgp) = h_matrix(i, j);
          }
        }
        const Real determinant = compute_det(h_matrix);
        h_dinv(ie, 0, 0, igp, jgp) =  h_matrix(1, 1) / determinant;
        h_dinv(ie, 1, 0, igp, jgp) = -h_matrix(1, 0) / determinant;
        h_dinv(ie, 0, 1, igp, jgp) = -h_matrix(0, 1) / determinant;
        h_dinv(ie, 1, 1, igp, jgp) =  h_matrix(0, 0) / determinant;
      }
    }
  }

  Kokkos::deep_copy(m_d,    h_d);
  Kokkos::deep_copy(m_dinv, h_dinv);
}

} // namespace Homme