#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "bio/locomotion.hpp"
#include "kids/session.hpp"

#include <omp.h>

TEST_CASE("Kids creature scenes use real factory species and recorded spawns") {
    omp_set_num_threads(2);
    kids::Session s;
    for (int i = 0; i < 4; ++i) {
        s.start(kids::Experience::Safari, i);
        REQUIRE(s.world);
        REQUIRE(s.world->cellCount() > 0);
        CHECK(s.selectedCreature() == i);
        CHECK_FALSE(s.run.interventions().entries().empty());
        CHECK(s.run.interventions().entries().back().key ==
              kids::creatures()[static_cast<std::size_t>(i)].id);
        Vec2 position;
        REQUIRE(s.selectedPosition(position));
        CHECK(position.x > 0);
        CHECK(position.y > 0);
        s.paused = true;
        const auto step = s.world->stepIndex();
        s.tick(0.1f);
        CHECK(s.world->stepIndex() == step);
        s.paused = false;
        s.tick(0.1f);
        CHECK(s.world->stepIndex() > step);
    }
}

TEST_CASE("Kids detective explanation matches both simulated locomotion types") {
    omp_set_num_threads(2);
    kids::Session s;
    s.start(kids::Experience::Detectives);
    const auto p = findSpecies("paramecium"), e = findSpecies("euglena");
    REQUIRE(p);
    REQUIRE(e);
    CHECK(speciesInfo(*p).locomotion == LocomotionType::Cilia);
    CHECK(speciesInfo(*e).locomotion == LocomotionType::Flagellum);
    bool foundP = false, foundE = false;
    for (auto entity : s.world->registry().view<Species>()) {
        const auto id = s.world->registry().get<Species>(entity).id;
        foundP |= id == *p;
        foundE |= id == *e;
    }
    CHECK(foundP);
    CHECK(foundE);
    CHECK_FALSE(s.setSalt(true));
    CHECK_FALSE(s.addCreature(-1, {}));
    CHECK_FALSE(s.addCreature(4, {}));
}

TEST_CASE("All three kids habitats are distinct supported preparations") {
    omp_set_num_threads(2);
    kids::Session s;
    for (int i = 0; i < 3; ++i) {
        s.start(kids::Experience::Pond, i);
        CHECK(s.world->cellCount() >= 3);
        CHECK(s.choice == i);
        CHECK_FALSE(s.paused);
        CHECK_FALSE(s.salty);
    }
    s.start(kids::Experience::Story);
    CHECK(s.selectedCreature() == 0);
    CHECK(s.follow);
}

TEST_CASE("Kids salt experiment records salinity and observes actual plasmolysis") {
    omp_set_num_threads(2);
    kids::Session s;
    s.start(kids::Experience::Lab);
    REQUIRE(s.world->tissue().present());
    CHECK_FALSE(s.changed());
    const auto before = s.meanVolume();
    REQUIRE(s.setSalt(true));
    CHECK(s.world->medium().salinity == doctest::Approx(35.0f));
    CHECK(s.run.interventions().entries().back().key == "environment.salinity");
    for (int i = 0; i < 200; ++i)
        s.tick(.1f);
    CHECK(s.meanVolume() < before);
    CHECK(s.changed());
    REQUIRE(s.setSalt(false));
    CHECK(s.world->medium().salinity == doctest::Approx(s.plate.config.medium.salinity));
    s.start(kids::Experience::Lab);
    CHECK_FALSE(s.salty);
    CHECK_FALSE(s.changed());
}

TEST_CASE("Amoeba food is real algal prey added through recorded actions") {
    omp_set_num_threads(1);
    kids::Session s;
    s.start(kids::Experience::Safari, 1);
    const auto cells = s.world->cellCount();
    REQUIRE(s.offerFood());
    CHECK(s.world->cellCount() == cells + 3);
    CHECK(s.run.interventions().entries().back().key == "chlamydomonas");
    CHECK(s.selectedCreature() == 1);
    CHECK(s.follow);
    CHECK(s.offerFood());
    CHECK(s.offerFood());
    CHECK_FALSE(s.offerFood());
    s.start(kids::Experience::Safari, 2);
    CHECK_FALSE(s.offerFood());
}
