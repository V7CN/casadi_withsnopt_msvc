/*
 *    This file is part of CasADi.
 *
 *    CasADi -- A symbolic framework for dynamic optimization.
 *    Copyright (C) 2010-2014 Joel Andersson, Joris Gillis, Moritz Diehl,
 *                            K.U. Leuven. All rights reserved.
 *    Copyright (C) 2011-2014 Greg Horn
 *
 *    CasADi is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    CasADi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with CasADi; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include "linsol_internal.hpp"
#include "mx_node.hpp"

using namespace std;
namespace casadi {

  Linsol::Linsol() {
  }

  Linsol::Linsol(const std::string& name, const std::string& solver,
                 const Sparsity& sp, const Dict& opts) {
    own(LinsolInternal::getPlugin(solver).creator(name, sp));
    (*this)->construct(opts);
  }

  LinsolInternal* Linsol::operator->() {
    return static_cast<LinsolInternal*>(SharedObject::operator->());
  }

  const LinsolInternal* Linsol::operator->() const {
    return static_cast<const LinsolInternal*>(SharedObject::operator->());
  }

  bool Linsol::test_cast(const SharedObjectInternal* ptr) {
    return dynamic_cast<const LinsolInternal*>(ptr)!=0;
  }

  bool Linsol::has_plugin(const std::string& name) {
    return LinsolInternal::has_plugin(name);
  }

  void Linsol::load_plugin(const std::string& name) {
    LinsolInternal::load_plugin(name);
  }

  std::string Linsol::doc(const std::string& name) {
    return LinsolInternal::getPlugin(name).doc;
  }

  std::string Linsol::plugin_name() const {
    return (*this)->plugin_name();
  }

  DM Linsol::solve(const DM& A, const DM& B, bool tr) const {
    casadi_assert(A.size1()==B.size1(),
      "Linsol::solve: Dimension mismatch. A and b must have matching row count. "
      "Got " + A.dim() + " and " + B.dim() + ".");

    // Symbolic factorization
    if (sfact(A.ptr())) casadi_error("Linsol::solve: 'sfact' failed");

    // Numeric factorization
    if (nfact(A.ptr())) casadi_error("Linsol::solve: 'nfact' failed");

    // Solve
    DM x = densify(B);
    if (solve(A.ptr(), x.ptr(), x.size2())) casadi_error("Linsol::solve: 'solve' failed");
    return x;
  }

  MX Linsol::solve(const MX& A, const MX& B, bool tr) const {
    return A->get_solve(B, tr, *this);
  }

  void Linsol::solve_cholesky(double* x, int nrhs, bool tr) const {
    (*this)->solve_cholesky((*this)->memory(0), x, nrhs, tr);
  }

  int Linsol::sfact(const double* A, int mem) const {
    if (A==0) return 1;
    auto m = static_cast<LinsolMemory*>((*this)->memory(mem));

    // Factorization will be needed after this step
    m->is_sfact = m->is_nfact = false;

    // Perform pivoting
    if ((*this)->sfact(m, A)) return 1;

    // Mark as (successfully) pivoted
    m->is_sfact = true;
    return 0;
  }

  int Linsol::nfact(const double* A, int mem) const {
    if (A==0) return 1;
    auto m = static_cast<LinsolMemory*>((*this)->memory(mem));

    // Perform pivoting, if required
    if (!m->is_sfact) {
      if (sfact(A)) return 1;
    }

    m->is_nfact = false;
    if ((*this)->nfact(m, A)) return 1;
    m->is_nfact = true;
    return 0;
  }

  int Linsol::neig() const {
    auto m = static_cast<LinsolMemory*>((*this)->memory(0));
    casadi_assert_dev(m->is_nfact);
    return (*this)->neig(m);
  }

  int Linsol::rank() const {
    auto m = static_cast<LinsolMemory*>((*this)->memory(0));
    casadi_assert_dev(m->is_nfact);
    return (*this)->rank(m);
  }

  int Linsol::solve(const double* A, double* x, int nrhs, bool tr, int mem) const {
    auto m = static_cast<LinsolMemory*>((*this)->memory(mem));
    casadi_assert(m->is_nfact, "Linear system has not been factorized");

    return (*this)->solve(m, A, x, nrhs, tr);
  }

  Sparsity Linsol::cholesky_sparsity(bool tr) const {
    return (*this)->linsol_cholesky_sparsity((*this)->memory(0), tr);
  }

  DM Linsol::cholesky(bool tr) const {
    return (*this)->linsol_cholesky((*this)->memory(0), tr);
  }

  bool has_linsol(const string& name) {
    return Linsol::has_plugin(name);
  }

  void load_linsol(const string& name) {
    Linsol::load_plugin(name);
  }

  string doc_linsol(const string& name) {
    return Linsol::doc(name);
  }

} // namespace casadi
