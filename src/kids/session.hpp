#pragma once

#include "game/plate_spec.hpp"
#include "game/run.hpp"

#include <array>
#include <memory>
#include <string>

namespace kids {

enum class Experience { Pond, Detectives, Safari, Lab, Story };
struct Creature {
    const char *id;
    const char *name;
    const char *title;
    const char *explanation;
    const char *detail;
    const char *question;
};
const std::array<Creature, 4> &creatures();

// La interfaz infantil usa la misma fisiología; solo prepara escenas y registra acciones.
class Session {
  public:
    void start(Experience experience, int choice = 0);
    bool addCreature(int index, Vec2 position);
    bool offerFood();
    void tick(float seconds);
    bool setSalt(bool salty);
    bool select(Vec2 position, float tolerance);
    bool focusFirst();
    bool selectedPosition(Vec2 &position);
    int selectedCreature() const;
    int meals() const;
    bool wrappingFood() const;
    float meanVolume() const;
    bool changed() const;
    std::string observation() const;

    Experience experience = Experience::Pond;
    int choice = 0;
    bool paused = false;
    bool modelView = false;
    bool follow = false;
    bool salty = false;
    bool predictionMade = false;
    int prediction = -1;
    int answer = -1;
    int chapter = 0;
    int foodOffers = 0;
    float elapsed = 0.0f;
    float beforeVolume = 1.0f;
    Picked selected;
    PlateSpec plate;
    Run run;
    std::unique_ptr<World> world;

  private:
    float accumulator_ = 0.0f;
};
} // namespace kids
