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

#ifndef EC_COMMUTATIVE_CIPHER_H_
#define EC_COMMUTATIVE_CIPHER_H_

#include <memory>
#include <string>

#include "absl/base/port.h"
#include "crypto/big_num.h"
#include "crypto/context.h"
#include "crypto/ec_group.h"
#include "crypto/ec_point.h"

namespace util {
template <typename T>
class StatusOr;
}  // namespace util

namespace private_join_and_compute {

// ECCommutativeCipher class with the property that K1(K2(a)) = K2(K1(a))
// where K(a) is encryption with the key K. https://eprint.iacr.org/2008/356.pdf
//
// This class allows two parties to determine if they share the same value,
// without revealing the sensitive value to each other.
//
// This class also allows homomorphically re-encrypting an ElGamal ciphertext
// with an EC cipher key K. If the original ciphertext was an encryption of m,
// then the re-encrypted ciphertext is effectively an encryption of K(m). This
// re-encryption does not re-randomize the ciphertext, and so is only secure
// when the underlying messages "m" are pseudorandom.
//
// The encryption is performed over an elliptic curve.
//
// This class is not thread-safe.
//
// Security: The provided bit security is half the number of bits of the
//  underlying curve. For example, using curve NID_secp224r1 gives 112 bit
//  security.
//
// Example: To generate a cipher with a new private key for the named curve
//    NID_secp224r1. The key can be securely stored and reused.
//    #include <openssl/obj_mac.h>
//    std::unique_ptr<ECCommutativeCipher> cipher =
//        ECCommutativeCipher::CreateWithNewKey(NID_secp224r1);
//    string key_bytes = cipher->GetPrivateKeyBytes();
//
//  Example: To generate a cipher with an existing private key for the named
//    curve NID_secp224r1.
//    #include <openssl/obj_mac.h>
//    std::unique_ptr<ECCommutativeCipher> cipher =
//        ECCommutativeCipher::CreateFromKey(NID_secp224r1, key_bytes);
//
// Example: To encrypt a message using a std::unique_ptr<ECCommutativeCipher>
//    cipher generated as above.
//    string encrypted_string = cipher->Encrypt("secret");
//
// Example: To re-encrypt a message already encrypted by another party using a
//    std::unique_ptr<ECCommutativeCipher> cipher generated as above.
//    StatusOr<string> double_encrypted_string =
//        cipher->ReEncrypt(encrypted_string);
//
// Example: To decrypt a message that has already been encrypted by the same
//    party using a std::unique_ptr<ECCommutativeCipher> cipher generated as
//    above.
//    StatusOr<string> decrypted_string =
//        cipher->Decrypt(encrypted_string);
//
// Example: To re-encrypt a message that has already been encrypted using a
// std::unique_ptr<CommutativeElGamal> ElGamal key:
//    StatusOr<std::pair<string, string>> double_encrypted_string =
//        cipher->ReEncryptElGamalCiphertext(elgamal_ciphertext);

class ECCommutativeCipher {
 public:
  // ECCommutativeCipher is neither copyable nor assignable.
  ECCommutativeCipher(const ECCommutativeCipher&) = delete;
  ECCommutativeCipher& operator=(const ECCommutativeCipher&) = delete;

  // Creates an ECCommutativeCipher object with a new random private key.
  // Use this method when the key is created for the first time or it needs to
  // be refreshed.
  // Returns INVALID_ARGUMENT status instead if the curve_id is not valid
  // or INTERNAL status when crypto operations are not successful.
  static util::StatusOr<std::unique_ptr<ECCommutativeCipher>> CreateWithNewKey(
      int curve_id);

  // Creates an ECCommutativeCipher object with the given private key.
  // A new key should be created for each session and all values should be
  // unique in one session because the encryption is deterministic.
  // Use this when the key is stored securely to be used at different steps of
  // the protocol in the same session or by multiple processes.
  // Returns INVALID_ARGUMENT status instead if the private_key is not valid for
  // the given curve or the curve_id is not valid.
  // Returns INTERNAL status when crypto operations are not successful.
  static util::StatusOr<std::unique_ptr<ECCommutativeCipher>> CreateFromKey(
      int curve_id, const std::string& key_bytes);

  // Encrypts a string with the private key to a point on the elliptic curve.
  //
  // To encrypt, the string is hashed to a point on the curve which is then
  // multiplied with the private key.
  //
  // The resulting point is returned encoded in compressed form as defined in
  // ANSI X9.62 ECDSA.
  //
  // Returns an INVALID_ARGUMENT error code if an error occurs.
  util::StatusOr<std::string> Encrypt(const std::string& plaintext) const;

  // Encrypts an encoded point with the private key.
  //
  // Returns an INVALID_ARGUMENT error code if the input is not a valid encoding
  // of a point on this curve as defined in ANSI X9.62 ECDSA.
  //
  // The result is a point encoded in compressed form.
  //
  // This method can also be used to encrypt a value that has already been
  // hashed to the curve.
  util::StatusOr<std::string> ReEncrypt(const std::string& ciphertext) const;

  // Encrypts an ElGamal ciphertext with the private key.
  //
  // Returns an INVALID_ARGUMENT error code if the input is not a valid encoding
  // of an ElGamal ciphertext on this curve as defined in ANSI X9.62 ECDSA.
  //
  // The result is another ElGamal ciphertext, encoded in compressed form.
  util::StatusOr<std::pair<std::string, std::string>>
  ReEncryptElGamalCiphertext(
      const std::pair<std::string, std::string>& elgamal_ciphertext) const;

  // Decrypts an encoded point with the private key.
  //
  // Returns an INVALID_ARGUMENT error code if the input is not a valid encoding
  // of a point on this curve as defined in ANSI X9.62 ECDSA.
  //
  // The result is a point encoded in compressed form.
  //
  // If the input point was double-encrypted, once with this key and once with
  // another key, then the result point is single-encrypted with the other key.
  //
  // If the input point was single encrypted with this key, then the result
  // point is the original, unencrypted point. Note that this will not reverse
  // hashing to the curve.
  util::StatusOr<std::string> Decrypt(const std::string& ciphertext) const;

  // Returns the private key bytes so the key can be stored and reused.
  std::string GetPrivateKeyBytes() const;

 private:
  // Creates a new ECCommutativeCipher object with the given private key for
  // the given EC group.
  ECCommutativeCipher(std::unique_ptr<Context> context, ECGroup group,
                      BigNum private_key);

  // Encrypts a point by multiplying the point with the private key.
  util::StatusOr<ECPoint> Encrypt(const ECPoint& point) const;

  // Context used for storing temporary values to be reused across openssl
  // function calls for better performance.
  std::unique_ptr<Context> context_;

  // The EC Group representing the curve definition.
  const ECGroup group_;

  // The private key used for encryption.
  const BigNum private_key_;

  // The private key inverse, used for decryption.
  const BigNum private_key_inverse_;
};

}  // namespace private_join_and_compute

#endif  // EC_COMMUTATIVE_CIPHER_H_
