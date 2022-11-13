/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <stdio.h>

#include <algorithm>
#include <boost/asio/ssl.hpp>
#include <cstddef>

// Classes that enable RAII functionality for OpenSSL objects.
namespace proxy::tcp::tls::openssl::ptrs {

struct in_place_t {};

// Symbol for constructing an OpenSSL pointer in place.
constexpr in_place_t in_place{};

namespace openssl_ptr_detail {

template <typename Type>
class openssl_base_ptr {
 public:
  openssl_base_ptr() {}

  openssl_base_ptr(std::nullptr_t) : native_(nullptr) {}

  openssl_base_ptr(Type* ptr) : native_(ptr) {}

  Type* native_handle() const { return native_; }

  Type* operator*() const { return native_; }

  operator bool() const { return native_ != nullptr; }

  bool operator!() const { return !native_; }

 protected:
  Type* native_ = nullptr;
};

template <typename Type, Type* (*NewFunction)(), void (*FreeFunction)(Type*)>
class openssl_unique_ptr : public openssl_base_ptr<Type> {
 public:
  using openssl_base_ptr<Type>::openssl_base_ptr;
  using openssl_base_ptr<Type>::native_;

  openssl_unique_ptr() : openssl_base_ptr<Type>(nullptr) {}

  openssl_unique_ptr(in_place_t) : openssl_base_ptr<Type>() { native_ = (*NewFunction)(); }

  ~openssl_unique_ptr() {
    if (native_ != nullptr) {
      (*FreeFunction)(native_);
    }
  }

  openssl_unique_ptr(const openssl_unique_ptr& ptr) = delete;

  openssl_unique_ptr(openssl_unique_ptr&& ptr) noexcept {
    if (native_ != ptr.native_) {
      (*FreeFunction)(native_);
      native_ = ptr.native_;
      ptr.native_ = nullptr;
    }
  }

  openssl_unique_ptr& operator=(openssl_unique_ptr&& ptr) noexcept {
    if (native_ != ptr.native_) {
      (*FreeFunction)(native_);
      native_ = ptr.native_;
      ptr.native_ = nullptr;
    }
    return *this;
  }
};

template <typename Type, Type* (*NewFunction)(), int (*IncrementFunction)(Type*), void (*FreeFunction)(Type*)>
class openssl_scoped_ptr : public openssl_base_ptr<Type> {
 public:
  using openssl_base_ptr<Type>::openssl_base_ptr;
  using openssl_base_ptr<Type>::native_;

  openssl_scoped_ptr() : openssl_base_ptr<Type>(nullptr) {}

  openssl_scoped_ptr(in_place_t) : openssl_base_ptr<Type>() { native_ = (*NewFunction)(); }

  ~openssl_scoped_ptr() {
    if (native_ != nullptr) {
      (*FreeFunction)(native_);
    }
  }

  openssl_scoped_ptr(Type* ptr) : openssl_base_ptr<Type>(ptr) { (*IncrementFunction)(native_); }

  openssl_scoped_ptr& operator=(Type* ptr) {
    if (native_ != ptr) {
      (*FreeFunction)(native_);
      native_ = ptr;
      (*IncrementFunction)(native_);
    }
    return *this;
  }

  openssl_scoped_ptr(const openssl_scoped_ptr& ptr) {
    if (native_ != ptr.native_) {
      (*FreeFunction)(native_);
      native_ = ptr.native_;
      (*IncrementFunction)(native_);
    }
  }

  openssl_scoped_ptr& operator=(const openssl_scoped_ptr& ptr) {
    if (native_ != ptr.native_) {
      (*FreeFunction)(native_);
      native_ = ptr.native_;
      (*IncrementFunction)(native_);
    }
    return *this;
  }

  openssl_scoped_ptr(openssl_scoped_ptr&& ptr) noexcept {
    if (native_ != ptr.native_) {
      (*FreeFunction)(native_);
      native_ = ptr.native_;
      ptr.native_ = nullptr;
    }
  }

  openssl_scoped_ptr& operator=(openssl_scoped_ptr&& ptr) noexcept {
    if (native_ != ptr.native_) {
      (*FreeFunction)(native_);
      native_ = ptr.native_;
      ptr.native_ = nullptr;
    }
    return *this;
  }

  void increment() { (*IncrementFunction)(native_); }
};

}  // namespace openssl_ptr_detail

class unique_native_file : public openssl_ptr_detail::openssl_base_ptr<FILE> {
 public:
  using openssl_ptr_detail::openssl_base_ptr<FILE>::openssl_base_ptr;
  using openssl_ptr_detail::openssl_base_ptr<FILE>::native_;

  inline ~unique_native_file() {
    if (native_ != nullptr) {
      std::fclose(native_);
    }
  }

  unique_native_file(const unique_native_file& ptr) = delete;

  unique_native_file(unique_native_file&& ptr) noexcept {
    if (native_ != ptr.native_) {
      std::swap(native_, ptr.native_);
    }
  }

  unique_native_file& operator=(unique_native_file&& ptr) noexcept {
    if (native_ != ptr.native_) {
      std::swap(native_, ptr.native_);
    }
    return *this;
  }

  inline ::errno_t open(const char* file_name, const char* mode) {
    errno = 0;
    native_ = ::fopen(file_name, mode);
    return errno;
  }

  constexpr FILE* native_handle() const { return native_; }

  constexpr FILE* operator*() const { return native_; }

  inline operator bool() const { return native_ != nullptr; }

  inline bool operator!() const { return !native_; }
};

using x509 = openssl_ptr_detail::openssl_scoped_ptr<X509, &X509_new, &X509_up_ref, &X509_free>;
using evp_pkey = openssl_ptr_detail::openssl_scoped_ptr<EVP_PKEY, &EVP_PKEY_new, &EVP_PKEY_up_ref, &EVP_PKEY_free>;
using rsa = openssl_ptr_detail::openssl_scoped_ptr<RSA, &RSA_new, &RSA_up_ref, &RSA_free>;

using bignum = openssl_ptr_detail::openssl_unique_ptr<BIGNUM, &BN_new, &BN_free>;
using x509_extension =
    openssl_ptr_detail::openssl_unique_ptr<X509_EXTENSION, &X509_EXTENSION_new, &X509_EXTENSION_free>;
using dh = openssl_ptr_detail::openssl_unique_ptr<DH, &DH_new, &DH_free>;
using general_names = openssl_ptr_detail::openssl_unique_ptr<GENERAL_NAMES, &GENERAL_NAMES_new, &GENERAL_NAMES_free>;

}  // namespace proxy::tcp::tls::openssl::ptrs
