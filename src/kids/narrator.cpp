#include "kids/narrator.hpp"
#include <cstdlib>
#include <cstring>
#if defined(__linux__)
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace kids {
Narrator::~Narrator() {
    stop();
    close();
}
void Narrator::close() {
#if defined(__linux__)
    if (socket_ >= 0)
        ::close(socket_);
#endif
    socket_ = -1;
}
bool Narrator::reply() {
#if defined(__linux__)
    // SSIP: las líneas intermedias llevan '-', la última un espacio.
    std::string row;
    for (int bytes = 0; bytes < 8192; ++bytes) {
        char ch;
        if (recv(socket_, &ch, 1, 0) != 1) {
            close();
            return false;
        }
        if (ch == '\n') {
            if (row.size() >= 4 && row[3] == ' ')
                return row[0] == '2';
            row.clear();
        } else
            row += ch;
    }
#endif
    close();
    return false;
}
bool Narrator::command(const std::string &text) {
#if defined(__linux__)
    const std::string wire = text + "\r\n";
    std::size_t sent = 0;
    while (sent < wire.size()) {
        const auto n = send(socket_, wire.data() + sent, wire.size() - sent, MSG_NOSIGNAL);
        if (n <= 0) {
            close();
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return reply();
#else
    (void)text;
    return false;
#endif
}
bool Narrator::connect() {
    if (socket_ >= 0)
        return true;
#if defined(__linux__)
    const char *runtime = std::getenv("XDG_RUNTIME_DIR");
    if (!runtime)
        return false;
    const std::string path = std::string(runtime) + "/speech-dispatcher/speechd.sock";
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (path.size() >= sizeof(address.sun_path))
        return false;
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
    socket_ = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (socket_ < 0)
        return false;
    timeval timeout{0, 250000};
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (::connect(socket_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
        close();
        return false;
    }
    if (command("SET SELF CLIENT_NAME kids:open-microcosm-kids:main") &&
        command("SET SELF LANGUAGE en") && command("SET SELF RATE -15") &&
        command("SET SELF PRIORITY TEXT"))
        return true;
    close();
#endif
    return false;
}
bool Narrator::speak(const std::string &text) {
    if (!connect())
        return false;
    if (!command("CANCEL SELF") || !command("SPEAK"))
        return false;
    // Neutralizar saltos de línea para que el texto no cierre el mensaje SSIP.
    std::string safe = text;
    for (char &ch : safe)
        if (ch == '\r' || ch == '\n')
            ch = ' ';
    return command(safe + "\r\n.");
}
void Narrator::stop() {
    if (socket_ >= 0 && !command("CANCEL SELF"))
        close();
}
} // namespace kids
