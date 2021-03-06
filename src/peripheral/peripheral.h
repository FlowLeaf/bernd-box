#pragma once

#include <Arduino.h>

#include <array>

#include "managers/io_types.h"

namespace bernd_box {
namespace peripheral {

class TaskFactory;

class Peripheral {
 public:
  Peripheral() = default;
  virtual ~Peripheral() = default;

  /**
   * Gives the type of the peripheral
   *
   * Overwritten by each concrete peripheral instantiation and used by the
   * peripheral polymorphism system.
   * 
   * \return The name of the type
   */
  virtual const String& getType() const = 0;

  /**
   * Checks if the peripheral is valid (often used after construction)
   *
   * \return True if it is valid
   */
  bool isValid() const;

  /**
   * Returns the error result which contains the cause of the errro
   *
   * \return The error result
   */
  ErrorResult getError() const;

 protected:
  /**
   * Mark the peripheral as invalid and should therefore not be used
   *
   * \see isValid()
   */
  void setInvalid();

  /**
   * Mark the peripheral as invalid and give the reason for the error
   *
   * The state can be checked with the isValid function and the reason can be
   * extracted with the getError() function.
   *
   * \see isValid() and getError()
   * \param error_message The reason for being marked invalid
   */
  void setInvalid(const String& error_message);

  // Common parameter keys can be reused
  static const __FlashStringHelper* data_point_type_key_;
  static const __FlashStringHelper* data_point_type_key_error_;

 private:
  /// If the peripheral was constructed correctly and is still functional
  bool valid_ = true;
  /// The error message if an error occured
  String error_message_;
};

}  // namespace peripheral
}  // namespace bernd_box
