#include "kids/narrator.hpp"
#include "kids/session.hpp"

#include "build_info.hpp"
#include "core/paths.hpp"
#include "render/photo.hpp"
#include "render/renderer.hpp"

#include <omp.h>
#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>

namespace {
constexpr float W = 1440, H = 900;
constexpr Color paper{248, 247, 239, 255}, ink{24, 58, 51, 255}, muted{91, 113, 104, 255};
constexpr Color green{37, 92, 72, 255}, pale{231, 237, 218, 255}, gold{245, 195, 78, 255};
constexpr Color coral{207, 94, 64, 255}, line{216, 223, 207, 255}, white{255, 255, 249, 255};
std::array<Texture2D, 3> habitatTextures{};
const std::array<const char *, 5> names{
    {"Pocket Pond", "Tiny Detectives", "Micro Safari", "Wonder Lab", "A Drop of Adventure"}};
const std::array<const char *, 5> verbs{{"Explore a habitat", "Solve a little mystery",
                                         "Meet a living thing", "Try an experiment",
                                         "Follow a story"}};
const std::array<const char *, 5> subtitles{
    {"A whole world in a drop.", "Look closely. What do you notice?",
     "Small lives. Amazing discoveries.", "Make a guess. Watch what changes.",
     "A morning in one tiny drop."}};

struct Photo {
    std::string file, name, caption;
    Texture2D texture{};
};

struct Ui {
    Font font{};
    Vector2 mouse{};
    bool click = false, used = false;
    int index = 0, focused = -1, count = 0;
    float scale = 1;
    Vector2 origin{};
    void begin() {
        const Vector2 raw = GetMousePosition();
        mouse = {(raw.x - origin.x) / scale, (raw.y - origin.y) / scale};
        click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        used = false;
        if (IsKeyPressed(KEY_TAB))
            focused =
                count > 0 ? (focused + (IsKeyDown(KEY_LEFT_SHIFT) ? count - 1 : 1)) % count : 0;
        index = 0;
    }
    void end() { count = index; }
    void text(const std::string &s, float x, float y, float size = 24, Color c = ink) const {
        DrawTextEx(font, s.c_str(), {x, y}, size, 0, c);
    }
    float wrap(const std::string &s, float x, float y, float width, float size = 24,
               Color c = muted) const {
        std::string word, row;
        float dy = 0;
        auto emit = [&] {
            text(row, x, y + dy, size, c);
            dy += size * 1.4f;
            row.clear();
        };
        for (std::size_t i = 0; i <= s.size(); ++i) {
            const char ch = i < s.size() ? s[i] : '\n';
            if (ch == ' ' || ch == '\n') {
                const std::string candidate = row.empty() ? word : row + " " + word;
                if (!row.empty() && MeasureTextEx(font, candidate.c_str(), size, 0).x > width)
                    emit();
                if (!word.empty())
                    row += (row.empty() ? "" : " ") + word;
                word.clear();
                if (ch == '\n' && !row.empty())
                    emit();
            } else
                word += ch;
        }
        return dy;
    }
    void box(Rectangle r, Color c, float round = 0.08f) const {
        DrawRectangleRounded(r, round, 12, c);
    }
    bool button(const std::string &s, Rectangle r, bool primary = false, bool active = false,
                bool enabled = true) {
        const int id = enabled ? index++ : -2;
        const bool hover = enabled && CheckCollisionPointRec(mouse, r);
        if (hover)
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        Color bg = !enabled ? Color{237, 238, 229, 255} : primary ? green : (active ? pale : white);
        if (hover)
            bg = primary ? Color{29, 77, 59, 255} : Color{235, 239, 225, 255};
        box(r, bg, 0.22f);
        if (!primary)
            DrawRectangleRoundedLinesEx(r, 0.22f, 12, 1.2f, active ? green : line);
        if (id == focused)
            DrawRectangleRoundedLinesEx({r.x - 3, r.y - 3, r.width + 6, r.height + 6}, 0.22f, 12, 3,
                                        coral);
        const float sz = 21;
        const float tw = MeasureTextEx(font, s.c_str(), sz, 0).x;
        text(s, r.x + (r.width - tw) / 2, r.y + (r.height - sz) / 2, sz,
             !enabled  ? muted
             : primary ? white
                       : ink);
        const bool activate = (hover && click) || (id == focused && IsKeyPressed(KEY_ENTER));
        if (enabled && activate && !used) {
            used = true;
            focused = id;
            return true;
        }
        return false;
    }
};

// Ilustración del hábitat, separada de la imagen científica del motor.
void habitatArt(Rectangle r, int variant) {
    const Texture2D image = habitatTextures[static_cast<std::size_t>(std::clamp(variant, 0, 2))];
    if (image.id) {
        const float ratio = std::max(r.width / static_cast<float>(image.width),
                                     r.height / static_cast<float>(image.height));
        const float sw = r.width / ratio, sh = r.height / ratio;
        DrawTexturePro(image, {(image.width - sw) / 2, (image.height - sh) / 2, sw, sh}, r, {0, 0},
                       0, WHITE);
        return;
    }
    BeginScissorMode(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width),
                     static_cast<int>(r.height));
    DrawRectangleRec(r, {223, 233, 205, 255});
    DrawCircleV({r.x + r.width * .74f, r.y + r.height * .2f}, r.height * .19f,
                {250, 226, 153, 255});
    for (int i = 0; i < 12; ++i) {
        const float x = r.x + static_cast<float>(i) * r.width / 10;
        DrawEllipse(static_cast<int>(x), static_cast<int>(r.y + r.height * .52f), r.width * .15f,
                    r.height * (.24f + .04f * std::sin(static_cast<float>(i))),
                    {176, 199, 156, 255});
    }
    DrawEllipse(static_cast<int>(r.x + r.width * .5f), static_cast<int>(r.y + r.height * .83f),
                r.width * .63f, r.height * .38f, {136, 183, 165, 255});
    for (int i = 0; i < 10; ++i) {
        const float x = r.x + r.width * (.12f + .08f * static_cast<float>(i));
        const float y = r.y + r.height * (.70f + .13f * std::sin(static_cast<float>(i) * 2));
        DrawEllipse(static_cast<int>(x), static_cast<int>(y), r.width * .05f, r.height * .018f,
                    {202, 221, 193, 255});
    }
    for (int i = 0; i < 18; ++i) {
        const float x = r.x + r.width * (i < 9 ? .03f + .022f * static_cast<float>(i)
                                               : .82f + .022f * static_cast<float>(i - 9));
        const float y = r.y + r.height;
        const float h = r.height * (.28f + .16f * (1 + std::sin(static_cast<float>(i) * 7)));
        DrawLineBezier({x, y}, {x + 12 * std::sin(static_cast<float>(i)), y - h}, 3,
                       {64, 111, 78, 255});
        DrawEllipse(static_cast<int>(x + 12 * std::sin(static_cast<float>(i))),
                    static_cast<int>(y - h), 4, 14, variant == 2 ? green : Color{99, 129, 69, 255});
    }
    if (variant == 1) {
        for (int i = 0; i < 12; ++i)
            DrawLineEx({r.x + 30 + static_cast<float>(i) * r.width / 12, r.y + 20},
                       {r.x + 23 + static_cast<float>(i) * r.width / 12, r.y + 40}, 2,
                       {128, 163, 164, 255});
    }
    EndScissorMode();
}

class Application {
  public:
    int run(int argc, char **argv);

  private:
    Ui ui;
    kids::Session session;
    std::unique_ptr<Renderer> renderer;
    RenderTexture2D canvas{}, view{}, before{};
    std::array<Texture2D, 4> previews{};
    Camera2D camera{};
    std::vector<Photo> photos;
    std::filesystem::path data;
    bool activity = false, notebook = false, extra = false, addMenu = false, landing = true;
    bool photoRequested = false, baselineRequested = false, quit = false;
    bool inFrame = false, pendingStart = false;
    int pendingChoice = 0, snapshotIndex = 0;
    int route = 0, choice = 0, notebookPage = 0, openPhoto = -1;
    float clock = 0, toastTime = 0;
    double tickMs = 0, renderMs = 0;
    std::string toast, error;
    Rectangle viewRect{250, 228, 804, 494};
    kids::Narrator narrator;
    void home();
    void sidebar();
    void chooser();
    void live();
    void notebookView();
    void photoView();
    void start(int selected);
    void renderView();
    void drawAnatomy();
    void drawTexture(Texture2D t, Rectangle target, Color tint = WHITE);
    void say(const std::string &s);
    void stopSpeech();
    void note(const std::string &s) {
        toast = s;
        toastTime = 4;
    }
    void save();
    void loadNotebook();
    void refreshNotebook();
    std::string narration() const;
};

void Application::drawTexture(Texture2D t, Rectangle r, Color tint) {
    if (t.id)
        DrawTexturePro(t, {0, 0, static_cast<float>(t.width), static_cast<float>(t.height)}, r,
                       {0, 0}, 0, tint);
}

void Application::stopSpeech() {
    narrator.stop();
}
void Application::say(const std::string &s) {
    if (narrator.speak(s))
        note("Reading aloud. Press Esc to stop.");
    else
        note("Read-aloud is unavailable. The same words are on screen.");
}

void Application::start(int selected) {
    if (inFrame) {
        pendingStart = true;
        pendingChoice = selected;
        return;
    }
    try {
        session.start(static_cast<kids::Experience>(route), selected);
        activity = true;
        baselineRequested = false;
        ui.focused = -1;
        landing = false;
        notebook = false;
        extra = false;
        addMenu = false;
        clock = 0;
        camera = {};
        camera.target = {session.world->width() / 2, session.world->height() / 2};
        camera.zoom = route == 3 ? 2.1f : 1.05f;
        camera.offset = {viewRect.width / 2, viewRect.height / 2};
        if (route == 2 || route == 4)
            camera.zoom = 2.0f;
        renderView();
        BeginTextureMode(before);
        DrawTextureRec(view.texture,
                       {0, 0, static_cast<float>(view.texture.width),
                        -static_cast<float>(view.texture.height)},
                       {0, 0}, WHITE);
        EndTextureMode();
    } catch (const std::exception &e) {
        error = e.what();
        note(error);
    }
}

void Application::sidebar() {
    DrawRectangle(0, 0, 218, 900, {239, 241, 229, 255});
    DrawLine(218, 0, 218, 900, line);
    DrawCircle(43, 49, 17, green);
    DrawCircleLines(43, 49, 10, gold);
    DrawLineEx({54, 60}, {63, 70}, 4, green);
    ui.text("MICROCOSM", 75, 29, 19);
    ui.text("K I D S", 75, 53, 16, muted);
    if (ui.button("Home", {22, 111, 174, 48}, false, landing && !notebook)) {
        activity = false;
        landing = true;
        notebook = false;
        stopSpeech();
    }
    ui.text("WAYS TO EXPLORE", 24, 202, 14, muted);
    for (int i = 0; i < 5; ++i) {
        const float y = 238 + static_cast<float>(i) * 82;
        const std::string label = i == 4 ? "Story time" : names[static_cast<std::size_t>(i)];
        if (ui.button(label, {22, y, 174, 55}, false, route == i && !notebook)) {
            route = i;
            choice = 0;
            activity = false;
            landing = false;
            notebook = false;
            extra = false;
            stopSpeech();
        }
    }
    if (ui.button("My notebook", {22, 713, 174, 50}, false, notebook)) {
        notebook = true;
        stopSpeech();
    }
    ui.wrap("Real science.\nRoom to wonder.", 25, 800, 170, 18, muted);
    ui.text("A computer model", 25, 864, 15, muted);
}

void Application::home() {
    ui.text("SMALL WORLD, BIG DISCOVERIES", 255, 44, 16, green);
    ui.text("What will you discover today?", 252, 77, 42);
    ui.text("Look closer. Try something. Follow your curiosity.", 255, 137, 23, muted);
    const Rectangle hero{254, 195, 1148, 262};
    habitatArt(hero, 0);
    ui.box({276, 218, 493, 216}, paper);
    ui.text("A whole world in a drop.", 301, 239, 32);
    ui.wrap("Visit a pond and meet the tiny living things inside it.", 301, 286, 430, 23);
    if (ui.button("Explore Pocket Pond", {301, 359, 310, 51}, true)) {
        route = 0;
        start(0);
    }
    ui.text("OR CHOOSE YOUR OWN ADVENTURE", 255, 492, 16, muted);
    for (int i = 1; i < 5; ++i) {
        const int col = (i - 1) % 2, row = (i - 1) / 2;
        const float x = 254 + static_cast<float>(col) * 586,
                    y = 535 + static_cast<float>(row) * 157;
        ui.box({x, y, 562, 135}, i == 2 ? pale : white);
        DrawCircle(static_cast<int>(x + 42), static_cast<int>(y + 43), 22,
                   i == 1 ? gold : (i == 3 ? Color{217, 232, 235, 255} : pale));
        ui.text(std::to_string(i + 1), x + 35, y + 30, 24);
        ui.text(names[static_cast<std::size_t>(i)], x + 79, y + 24, 27);
        ui.text(verbs[static_cast<std::size_t>(i)], x + 79, y + 67, 21, muted);
        if (ui.button("Go", {x + 453, y + 70, 85, 44})) {
            route = i;
            choice = 0;
            activity = false;
            landing = false;
        }
    }
}

void Application::chooser() {
    ui.text("CHOOSE YOUR ADVENTURE", 255, 43, 16, green);
    ui.text(names[static_cast<std::size_t>(route)], 252, 79, 46);
    ui.text(subtitles[static_cast<std::size_t>(route)], 255, 143, 24, muted);
    if (route == 0) {
        const char *labels[] = {"Pond", "Puddle", "Moss"};
        const char *desc[] = {"Still water. Lots to discover.", "A little world after the rain.",
                              "A wet home among tiny leaves."};
        for (int i = 0; i < 3; ++i) {
            const float x = 254 + static_cast<float>(i) * 389;
            habitatArt({x, 220, 367, 304}, i);
            ui.text(labels[i], x + 3, 552, 33);
            ui.wrap(desc[i], x + 3, 602, 347, 23);
            if (ui.button(std::string("Explore ") + labels[i], {x, 700, 367, 58}, true))
                start(i);
        }
        ui.text("A small, prepared sample of life from each habitat.", 255, 824, 19, muted);
    } else if (route == 2) {
        for (int i = 0; i < 4; ++i) {
            const float x = 254 + static_cast<float>(i % 2) * 586,
                        y = 212 + static_cast<float>(i / 2) * 285;
            ui.box({x, y, 562, 258}, white);
            drawTexture(previews[static_cast<std::size_t>(i)], {x + 16, y + 18, 218, 173});
            const auto &c = kids::creatures()[static_cast<std::size_t>(i)];
            ui.text(c.name, x + 254, y + 28, 28);
            ui.wrap(c.title, x + 254, y + 76, 275, 23);
            if (ui.button(std::string("Meet ") + c.name, {x + 254, y + 170, 280, 58}, true))
                start(i);
        }
        ui.text("These are simulated living things, not photographs.", 255, 824, 19, muted);
    } else {
        const char *heading = route == 1   ? "Who swims with tiny hairs?"
                              : route == 3 ? "What if we add salt?"
                                           : "A morning in one tiny drop";
        const char *body = route == 1 ? "Meet two swimmers. Look at how they move, then see if you "
                                        "can spot the one with tiny hairs."
                           : route == 3 ? "Look at onion cells. Make a guess, add salt to their "
                                          "water, and watch the cell contents."
                                        : "A drop of pond water is full of life. Follow a swimmer "
                                          "and find out how one tiny cell gets around.";
        ui.box({254, 218, 1148, 515}, white);
        if (route == 4)
            habitatArt({278, 242, 500, 466}, 0);
        else
            drawTexture(previews[route == 3 ? 1 : 0], {278, 242, 500, 466});
        ui.text(route == 1   ? "CASE 01"
                : route == 3 ? "TRY IT AND SEE"
                             : "LISTEN AND LOOK",
                821, 253, 16, green);
        ui.wrap(heading, 817, 303, 530, 38, ink);
        ui.wrap(body, 821, 420, 510, 25);
        if (ui.button("Let's begin", {821, 620, 270, 60}, true))
            start(0);
        if (ui.button("Hear it", {1110, 620, 230, 60}))
            say(body);
        ui.text("No score. No rush. You can always try again.", 255, 793, 23, muted);
    }
}

std::string Application::narration() const {
    if (route == 3)
        return session.observation();
    if (route == 1 && session.answer >= 0)
        return session.answer == 0 ? "Paramecium uses cilia around its edge. Euglena uses a longer "
                                     "thread called a flagellum."
                                   : "Euglena uses a long thread. Look at Paramecium's edge for "
                                     "many tiny hair-like parts.";
    if (route == 1)
        return "Which swimmer has tiny hairs called cilia? Look closely at both. Switch on See the "
               "parts if you need a clue.";
    if (route == 4) {
        const char *story[] = {"A pond can hold many tiny living things. Follow one swimmer. It is "
                               "a single cell, called Paramecium.",
                               "Hair-like parts called cilia beat around its edge. They help it "
                               "swim and sweep bacteria towards its feeding groove.",
                               "Food goes into little spaces called food vacuoles. This one cell "
                               "moves, eats and grows. What else can you find in the drop?"};
        return story[std::clamp(session.chapter, 0, 2)];
    }
    const int i = session.selectedCreature();
    if (i == 1 && !extra) {
        if (session.wrappingFood())
            return "The amoeba is wrapping around a smaller cell. When it closes around it, the "
                   "food goes inside.";
        if (session.meals() > 0)
            return "The amoeba has taken in a smaller cell. It holds food in a food vacuole while "
                   "digesting it.";
    }
    if (i >= 0)
        return extra ? kids::creatures()[static_cast<std::size_t>(i)].detail
                     : kids::creatures()[static_cast<std::size_t>(i)].explanation;
    if (session.selected.kind == Picked::Kind::Cell)
        return "You found a smaller living cell. The little green algae in this sample use light "
               "to make food.";
    if (session.selected)
        return "You found something in the drop. Some tiny specks are bacteria; others are bits of "
               "material. Tap a larger cell to meet it.";
    return "Tap a living thing in the microscope. You can pause it first if it moves too quickly.";
}

void Application::live() {
    const bool compareLocked = route == 3 && session.predictionMade;
    ui.text(names[static_cast<std::size_t>(route)], 254, 35, 18, green);
    const int current = session.selectedCreature();
    const std::string title =
        route == 3   ? "What happens in salty water?"
        : route == 1 ? "Who swims with tiny hairs?"
        : route == 4 ? "A morning in one tiny drop"
        : current >= 0
            ? std::string("Meet ") + kids::creatures()[static_cast<std::size_t>(current)].name
            : "Life in a drop";
    ui.text(title, 252, 72, 39);
    ui.text(route == 3   ? "Look at the contents, and the walls around them."
            : route == 4 ? "Follow Paramecium. Zoom or pause to look closer."
            : route == 1 ? "Look at both swimmers. Press Play to watch them move."
                         : "Tap to discover. Scroll to look closer. Right-drag to move around.",
            255, 129, 22, muted);
    if (ui.button("Start over", {1230, 40, 173, 48}))
        start(session.choice);
    if (ui.button("Microscope", {254, 174, 180, 43}, false, !session.modelView, !compareLocked))
        session.modelView = false;
    if (ui.button("See the parts", {444, 174, 194, 43}, false, session.modelView, !compareLocked))
        session.modelView = true;
    ui.text(compareLocked       ? "Comparison view is fixed. Start over to change it."
            : session.modelView ? "Diagram view: some parts are enlarged for learning."
                                : "Simulated microscope view",
            655, 187, 18, muted);
    DrawTexturePro(
        view.texture,
        {0, 0, static_cast<float>(view.texture.width), -static_cast<float>(view.texture.height)},
        viewRect, {0, 0}, 0, WHITE);
    DrawRectangleRoundedLinesEx(viewRect, .02f, 8, 2, line);
    if (!compareLocked)
        drawAnatomy();
    if (route == 3 && session.predictionMade) {
        const float half = viewRect.width / 2;
        const Rectangle crop{(static_cast<float>(view.texture.width) - half) / 2, 0, half,
                             -static_cast<float>(view.texture.height)};
        DrawTexturePro(before.texture, crop, {viewRect.x, viewRect.y, half, viewRect.height},
                       {0, 0}, 0, WHITE);
        DrawTexturePro(view.texture, crop, {viewRect.x + half, viewRect.y, half, viewRect.height},
                       {0, 0}, 0, WHITE);
        DrawRectangle(static_cast<int>(viewRect.x + half - 2), static_cast<int>(viewRect.y), 4,
                      static_cast<int>(viewRect.height), paper);
        ui.box({viewRect.x + 14, viewRect.y + 14, 100, 35}, paper);
        ui.text("Before", viewRect.x + 26, viewRect.y + 23, 20);
        ui.box({viewRect.x + half + 14, viewRect.y + 14, 116, 35}, paper);
        ui.text("Live now", viewRect.x + half + 26, viewRect.y + 23, 20);
    }
    ui.box(
        {viewRect.x + 14, viewRect.y + viewRect.height - 47, session.paused ? 120.0f : 102.0f, 31},
        paper);
    DrawCircle(static_cast<int>(viewRect.x + 31),
               static_cast<int>(viewRect.y + viewRect.height - 31), 4,
               session.paused ? coral : green);
    ui.text(session.paused ? "Paused" : "Live", viewRect.x + 45, viewRect.y + viewRect.height - 41,
            20);
    // La barra representa distancias del mundo, no un aumento de objetivo inexistente.
    const float desired = 100.0f / camera.zoom;
    const float magnitude = std::pow(10.0f, std::floor(std::log10(desired)));
    const float normalized = desired / magnitude;
    const float microns = (normalized >= 5 ? 5.0f : normalized >= 2 ? 2.0f : 1.0f) * magnitude;
    const float px = microns * camera.zoom;
    const float sx = viewRect.x + viewRect.width - px - 24, sy = viewRect.y + viewRect.height - 28;
    DrawLineEx({sx, sy}, {sx + px, sy}, 3, ink);
    ui.text(std::to_string(static_cast<int>(microns)) + " um", sx, sy - 24, 17, ink);
    if (route == 1 && current >= 0) {
        const std::string picked =
            std::string("Looking at ") + kids::creatures()[static_cast<std::size_t>(current)].name;
        const float width = MeasureTextEx(ui.font, picked.c_str(), 18, 0).x + 24;
        ui.box({viewRect.x + 14, viewRect.y + viewRect.height - 88, width, 33}, paper);
        ui.text(picked, viewRect.x + 26, viewRect.y + viewRect.height - 79, 18);
    }
    const float x = 1080;
    ui.text(route == 1   ? "YOUR MYSTERY"
            : route == 3 ? "MAKE A DISCOVERY"
            : route == 4 ? "STORY TIME"
                         : "LOOK CLOSER",
            x, 235, 15, green);
    float yy = 277;
    if (route != 1 && route != 3 && route != 4) {
        yy += ui.wrap(current >= 0 ? kids::creatures()[static_cast<std::size_t>(current)].title
                                   : "What can you find?",
                      x, yy, 315, 29, ink) +
              20;
    }
    ui.wrap(narration(), x, yy, 315, 24, ink);
    yy = 520;
    if (ui.button("Hear it", {x, yy, 148, 47})) {
        std::string spoken = narration();
        if (route == 3 && !session.predictionMade)
            spoken += " What will happen to the contents: smaller, the same, or not sure?";
        if (route == 1 && session.answer < 0)
            spoken += " Do you think it is Paramecium or Euglena?";
        say(spoken);
    }
    if (ui.button("Stop", {x + 162, yy, 148, 47}))
        stopSpeech();
    yy += 70;
    if (route == 1) {
        if (session.answer < 0) {
            ui.wrap("Which one do you think?", x, yy, 310, 22);
            yy += 63;
            if (ui.button("Paramecium", {x, yy, 310, 48}))
                session.answer = 0;
            if (ui.button("Euglena", {x, yy + 58, 310, 48}))
                session.answer = 2;
        } else {
            ui.wrap("Compare the two swimmers again whenever you like.", x, yy, 310, 22, green);
            if (ui.button("Look again", {x, 748, 310, 48}))
                session.answer = -1;
        }
    } else if (route == 3) {
        if (!session.predictionMade) {
            ui.wrap("What will happen to the contents?", x, yy, 310, 22);
            yy += 70;
            if (ui.button("Smaller", {x, yy, 98, 47}, false, session.prediction == 0))
                session.prediction = 0;
            if (ui.button("Same", {x + 106, yy, 98, 47}, false, session.prediction == 1))
                session.prediction = 1;
            if (ui.button("Not sure", {x + 212, yy, 98, 47}, false, session.prediction == 2))
                session.prediction = 2;
            if (ui.button("Try salty water", {x, yy + 62, 310, 52}, true)) {
                if (session.prediction < 0)
                    note("Make a guess first. Not sure is a good answer too.");
                else {
                    baselineRequested = true;
                    session.predictionMade = true;
                    session.follow = false;
                    session.paused = false;
                }
            }
        } else {
            ui.text(session.changed() ? "A change we can measure" : "Watch patiently", x, yy, 23,
                    green);
            ui.wrap("Water crosses a thin layer called a membrane. The cell wall stays stiff.", x,
                    yy + 42, 310, 22);
            if (ui.button(session.salty ? "Back to fresh water" : "Start a new experiment",
                          {x, 748, 310, 52}, true)) {
                if (session.salty)
                    session.setSalt(false);
                else
                    start(0);
            }
        }
    } else if (route == 4) {
        ui.text("Chapter " + std::to_string(session.chapter + 1) + " of 3", x, 600, 21, muted);
        if (ui.button(session.chapter < 2 ? "Next discovery" : "Explore on my own",
                      {x, 642, 310, 53}, true)) {
            if (session.chapter < 2)
                ++session.chapter;
            else {
                route = 0;
                session.experience = kids::Experience::Pond;
                extra = false;
            }
        }
    } else {
        if (ui.button(extra ? "A little less" : "Tell me more", {x, yy, 310, 48}))
            extra = !extra;
        if (current >= 0)
            ui.wrap(kids::creatures()[static_cast<std::size_t>(current)].question, x, yy + 77, 310,
                    22, muted);
    }
    const float by = 748;
    if (ui.button(session.paused ? "Play" : "Pause", {254, by, 130, 53}, session.paused))
        session.paused = !session.paused;
    const bool cameraLocked = route == 3 && session.predictionMade;
    if (cameraLocked)
        ui.text("Same view for a fair comparison", 399, by + 18, 21, muted);
    if (!cameraLocked && ui.button("-", {399, by, 53, 53}))
        camera.zoom = std::max(.35f, camera.zoom / 1.25f);
    if (!cameraLocked)
        ui.text("Zoom", 465, by + 17, 20, muted);
    if (!cameraLocked && ui.button("+", {526, by, 53, 53}))
        camera.zoom = std::min(9.0f, camera.zoom * 1.25f);
    if (!cameraLocked && ui.button(session.follow ? "Following" : "Follow a cell",
                                   {594, by, 185, 53}, false, session.follow)) {
        if (!session.selected)
            session.focusFirst();
        session.follow = !session.follow;
    }
    if (ui.button("Save photo", {794, by, 170, 53}, true))
        photoRequested = true;
    if (route == 0 && ui.button("Add life", {979, by, 150, 53}, false, addMenu))
        addMenu = !addMenu;
    if (route == 2 && session.choice == 1 &&
        ui.button(session.foodOffers < 3 ? "Offer algal food" : "Food added", {1080, by, 310, 53},
                  true, false, session.foodOffers < 3)) {
        if (session.offerFood())
            note("Small algae added beside the amoeba. Watch what it does.");
    }
    if (addMenu) {
        ui.box({734, 465, 306, 269}, paper);
        for (int i = 0; i < 4; ++i)
            if (ui.button(kids::creatures()[static_cast<std::size_t>(i)].name,
                          {750, 482 + static_cast<float>(i) * 59, 274, 48})) {
                session.addCreature(i, {camera.target.x, camera.target.y});
                addMenu = false;
            }
    }
    ui.text("Growth and digestion are sped up (60x); swimming is not.", 254, 835, 18, muted);
    ui.text("Prepared sample  /  computer simulation", 254, 866, 16, muted);
    if (!ui.used && !addMenu && CheckCollisionPointRec(ui.mouse, viewRect)) {
        if (ui.click) {
            const Vector2 world =
                GetScreenToWorld2D({ui.mouse.x - viewRect.x, ui.mouse.y - viewRect.y}, camera);
            if (route != 4)
                session.select({world.x, world.y}, 10 / camera.zoom);
            else {
                session.focusFirst();
                session.follow = true;
            }
            extra = false;
        }
        const float wheel = GetMouseWheelMove();
        if (!cameraLocked && wheel != 0)
            camera.zoom = std::clamp(camera.zoom * std::pow(1.15f, wheel), .35f, 9.0f);
        if (!cameraLocked &&
            (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))) {
            const Vector2 delta = GetMouseDelta();
            camera.target.x -= delta.x / ui.scale / camera.zoom;
            camera.target.y -= delta.y / ui.scale / camera.zoom;
            session.follow = false;
        }
    }
    if (IsKeyPressed(KEY_SPACE))
        session.paused = !session.paused;
    if (!cameraLocked && IsKeyPressed(KEY_EQUAL))
        camera.zoom = std::min(9.0f, camera.zoom * 1.25f);
    if (!cameraLocked && IsKeyPressed(KEY_MINUS))
        camera.zoom = std::max(.35f, camera.zoom / 1.25f);
}

void Application::drawAnatomy() {
    if (!session.world || !session.modelView || session.selectedCreature() < 0)
        return;
    const auto &reg = session.world->registry();
    const auto *body = reg.try_get<RingBody>(session.selected.entity);
    const auto *anatomy = reg.try_get<CellAnatomy>(session.selected.entity);
    if (!body || !anatomy)
        return;
    const auto &bp = session.world->bodies();
    auto label = [&](Vec2 world, const char *text, bool right) {
        Vector2 point = GetWorldToScreen2D({world.x, world.y}, camera);
        point.x += viewRect.x;
        point.y += viewRect.y;
        if (!CheckCollisionPointRec(point, viewRect))
            return;
        const float width = MeasureTextEx(ui.font, text, 18, 0).x + 24;
        const Rectangle box{right ? viewRect.x + viewRect.width - width - 16 : viewRect.x + 16,
                            viewRect.y + 16, width, 35};
        DrawLineEx(point, {box.x + box.width / 2, box.y + box.height}, 2, green);
        DrawCircleV(point, 4, gold);
        ui.box(box, paper);
        ui.text(text, box.x + 12, box.y + 9, 18, ink);
    };
    auto rightDistance = [&](std::uint32_t slot) {
        const auto at = GetWorldToScreen2D({bp.x[slot], bp.y[slot]}, camera);
        const float dx = at.x - (viewRect.width - 100), dy = at.y - 35;
        return dx * dx + dy * dy;
    };
    const int creature = session.selectedCreature();
    if (creature == 3) {
        if (!body->ring.empty()) {
            auto slot = body->ring.front();
            for (auto candidate : body->ring)
                if (rightDistance(candidate) < rightDistance(slot))
                    slot = candidate;
            label({bp.x[slot], bp.y[slot]}, "A colony of cells", true);
        }
        return;
    }
    if (anatomy->nucleus < bp.capacity() && bp.alive[anatomy->nucleus])
        label({bp.x[anatomy->nucleus], bp.y[anatomy->nucleus]}, "Nucleus", false);
    if (creature == 2) {
        std::uint32_t nearest = ~0u;
        for (auto slot : body->interior)
            if (bp.alive[slot] &&
                bp.tag[slot] == static_cast<std::uint8_t>(OrganelleTag::Chloroplast))
                if (nearest == ~0u || rightDistance(slot) < rightDistance(nearest))
                    nearest = slot;
        if (nearest != ~0u)
            label({bp.x[nearest], bp.y[nearest]}, "Chloroplast: uses light", true);
    } else if (!body->ring.empty()) {
        auto slot = body->ring.front();
        for (auto candidate : body->ring)
            if (rightDistance(candidate) < rightDistance(slot))
                slot = candidate;
        label({bp.x[slot], bp.y[slot]}, creature == 0 ? "Cilia: hair-like parts" : "Cell boundary",
              true);
    }
}

void Application::renderView() {
    if (!session.world)
        return;
    if (session.follow) {
        Vec2 pos;
        if (session.selectedPosition(pos))
            camera.target = {pos.x, pos.y};
        else {
            session.follow = false;
            session.selected = {};
        }
    }
    renderer->setViewMode(session.modelView ? ViewMode::Model : ViewMode::Microscope);
    renderer->setInspection(session.selected);
    renderer->drawWorldTo(*session.world, camera, Overlay::None, view);
}

void Application::save() {
    if (!session.world)
        return;
    std::error_code storageError;
    std::filesystem::create_directories(data / "photos", storageError);
    if (storageError) {
        note("The photo folder could not be opened.");
        return;
    }
    const std::string file = "discovery-" +
                             std::to_string(static_cast<long long>(
                                 std::chrono::system_clock::now().time_since_epoch().count())) +
                             ".png";
    const int i = session.selectedCreature();
    const std::string label = i >= 0       ? kids::creatures()[static_cast<std::size_t>(i)].name
                              : route == 3 ? "Onion cells"
                                           : "Life in a drop";
    std::string caption;
    if (route == 3)
        caption = session.salty
                      ? (session.changed() ? "Onion cell contents became smaller after adding salt."
                                           : "Salt was added to the water around the onion cells.")
                  : session.predictionMade ? "Onion cells after returning to fresh water."
                                           : "Onion cells in fresh water.";
    else if (i >= 0)
        caption = kids::creatures()[static_cast<std::size_t>(i)].explanation;
    else if (route == 1)
        caption = "Paramecium and Euglena in a prepared sample.";
    else
        caption = std::string("A prepared ") +
                  biome(static_cast<BiomeId>(session.plate.biome)).key + " sample.";
    caption += session.modelView ? " Diagram view." : " Simulated microscope view.";
    PhotoInfo info;
    info.plate =
        route == 3 ? "onion_epidermis" : biome(static_cast<BiomeId>(session.plate.biome)).key;
    info.seed = session.plate.seed;
    info.labelFont = ui.font;
    info.showRunStamp = false;
    info.provenance = std::string(BIO_VERSION) + " " + BIO_GIT +
                      (session.modelView ? "; diagram view" : "; simulated microscope view");
    info.lessonOrScene = names[static_cast<std::size_t>(route)];
    if (savePhoto(*renderer, *session.world, camera, Overlay::None, 804, 494,
                  (data / "photos" / file).string(), info)) {
        Photo p{file, label, caption, {}};
        photos.push_back(std::move(p));
        std::ofstream out(data / "notebook.tsv", std::ios::app);
        out << file << '\t' << label << '\t' << caption << '\n';
        out.flush();
        if (out.good())
            note("Saved to My notebook.");
        else
            note("Photo saved, but the notebook could not be updated.");
    } else
        note("The photo could not be saved. Check your storage space.");
}
void Application::loadNotebook() {
    std::ifstream in(data / "notebook.tsv");
    std::string row;
    while (std::getline(in, row)) {
        const auto a = row.find('\t'), b = a == std::string::npos ? a : row.find('\t', a + 1);
        if (a == std::string::npos || b == std::string::npos)
            continue;
        Photo p{row.substr(0, a), row.substr(a + 1, b - a - 1), row.substr(b + 1), {}};
        if (p.file.find('/') != std::string::npos || p.file.find("..") != std::string::npos)
            continue;
        if (std::filesystem::exists(data / "photos" / p.file)) {
            photos.push_back(std::move(p));
        }
    }
}
void Application::refreshNotebook() {
    const int pages = std::max(1, (static_cast<int>(photos.size()) + 3) / 4);
    notebookPage = std::clamp(notebookPage, 0, pages - 1);
    for (std::size_t i = 0; i < photos.size(); ++i) {
        auto &photo = photos[i];
        const bool visible = static_cast<int>(i) / 4 == notebookPage;
        if (visible && !photo.texture.id)
            photo.texture = LoadTexture((data / "photos" / photo.file).string().c_str());
        else if (!visible && photo.texture.id) {
            UnloadTexture(photo.texture);
            photo.texture = {};
        }
    }
}

void Application::notebookView() {
    ui.text("MY FIELD NOTEBOOK", 255, 44, 16, green);
    ui.text("Little things you noticed.", 252, 80, 44);
    ui.text("Your photos stay on this computer.", 255, 145, 23, muted);
    if (photos.empty()) {
        habitatArt({254, 220, 1148, 390}, 0);
        ui.box({535, 280, 590, 225}, paper);
        ui.text("Your first discovery starts here.", 561, 308, 31);
        ui.wrap("Meet a living thing, then choose Save photo. You can come back to it here.", 561,
                365, 530, 24);
        if (ui.button("Meet something tiny", {561, 440, 390, 48}, true)) {
            route = 2;
            activity = false;
            landing = false;
            notebook = false;
        }
    } else {
        const int pages = (static_cast<int>(photos.size()) + 3) / 4;
        notebookPage = std::clamp(notebookPage, 0, pages - 1);
        for (int n = 0; n < 4; ++n) {
            const int i = notebookPage * 4 + n;
            if (i >= static_cast<int>(photos.size()))
                break;
            const auto &p = photos[static_cast<std::size_t>(i)];
            const float x = 254 + static_cast<float>(n % 2) * 586,
                        y = 216 + static_cast<float>(n / 2) * 290;
            drawTexture(p.texture, {x, y, 280, 173});
            ui.text(p.name, x + 301, y + 12, 25);
            ui.wrap(p.caption, x + 301, y + 58, 252, 20);
            ui.text("Discovery " + std::to_string(i + 1), x, y + 193, 18, muted);
            const bool overPhoto = CheckCollisionPointRec(ui.mouse, {x, y, 280, 173});
            if (overPhoto)
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            const bool imageClick = overPhoto && ui.click && !ui.used;
            if (ui.button("Open photo", {x, y + 226, 280, 43}) || imageClick) {
                openPhoto = i;
                ui.used = true;
                ui.focused = -1;
            }
        }
        if (ui.button("Previous", {254, 810, 150, 48}, false, false, notebookPage > 0))
            --notebookPage;
        ui.text(std::to_string(notebookPage + 1) + " / " + std::to_string(pages), 440, 826, 21);
        if (ui.button("Next", {540, 810, 150, 48}, false, false, notebookPage + 1 < pages))
            ++notebookPage;
    }
}

void Application::photoView() {
    if (openPhoto < 0 || openPhoto >= static_cast<int>(photos.size())) {
        openPhoto = -1;
        return;
    }
    const auto &p = photos[static_cast<std::size_t>(openPhoto)];
    ui.text("MY FIELD NOTEBOOK", 50, 32, 16, green);
    ui.text(p.name, 48, 72, 42);
    if (ui.button("Back to notebook", {1170, 55, 220, 53})) {
        openPhoto = -1;
        stopSpeech();
    }
    drawTexture(p.texture, {50, 160, 1000, 614});
    ui.wrap(p.caption, 1080, 170, 310, 24, ink);
    if (ui.button("Hear it", {1080, 470, 148, 47}))
        say(p.caption);
    if (ui.button("Stop", {1242, 470, 148, 47}))
        stopSpeech();
    ui.text("Your saved discovery. The photo stays on this computer.", 50, 825, 21, muted);
}

int Application::run(int argc, char **argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "Open Microcosm Kids");
    SetWindowMinSize(1024, 640);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    if (Image icon = LoadImage(assetPath("kids/icon.png").c_str()); icon.data) {
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
    omp_set_num_threads(1);
    ui.font = LoadFontEx(assetPath("fonts/Roboto-Medium.ttf").c_str(), 64, nullptr, 0);
    SetTextureFilter(ui.font.texture, TEXTURE_FILTER_BILINEAR);
    canvas = LoadRenderTexture(1440, 900);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
    view = LoadRenderTexture(804, 494);
    before = LoadRenderTexture(804, 494);
    renderer = std::make_unique<Renderer>();
    renderer->setOpticFlaws(false);
    renderer->setPostProcess(true);
#if defined(_WIN32)
    const char *root = std::getenv("APPDATA");
    data = std::filesystem::path(root ? root : ".") / "open-microcosm-kids";
#elif defined(__APPLE__)
    const char *root = std::getenv("HOME");
    data = std::filesystem::path(root ? root : ".") /
           "Library/Application Support/open-microcosm-kids";
#else
    if (const char *xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg)
        data = std::filesystem::path(xdg) / "open-microcosm-kids";
    else {
        const char *root = std::getenv("HOME");
        data = std::filesystem::path(root ? root : ".") / ".local/share/open-microcosm-kids";
    }
#endif
    if (const char *overrideDir = std::getenv("MICROCOSM_KIDS_DATA"))
        data = overrideDir;
    std::filesystem::create_directories(data);
    loadNotebook();
    const char *habitats[] = {"pond", "puddle", "moss"};
    for (int i = 0; i < 3; ++i) {
        const std::string file = assetPath(std::string("kids/") + habitats[i] + ".png");
        if (FileExists(file.c_str())) {
            habitatTextures[static_cast<std::size_t>(i)] = LoadTexture(file.c_str());
            SetTextureFilter(habitatTextures[static_cast<std::size_t>(i)], TEXTURE_FILTER_BILINEAR);
        }
    }
    for (int i = 0; i < 4; ++i) {
        session.start(kids::Experience::Safari, i);
        camera = {};
        camera.zoom = i == 3 ? .65f : 1.4f;
        camera.offset = {402, 247};
        Vec2 pos;
        if (session.selectedPosition(pos))
            camera.target = {pos.x, pos.y};
        else
            camera.target = {session.world->width() / 2, session.world->height() / 2};
        renderView();
        Image image = LoadImageFromTexture(view.texture);
        ImageFlipVertical(&image);
        previews[static_cast<std::size_t>(i)] = LoadTextureFromImage(image);
        UnloadImage(image);
    }
    int screenshotFrames = 0;
    std::string screenshot;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--experience" && i + 1 < argc) {
            route = std::clamp(std::atoi(argv[++i]), 0, 4);
            start(0);
        } else if (arg == "--screenshot" && i + 1 < argc) {
            screenshot = argv[++i];
            screenshotFrames = 90;
        }
    }
    while (!WindowShouldClose() && !quit) {
        ui.scale = std::min(static_cast<float>(GetScreenWidth()) / W,
                            static_cast<float>(GetScreenHeight()) / H);
        ui.origin = {(GetScreenWidth() - W * ui.scale) / 2, (GetScreenHeight() - H * ui.scale) / 2};
        ui.begin();
        const float dt = GetFrameTime();
        clock += dt;
        toastTime -= dt;
        if (pendingStart) {
            pendingStart = false;
            start(pendingChoice);
        }

        if (baselineRequested) {
            renderView();
            BeginTextureMode(before);
            DrawTextureRec(view.texture, {0, 0, 804, -494}, {0, 0}, WHITE);
            EndTextureMode();
            baselineRequested = false;
            if (!session.setSalt(true)) {
                session.predictionMade = false;
                note("The water could not be changed. Please start over.");
            }
        }
        const double tickStart = GetTime();
        if (activity && !notebook)
            session.tick(dt);
        tickMs = (GetTime() - tickStart) * 1000;
        const double renderStart = GetTime();
        if (activity && !notebook)
            renderView();
        renderMs = (GetTime() - renderStart) * 1000;
        if (photoRequested) {
            save();
            photoRequested = false;
        }
        if (notebook)
            refreshNotebook();
        if (IsKeyPressed(KEY_ESCAPE)) {
            stopSpeech();
            addMenu = false;
            openPhoto = -1;
        }
        inFrame = true;
        BeginTextureMode(canvas);
        ClearBackground(paper);
        if (notebook && openPhoto >= 0)
            photoView();
        else {
            sidebar();
            if (notebook)
                notebookView();
            else if (activity)
                live();
            else if (landing)
                home();
            else
                chooser();
        }
        if (toastTime > 0) {
            ui.box({480, 15, 680, 44}, ink);
            ui.text(toast, 500, 27, 20, white);
        }
        ui.end();
        EndTextureMode();
        inFrame = false;
        BeginDrawing();
        ClearBackground(paper);
        DrawTexturePro(canvas.texture, {0, 0, W, -H},
                       {ui.origin.x, ui.origin.y, W * ui.scale, H * ui.scale}, {0, 0}, 0, WHITE);
        if (IsKeyPressed(KEY_F10)) {
            rlDrawRenderBatchActive();
            std::filesystem::path folder = data / "screenshots";
            if (const char *qa = std::getenv("MICROCOSM_KIDS_QA_DIR"))
                folder = qa;
            std::filesystem::create_directories(folder);
            const std::string stem =
                "ui-" +
                std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "-" +
                std::to_string(++snapshotIndex);
            Image shot = LoadImageFromScreen();
            ExportImage(shot, (folder / (stem + ".png")).string().c_str());
            UnloadImage(shot);
            std::ofstream out(folder / (stem + ".json"));
            out << "{\"route\":" << route << ",\"activity\":" << activity
                << ",\"landing\":" << landing << ",\"open_photo\":" << openPhoto
                << ",\"notebook\":" << notebook << ",\"paused\":" << session.paused
                << ",\"diagram\":" << session.modelView << ",\"follow\":" << session.follow
                << ",\"photos\":" << photos.size() << ",\"selected\":" << session.selectedCreature()
                << ",\"food_offers\":" << session.foodOffers << ",\"meals\":" << session.meals()
                << ",\"chapter\":" << session.chapter << ",\"prediction\":" << session.prediction
                << ",\"answer\":" << session.answer << ",\"salty\":" << session.salty
                << ",\"changed\":" << session.changed() << ",\"volume\":" << session.meanVolume()
                << ",\"elapsed\":" << session.elapsed << ",\"frame_ms\":" << GetFrameTime() * 1000
                << ",\"tick_ms\":" << tickMs << ",\"render_ms\":" << renderMs
                << ",\"narration\":" << std::quoted(narration()) << "}\n";
        }
        if (screenshotFrames > 0 && --screenshotFrames == 0) {
            rlDrawRenderBatchActive();
            Image shot = LoadImageFromScreen();
            ExportImage(shot, screenshot.c_str());
            UnloadImage(shot);
            quit = true;
        }
        EndDrawing();
    }
    stopSpeech();
    for (auto &p : photos)
        if (p.texture.id)
            UnloadTexture(p.texture);
    for (auto t : previews)
        if (t.id)
            UnloadTexture(t);
    for (auto t : habitatTextures)
        if (t.id)
            UnloadTexture(t);
    session.world.reset();
    renderer.reset();
    UnloadRenderTexture(view);
    UnloadRenderTexture(before);
    UnloadRenderTexture(canvas);
    UnloadFont(ui.font);
    CloseWindow();
    return 0;
}
} // namespace
int main(int argc, char **argv) {
    Application app;
    return app.run(argc, argv);
}
