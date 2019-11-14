#include <c10d/GlooDeviceFactory.hpp>

#include <stdlib.h>

#include <c10/util/Exception.h>

#if GLOO_HAVE_TRANSPORT_TCP
#include <gloo/transport/tcp/device.h>
#endif

#if GLOO_HAVE_TRANSPORT_UV
#include <gloo/transport/uv/device.h>
#endif

// On Linux, check that the tcp transport is available.
#ifdef __linux__
#if !GLOO_HAVE_TRANSPORT_TCP
#error "Expected the tcp transport to be available on Linux."
#endif
#endif

// On macOS, check that the uv transport is available.
#ifdef __APPLE__
#if !GLOO_HAVE_TRANSPORT_UV
#error "Expected the uv transport to be available on macOS."
#endif
#endif

namespace c10d {

C10_DEFINE_SHARED_REGISTRY(
    GlooDeviceRegistry,
    ::gloo::transport::Device,
    const std::string& /* interface */,
    const std::string& /* hostname */);

#if GLOO_HAVE_TRANSPORT_TCP
static std::shared_ptr<::gloo::transport::Device> makeTCPDevice(
    const std::string& interface,
    const std::string& hostname) {
  TORCH_CHECK(
      !interface.empty() || !hostname.empty(),
      "GlooDeviceFactory::makeTCPDevice(): interface or hostname "
      "can't be empty");

  ::gloo::transport::tcp::attr attr;
  if (!interface.empty()) {
    attr.iface = interface;
  } else {
    attr.hostname = hostname;
  }
  return ::gloo::transport::tcp::CreateDevice(attr);
}

C10_REGISTER_CREATOR(GlooDeviceRegistry, LINUX, makeTCPDevice);
C10_REGISTER_CREATOR(GlooDeviceRegistry, TCP, makeTCPDevice);
#endif

#if GLOO_HAVE_TRANSPORT_UV
static std::shared_ptr<::gloo::transport::Device> makeUVDevice(
    const std::string& interface,
    const std::string& hostname) {
  TORCH_CHECK(
      !interface.empty() || !hostname.empty(),
      "GlooDeviceFactory::makeUVDevice(): interface or hostname "
      "can't be empty");

  ::gloo::transport::uv::attr attr;
  if (!interface.empty()) {
    attr.iface = interface;
  } else {
    attr.hostname = hostname;
  }
  return ::gloo::transport::uv::CreateDevice(attr);
}

C10_REGISTER_CREATOR(GlooDeviceRegistry, APPLE, makeUVDevice);
C10_REGISTER_CREATOR(GlooDeviceRegistry, UV, makeUVDevice);
#endif

static const char* glooDevice = getenv("gloo_device");

std::shared_ptr<::gloo::transport::Device> GlooDeviceFactory::
    makeDeviceForInterface(const std::string& interface) {
  if (glooDevice) {
    return GlooDeviceRegistry()->Create(glooDevice, interface, "");
  }

#ifdef __linux__
  return GlooDeviceRegistry()->Create("LINUX", interface, "");
#endif

#ifdef __APPLE__
  return GlooDeviceRegistry()->Create("APPLE", interface, "");
#endif

  return nullptr;
}

std::shared_ptr<::gloo::transport::Device> GlooDeviceFactory::
    makeDeviceForHostname(const std::string& hostname) {
  if (glooDevice) {
    return GlooDeviceRegistry()->Create(glooDevice, "", hostname);
  }

#ifdef __linux__
  return GlooDeviceRegistry()->Create("LINUX", "", hostname);
#endif

#ifdef __APPLE__
  return GlooDeviceRegistry()->Create("APPLE", "", hostname);
#endif

  return nullptr;
}

} // namespace c10d
