/*
 * Copyright 2019 Google Inc.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// A class for modular exponentiating a fixed base with arbitrary exponents
// based on a modulus. This class delegates the modular exponentiation
// operation to one of its subclasses.

#ifndef CRYPTO_FIXED_BASE_H_
#define CRYPTO_FIXED_BASE_H_

#include "gflags/gflags_declare.h"
#include "crypto/big_num.h"
#include "crypto/context.h"

// Declared for test-only.
DECLARE_bool(two_k_ary_exp);

namespace util {
template <typename T>
class StatusOr;
}  // namespace util

namespace private_join_and_compute {
namespace internal {
class FixedBaseExpImplBase;
}  // namespace internal

class FixedBaseExp {
 public:
  // FixedBaseExp is neither copyable nor movable.
  FixedBaseExp(const FixedBaseExp&) = delete;
  FixedBaseExp& operator=(const FixedBaseExp&) = delete;

  ~FixedBaseExp();

  // Computes fixed_base^exp mod modulus.
  // Returns INVALID_ARGUMENT if the exponent is negative.
  util::StatusOr<BigNum> ModExp(const BigNum& exp) const;

  static std::unique_ptr<FixedBaseExp> GetFixedBaseExp(Context* ctx,
                                                       const BigNum& fixed_base,
                                                       const BigNum& modulus);

 private:
  explicit FixedBaseExp(internal::FixedBaseExpImplBase* impl);

  std::unique_ptr<internal::FixedBaseExpImplBase> impl_;
};

}  // namespace private_join_and_compute

#endif  // CRYPTO_FIXED_BASE_H_
