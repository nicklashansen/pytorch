#include <ATen/ATen.h>
#include <ATen/NativeFunctions.h>
#include <c10/util/Optional.h>

#include <c10/core/impl/DeviceGuardImplInterface.h>

namespace at {
namespace native {

// Take a Device that may not have device_index set (i.e., having it as -1
// representing the current device) and return the corresponding Device
// according to the actual device at the time of this function call.  No-op
// if the device_index is set.
static inline Device ensure_has_index(Device device) {
  if (device.is_cpu() || device.has_index()) {
    return device;
  }
  const c10::impl::DeviceGuardImplInterface* impl = c10::impl::getDeviceGuardImpl(device.type());
  return impl->getDevice();
}

static inline Tensor to_impl(const Tensor& self, const TensorOptions& options, bool non_blocking, bool copy, c10::optional<c10::MemoryFormat> optional_memory_format) {
  auto memory_format =
      optional_memory_format.value_or(MemoryFormat::Contiguous);

  if (self.options() == options && !copy &&
      (memory_format == MemoryFormat::Preserve ||
       self.suggest_memory_format() == memory_format)) {
    return self;
  }

  if (memory_format == MemoryFormat::Preserve) {
    if (self.is_non_overlapping_and_dense()) {
      // Copy all strides
      auto r = at::empty_strided(self.sizes(), self.strides(), options);
      r.copy_(self);
      return r;
    } else {
      memory_format = self.suggest_memory_format();
    }
  }
  auto r = at::empty(self.sizes(), options, memory_format);
  r.copy_(self, non_blocking);
  return r;
}

Tensor to(const Tensor& self, ScalarType dtype, Layout layout, Device device, bool pin_memory, bool non_blocking, bool copy, c10::optional<c10::MemoryFormat> optional_memory_format) {
  // [CHECK THIS]
  /*
  TORCH_CHECK(options.requires_grad_opt() == c10::nullopt,
           "to(options) expects unset requires_grad flag, but got "
           "options.requires_grad set as ", options.requires_grad());

  const auto & layout_opt = options.layout_opt();
  TORCH_CHECK(!layout_opt || self.layout() == layout_opt.value(),
           "to(options) doesn't support converting to a different layout, "
           "but got self.layout being ", self.layout(),
           " and options.layout set as ", options.layout());
  */
  auto specified_options = self.options();
  auto device_opt = ensure_has_index(device);
  specified_options = specified_options.device(device);
  specified_options = specified_options.dtype(dtype);
  
  return to_impl(self, specified_options, non_blocking, copy, optional_memory_format);
}

Tensor to(const Tensor& self, Device device, ScalarType dtype, bool non_blocking, bool copy, c10::optional<c10::MemoryFormat> optional_memory_format) {
  device = ensure_has_index(device);
  return to_impl(
      self,
      self.options().device(device).dtype(dtype),
      non_blocking,
      copy,
      optional_memory_format);
}

Tensor to(const Tensor& self, ScalarType dtype, bool non_blocking, bool copy, c10::optional<c10::MemoryFormat> optional_memory_format) {
  return to_impl(
      self, self.options().dtype(dtype), non_blocking, copy, optional_memory_format);
}

Tensor to(const Tensor& self, const Tensor& other, bool non_blocking, bool copy, c10::optional<c10::MemoryFormat> optional_memory_format) {
  auto options = other.options();
  return to_impl(self, options, non_blocking, copy, optional_memory_format);
}

Tensor to_dense_backward(const Tensor& grad, const Tensor& input_) {
  AT_ASSERT(input_.layout() != c10::kStrided);
  if (input_.layout() == c10::kSparse) {
    auto input = input_.coalesce();
    return grad.sparse_mask(input);
  } else if (input_.layout() == c10::kMkldnn) {
    return grad.to_mkldnn();
  } else {
    AT_ERROR("Unsupported input layout: ", input_.layout());
  }
}

Tensor to_mkldnn_backward(const Tensor& grad, const Tensor& input_) {
  AT_ASSERT(input_.layout() == c10::kStrided);
  return grad.to_dense();
}

}} // namespace at::native
