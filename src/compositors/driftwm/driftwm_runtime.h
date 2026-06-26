#pragma once

#include <json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace compositors::driftwm {

  // driftwm speaks line-delimited JSON over a Unix socket. Each request is one
  // serde-tagged `Request` object; each reply is a serde `Result<Response, String>`
  // i.e. `{"Ok": <payload>}` or `{"Err": "message"}`. Unlike Mango, driftwm does
  // not export a socket-path env var: the socket name derives from WAYLAND_DISPLAY,
  // mirroring how the `driftwm msg` client locates the running instance.
  class DriftwmRuntime {
  public:
    DriftwmRuntime() = default;

    [[nodiscard]] bool available() const;
    [[nodiscard]] const std::string& socketPath() const;

    // Sends `request` and returns the unwrapped `Ok` payload (nullopt on error,
    // disconnect, or an `Err` reply).
    [[nodiscard]] std::optional<nlohmann::json> request(const nlohmann::json& request) const;

    // Runs a config-grammar action (e.g. "switch-layout next", "quit") via the
    // `Action` request. Returns true when the compositor replies `Ok`.
    [[nodiscard]] bool action(std::string_view command) const;

    void refresh();

  private:
    void ensureResolved() const;
    void resolveSocketPath() const;

    mutable bool m_resolved = false;
    mutable std::string m_socketPath;
  };

} // namespace compositors::driftwm
