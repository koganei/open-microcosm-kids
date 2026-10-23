#pragma once
#include <string>

namespace kids {
// Conexión local persistente: detener solo la voz de esta aplicación.
class Narrator {
  public:
    ~Narrator();
    bool speak(const std::string &text);
    void stop();

  private:
    int socket_ = -1;
    bool connect();
    bool command(const std::string &text);
    bool reply();
    void close();
};
} // namespace kids
