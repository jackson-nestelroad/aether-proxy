/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <openssl/bio.h>
#include <openssl/decoder.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <stdio.h>

#include <boost/asio/ssl.hpp>
#include <cstddef>
#include <string_view>
#include <utility>

#include "aether/util/string_literal.hpp"

// Classes that enable RAII functionality for OpenSSL objects.
namespace proxy::tls::openssl::ptrs {

// Constructor tags are required for wrapping a native pointer in one of the OpenSSL smart pointer objects, because it
// is not always clear whether or not the pointer should be wrapped uniquely or wrapped as a reference using OpenSSL's
// internal reference counting.

struct in_place_t {};

// Symbol for constructing an OpenSSL pointer in place.
constexpr in_place_t in_place{};

struct wrap_unique_t {};

// Symbol for wrapping an OpenSSL pointer in a unique pointer object.
constexpr wrap_unique_t wrap_unique{};

struct reference_t {};

// Symbol for wrapping a reference to an OpenSSL pointer.
constexpr reference_t reference{};

namespace openssl_ptr_detail {

// Base OpenSSL pointer.
//
// Constructors:
//  nullptr (default).
//  Native pointer.
//
// This class does not implement allocation or deallocation and simply wraps an underlying pointer type.
template <typename Type>
class openssl_base_ptr {
 public:
  openssl_base_ptr() {}
  openssl_base_ptr(std::nullptr_t) : native_(nullptr) {}
  openssl_base_ptr(Type* ptr) : native_(ptr) {}

  openssl_base_ptr(openssl_base_ptr&& ptr) noexcept { *this = std::move(ptr); }

  openssl_base_ptr& operator=(openssl_base_ptr&& ptr) noexcept {
    if (native_ != ptr.native_) {
      std::swap(native_, ptr.native_);
    }
    return *this;
  }

  Type* native_handle() const { return native_; }

  Type* operator*() const { return native_; }

  bool operator==(std::nullptr_t) { return native_ == nullptr; }
  bool operator==(const openssl_base_ptr& rhs) { return native_ == rhs.native_; }
  bool operator!=(std::nullptr_t) { return native_ != nullptr; }
  bool operator!=(const openssl_base_ptr& rhs) { return !(this == rhs); }

  operator bool() const { return native_ != nullptr; }

  bool operator!() const { return !native_; }

 protected:
  Type* native_ = nullptr;
};

// Unique pointer around a OpenSSL pointer type.
//
// Constructors:
//  nullptr (default).
//  In-place initialization of a new pointer using the `openssl::ptrs::in_place` constructor tag.
//  Unique wrapper around an external pointer using the `openssl::ptrs::wrap_unique` constructor tag.
//
// This smart pointer type is used for all OpenSSL types that do not have built-in reference counting.
template <typename Type, Type* (*NewFunction)(), void (*FreeFunction)(Type*)>
class openssl_unique_ptr : public openssl_base_ptr<Type> {
 private:
  using base = openssl_base_ptr<Type>;

 public:
  openssl_unique_ptr() : base() {}
  openssl_unique_ptr(std::nullptr_t) : base(nullptr) {}
  openssl_unique_ptr(in_place_t) : base() { native_ = (*NewFunction)(); }
  openssl_unique_ptr(wrap_unique_t, Type* ptr) : base(ptr) {}

  ~openssl_unique_ptr() {
    if (native_ != nullptr) {
      (*FreeFunction)(native_);
    }
  }

  openssl_unique_ptr(openssl_unique_ptr&& ptr) noexcept { *this = std::move(ptr); }
  openssl_unique_ptr& operator=(openssl_unique_ptr&& ptr) noexcept {
    base::operator=(std::move(ptr));
    return *this;
  }

 protected:
  using base::native_;
};

// Scoped pointer around a OpenSSL pointer type, with reference counting using the internal OpenSSL implementation.
//
// Constructors:
//  nullptr (default).
//  In-place initialization of a new pointer using the `openssl::ptrs::in_place` constructor tag.
//  Unique wrapper around an external pointer using the `openssl::ptrs::wrap_unique` constructor tag.
//  Increment reference count for external pointer using the `openssl::ptrs::reference` constructor tag.
//
// This smart pointer type is used for all OpenSSL types that do not have built-in reference counting.
template <typename Type, Type* (*NewFunction)(), int (*IncrementFunction)(Type*), void (*FreeFunction)(Type*)>
class openssl_scoped_ptr : public openssl_unique_ptr<Type, NewFunction, FreeFunction> {
 public:
  using unique_ptr = openssl_unique_ptr<Type, NewFunction, FreeFunction>;

  openssl_scoped_ptr() : unique_ptr() {}
  openssl_scoped_ptr(std::nullptr_t) : unique_ptr(nullptr) {}
  openssl_scoped_ptr(in_place_t) : unique_ptr(in_place) {}
  openssl_scoped_ptr(wrap_unique_t, Type* ptr) : unique_ptr(wrap_unique, ptr) {}
  openssl_scoped_ptr(reference_t, Type* ptr) : unique_ptr(wrap_unique, ptr) { (*IncrementFunction)(native_); }

  openssl_scoped_ptr(const openssl_scoped_ptr& ptr) { *this = ptr; }

  openssl_scoped_ptr& operator=(const openssl_scoped_ptr& ptr) {
    if (native_ != ptr.native_) {
      if (native_ != nullptr) {
        (*FreeFunction)(native_);
      }
      native_ = ptr.native_;
      if (native_ != nullptr) {
        (*IncrementFunction)(native_);
      }
    }
    return *this;
  }

  openssl_scoped_ptr(openssl_scoped_ptr&& ptr) noexcept { *this = std::move(ptr); }
  openssl_scoped_ptr& operator=(openssl_scoped_ptr&& ptr) noexcept {
    unique_ptr::operator=(std::move(ptr));
    return *this;
  }

  void increment() { (*IncrementFunction)(native_); }

 protected:
  using unique_ptr::native_;
};

}  // namespace openssl_ptr_detail

// Unique pointer for a native FILE* pointer.
class unique_native_file : public openssl_ptr_detail::openssl_base_ptr<FILE> {
 private:
  using base = openssl_ptr_detail::openssl_base_ptr<FILE>;

 public:
  unique_native_file() : base() {}
  unique_native_file(std::nullptr_t) : base(nullptr) {}
  unique_native_file(wrap_unique_t, FILE* ptr) : base(ptr) {}

  ~unique_native_file() {
    if (native_ != nullptr) {
      std::fclose(native_);
    }
  }

  unique_native_file(unique_native_file&& ptr) noexcept { *this = std::move(ptr); }
  unique_native_file& operator=(unique_native_file&& ptr) noexcept {
    base::operator=(std::move(ptr));
    return *this;
  }

  inline ::errno_t open(const char* file_name, const char* mode) {
    errno = 0;
    native_ = std::fopen(file_name, mode);
    return errno;
  }

 protected:
  using base::native_;

  friend class bio;
};

// Unique pointer for OpenSSL BIO, which is an input-output object, like a file.
class bio : public openssl_ptr_detail::openssl_base_ptr<BIO> {
 private:
  using base = openssl_ptr_detail::openssl_base_ptr<BIO>;

 public:
  bio() : base() {}
  bio(std::nullptr_t) : base(nullptr) {}
  bio(wrap_unique_t, BIO* ptr) : base(ptr) {}
  bio(wrap_unique_t, FILE* ptr) : base() { native_ = BIO_new_fp(ptr, BIO_CLOSE); }
  bio(wrap_unique_t, unique_native_file&& file) : bio(wrap_unique, file.native_handle()) { file.native_ = nullptr; }

  ~bio() {
    if (native_ != nullptr) {
      BIO_free(native_);
    }
  }

  bio(bio&& ptr) noexcept { *this = std::move(ptr); }
  bio& operator=(bio&& ptr) noexcept {
    base::operator=(std::move(ptr));
    return *this;
  }

  inline ::errno_t open(const char* file_name, const char* mode) {
    errno = 0;
    native_ = BIO_new_file(file_name, mode);
    return errno;
  }

 protected:
  using base::native_;
};

// Smart OpenSSL pointer type for EVP_PKEY_CTX.
//
// Implemented manually because different context types can be constructed using different names.
template <util::string_literal literal>
class evp_pkey_context : public openssl_ptr_detail::openssl_base_ptr<EVP_PKEY_CTX> {
 private:
  using base = openssl_ptr_detail::openssl_base_ptr<EVP_PKEY_CTX>;

 public:
  static constexpr const char* name = literal.value;

  evp_pkey_context() : base() {}
  evp_pkey_context(std::nullptr_t) : base(nullptr) {}
  evp_pkey_context(in_place_t) : base() { native_ = EVP_PKEY_CTX_new_from_name(nullptr, name, nullptr); }
  evp_pkey_context(wrap_unique_t, EVP_PKEY_CTX* ptr) : base(ptr) {}

  ~evp_pkey_context() {
    if (native_ != nullptr) {
      EVP_PKEY_CTX_free(native_);
    }
  }

  evp_pkey_context(evp_pkey_context&& ptr) noexcept { *this = std::move(ptr); }
  evp_pkey_context& operator=(evp_pkey_context&& ptr) noexcept {
    base::operator=(std::move(ptr));
    return *this;
  }

 protected:
  using base::native_;
};

// Smart OpenSSL pointer type for OSSL_DECODER_CTX.
//
// Implemented manually because different context types can be constructed using different names.
template <util::string_literal literal>
class decoder_context : public openssl_ptr_detail::openssl_base_ptr<OSSL_DECODER_CTX> {
 private:
  using base = openssl_ptr_detail::openssl_base_ptr<OSSL_DECODER_CTX>;

 public:
  static constexpr const char* name = literal.value;

  decoder_context() : base() {}
  decoder_context(std::nullptr_t) : base(nullptr) {}
  decoder_context(in_place_t, EVP_PKEY** pkey) : base() {
    native_ =
        OSSL_DECODER_CTX_new_for_pkey(pkey, "PEM", nullptr, name, OSSL_KEYMGMT_SELECT_ALL_PARAMETERS, nullptr, nullptr);
  }
  decoder_context(wrap_unique_t, OSSL_DECODER_CTX* ptr) : base(ptr) {}

  ~decoder_context() {
    if (native_ != nullptr) {
      OSSL_DECODER_CTX_free(native_);
    }
  }

  decoder_context(decoder_context&& ptr) noexcept { *this = std::move(ptr); }
  decoder_context& operator=(decoder_context&& ptr) noexcept {
    base::operator=(std::move(ptr));
    return *this;
  }

 protected:
  using base::native_;
};

using x509 = openssl_ptr_detail::openssl_scoped_ptr<X509, &X509_new, &X509_up_ref, &X509_free>;
using evp_pkey = openssl_ptr_detail::openssl_scoped_ptr<EVP_PKEY, &EVP_PKEY_new, &EVP_PKEY_up_ref, &EVP_PKEY_free>;

using rsa = evp_pkey_context<util::string_literal("RSA")>;
using dh_decoder_context = decoder_context<util::string_literal("DH")>;

using bignum = openssl_ptr_detail::openssl_unique_ptr<BIGNUM, &BN_new, &BN_free>;
using x509_extension =
    openssl_ptr_detail::openssl_unique_ptr<X509_EXTENSION, &X509_EXTENSION_new, &X509_EXTENSION_free>;
using general_names = openssl_ptr_detail::openssl_unique_ptr<GENERAL_NAMES, &GENERAL_NAMES_new, &GENERAL_NAMES_free>;

}  // namespace proxy::tls::openssl::ptrs
