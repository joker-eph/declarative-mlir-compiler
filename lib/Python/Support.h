#include <llvm/Support/raw_ostream.h>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>

/// Create a printer for MLIR objects to std::string.
template <typename T>
struct StringPrinter {
  std::string operator()(T t) const {
    std::string buf;
    llvm::raw_string_ostream os{buf};
    t.print(os);
    return std::move(os.str());
  }
};

/// Cast to an overloaded function type.
template <typename FcnT>
auto overload(FcnT fcn) { return fcn; }

/// Move a value to the heap and let Python manage its lifetime.
template <typename T>
std::unique_ptr<T> moveToHeap(T &&t) {
  auto ptr = std::make_unique<T>();
  *ptr = std::move(t);
  return ptr;
}

/// Automatically wrap function calls in a nullcheck of the primary argument.
template <typename FcnT>
std::function<pybind11::detail::function_signature_t<FcnT>>
nullcheck(FcnT fcn, std::string name,
          std::enable_if_t<!std::is_member_function_pointer_v<FcnT>> * = 0) {
  return [fcn, name](auto t, auto ...ts) {
    if (!t)
      throw std::invalid_argument{name + " is null"};
    return fcn(t, ts...);
  };
}

/// Automatically wrap member function calls in a nullcheck of the object.
template <typename RetT, typename ObjT, typename... ArgTs>
std::function<RetT(ObjT, ArgTs...)>
nullcheck(RetT(ObjT::*fcn)(ArgTs...), std::string name) {
  return [fcn, name](auto t, ArgTs ...args) -> RetT {
    if (!t)
      throw std::invalid_argument{name + " is null"};
    return (t.*fcn)(args...);
  };
}

/// For const member functions.
template <typename RetT, typename ObjT, typename... ArgTs>
std::function<RetT(const ObjT, ArgTs...)>
nullcheck(RetT(ObjT::*fcn)(ArgTs...) const, std::string name) {
  return [fcn, name](auto t, ArgTs ...args) -> RetT {
    if (!t)
      throw std::invalid_argument{name + " is null"};
    return (t.*fcn)(args...);
  };
}

/// Create an isa<> check.
template <typename From, typename To>
auto isa() {
  return [](From f) { return f.template isa<To>(); };
}

/// Automatically generate implicit conversions to parent class with
/// LLVM polymorphism: implicit conversion statements and constuctors.
namespace detail {

template <typename, typename...>
struct implicitly_convertible_from_all_helper;

template <typename BaseT, typename FirstT>
struct implicitly_convertible_from_all_helper<BaseT, FirstT> {
  static void doit(pybind11::class_<BaseT> &cls) {
    cls.def(pybind11::init<FirstT>());
    pybind11::implicitly_convertible<FirstT, BaseT>();
  }
};

template <typename BaseT, typename FirstT, typename... DerivedTs>
struct implicitly_convertible_from_all_helper<BaseT, FirstT, DerivedTs...> {
  static void doit(pybind11::class_<BaseT> &cls) {
    implicitly_convertible_from_all_helper<BaseT, FirstT>::doit(cls);
    implicitly_convertible_from_all_helper<BaseT, DerivedTs...>::doit(cls);
  }
};

} // end namespace detail

template <typename BaseT, typename... DerivedTs>
void implicitly_convertible_from_all(pybind11::class_<BaseT> &cls) {
  detail::implicitly_convertible_from_all_helper<
      BaseT, DerivedTs...>::doit(cls);
}

/// Shorthand for declaring polymorphic type hooks for MLIR-style RTTI.
namespace detail {

template <typename, typename...>
struct polymorphic_type_hooks_impl;

template <typename BaseT, typename DerivedT>
struct polymorphic_type_hooks_impl<BaseT, DerivedT> {
  static const void *get(const BaseT *src,
                         const std::type_info *&type) {
    if (src->template isa<DerivedT>()) {
      type = &typeid(DerivedT);
      return static_cast<const DerivedT *>(src);
    }
    return nullptr;
  }
};

template <typename BaseT, typename DerivedT, typename... DerivedTs>
struct polymorphic_type_hooks_impl<BaseT, DerivedT, DerivedTs...> {
  static const void *get(const BaseT *src,
                         const std::type_info *&type) {
    auto ptr = polymorphic_type_hooks_impl<BaseT, DerivedT>::get(src, type);
    return ptr ? ptr :
        polymorphic_type_hooks_impl<BaseT, DerivedTs...>::get(src, type);
  }
};

} // end namespace detail

template <typename BaseT, typename... DerivedTs>
struct polymorphic_type_hooks {
  static const void *get(const BaseT *src,
                         const std::type_info *&type) {
    if (!src)
      return src;
    return detail::polymorphic_type_hooks_impl<BaseT, DerivedTs...>
          ::get(src, type);
  }
};

