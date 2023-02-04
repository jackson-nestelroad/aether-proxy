#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#define RETURN_IF_ERROR(rexpr)                       \
  {                                                  \
    auto __result = (rexpr);                         \
    if (__result.is_err()) {                         \
      return {util::err, std::move(__result).err()}; \
    }                                                \
  }

#define __RESULT_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define __RESULT_MACROS_CONCAT_NAME(x, y) __RESULT_MACROS_CONCAT_NAME_INNER(x, y)

#define __ASSIGN_OR_RETURN_IMPL(temp, lhs, rexpr) \
  auto temp = (rexpr);                            \
  if (temp.is_err()) {                            \
    return {util::err, std::move(temp).err()};    \
  }                                               \
  lhs = temp.ok();

#define ASSIGN_OR_RETURN(lhs, rexpr) \
  __ASSIGN_OR_RETURN_IMPL(__RESULT_MACROS_CONCAT_NAME(__result, __COUNTER__), lhs, rexpr);

namespace util {

// Type tag representing "OK" for a result object.
struct ok_t {
  struct init_tag {};
  explicit constexpr ok_t(init_tag) {}
};

// Type tag representing "Error" for a result object.
struct err_t {
  struct init_tag {};
  explicit constexpr err_t(init_tag) {}
};

// Tag representing "OK" for a result object.
constexpr ok_t ok((ok_t::init_tag()));

// Tag representing "Error" for a result object.
constexpr err_t err((err_t::init_tag()));

// Exception for bad access to a result object.
class bad_result_access : public std::logic_error {
 public:
  explicit bad_result_access(ok_t) : std::logic_error("Attempted to access the value of a result with an error") {}
  explicit bad_result_access(err_t) : std::logic_error("Attempted to access the error of a result with no error") {}
};

namespace result_detail {

// Compile-time maximum of multiple integers.
template <std::size_t N, std::size_t... Ns>
struct max;

template <std::size_t N>
struct max<N> {
  static constexpr std::size_t value = N;
};

template <std::size_t N1, std::size_t N2, std::size_t... Ns>
struct max<N1, N2, Ns...> {
  static constexpr std::size_t value = N1 > N2 ? max<N1, Ns...>::value : max<N2, Ns...>::value;
};

// Compile-time maximum of the size of multiple types.
template <typename... Ts>
struct max_sizeof : public max<sizeof(Ts)...> {};

// Compile-time maximum of the alignment of multiple types.
template <typename... Ts>
struct max_alignof : public max<alignof(Ts)...> {};

// Helper trait for checking if two types can form a valid result.
//
// The two types must be distinct from one another to avoid amiguity in constructors and other methods.
template <typename T, typename E>
struct valid_result_params {
  static constexpr bool value = !std::is_same_v<std::decay_t<T>, std::decay_t<E>>;
};

// Type tag for in-place initialization.
struct in_place_init_t {
  struct init_tag {};
  explicit constexpr in_place_init_t(init_tag) {}
};

// Tag for in-place initialization.
constexpr in_place_init_t in_place_init((in_place_init_t::init_tag()));

// Base implementation of result.
template <typename T, typename E, bool B = valid_result_params<T, E>::value>
struct result_base;

// Specialization of result_base, when T and E form a valid result.
//
// `result` is implemented by keeping one block of memory that can hold either a value of type T (a result) or a value
// of type E (an error). Because a result and an error cannot exist at the same time, a single boolean flag keeps track
// of which data is stored in the wrapper at any given time.
//
// The `ok_t` and `err_t` type tags are used to specify which type of value is set by helper methods. If a result is
// switching states (i.e., it contained a result but now contains an error), then the previous value should be properly
// deallocated (destructor called) and the new value should be properly allocated (constructor called). If a result is
// changing values, a simple assignment (copy or move constructor) will suffice.
template <typename T, typename E>
class result_base<T, E, true> {
 protected:
  using value_type = T;
  using unqualified_value_type = std::remove_cv_t<T>;
  using error_type = E;
  using unqualified_error_type = std::remove_cv_t<E>;

  using value_reference_type = T&;
  using value_reference_const_type = const T&;
  using value_rval_reference_type = T&&;
  using value_pointer_type = T*;
  using value_pointer_const_type = T const*;
  using value_argument_type = const T&;

  using error_reference_type = E&;
  using error_reference_const_type = const E&;
  using error_rval_reference_type = E&&;
  using error_pointer_type = E*;
  using error_pointer_const_type = E const*;
  using error_argument_type = const E&;

 public:
  // Checks if the result is OK and does not contain an error.
  bool is_ok() const { return !error_; }
  // Checks if the result contains an error.
  bool is_err() const { return error_; }

  // Returns a pointer to the stored value, or nullptr if the result contains an error.
  value_pointer_const_type get_ok_ptr() const { return is_ok() ? get_ok_ptr_impl() : nullptr; }
  // Returns a pointer to the stored value, or nullptr if the result contains an error.
  value_pointer_type get_ok_ptr() { return is_ok() ? get_ok_ptr_impl() : nullptr; }
  // Returns a pointer to the stored error, or nullptr if the result is ok.
  error_pointer_const_type get_err_ptr() const { return !is_ok() ? get_err_ptr_impl() : nullptr; }
  // Returns a pointer to the stored error, or nullptr if the result is ok.
  error_pointer_type get_err_ptr() { return !is_ok() ? get_err_ptr_impl() : nullptr; }

 protected:
  result_base(ok_t, value_argument_type val) { construct(ok, val); }
  result_base(ok_t, value_rval_reference_type val) { construct(ok, std::move(val)); }
  result_base(err_t, error_argument_type val) { construct(err, val); }
  result_base(err_t, error_rval_reference_type val) { construct(err, std::move(val)); }

  template <typename... Args>
  result_base(ok_t, Args&&... args) {
    construct(ok, in_place_init, std::forward<Args>(args)...);
  }
  template <typename... Args>
  result_base(err_t, Args&&... args) {
    construct(err, in_place_init, std::forward<Args>(args)...);
  }

  result_base(const result_base& rhs) {
    if (rhs.is_ok()) {
      construct(ok, rhs.get_ok_impl());
    } else {
      construct(err, rhs.get_err_impl());
    }
  }
  result_base(result_base&& rhs) noexcept {
    if (rhs.is_ok()) {
      construct(ok, std::move(rhs).get_ok_impl());
    } else {
      construct(err, std::move(rhs).get_err_impl());
    }
  }

  result_base& operator=(const result_base& rhs) {
    assign(rhs);
    return *this;
  }
  result_base& operator=(result_base&& rhs) noexcept {
    assign(std::move(rhs));
    return *this;
  }

  ~result_base() { destroy(); }

  void assign(const result_base& rhs) {
    if (is_ok()) {
      if (rhs.is_ok()) {
        assign_value(ok, rhs.get_ok_impl());
      } else {
        destroy();
        construct(err, rhs.get_err_impl());
      }
    } else if (rhs.is_ok()) {
      destroy();
      construct(ok, rhs.get_ok_impl());
    } else {
      assign_value(err, rhs.get_err_impl());
    }
  }

  void assign(result_base&& rhs) {
    if (is_ok()) {
      if (rhs.is_ok()) {
        assign_value(ok, std::move(rhs).get_ok_impl());
      } else {
        destroy();
        construct(err, std::move(rhs).get_err_impl());
      }
    } else if (rhs.is_ok()) {
      destroy();
      construct(ok, std::move(rhs).get_ok_impl());
    } else {
      assign_value(err, std::move(rhs).get_err_impl());
    }
  }

  void assign(value_argument_type val) {
    if (is_ok()) {
      assign_value(ok, val);
    } else {
      destroy();
      construct(ok, val);
    }
  }

  void assign(value_rval_reference_type val) {
    if (is_ok()) {
      assign_value(ok, std::move(val));
    } else {
      destroy();
      construct(ok, std::move(val));
    }
  }

  void assign(error_argument_type val) {
    if (is_err()) {
      assign_value(err, val);
    } else {
      destroy();
      construct(err, val);
    }
  }

  void assign(error_rval_reference_type val) {
    if (is_err()) {
      assign_value(err, std::move(val));
    } else {
      destroy();
      construct(err, std::move(val));
    }
  }

  void construct(ok_t, value_argument_type val) {
    new (&storage_) unqualified_value_type(val);
    error_ = false;
  }

  void construct(ok_t, value_rval_reference_type val) {
    new (&storage_) unqualified_value_type(std::move(val));
    error_ = false;
  }

  template <typename... Args>
  void construct(ok_t, in_place_init_t, Args&&... args) {
    new (&storage_) unqualified_value_type(std::forward<Args>(args)...);
    error_ = false;
  }

  void construct(err_t, error_argument_type val) {
    new (&storage_) unqualified_error_type(val);
    error_ = true;
  }

  void construct(err_t, error_rval_reference_type val) {
    new (&storage_) unqualified_error_type(std::move(val));
    error_ = true;
  }

  template <typename... Args>
  void construct(err_t, in_place_init_t, Args&&... args) {
    new (&storage_) unqualified_error_type(std::forward<Args>(args)...);
    error_ = true;
  }

  template <typename... Args>
  void emplace_assign(ok_t, Args&&... args) {
    destroy();
    construct(ok, in_place_init, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void emplace_assign(err_t, Args&&... args) {
    destroy();
    construct(err, in_place_init, std::forward<Args>(args)...);
  }

  void assign_value(ok_t, value_argument_type val) {
    get_ok_impl() = val;
    error_ = false;
  }

  void assign_value(ok_t, value_rval_reference_type val) {
    get_ok_impl() = std::move(val);
    error_ = false;
  }

  void assign_value(err_t, error_argument_type val) {
    get_err_impl() = val;
    error_ = true;
  }

  void assign_value(err_t, error_rval_reference_type val) {
    get_err_impl() = std::move(val);
    error_ = true;
  }

  value_pointer_const_type get_ok_ptr_impl() const { return reinterpret_cast<value_pointer_const_type>(&storage_); }

  value_pointer_type get_ok_ptr_impl() { return reinterpret_cast<value_pointer_type>(&storage_); }

  error_pointer_const_type get_err_ptr_impl() const { return reinterpret_cast<error_pointer_const_type>(&storage_); }

  error_pointer_type get_err_ptr_impl() { return reinterpret_cast<error_pointer_type>(&storage_); }

  value_reference_const_type get_ok_impl() const { return *get_ok_ptr_impl(); }

  value_reference_type get_ok_impl() { return *get_ok_ptr_impl(); }

  error_reference_const_type get_err_impl() const { return *get_err_ptr_impl(); }

  error_reference_type get_err_impl() { return *get_err_ptr_impl(); }

  // Destroys the stored value, calling the appropriate destructor.
  //
  // Note: `destroy` should NEVER be called on its own without replacing the storage with a new value, because a
  // `result` object should always contain some value.
  void destroy() {
    if (is_ok()) {
      get_ok_ptr_impl()->~T();
    } else {
      get_err_ptr_impl()->~E();
    }
  }

 private:
  static constexpr std::size_t storage_sizeof = max_sizeof<T, E>::value;
  static constexpr std::size_t storage_alignof = max_alignof<T, E>::value;

  using storage_type = alignas(storage_alignof) std::uint8_t[storage_sizeof];

  bool error_;
  storage_type storage_;
};

// Specialization of result_base, when T is void.
//
// A result with a void OK type is conceptually equivalent to an optional error. If there is no error, no additional
// value should be stored, so the block of memory need only represent an optional error value. This functionality is
// borrowed from the `std::optional` class, and its API is adapted here.
template <typename E>
class result_base<void, E, true> {
 protected:
  using value_type = void;
  using error_type = E;
  using unqualified_error_type = typename std::remove_cv<E>::type;

  using value_reference_type = void;
  using value_reference_const_type = void;
  using value_rval_reference_type = void;
  using value_pointer_type = void;
  using value_pointer_const_type = void;
  using value_argument_type = void;

  using error_reference_type = E&;
  using error_reference_const_type = const E&;
  using error_rval_reference_type = E&&;
  using error_pointer_type = E*;
  using error_pointer_const_type = E const*;
  using error_argument_type = const E&;

 public:
  // Checks if the result is OK and does not contain an error.
  bool is_ok() const { return !error_.has_value(); }
  // Checks if the result contains an error.
  bool is_err() const { return error_.has_value(); }

  // Returns a pointer to the stored error, or nullptr if the result is ok.
  error_pointer_const_type get_err_ptr() const { return !is_ok() ? get_err_ptr_impl() : nullptr; }
  // Returns a pointer to the stored error, or nullptr if the result is ok.
  error_pointer_type get_err_ptr() { return !is_ok() ? get_err_ptr_impl() : nullptr; }

 protected:
  result_base(ok_t) {}
  result_base(err_t, error_argument_type val) : error_(val) {}
  result_base(err_t, error_rval_reference_type val) : error_(std::move(val)) {}

  template <typename... Args>
  result_base(err_t, Args&&... args) : error_(std::forward<Args>(args)...) {}

  result_base(const result_base& rhs) = default;
  result_base(result_base&& rhs) = default;

  result_base& operator=(const result_base& rhs) = default;
  result_base& operator=(result_base&& rhs) noexcept = default;

  ~result_base() = default;

  template <typename... Args>
  void emplace_assign(err_t, Args&&... args) {
    error_.emplace(std::forward<Args>(args)...);
  }

  void assign(error_argument_type val) { error_ = val; }

  void assign(error_rval_reference_type val) { error_ = std::move(val); }

  error_pointer_const_type get_err_ptr_impl() const { return error_.has_value() ? &error_.value() : nullptr; }

  error_pointer_type get_err_ptr_impl() { return error_.has_value() ? &error_.value() : nullptr; }

  error_reference_const_type get_err_impl() const { return error_.value(); }

  error_reference_type get_err_impl() { return error_.value(); }

  void destroy() { error_.reset(); }

 private:
  std::optional<E> error_;
};

}  // namespace result_detail

// Data structure for representing the result of some operation that returns a value and may also fail with an error.
//
// `result` is similar to a type union between two types: a single block of memory is interpreted to be one of two types
// at any given time. Several methods are provided for accessing, chaining, and modifying a result object.
//
// Result and error types may be anything, but the two types must be distinct. The `ok` and `err` values can be used
// when initializing a result if constructors are ambiguous due to implicit type conversions.
template <typename T, typename E>
class result : public result_detail::result_base<T, E> {
 private:
  using base = result_detail::result_base<T, E>;

 public:
  using value_type = T;
  using error_type = E;

  using value_reference_type = typename base::value_reference_type;
  using value_reference_const_type = typename base::value_reference_const_type;
  using value_rval_reference_type = typename base::value_rval_reference_type;
  using value_pointer_type = typename base::value_pointer_type;
  using value_pointer_const_type = typename base::value_pointer_const_type;
  using value_argument_type = typename base::value_argument_type;

  using error_reference_type = typename base::error_reference_type;
  using error_reference_const_type = typename base::error_reference_const_type;
  using error_rval_reference_type = typename base::error_rval_reference_type;
  using error_pointer_type = typename base::error_pointer_type;
  using error_pointer_const_type = typename base::error_pointer_const_type;
  using error_argument_type = typename base::error_argument_type;

  result(value_reference_type val) : base(util::ok, val) {}
  result(value_rval_reference_type val) : base(util::ok, std::forward<T>(val)) {}
  result(error_reference_type val) : base(util::err, val) {}
  result(error_rval_reference_type val) : base(util::err, std::forward<E>(val)) {}
  // Explicitly construct an OK value using a copy.
  result(ok_t, value_reference_type val) : base(util::ok, val) {}
  // Explicitly construct an OK value using a move.
  result(ok_t, value_rval_reference_type val) : base(util::ok, std::forward<T>(val)) {}
  // Explicitly construct an error value using a copy.
  result(err_t, error_reference_type val) : base(util::err, val) {}
  // Explicitly construct an error value using a move.
  result(err_t, error_rval_reference_type val) : base(util::err, std::forward<E>(val)) {}

  // In-place initialization for OK values.
  template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, bool> = true>
  result(Args&&... args) : base(util::ok, std::forward<Args>(args)...) {}

  // In-place initializaiton for error values.
  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, bool> = true>
  result(Args&&... args) : base(util::err, std::forward<Args>(args)...) {}

  result(const result&) = default;
  result(result&&) = default;
  result& operator=(const result&) = default;
  result& operator=(result&&) noexcept = default;
  ~result() = default;

  // Emplaces an OK value using in-place initialization.
  template <class... Args>
  void emplace(ok_t, Args&&... args) {
    this->emplace_assign(util::ok, std::forward<Args>(args)...);
  }

  // Emplaces an error value using in-place initialization.
  template <class... Args>
  void emplace(err_t, Args&&... args) {
    this->emplace_assign(util::err, std::forward<Args>(args)...);
  }

  // Returns a reference to the stored OK value, assuming it is an OK value.
  value_reference_const_type ok() const& {
    if (this->is_ok()) {
      return this->get_ok_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a reference to the stored OK value, assuming it is an OK value.
  value_reference_type ok() & {
    if (this->is_ok()) {
      return this->get_ok_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a reference to the stored OK value, assuming it is an OK value.
  value_rval_reference_type ok() && {
    if (this->is_ok()) {
      return std::move(*this).get_ok_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a reference to the stored error value, assuming it is an error value.
  error_reference_const_type err() const& {
    if (this->is_err()) {
      return this->get_err_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a reference to the stored error value, assuming it is an error value.
  error_reference_type err() & {
    if (this->is_err()) {
      return this->get_err_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a reference to the stored error value, assuming it is an error value.
  error_rval_reference_type err() && {
    if (this->is_err()) {
      return std::move(*this).get_err_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a copy of the stored OK value if it exists. If not, a default value is constructed and returned instead.
  template <typename U>
  T ok_or(U&& other) const& {
    if (this->is_ok()) {
      return this->get_ok_impl();
    } else {
      return std::forward<U>(other);
    }
  }

  // Returns a copy of the stored OK value if it exists. If not, a default value is constructed and returned instead.
  template <typename U>
  T ok_or(U&& other) && {
    if (this->is_ok()) {
      return std::move(*this).get_ok_impl();
    } else {
      return std::forward<U>(other);
    }
  }

  // Returns a copy of the stored error value if it exists. If not, a default value is constructed and returned instead.
  template <typename U>
  E err_or(U&& other) const& {
    if (this->is_err()) {
      return this->get_err_impl();
    } else {
      return std::forward<U>(other);
    }
  }

  // Returns a copy of the stored error value if it exists. If not, a default value is constructed and returned instead.
  template <typename U>
  E err_or(U&& other) && {
    if (this->is_err()) {
      return std::move(*this).get_err_impl();
    } else {
      return std::forward<U>(other);
    }
  }

  // Chains another operation that produces a `result`.
  //
  // If the current result is an error, the operation is ignored and the same error is returned back immediately. If the
  // current result is OK, then the result of the operation on the OK value is returned.
  template <typename U, typename F,
            std::enable_if_t<std::is_same_v<result<U, E>, std::result_of_t<F(const T&)>>, bool> = true>
  result<U, E> and_then(F f) const& {
    if (this->is_ok()) {
      return f(this->get_ok_impl());
    } else {
      return {util::err, this->get_err_impl()};
    }
  }

  // Chains another operation that produces a `result`.
  //
  // If the current result is an error, the operation is ignored and the same error is returned back immediately. If the
  // current result is OK, then the result of the operation on the OK value is returned.
  template <typename U, typename F,
            std::enable_if_t<std::is_same_v<result<U, E>, std::result_of_t<F(T&)>>, bool> = true>
  result<U, E> and_then(F f) & {
    if (this->is_ok()) {
      return f(this->get_ok_impl());
    } else {
      return {util::err, this->get_err_impl()};
    }
  }

  // Chains another operation that produces a `result`.
  //
  // If the current result is an error, the operation is ignored and the same error is returned back immediately. If the
  // current result is OK, then the result of the operation on the OK value is returned.
  template <typename U, typename F,
            std::enable_if_t<std::is_same_v<result<U, E>, std::result_of_t<F(T&&)>>, bool> = true>
  result<U, E> and_then(F f) && {
    if (this->is_ok()) {
      return f(std::move(*this).get_ok_impl());
    } else {
      return {util::err, std::move(*this).get_err_impl()};
    }
  }

  // Maps the stored value using the given function only if the result contains an OK value.
  //
  // If the result contains an error, the same error is copied and returned.
  template <typename F, typename U = std::result_of_t<F(const T&)>>
  result<U, E> map(F f) const& {
    if (this->is_ok()) {
      return {util::ok, f(this->get_ok_impl())};
    } else {
      return {util::err, this->get_err_impl()};
    }
  }

  // Maps the stored value using the given function only if the result contains an OK value.
  //
  // If the result contains an error, the same error is copied and returned.
  template <typename F, typename U = std::result_of_t<F(T&)>>
  result<U, E> map(F f) & {
    if (this->is_ok()) {
      return {util::ok, f(this->get_ok_impl())};
    } else {
      return {util::err, this->get_err_impl()};
    }
  }

  // Maps the stored value using the given function only if the result contains an OK value.
  //
  // If the result contains an error, the same error is copied and returned.
  template <typename F, typename U = std::result_of_t<F(T&&)>>
  result<U, E> map(F f) && {
    if (this->is_ok()) {
      return {util::ok, f(std::move(*this).get_ok_impl())};
    } else {
      return {util::err, std::move(*this).get_err_impl()};
    }
  }

  // Maps the stored error value using the given function only if the result contains an error value.
  //
  // If the result does not contain an error, the same value is copied and returned.
  template <typename F, typename R = std::result_of_t<F(const E&)>>
  result<T, R> map_err(F f) const& {
    if (this->is_ok()) {
      return {util::ok, this->get_ok_impl()};
    } else {
      return {util::err, f(this->get_err_impl())};
    }
  }

  // Maps the stored error value using the given function only if the result contains an error value.
  //
  // If the result does not contain an error, the same value is copied and returned.
  template <typename F, typename R = std::result_of_t<F(E&)>>
  result<T, R> map_err(F f) & {
    if (this->is_ok()) {
      return {util::ok, this->get_ok_impl()};
    } else {
      return {util::err, f(this->get_err_impl())};
    }
  }

  // Maps the stored error value using the given function only if the result contains an error value.
  //
  // If the result does not contain an error, the same value is copied and returned.
  template <typename F, typename R = std::result_of_t<F(E&&)>>
  result<T, R> map_err(F f) && {
    if (this->is_ok()) {
      return {util::ok, std::move(this->get_ok_impl())};
    } else {
      return {util::err, f(std::move(this->get_err_impl()))};
    }
  }
};

// Data structure for representing the result of some operation that returns no value and may also fail with an error.
//
// `result<void, E>` is conceptually equivalent to `std::optional<E>` with an adapted API.
template <typename E>
class result<void, E> : public result_detail::result_base<void, E> {
 private:
  using base = result_detail::result_base<void, E>;

 public:
  using value_type = void;
  using error_type = E;

  using value_reference_type = typename base::value_reference_type;
  using value_reference_const_type = typename base::value_reference_const_type;
  using value_rval_reference_type = typename base::value_rval_reference_type;
  using value_pointer_type = typename base::value_pointer_type;
  using value_pointer_const_type = typename base::value_pointer_const_type;
  using value_argument_type = typename base::value_argument_type;

  using error_reference_type = typename base::error_reference_type;
  using error_reference_const_type = typename base::error_reference_const_type;
  using error_rval_reference_type = typename base::error_rval_reference_type;
  using error_pointer_type = typename base::error_pointer_type;
  using error_pointer_const_type = typename base::error_pointer_const_type;
  using error_argument_type = typename base::error_argument_type;

  result() : base(util::ok) {}
  result(ok_t) : base(util::ok) {}
  result(error_reference_type val) : base(util::err, val) {}
  result(error_rval_reference_type val) : base(util::err, std::forward<E>(val)) {}
  // Explicitly construct an error value using a copy.
  result(err_t, error_reference_type val) : base(util::err, val) {}
  // Explicitly construct an error value using a move.
  result(err_t, error_rval_reference_type val) : base(util::err, std::forward<E>(val)) {}

  // In-place initializaiton for error values.
  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, bool> = true>
  result(Args&&... args) : base(util::err, std::forward<Args>(args)...) {}

  result(const result&) = default;
  result(result&&) = default;
  result& operator=(const result&) = default;
  result& operator=(result&&) noexcept = default;
  ~result() = default;

  // Emplaces an OK value by clearing any error.
  void emplace(ok_t) { this->destroy(); }

  // Emplaces an error value using in-place initialization.
  template <class... Args>
  void emplace(err_t, Args&&... args) {
    this->emplace_assign(util::err, std::forward<Args>(args)...);
  }

  void set_ok() { this->destroy(); }

  error_reference_const_type err() const& {
    if (this->is_err()) {
      return this->get_err_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a reference to the stored error value, assuming it is an error value.
  error_reference_type err() & {
    if (this->is_err()) {
      return this->get_err_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a reference to the stored error value, assuming it is an error value.
  error_rval_reference_type err() && {
    if (this->is_err()) {
      return std::move(*this).get_err_impl();
    } else {
      throw bad_result_access(util::ok);
    }
  }

  // Returns a copy of the stored error value if it exists. If not, a default value is constructed and returned instead.
  template <typename U>
  E err_or(U&& other) const& {
    if (this->is_err()) {
      return this->get_err_impl();
    } else {
      return std::forward<U>(other);
    }
  }

  // Returns a copy of the stored error value if it exists. If not, a default value is constructed and returned instead.
  template <typename U>
  E err_or(U&& other) && {
    if (this->is_err()) {
      return std::move(*this).get_err_impl();
    } else {
      return std::forward<U>(other);
    }
  }

  // Chains another operation that produces a `result`.
  //
  // If the current result is an error, the operation is ignored and the same error is returned back immediately. If the
  // current result is OK, then the result of the operation on the OK value is returned.
  template <typename U, typename F, std::enable_if_t<std::is_same_v<result<U, E>, std::result_of_t<F()>>, bool> = true>
  result<U, E> and_then(F f) const& {
    if (this->is_ok()) {
      return f();
    } else {
      return {util::err, this->get_err_impl()};
    }
  }

  // Chains another operation that produces a `result`.
  //
  // If the current result is an error, the operation is ignored and the same error is returned back immediately. If the
  // current result is OK, then the result of the operation on the OK value is returned.
  template <typename U, typename F, std::enable_if_t<std::is_same_v<result<U, E>, std::result_of_t<F()>>, bool> = true>
  result<U, E> and_then(F f) & {
    if (this->is_ok()) {
      return f();
    } else {
      return {util::err, this->get_err_impl()};
    }
  }

  // Chains another operation that produces a `result`.
  //
  // If the current result is an error, the operation is ignored and the same error is returned back immediately. If the
  // current result is OK, then the result of the operation on the OK value is returned.
  template <typename U, typename F, std::enable_if_t<std::is_same_v<result<U, E>, std::result_of_t<F()>>, bool> = true>
  result<U, E> and_then(F f) && {
    if (this->is_ok()) {
      return f();
    } else {
      return {util::err, std::move(*this).get_err_impl()};
    }
  }

  // Maps the stored value using the given function only if the result contains an OK value.
  //
  // If the result contains an error, the same error is copied and returned.
  template <typename F, typename U = std::result_of_t<F()>>
  result<U, E> map(F f) const& {
    if (this->is_ok()) {
      return {util::ok, f()};
    } else {
      return {util::err, this->get_err_impl()};
    }
  }

  // Maps the stored value using the given function only if the result contains an OK value.
  //
  // If the result contains an error, the same error is copied and returned.
  template <typename F, typename U = std::result_of_t<F()>>
  result<U, E> map(F f) & {
    if (this->is_ok()) {
      return {util::ok, f()};
    } else {
      return {util::err, this->get_err_impl()};
    }
  }

  // Maps the stored value using the given function only if the result contains an OK value.
  //
  // If the result contains an error, the same error is copied and returned.
  template <typename F, typename U = std::result_of_t<F()>>
  result<U, E> map(F f) && {
    if (this->is_ok()) {
      return {util::ok, f()};
    } else {
      return {util::err, std::move(*this).get_err_impl()};
    }
  }

  // Maps the stored error value using the given function only if the result contains an error value.
  //
  // If the result does not contain an error, the same value is copied and returned.
  template <typename F, typename R = std::result_of_t<F(const E&)>>
  result<void, R> map_err(F f) const& {
    if (this->is_ok()) {
      return util::ok;
    } else {
      return {util::err, f(this->get_err_impl())};
    }
  }

  // Maps the stored error value using the given function only if the result contains an error value.
  //
  // If the result does not contain an error, the same value is copied and returned.
  template <typename F, typename R = std::result_of_t<F(E&)>>
  result<void, R> map_err(F f) & {
    if (this->is_ok()) {
      return util::ok;
    } else {
      return {util::err, f(this->get_err_impl())};
    }
  }

  // Maps the stored error value using the given function only if the result contains an error value.
  //
  // If the result does not contain an error, the same value is copied and returned.
  template <typename F, typename R = std::result_of_t<F(E&&)>>
  result<void, R> map_err(F f) && {
    if (this->is_ok()) {
      return util::ok;
    } else {
      return {util::err, f(std::move(*this).get_err_impl())};
    }
  }
};

template <typename T, typename E>
std::ostream& operator<<(std::ostream& out, const util::result<T, E> res) {
  if (res.is_ok()) {
    out << res.ok();
  } else {
    out << res.err();
  }
  return out;
}

template <typename E>
std::ostream& operator<<(std::ostream& out, const util::result<void, E> res) {
  if (res.is_ok()) {
    out << "OK";
  } else {
    out << res.err();
  }
  return out;
}

}  // namespace util
