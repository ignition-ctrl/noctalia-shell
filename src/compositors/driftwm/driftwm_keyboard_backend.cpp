#include "compositors/driftwm/driftwm_keyboard_backend.h"

#include "compositors/driftwm/driftwm_runtime.h"

#include <json.hpp>

DriftwmKeyboardBackend::DriftwmKeyboardBackend(compositors::driftwm::DriftwmRuntime& runtime) : m_runtime(runtime) {}

bool DriftwmKeyboardBackend::isAvailable() const noexcept { return m_runtime.available(); }

bool DriftwmKeyboardBackend::cycleLayout() const { return m_runtime.action("switch-layout next"); }

std::optional<KeyboardLayoutState> DriftwmKeyboardBackend::layoutState() const {
  // driftwm reports only the active layout's display name, not the full ring or
  // its index, so the state is a single entry at index 0.
  const auto current = currentLayoutName();
  if (!current.has_value()) {
    return std::nullopt;
  }
  return KeyboardLayoutState{{*current}, 0};
}

std::optional<std::string> DriftwmKeyboardBackend::currentLayoutName() const {
  nlohmann::json payload;
  payload["Layout"] = nlohmann::json{{"short", false}};
  const auto response = m_runtime.request(payload);
  if (!response.has_value() || !response->is_object()) {
    return std::nullopt;
  }
  const auto layoutIt = response->find("Layout");
  if (layoutIt == response->end() || !layoutIt->is_string()) {
    return std::nullopt;
  }
  return layoutIt->get<std::string>();
}
