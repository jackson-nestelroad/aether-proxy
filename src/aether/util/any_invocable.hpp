/*********************************************

  Copyright (c) Jackson Nestelroad 2022
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "aether/util/assert.hpp"

// Adapted from absl::AnyInvocable.
//
// Rewritten here to align with standards and formatting of this project.

namespace util {

template <typename Sig>
class any_invocable;

namespace any_invocable_detail {

template <typename Sig>
struct is_any_invocable : std::false_type {};

template <typename Sig>
struct is_any_invocable<any_invocable<Sig>> : std::true_type {};

template <typename Sig>
static constexpr bool is_any_invocable_v = is_any_invocable<Sig>::value;

// Properties for small object storage.
//
// Two key ideas in this implementation are local storage and remote storage:
// - A function in local storage is stored on the stack directly in the any_invocable object.
// - A function in remote storage is stored on the heap and is accessed through a pointer.
enum class small_object_storage_property : std::size_t {
  align = alignof(std::max_align_t),
  size = sizeof(void*) * 2,
};

template <typename T>
constexpr bool is_stored_locally_v = sizeof(T) <= static_cast<std::size_t>(small_object_storage_property::size) &&
                                     alignof(T) <= static_cast<std::size_t>(small_object_storage_property::align) &&
                                     static_cast<std::size_t>(small_object_storage_property::align) % alignof(T) == 0 &&
                                     std::is_nothrow_move_constructible_v<T>;

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename ReturnType, typename F, typename... P, typename = std::enable_if_t<std::is_void_v<ReturnType>>>
void invoke_r(F&& f, P&&... args) {
  std::invoke(std::forward<F>(f), std::forward<P>(args)...);
}

template <typename ReturnType, typename F, typename... P, std::enable_if_t<!std::is_void_v<ReturnType>, int> = 0>
ReturnType invoke_r(F&& f, P&&... args) {
  return std::invoke(std::forward<F>(f), std::forward<P>(args)...);
}

template <typename T>
T forward_impl(std::true_type);

template <typename T>
T&& forward_impl(std::false_type);

// Forwards a parameter only if it is a non-scalar type.
template <typename T>
using forwarded_parameter_type = decltype((forward_impl<T>)(std::integral_constant<bool, std::is_scalar_v<T>>()));

enum class function_to_call : bool {
  // Specifies the manager should perform a move.
  relocate_from_to,
  // Specifies the manager should perform a destroy.
  dispose,
};

union type_erased_state {
  // Pointer to the target object in remote storage.
  struct {
    void* target;
    std::size_t size;
  } remote;

  // The object itself in local storage.
  alignas(static_cast<std::size_t>(small_object_storage_property::align)) char storage[static_cast<std::size_t>(
      small_object_storage_property::size)];
};

template <typename T>
T& object_in_local_storage(type_erased_state* const state) {
  return *std::launder(reinterpret_cast<T*>(&state->storage));
}

// The type for functions issuing lifetime-related operations.
using manager_type = void(function_to_call, type_erased_state*, type_erased_state*);

template <bool NoExcept, typename ReturnType, typename... P>
using invoker_type = ReturnType(type_erased_state*, forwarded_parameter_type<P>...) noexcept(NoExcept);

// Manager used when any_invocable is empty.
inline void empty_manager(function_to_call, type_erased_state*, type_erased_state*) noexcept {}

// Manager used when a target function is in local storage and is a trivially copyable type.
inline void local_manager_trivial(function_to_call, type_erased_state* const from,
                                  type_erased_state* const to) noexcept {
  *to = *from;
}

// Manager used when a target function is in local storage and is not a trivially copyable type.
template <typename T>
void local_manager_nontrivial(function_to_call operation, type_erased_state* const from,
                              type_erased_state* const to) noexcept {
  static_assert(is_stored_locally_v<T>, "Local storage must only be used for supported types.");
  static_assert(!std::is_trivially_copyable_v<T>, "Locally stored types must be trivially copyable.");

  T& from_object = (object_in_local_storage<T>)(from);

  switch (operation) {
    case function_to_call::relocate_from_to:
      new (static_cast<void*>(&to->storage)) T(std::move(from_object));
      [[fallthrough]];
    case function_to_call::dispose:
      from_object.~T();
  }
}

// Invoker used when a target function is in local storage.
template <bool NoExcept, typename ReturnType, typename QualTRef, typename... P>
ReturnType local_invoker(type_erased_state* const state, forwarded_parameter_type<P>... args) noexcept(NoExcept) {
  using RawT = remove_cvref_t<QualTRef>;
  static_assert(is_stored_locally_v<RawT>, "Target object must be in local storage in order to be invoked from it.");

  auto& f = (object_in_local_storage<RawT>)(state);
  return (invoke_r<ReturnType>)(static_cast<QualTRef>(f), static_cast<forwarded_parameter_type<P>>(args)...);
}

// Manager used when a target function is in remote storage and has a trivial destructor.
inline void remote_manager_trivial(function_to_call operation, type_erased_state* const from,
                                   type_erased_state* const to) noexcept {
  switch (operation) {
    case function_to_call::relocate_from_to:
      to->remote = from->remote;
      return;
    case function_to_call::dispose:
      ::operator delete(from->remote.target, from->remote.size);
      return;
  }
}

// Manager used when a target function is in remote storage and has a non-trivial destructor.
template <typename T>
void remote_manager_nontrivial(function_to_call operation, type_erased_state* const from,
                               type_erased_state* const to) noexcept {
  static_assert(!is_stored_locally_v<T>,
                "Remote storage must only be used for types that do not qualify for local storage.");

  switch (operation) {
    case function_to_call::relocate_from_to:
      to->remote.target = from->remote.target;
      return;
    case function_to_call::dispose:
      delete static_cast<T*>(from->remote.target);
  }
}

// Inverok used when a target function is in remote storage.
template <bool NoExcept, typename ReturnType, typename QualTRef, typename... P>
ReturnType remote_invoker(type_erased_state* const state, forwarded_parameter_type<P>... args) noexcept(NoExcept) {
  using RawT = remove_cvref_t<QualTRef>;
  static_assert(!is_stored_locally_v<RawT>, "Target object must be in remote storage in order to be invoked from it.");

  auto& f = *static_cast<RawT*>(state->remote.target);
  return (invoke_r<ReturnType>)(static_cast<QualTRef>(f), static_cast<forwarded_parameter_type<P>>(args)...);
}

template <typename T>
struct is_in_place_type : std::false_type {};

template <typename T>
struct is_in_place_type<std::in_place_type_t<T>> : std::true_type {};

template <typename T>
constexpr bool is_in_place_type_v = is_in_place_type<T>::value;

// Constructor tag to request the conversion-constructor.
template <typename QualDecayedTRef>
struct typed_conversion_construct {};

template <typename Sig>
class any_invocable_impl {};

class trivial_deleter {
 public:
  explicit trivial_deleter(std::size_t size) : size_(size) {}

  void operator()(void* target) const { ::operator delete(target, size_); }

 private:
  std::size_t size_;
};

template <bool NoExcept, typename ReturnType, typename... P>
class any_invocable_core_impl;

constexpr bool is_compatible_conversion(void*, void*) { return false; }
template <bool NoExceptSrc, bool NoExceptDest, typename... T>
constexpr bool is_compatible_conversion(any_invocable_core_impl<NoExceptSrc, T...>*,
                                        any_invocable_core_impl<NoExceptDest, T...>*) {
  return !NoExceptDest || NoExceptSrc;
}

// Helper base class for all core operations that do not depend on the cv/ref qualifiers of the function type.
template <bool NoExcept, typename ReturnType, typename... P>
class any_invocable_core_impl {
 public:
  using result_type = ReturnType;

  any_invocable_core_impl() noexcept : manager_(empty_manager), invoker_(nullptr) {}

  enum class target_type : int {
    pointer = 0,
    compatible_any_invocable = 1,
    incompatible_any_invocable = 2,
    other = 3,
  };

  template <typename QualDecayedTRef, typename F>
  explicit any_invocable_core_impl(typed_conversion_construct<QualDecayedTRef>, F&& f) {
    using DecayedT = remove_cvref_t<QualDecayedTRef>;

    static constexpr target_type target =
        (std::is_pointer_v<DecayedT> || std::is_member_pointer_v<DecayedT>) ? target_type::pointer
        : is_compatible_any_invocable_v<DecayedT>                           ? target_type::compatible_any_invocable
        : is_any_invocable_v<DecayedT>                                      ? target_type::incompatible_any_invocable
                                                                            : target_type::other;
    initialize<target, QualDecayedTRef>(std::forward<F>(f));
  }

  template <typename QualTRef, typename... Args>
  explicit any_invocable_core_impl(std::in_place_type_t<QualTRef>, Args&&... args) {
    initialize_storage<QualTRef>(std::forward<Args>(args)...);
  }

  any_invocable_core_impl(any_invocable_core_impl&& other) noexcept {
    other.manager_(function_to_call::relocate_from_to, &other.state_, &state_);
    manager_ = other.manager_;
    invoker_ = other.invoker_;
    other.manager_ = empty_manager;
    other.invoker_ = nullptr;
  }

  any_invocable_core_impl& operator=(any_invocable_core_impl&& other) noexcept {
    clear();

    other.manager_(function_to_call::relocate_from_to, &other.state_, &state_);
    manager_ = other.manager_;
    invoker_ = other.invoker_;
    other.manager_ = empty_manager;
    other.invoker_ = nullptr;

    return *this;
  }

  ~any_invocable_core_impl() { manager_(function_to_call::dispose, &state_, &state_); }

  bool has_value() const { return invoker_ != nullptr; }

  void clear() {
    manager_(function_to_call::dispose, &state_, &state_);
    manager_ = empty_manager;
    invoker_ = nullptr;
  }

  template <target_type target, typename QualDecayedTRef, typename F,
            std::enable_if_t<target == target_type::pointer, int> = 0>
  void initialize(F&& f) {
    // This condition handles types that decay into pointers, which includes function references. Function references
    // cannot actually be null, so this check is safe.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Waddress"
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#pragma GCC diagnostic push
#endif
    if (static_cast<remove_cvref_t<QualDecayedTRef>>(f) == nullptr) {
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
      manager_ = empty_manager;
      invoker_ = nullptr;
      return;
    }
    initialize_storage<QualDecayedTRef>(std::forward<F>(f));
  }

  template <target_type target, typename QualDecayedTRef, typename F,
            std::enable_if_t<target == target_type::compatible_any_invocable, int> = 0>
  void initialize(F&& f) {
    f.manager_(function_to_call::relocate_from_to, &f.state_, &state_);
    manager_ = f.manager_;
    invoker_ = f.invoker_;

    f.manager_ = empty_manager;
    f.invoker_ = nullptr;
  }

  template <target_type target, typename QualDecayedTRef, typename F,
            std::enable_if_t<target == target_type::incompatible_any_invocable, int> = 0>
  void initialize(F&& f) {
    if (f.has_value()) {
      initialize_storage<QualDecayedTRef>(std::forward<F>(f));
    } else {
      manager_ = empty_manager;
      invoker_ = nullptr;
    }
  }

  template <target_type target, typename QualDecayedTRef, typename F,
            std::enable_if_t<target == target_type::other, int> = 0>
  void initialize(F&& f) {
    initialize_storage<QualDecayedTRef>(std::forward<F>(f));
  }

  // Use local storage for applicable target object types.
  template <typename QualTRef, typename... Args,
            typename = std::enable_if_t<is_stored_locally_v<remove_cvref_t<QualTRef>>>>
  void initialize_storage(Args&&... args) {
    using RawT = remove_cvref_t<QualTRef>;
    new (static_cast<void*>(&state_.storage)) RawT(std::forward<Args>(args)...);
    invoker_ = local_invoker<NoExcept, ReturnType, QualTRef, P...>;
    initialize_local_manager<RawT>();
  }

  // Use remote storage for target objects that cannot be stored locally.
  template <typename QualTRef, typename... Args,
            std::enable_if_t<!is_stored_locally_v<remove_cvref_t<QualTRef>>, int> = 0>
  void initialize_storage(Args&&... args) {
    initialize_remote_manager<remove_cvref_t<QualTRef>>(std::forward<Args>(args)...);
    invoker_ = remote_invoker<NoExcept, ReturnType, QualTRef, P...>;
  }

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
  void initialize_local_manager() {
    manager_ = local_manager_trivial;
  }

  template <typename T, std::enable_if_t<!std::is_trivially_copyable_v<T>, int> = 0>
  void initialize_local_manager() {
    manager_ = local_manager_nontrivial<T>;
  }

  template <typename T>
  static constexpr bool has_trivial_remote_storage_v =
      std::is_trivially_destructible_v<T> && alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__;

  template <typename T, typename... Args, typename = std::enable_if_t<has_trivial_remote_storage_v<T>>>
  void initialize_remote_manager(Args&&... args) {
    // unique_ptr is used for exception-safety in case constructor throws.
    std::unique_ptr<void, trivial_deleter> uninitialized_target(::operator new(sizeof(T)), trivial_deleter(sizeof(T)));
    new (uninitialized_target.get()) T(std::forward<Args>(args)...);
    state_.remote.target = uninitialized_target.release();
    state_.remote.size = sizeof(T);
    manager_ = remote_manager_trivial;
  }

  template <typename T, typename... Args, std::enable_if_t<!has_trivial_remote_storage_v<T>, int> = 0>
  void initialize_remote_manager(Args&&... args) {
    state_.remote.target = new T(std::forward<Args>(args)...);
    manager_ = remote_manager_nontrivial<T>;
  }

  template <typename Other>
  struct is_compatible_any_invocable {
    static constexpr bool value = false;
  };

  template <typename Sig>
  struct is_compatible_any_invocable<any_invocable<Sig>> {
    static constexpr bool value =
        (is_compatible_conversion)(static_cast<typename any_invocable<Sig>::any_invocable_core_impl*>(nullptr),
                                   static_cast<any_invocable_core_impl*>(nullptr));
  };

  template <typename Other>
  static constexpr bool is_compatible_any_invocable_v = is_compatible_any_invocable<Other>::value;

  type_erased_state state_;
  manager_type* manager_;
  invoker_type<NoExcept, ReturnType, P...>* invoker_;
};

// Constructor tag to request the conversion-constructor.
struct conversion_construct {};

template <typename T>
struct unwrap_reference_wrapper_impl {
  using type = T;
};

template <typename T>
struct unwrap_reference_wrapper_impl<std::reference_wrapper<T>> {
  using type = T&;
};

template <typename T>
using unwrap_reference_wrapper_t = typename unwrap_reference_wrapper_impl<T>::type;

// Alias that always yiels std::true_type where substitution failures happen when forming the template arguments.
template <typename... T>
using true_alias = std::integral_constant<bool, sizeof(std::void_t<T...>*) != 0>;

// SFINAE constraints for the conversin-constructor.
template <typename Sig, typename F, typename = std::enable_if_t<!std::is_same_v<remove_cvref_t<F>, any_invocable<Sig>>>>
using can_convert =
    true_alias<std::enable_if_t<!is_in_place_type_v<remove_cvref_t<F>>>,
               std::enable_if_t<any_invocable_impl<Sig>::template call_is_valid<F>::value>,
               std::enable_if_t<any_invocable_impl<Sig>::template call_is_noexcept_if_sig_is_noexcept<F>::value>,
               std::enable_if_t<std::is_constructible_v<std::decay_t<F>, F>>>;

// SFINAE constraints for the std::in_place constructors.
template <typename Sig, typename F, typename... Args>
using can_emplace =
    true_alias<std::enable_if_t<any_invocable_impl<Sig>::template call_is_valid<F>::value>,
               std::enable_if_t<any_invocable_impl<Sig>::template call_is_noexcept_if_sig_is_noexcept<F>::value>,
               std::enable_if_t<std::is_constructible_v<std::decay_t<F>, Args...>>>;

// SFINAE constraints for the conversion-assign operator.
template <typename Sig, typename F, typename = std::enable_if_t<!std::is_same_v<remove_cvref_t<F>, any_invocable<Sig>>>>
using can_assign =
    true_alias<std::enable_if_t<any_invocable_impl<Sig>::template call_is_valid<F>::value>,
               std::enable_if_t<any_invocable_impl<Sig>::template call_is_noexcept_if_sig_is_noexcept<F>::value>,
               std::enable_if_t<std::is_constructible_v<std::decay_t<F>, F>>>;

// SFINAE constraints for the reference-wrapper conversion-assign operator.
template <typename Sig, typename F>
using can_assign_reference_wrapper =
    true_alias<std::enable_if_t<any_invocable_impl<Sig>::template call_is_valid<std::reference_wrapper<F>>::value>,
               std::enable_if_t<any_invocable_impl<Sig>::template call_is_noexcept_if_sig_is_noexcept<
                   std::reference_wrapper<F>>::value>>;

// A macro to generate partial specializations of any_invocable_impl with the different combinations of supported
// cv/refernce qualifiers and noexcept specifier.
//
// `cv` are the cv-qualifiers, if any.
// `ref` is the ref-qualifier, if any.
// `inv_quals` is the reference type to be used when invoking the target.
// `noex` is true if the function type is noexcept, false if not.
#define INTERNAL_ANY_INVOCABLE_IMPL_NOEXEC(cv, ref, inv_quals, noex)                                             \
  template <typename ReturnType, typename... P>                                                                  \
  class any_invocable_impl<ReturnType(P...) cv ref noexcept(noex)>                                               \
      : public any_invocable_core_impl<noex, ReturnType, P...> {                                                 \
   public:                                                                                                       \
    using core = any_invocable_core_impl<noex, ReturnType, P...>;                                                \
                                                                                                                 \
    /* SFINAE constraint to check if F is invocable with the proper signature. */                                \
    template <typename F>                                                                                        \
    using call_is_valid = true_alias<std::enable_if_t<                                                           \
        std::disjunction_v<std::is_invocable_r<ReturnType, std::decay_t<F> inv_quals, P...>,                     \
                           std::is_same<ReturnType, std::invoke_result_t<std::decay_t<F> inv_quals, P...>>>>>;   \
                                                                                                                 \
    /* SFINAE constraint to check if F is nothrow-invocable when necessary. */                                   \
    template <typename F>                                                                                        \
    using call_is_noexcept_if_sig_is_noexcept = true_alias<std::enable_if_t<std::disjunction_v<                  \
        std::integral_constant<bool, !noex>,                                                                     \
        std::is_nothrow_invocable_r<ReturnType, unwrap_reference_wrapper_t<std::decay_t<F>>, P...>>>>;           \
                                                                                                                 \
    /* Put the any_invocable into an empty state. */                                                             \
    any_invocable_impl() = default;                                                                              \
                                                                                                                 \
    /* The implementation of a conversion-constructor from F. */                                                 \
    /* This forwards to core, attaching inv_quals so that the base class knows how to properly type-erase the */ \
    /* invocation. */                                                                                            \
    template <typename F>                                                                                        \
    explicit any_invocable_impl(conversion_construct, F&& f)                                                     \
        : core(typed_conversion_construct<std::decay_t<F> inv_quals>(), std::forward<F>(f)) {}                   \
                                                                                                                 \
    /* Forward along the in-place constructor parameters. */                                                     \
    template <typename T, typename... Args>                                                                      \
    explicit any_invocable_impl(std::in_place_type_t<T>, Args&&... args)                                         \
        : core(std::in_place_type<std::decay_t<T> inv_quals>, std::forward<Args>(args)...) {}                    \
                                                                                                                 \
    invoker_type<noex, ReturnType, P...>* extract_invoker() cv {                                                 \
      using QualifiedTestType = int cv ref;                                                                      \
      auto* invoker = this->invoker_;                                                                            \
      if (!std::is_const_v<QualifiedTestType> && std::is_rvalue_reference_v<QualifiedTestType>) {                \
        /* We checked that this isn't const above, so const_cast is safe. */                                     \
        const_cast<any_invocable_impl*>(this)->invoker_ =                                                        \
            [](type_erased_state*, forwarded_parameter_type<P>...) noexcept(noex) -> ReturnType {                \
          AETHER_ASSERT(false, "any_invocable use-after-move");                                                  \
        };                                                                                                       \
      }                                                                                                          \
      return invoker;                                                                                            \
    }                                                                                                            \
                                                                                                                 \
    /* The actual invocation operation with the proper signature. */                                             \
    ReturnType operator()(P... args) cv ref noexcept(noex) {                                                     \
      AETHER_ASSERT(this->invoker_ != nullptr, "any_invocable invoker is null");                                 \
      return this->extract_invoker()(const_cast<type_erased_state*>(&this->state_),                              \
                                     static_cast<forwarded_parameter_type<P>>(args)...);                         \
    }                                                                                                            \
  };

#define INTERNAL_ANY_INVOCABLE_IMPL(cv, ref, inv_quals)         \
  INTERNAL_ANY_INVOCABLE_IMPL_NOEXEC(cv, ref, inv_quals, false) \
  INTERNAL_ANY_INVOCABLE_IMPL_NOEXEC(cv, ref, inv_quals, true)

INTERNAL_ANY_INVOCABLE_IMPL(, , &);
INTERNAL_ANY_INVOCABLE_IMPL(const, , const&);
INTERNAL_ANY_INVOCABLE_IMPL(, &, &);
INTERNAL_ANY_INVOCABLE_IMPL(const, &, const&);
INTERNAL_ANY_INVOCABLE_IMPL(, &&, &&);
INTERNAL_ANY_INVOCABLE_IMPL(const, &&, const&&);

#undef INTERNAL_ANY_INVOCABLE_IMPL
#undef INTERNAL_ANY_INVOCABLE_IMPL_NOEXEC

}  // namespace any_invocable_detail

// A functional wrapper type, like `std::function`, that assumes ownership of an invocable object.
//
// Unlike `std::function`, though, `util::any_invocable` is more type-safe and provides the additional benefits:
//  * Properly adheres to const correctness of the underlying type.
//  * Is move-only, avoiding concurrency problems with copied invocables and unnecessary copies in general.
//  * Supports reference qualifiers allowing it to perform unique actions.
template <typename Sig>
class any_invocable : private any_invocable_detail::any_invocable_impl<Sig> {
 private:
  static_assert(std::is_function_v<Sig>, "The template argument of any_invocable must be a function type.");

  using impl = any_invocable_detail::any_invocable_impl<Sig>;

 public:
  using result_type = typename impl::result_type;

  any_invocable() noexcept = default;
  any_invocable(std::nullptr_t) noexcept {}
  any_invocable(const any_invocable&) = delete;
  any_invocable(any_invocable&&) noexcept = default;

  // Constructs from an invocable object.
  template <typename F, typename = std::enable_if_t<any_invocable_detail::can_convert<Sig, F>::value>>
  any_invocable(F&& f) : impl(any_invocable_detail::conversion_construct(), std::forward<F>(f)) {}

  // Constructs with an object of type T, which is constructed in-place from the given arguments.
  template <typename T, typename... Args,
            typename = std::enable_if_t<any_invocable_detail::can_emplace<Sig, T, Args...>::value>>
  explicit any_invocable(std::in_place_type_t<T>, Args&&... args)
      : impl(std::in_place_type<std::decay_t<T>>, std::forward<Args>(args)...) {
    static_assert(std::is_same_v<T, std::decay_t<T>>,
                  "The explicit template argument of in_place_type is required to be an unqualified object type.");
  }

  // Overload of the above constructor to support list-initialization.
  template <
      typename T, typename U, typename... Args,
      typename = std::enable_if_t<any_invocable_detail::can_emplace<Sig, T, std::initializer_list<U>&, Args...>::value>>
  explicit any_invocable(std::in_place_type_t<T>, std::initializer_list<U> list, Args&&... args)
      : impl(std::in_place_type<std::decay_t<T>>, list, std::forward<Args>(args)...) {
    static_assert(std::is_same_v<T, std::decay_t<T>>,
                  "The explicit template argument of in_place_type is required to be an unqualified object type.");
  }

  any_invocable& operator=(const any_invocable&) = delete;
  any_invocable& operator=(any_invocable&&) noexcept = default;

  any_invocable& operator=(std::nullptr_t) noexcept {
    this->clear();
    return *this;
  }

  template <typename F, typename = std::enable_if_t<any_invocable_detail::can_assign<Sig, F>::value>>
  any_invocable& operator=(F&& f) {
    *this = any_invocable(std::forward<F>(f));
    return *this;
  }

  template <typename F, typename = std::enable_if_t<any_invocable_detail::can_assign_reference_wrapper<Sig, F>::value>>
  any_invocable& operator=(std::reference_wrapper<F> f) noexcept {
    *this = any_invocable(f);
    return *this;
  }

  ~any_invocable() = default;

  void swap(any_invocable& other) noexcept { std::swap(*this, other); }

  explicit operator bool() const noexcept { return this->has_value(); }

  // Invokes the target object, which must not be empty.
  using impl::operator();

  friend bool operator==(const any_invocable& f, std::nullptr_t) noexcept { return !f.has_value(); }
  friend bool operator==(std::nullptr_t, const any_invocable& f) noexcept { return !f.has_value(); }
  friend bool operator!=(const any_invocable& f, std::nullptr_t) noexcept { return f.has_value(); }
  friend bool operator!=(std::nullptr_t, const any_invocable& f) noexcept { return f.has_value(); }

  friend void swap(any_invocable& f1, any_invocable& f2) noexcept { f1.swap(f2); }

 private:
  template <bool, typename, typename...>
  friend class any_invocable_detail::any_invocable_core_impl;
};

}  // namespace util
