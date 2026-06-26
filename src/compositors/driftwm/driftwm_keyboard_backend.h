#pragma once

#include "compositors/keyboard_backend.h"

#include <optional>
#include <string>

namespace compositors::driftwm {
  class DriftwmRuntime;
}

class DriftwmKeyboardBackend {
public:
  explicit DriftwmKeyboardBackend(compositors::driftwm::DriftwmRuntime& runtime);
  ~DriftwmKeyboardBackend() = default;

  DriftwmKeyboardBackend(const DriftwmKeyboardBackend&) = delete;
  DriftwmKeyboardBackend& operator=(const DriftwmKeyboardBackend&) = delete;
  DriftwmKeyboardBackend(DriftwmKeyboardBackend&&) = delete;
  DriftwmKeyboardBackend& operator=(DriftwmKeyboardBackend&&) = delete;

  [[nodiscard]] bool isAvailable() const noexcept;
  [[nodiscard]] bool cycleLayout() const;
  [[nodiscard]] std::optional<KeyboardLayoutState> layoutState() const;
  [[nodiscard]] std::optional<std::string> currentLayoutName() const;

private:
  compositors::driftwm::DriftwmRuntime& m_runtime;
};
