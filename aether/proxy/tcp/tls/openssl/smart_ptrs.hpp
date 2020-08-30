/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio/ssl.hpp>

/*
    Classes that enable RAII functionality for OpenSSL objects.
*/
namespace proxy::tcp::tls::openssl::ptrs {
    namespace _impl {

        template <typename Type>
        class openssl_base_ptr {
        protected:
            Type *native = nullptr;

        public:
            openssl_base_ptr() { }

            openssl_base_ptr(std::nullptr_t) {
                native = nullptr;
            }

            openssl_base_ptr(Type *ptr) {
                native = ptr;
            }

            Type *native_handle() {
                return native;
            }

            Type *operator *() {
                return native;
            }
        };

        template <typename Type,
            Type *(*NewFunction)(),
            void (*FreeFunction)(Type *)>
            class openssl_unique_ptr 
                : public openssl_base_ptr<Type> {
            public:
                using openssl_base_ptr<Type>::openssl_base_ptr;

                openssl_unique_ptr() 
                    : openssl_base_ptr<Type>() {
                    this->native = (*NewFunction)();
                }

                ~openssl_unique_ptr() {
                    if (this->native) {
                        (*FreeFunction)(this->native);
                    }
                }

                openssl_unique_ptr(const openssl_unique_ptr &ptr) = delete;

                openssl_unique_ptr(openssl_unique_ptr &&ptr) {
                    if (this->native != ptr.native) {
                        (*FreeFunction)(this->native);
                        this->native = ptr.native;
                    }
                }

                openssl_unique_ptr &operator=(openssl_unique_ptr &&ptr) {
                    if (this->native != ptr.native) {
                        (*FreeFunction)(this->native);
                        this->native = ptr.native;
                    }
                }
        };

        template <typename Type, 
            Type *(*NewFunction)(), 
            int (*IncrementFunction)(Type*), 
            void (*FreeFunction)(Type*)>
        class openssl_scoped_ptr 
            : public openssl_base_ptr<Type> {
        public:
            using openssl_base_ptr<Type>::openssl_base_ptr;

            openssl_scoped_ptr() 
                : openssl_base_ptr<Type>() {
                this->native = (*NewFunction)();
            }

            ~openssl_scoped_ptr() {
                if (this->native) {
                    (*FreeFunction)(this->native);
                }
            }

            openssl_scoped_ptr(Type *ptr) 
                : openssl_base_ptr<Type>(ptr) {
                (*IncrementFunction)(this->native);
            }

            openssl_scoped_ptr &operator=(Type *ptr) {
                if (this->native != ptr) {
                    (*FreeFunction)(this->native);
                    this->native = ptr;
                    (*IncrementFunction)(this->native);
                }
                return *this;
            }

            openssl_scoped_ptr(const openssl_scoped_ptr &ptr) {
                if (this->native != ptr.native) {
                    (*FreeFunction)(this->native);
                    this->native = ptr.native;
                    (*IncrementFunction)(this->native);
                }
            }

            openssl_scoped_ptr &operator=(const openssl_scoped_ptr &ptr) {
                if (this->native != ptr.native) {
                    (*FreeFunction)(this->native);
                    this->native = ptr.native;
                    (*IncrementFunction)(this->native);
                }
                return *this;
            }

            openssl_scoped_ptr(openssl_scoped_ptr &&ptr) {
                if (this->native != ptr.native) {
                    (*FreeFunction)(this->native);
                    this->native = ptr.native;
                }
            }

            openssl_scoped_ptr &operator=(openssl_scoped_ptr &&ptr) noexcept {
                if (this->native != ptr.native) {
                    (*FreeFunction)(this->native);
                    this->native = ptr.native;
                }
                return *this;
            }

            void increment() {
                (*IncrementFunction)(this->native);
            }
        };
    }

    using x509 = _impl::openssl_scoped_ptr<X509, &X509_new, &X509_up_ref, &X509_free>;
    using evp_pkey = _impl::openssl_scoped_ptr<EVP_PKEY, &EVP_PKEY_new, &EVP_PKEY_up_ref, &EVP_PKEY_free>;
    using rsa = _impl::openssl_scoped_ptr<RSA, &RSA_new, &RSA_up_ref, &RSA_free>;

    using bignum = _impl::openssl_unique_ptr<BIGNUM, &BN_new, &BN_free>;
    using x509_extension = _impl::openssl_unique_ptr<X509_EXTENSION, &X509_EXTENSION_new, &X509_EXTENSION_free>;
}