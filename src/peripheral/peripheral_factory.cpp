#include "peripheral_factory.h"

namespace bernd_box {
namespace peripheral {

PeripheralFactory::PeripheralFactory(Server& server) : server_(server) {}

bool PeripheralFactory::registerFactory(const String& name, Callback factory) {
  if (getFactories().count(name) == 0) {
    getFactories().emplace(name, factory);
    return true;
  }
  return false;
}

std::shared_ptr<Peripheral> PeripheralFactory::createPeripheral(
    const JsonObjectConst& parameter) {
  const JsonVariantConst type = parameter[type_key_];
  if (type.isNull() || !type.is<char*>()) {
    return std::make_shared<InvalidPeripheral>(type_key_error_);
  }

  auto factory = getFactories().find(type);
  if (factory != getFactories().end()) {
    return factory->second(parameter);
  } else {
    return std::make_shared<InvalidPeripheral>(
        unknownTypeError(type.as<char*>()));
  }
}

std::vector<String> PeripheralFactory::getFactoryNames() {
  std::vector<String> names;
  for (const auto& factory : getFactories()) {
    names.push_back(factory.first);
  }
  return names;
}

std::map<const String, PeripheralFactory::Callback>&
PeripheralFactory::getFactories() {
  static std::map<const String, Callback> factories;
  return factories;
}

String PeripheralFactory::unknownTypeError(const String& type) {
  String error(F("Unknown peripheral type: "));
  error += type;
  return error;
}

const __FlashStringHelper* PeripheralFactory::type_key_ = F("type");
const __FlashStringHelper* PeripheralFactory::type_key_error_ =
    F("Missing property: type (string)");

}  // namespace peripheral
}  // namespace bernd_box
