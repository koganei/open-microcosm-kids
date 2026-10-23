#include "kids/session.hpp"

#include "bio/species.hpp"
#include "game/biome.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace kids {
const std::array<Creature, 4> &creatures() {
    static const std::array<Creature, 4> all{
        {{"paramecium", "Paramecium", "A swimmer with tiny hairs",
          "Tiny hair-like parts called cilia beat to help this single cell swim and sweep in food.",
          "Paramecium eats bacteria. Food is held in little spaces called food vacuoles while it "
          "is digested.",
          "Can you spot the hairs around its edge?"},
         {"amoeba", "Amoeba", "A cell that changes shape",
          "An amoeba reaches out with parts of its body to move and wrap around food.",
          "These changing extensions are called pseudopods. An amoeba has no legs and no mouth "
          "like ours.",
          "How does its outline change as you watch?"},
         {"euglena", "Euglena", "A green cell with a whip-like thread",
          "A thread called a flagellum helps Euglena swim. Green parts use light to make food.",
          "The green parts are chloroplasts. A green Euglena is one cell, even though it can both "
          "swim and use light.",
          "Can you find its swimming thread?"},
         {"volvox", "Volvox", "A colony of tiny green cells",
          "Many little cells live together in a hollow ball. Their tiny threads help the colony "
          "swim.",
          "Each little green cell is alive. Smaller daughter colonies can grow inside the parent "
          "colony.",
          "Can you spot the smaller colonies inside?"}}};
    return all;
}

void Session::start(Experience e, int c) {
    experience = e;
    choice = std::clamp(c, 0, e == Experience::Safari ? 3 : 2);
    paused = modelView = follow = salty = predictionMade = false;
    prediction = answer = -1;
    chapter = foodOffers = 0;
    elapsed = accumulator_ = 0.0f;
    selected = {};
    plate = PlateSpec{};
    plate.seed = 20271023;
    if (e == Experience::Lab) {
        plate.prepared = "onion_epidermis";
        plate.rebuild();
        // Un campo de 512 um conserva la resolución y muestra menos células.
        plate.config.gridX = plate.config.gridY = 64;
    } else {
        const char *habitat[] = {"pond", "puddle", "moss"};
        const auto biomeId = findBiome(habitat[e == Experience::Pond ? choice : 0]);
        if (!biomeId)
            throw std::runtime_error("The habitat content could not be loaded.");
        plate.biome = static_cast<int>(*biomeId);
        plate.content = DropContent::EmptyPetri;
        plate.rebuild();
        // Un campo pequeño y pocos seres permiten observar sin saturar la pantalla.
        plate.config.gridX = plate.config.gridY = 128;
        plate.config.bacteria = 900;
        plate.config.detritus = 12;
        plate.config.medium.detritusSpawnRate = 0.0f;
    }
    world = std::make_unique<World>(plate.config);
    RunConfig config;
    config.seed = plate.seed;
    config.biome = plate.biome;
    config.spawnFounder = false;
    config.wildlife = 0;
    run.start(*world, config, 1);
    const Vec2 center{world->width() / 2, world->height() / 2};
    if (e == Experience::Safari) {
        addCreature(choice, center);
    } else if (e == Experience::Detectives) {
        addCreature(0, center + Vec2{-120, 0});
        addCreature(2, center + Vec2{140, 0});
    } else if (e == Experience::Story) {
        addCreature(0, center);
        addCreature(2, center + Vec2{150, 100});
    } else if (e != Experience::Lab) {
        // Selección didáctica del hábitat, no censo de una muestra natural.
        const int sets[3][3] = {{0, 2, 3}, {0, 1, 2}, {1, 2, 0}};
        for (int i = 0; i < 3; ++i)
            addCreature(sets[e == Experience::Pond ? choice : 0][i],
                        center + Vec2{static_cast<float>((i - 1) * 180), i == 1 ? 120.0f : -40.0f});
    }
    if (e == Experience::Detectives)
        paused = true;
    beforeVolume = meanVolume();
    if (e == Experience::Safari || e == Experience::Story) {
        focusFirst();
        follow = true;
    }
}

bool Session::addCreature(int index, Vec2 position) {
    if (!world || index < 0 || index >= static_cast<int>(creatures().size()))
        return false;
    const auto species = findSpecies(creatures()[static_cast<std::size_t>(index)].id);
    if (!species)
        return false;
    Intervention iv;
    iv.kind = InterventionKind::Spawn;
    iv.index = static_cast<int>(BeingKind::Species);
    iv.v[0] = static_cast<float>(*species);
    iv.key = creatures()[static_cast<std::size_t>(index)].id;
    iv.pos = position;
    return run.interventions().record(*world, iv);
}

bool Session::offerFood() {
    if (!world || experience != Experience::Safari || choice != 1 || foodOffers >= 3)
        return false;
    const auto food = findSpecies("chlamydomonas");
    if (!food || !focusFirst())
        return false;
    Vec2 center;
    if (!selectedPosition(center))
        return false;
    const float radius = buildInspection(*world, selected).radius;
    for (int i = 0; i < 3; ++i) {
        Intervention iv;
        iv.kind = InterventionKind::Spawn;
        iv.index = static_cast<int>(BeingKind::Species);
        iv.v[0] = static_cast<float>(*food);
        iv.key = "chlamydomonas";
        iv.pos = center + Vec2{radius + 12, static_cast<float>((i - 1) * 16)};
        if (!run.interventions().record(*world, iv))
            return false;
    }
    ++foodOffers;
    follow = true;
    return true;
}

void Session::tick(float seconds) {
    if (!world || paused)
        return;
    accumulator_ += std::min(seconds, 0.1f);
    constexpr float dt = 1.0f / 60.0f;
    int steps = 0;
    while (accumulator_ >= dt && steps++ < 6) {
        run.interventions().beforeStep(*world, dt);
        world->step(dt);
        run.update(*world, dt);
        elapsed += dt;
        accumulator_ -= dt;
    }
}

bool Session::setSalt(bool value) {
    if (!world || experience != Experience::Lab)
        return false;
    Intervention iv;
    iv.kind = InterventionKind::Parameter;
    iv.key = "environment.salinity";
    iv.v[0] = value ? 35.0f : plate.config.medium.salinity;
    const bool ok = run.interventions().record(*world, iv);
    if (ok)
        salty = value;
    return ok;
}

bool Session::select(Vec2 p, float tolerance) {
    if (!world)
        return false;
    selected = world->pick(p, tolerance);
    follow = false;
    return static_cast<bool>(selected);
}

bool Session::focusFirst() {
    if (!world)
        return false;
    const int wanted = experience == Experience::Safari ? choice : 0;
    entt::entity fallback = entt::null;
    for (auto e : world->registry().view<Species, RingBody>()) {
        if (fallback == entt::null)
            fallback = e;
        const auto species = world->registry().get<Species>(e).id;
        if (std::string_view(speciesInfo(species).key) ==
            creatures()[static_cast<std::size_t>(wanted)].id) {
            fallback = e;
            break;
        }
    }
    if (fallback != entt::null) {
        selected = {};
        selected.kind = Picked::Kind::Cell;
        selected.entity = fallback;
        return true;
    }
    return false;
}

bool Session::selectedPosition(Vec2 &p) {
    return world && locatePicked(*world, selected, p);
}

int Session::selectedCreature() const {
    if (!world || selected.kind != Picked::Kind::Cell || !world->registry().valid(selected.entity))
        return -1;
    const auto *s = world->registry().try_get<Species>(selected.entity);
    if (!s)
        return -1;
    const auto &id = speciesInfo(s->id).key;
    for (std::size_t i = 0; i < creatures().size(); ++i)
        if (std::string_view(id) == creatures()[i].id)
            return static_cast<int>(i);
    return -1;
}

int Session::meals() const {
    if (!world || selected.kind != Picked::Kind::Cell || !world->registry().valid(selected.entity))
        return 0;
    const auto *digestion = world->registry().try_get<Digestion>(selected.entity);
    return digestion ? digestion->cellsEaten : 0;
}
bool Session::wrappingFood() const {
    return world && selected.kind == Picked::Kind::Cell &&
           world->registry().valid(selected.entity) &&
           world->registry().all_of<PhagocyticCup>(selected.entity);
}

float Session::meanVolume() const {
    if (!world || world->tissue().cells.empty())
        return 1.0f;
    float sum = 0;
    for (const auto &c : world->tissue().cells)
        sum += c.volume;
    return sum / static_cast<float>(world->tissue().cells.size());
}
bool Session::changed() const {
    return meanVolume() < beforeVolume * 0.95f;
}
std::string Session::observation() const {
    if (!salty)
        return predictionMade
                   ? "We changed back to fresh water. Watch whether the contents fill the walls "
                     "again."
                   : "The cells are in fresh water. Look at the walls and the cell contents.";
    if (!changed())
        return "Keep watching. Water moves gradually, so a change may take a little time.";
    return "The living contents got smaller inside the walls. Water moved out into the saltier "
           "water.";
}
} // namespace kids
