#include "Context.h"
#include "Support.h"
#include "Type.h"
#include "Expose.h"

#include <pybind11/stl.h>

using namespace pybind11;

namespace mlir {
namespace py {

template <typename FcnT> auto nullcheck(FcnT fcn) {
  return ::nullcheck(fcn, "function type");
}

FunctionType getFunctionType(const std::vector<Type> &inputs,
                             const std::vector<Type> &results) {
  return FunctionType::get(inputs, results, getMLIRContext());
}

void exposeFunctionType(pybind11::module &m, TypeClass &type) {
  class_<FunctionType>(m, "FunctionType", type)
      .def(init(&getFunctionType))
      .def_property_readonly("inputs", nullcheck([](FunctionType ty) {
        auto inputs = ty.getInputs();
        return new std::vector<Type>{std::begin(inputs), std::end(inputs)};
      }))
      .def_property_readonly("results", nullcheck([](FunctionType ty) {
        auto inputs = ty.getResults();
        return new std::vector<Type>{std::begin(inputs), std::end(inputs)};
      }));
}

} // end namespace py
} // end namespace mlir