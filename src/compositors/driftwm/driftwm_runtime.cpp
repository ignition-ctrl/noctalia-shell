#include "compositors/driftwm/driftwm_runtime.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace compositors::driftwm {

  namespace {

    [[nodiscard]] bool isSocketPath(const std::string& path) {
      struct stat st{};
      return !path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISSOCK(st.st_mode);
    }

  } // namespace

  bool DriftwmRuntime::available() const {
    ensureResolved();
    return !m_socketPath.empty();
  }

  const std::string& DriftwmRuntime::socketPath() const {
    ensureResolved();
    return m_socketPath;
  }

  std::optional<nlohmann::json> DriftwmRuntime::request(const nlohmann::json& request) const {
    ensureResolved();
    if (m_socketPath.empty()) {
      return std::nullopt;
    }

    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
      return std::nullopt;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (m_socketPath.size() >= sizeof(addr.sun_path)) {
      ::close(fd);
      return std::nullopt;
    }
    std::memcpy(addr.sun_path, m_socketPath.c_str(), m_socketPath.size() + 1);

    if (::connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
      ::close(fd);
      return std::nullopt;
    }

    std::string payload = request.dump();
    payload.push_back('\n');
    std::size_t offset = 0;
    while (offset < payload.size()) {
      const ssize_t written = ::send(fd, payload.data() + offset, payload.size() - offset, MSG_NOSIGNAL);
      if (written <= 0) {
        if (written < 0 && errno == EINTR) {
          continue;
        }
        ::close(fd);
        return std::nullopt;
      }
      offset += static_cast<std::size_t>(written);
    }

    std::string response;
    char buffer[4096];
    while (true) {
      const ssize_t count = ::read(fd, buffer, sizeof(buffer));
      if (count > 0) {
        response.append(buffer, static_cast<std::size_t>(count));
        if (response.contains('\n')) {
          break;
        }
        continue;
      }
      if (count == 0) {
        break;
      }
      if (errno == EINTR) {
        continue;
      }
      ::close(fd);
      return std::nullopt;
    }

    ::close(fd);

    const std::size_t newline = response.find('\n');
    if (newline != std::string::npos) {
      response.resize(newline);
    }
    if (response.empty()) {
      return std::nullopt;
    }

    nlohmann::json reply;
    try {
      reply = nlohmann::json::parse(response);
    } catch (const nlohmann::json::exception&) {
      return std::nullopt;
    }

    // Reply is `{"Ok": <payload>}` or `{"Err": "message"}`.
    if (!reply.is_object()) {
      return std::nullopt;
    }
    const auto okIt = reply.find("Ok");
    if (okIt == reply.end()) {
      return std::nullopt;
    }
    return *okIt;
  }

  bool DriftwmRuntime::action(std::string_view command) const {
    nlohmann::json payload;
    payload["Action"] = std::string(command);
    return request(payload).has_value();
  }

  void DriftwmRuntime::refresh() {
    m_socketPath.clear();
    m_resolved = false;
    resolveSocketPath();
  }

  void DriftwmRuntime::ensureResolved() const {
    if (!m_resolved) {
      resolveSocketPath();
    }
  }

  void DriftwmRuntime::resolveSocketPath() const {
    m_resolved = true;

    if (const char* explicitPath = std::getenv("DRIFTWM_SOCKET");
        explicitPath != nullptr && explicitPath[0] != '\0' && isSocketPath(explicitPath)) {
      m_socketPath = explicitPath;
      return;
    }

    const char* display = std::getenv("WAYLAND_DISPLAY");
    if (display == nullptr || display[0] == '\0') {
      return;
    }
    const char* runtimeDir = std::getenv("XDG_RUNTIME_DIR");
    std::string base = (runtimeDir != nullptr && runtimeDir[0] != '\0') ? runtimeDir : "/tmp";
    std::string candidate = base + "/driftwm/ipc-" + display + ".sock";
    if (isSocketPath(candidate)) {
      m_socketPath = std::move(candidate);
    }
  }

} // namespace compositors::driftwm