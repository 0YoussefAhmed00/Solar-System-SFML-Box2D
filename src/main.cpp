#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include <vector>
#include <random>
#include <memory>
#include <cmath>
#include <iostream>

using namespace std;
using namespace sf;

constexpr float PIXELS_PER_METER = 30.f;
constexpr float INV_PPM = 1.0f / PIXELS_PER_METER;

static mt19937 rng((unsigned)time(nullptr));
static uniform_real_distribution<float> randRadius(6.f, 18.f);
static uniform_real_distribution<float> randSpeed(0.6f, 0.9f);
static uniform_real_distribution<float> randSign(0.0f, 1.0f);

class Planet;

class Sun {
public:
    b2Body* body = nullptr;
    float radius_px;
    CircleShape shape;
    Sprite sprite;

    Sun(b2World& world, const Vector2f& pos_px, float r_px = 60.f)
        : radius_px(r_px)
    {
        shape.setRadius(radius_px);
        shape.setOrigin(radius_px, radius_px);
        shape.setFillColor(Color::Yellow);
        shape.setPosition(pos_px);

        b2BodyDef bd;
        bd.type = b2_staticBody;
        bd.position.Set(pos_px.x * INV_PPM, pos_px.y * INV_PPM);
        body = world.CreateBody(&bd);

        b2CircleShape circ;
        circ.m_radius = radius_px * INV_PPM;

        b2FixtureDef fd;
        fd.shape = &circ;
        fd.density = 1.0f;
        body->CreateFixture(&fd);
    }

    void applyTexture(const Texture& tex) {
        sprite.setTexture(tex);
        auto ts = tex.getSize();
        sprite.setOrigin(ts.x * 0.5f, ts.y * 0.5f);

        float s = (radius_px * 2.0f) / (float)ts.x;
        sprite.setScale(s, s);
        sprite.setPosition(shape.getPosition());
    }
};

class Planet {
public:
    b2Body* body = nullptr;
    float radius_px = 10.f;

    Sprite sprite;
    CircleShape colShape;

    CircleShape orbitRing;
    float orbitRadius_px = 0.f;
    Vector2f orbitCenter_px{};

    Planet(b2World& world, Sun& sun, const Vector2f& spawn_px, const Texture& tex)
    {
        radius_px = randRadius(rng);

        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x * 0.5f, tex.getSize().y * 0.5f);

        float scale = (radius_px * 2) / (float)tex.getSize().x;
        sprite.setScale(scale, scale);
        sprite.setPosition(spawn_px);

        colShape.setRadius(radius_px);
        colShape.setOrigin(radius_px, radius_px);
        colShape.setFillColor(Color::Transparent);
        colShape.setPosition(spawn_px);

        b2BodyDef bd;
        bd.type = b2_dynamicBody;
        bd.position.Set(spawn_px.x * INV_PPM, spawn_px.y * INV_PPM);
        bd.fixedRotation = false;
        body = world.CreateBody(&bd);

        b2CircleShape circle;
        circle.m_radius = radius_px * INV_PPM;

        b2FixtureDef fd;
        fd.shape = &circle;
        fd.density = 0.5f;
        fd.friction = 0.1f;
        fd.restitution = 0.0f;
        body->CreateFixture(&fd);

        body->SetGravityScale(0.0f);

      
        b2Vec2 sunPos = sun.body->GetPosition();
        b2Vec2 planetPos = body->GetPosition();
        b2Vec2 r = planetPos - sunPos;
        float distance = r.Length();
        if (distance < 0.01f) distance = 0.01f;

        b2Vec2 tang(-r.y, r.x);
        if (tang.LengthSquared() > 0.0f) {
            tang.Normalize();

            const float INITIAL_GUESS_G = 3.0f;   
            const float INITIAL_SUN_MASS = 10000.0f;
            float orbitalSpeed = sqrt(INITIAL_GUESS_G * INITIAL_SUN_MASS / distance);

            if (randSign(rng) < 0.5f) orbitalSpeed = -orbitalSpeed;

            b2Vec2 vel(orbitalSpeed * tang.x, orbitalSpeed * tang.y);
            body->SetLinearVelocity(vel);
        }

        orbitCenter_px = sun.shape.getPosition();
        float dx = spawn_px.x - orbitCenter_px.x;
        float dy = spawn_px.y - orbitCenter_px.y;
        orbitRadius_px = sqrt(dx * dx + dy * dy);

        orbitRing.setRadius(orbitRadius_px);
        orbitRing.setOrigin(orbitRadius_px, orbitRadius_px);
        orbitRing.setPosition(orbitCenter_px);
        orbitRing.setFillColor(Color::Transparent);
        orbitRing.setOutlineThickness(1.0f);
        orbitRing.setOutlineColor(Color(120, 120, 180, 160));
        orbitRing.setPointCount(static_cast<int>(max(60.f, orbitRadius_px * 0.5f)));
    }

    void updateSFML() {
        b2Vec2 p = body->GetPosition();
        float ang = body->GetAngle() * 180.f / b2_pi;

        float px = p.x * PIXELS_PER_METER;
        float py = p.y * PIXELS_PER_METER;

        sprite.setPosition(px, py);
        sprite.setRotation(ang);

        colShape.setPosition(px, py);

        Vector2f currentPos = sprite.getPosition();
        float dx = currentPos.x - orbitCenter_px.x;
        float dy = currentPos.y - orbitCenter_px.y;
        float newOrbitRadius = sqrt(dx * dx + dy * dy);

        if (abs(newOrbitRadius - orbitRadius_px) > 0.25f) {
            orbitRadius_px = newOrbitRadius;
            orbitRing.setRadius(orbitRadius_px);
            orbitRing.setOrigin(orbitRadius_px, orbitRadius_px);
            orbitRing.setPointCount(static_cast<int>(max(60.f, orbitRadius_px * 0.5f)));
        }

        orbitRing.setPosition(orbitCenter_px);
    }
};

class SolarSystem {
public:
    b2World world;
    unique_ptr<Sun> sun;
    vector<unique_ptr<Planet>> planets;

    vector<Texture> planetTextures;
    Texture sunTexture;

    float GRAVITATIONAL_CONSTANT = 3.0f; 
    float SUN_MASS = 10000.0f;           

    SolarSystem() : world(b2Vec2(0.0f, 0.0f))
    {
        planetTextures.resize(8);

        for (int i = 0; i < 8; i++) {
            string path = "Assets/" + to_string(i + 1) + ".png";
            if (!planetTextures[i].loadFromFile(path)) {
                cout << "Failed to load texture: " << path << "\n";
            }
        }

        if (!sunTexture.loadFromFile("Assets/9.png")) {
            cout << "Failed to load sun texture: Assets/9.png\n";
        }
        else {
            sunTexture.setSmooth(true);
        }
    }

    void createSun(const Vector2f& pos_px, float radius_px = 60.f) {
        sun = make_unique<Sun>(world, pos_px, radius_px);
        if (sunTexture.getSize().x > 0)
            sun->applyTexture(sunTexture);
    }

    void spawnPlanetAt(const Vector2f& pos_px) {
        if (!sun) return;

        float dx = pos_px.x - sun->shape.getPosition().x;
        float dy = pos_px.y - sun->shape.getPosition().y;
        float d = sqrt(dx * dx + dy * dy);

        if (d < sun->radius_px + 5.0f) return;

        int index = rng() % planetTextures.size();
        planets.push_back(make_unique<Planet>(world, *sun, pos_px, planetTextures[index]));

        Planet* p = planets.back().get();
        b2Vec2 sunPos = sun->body->GetPosition();
        b2Vec2 planetPos = p->body->GetPosition();
        b2Vec2 r = planetPos - sunPos;
        float distance = r.Length();
        if (distance < 0.01f) distance = 0.01f;

        b2Vec2 tang(-r.y, r.x);
        if (tang.LengthSquared() > 0.0f) {
            tang.Normalize();
            float orbitalSpeed = sqrt(GRAVITATIONAL_CONSTANT * SUN_MASS / distance);
            if (randSign(rng) < 0.5f) orbitalSpeed = -orbitalSpeed;
            b2Vec2 vel(orbitalSpeed * tang.x, orbitalSpeed * tang.y);
            p->body->SetLinearVelocity(vel);
        }
    }

    void applyGravity(float dt)
    {
        if (!sun) return;

        b2Body* sunBody = sun->body;
        const float G = GRAVITATIONAL_CONSTANT;
        const float m1 = SUN_MASS;

        for (auto& p : planets)
        {
            if (!p || !p->body) continue;

            b2Body* planetBody = p->body;

            b2Vec2 dir = sunBody->GetPosition() - planetBody->GetPosition();
            float distance = dir.Length();
            const float minDist = (sun->radius_px * INV_PPM) * 0.5f; 
            if (distance < minDist) distance = minDist;

            dir.Normalize();

            float m2 = planetBody->GetMass();
            float forceMag = G * (m1 * m2) / (distance * distance);

            b2Vec2 force = forceMag * dir;
            planetBody->ApplyForceToCenter(force, true);
        }
    }

    void step(float dt) {
        applyGravity(dt);

        world.Step(dt, 8, 3);

        for (auto& p : planets)
            p->updateSFML();

        cleanupDead();
    }

    void cleanupDead() {
        vector<unique_ptr<Planet>> alive;
        alive.reserve(planets.size());

        for (size_t i = 0; i < planets.size(); i++) {
            Planet* p = planets[i].get();
            bool destroyed = false;

            Vector2f pp = p->sprite.getPosition();
            Vector2f sp = sun->shape.getPosition();
            float dx = pp.x - sp.x;
            float dy = pp.y - sp.y;
            float dist = sqrt(dx * dx + dy * dy);

            if (dist < (sun->radius_px + p->radius_px)) {
                world.DestroyBody(p->body);
                destroyed = true;
            }

            if (destroyed) continue;

            for (size_t j = i + 1; j < planets.size(); j++) {
                Planet* o = planets[j].get();

                float dx2 = p->sprite.getPosition().x - o->sprite.getPosition().x;
                float dy2 = p->sprite.getPosition().y - o->sprite.getPosition().y;
                float d2 = sqrt(dx2 * dx2 + dy2 * dy2);

                if (d2 < (p->radius_px + o->radius_px)) {
                    world.DestroyBody(p->body);
                    world.DestroyBody(o->body);

                    o->radius_px = -1.0f;
                    destroyed = true;
                    break;
                }
            }

            if (!destroyed)
                alive.push_back(move(planets[i]));
        }

        planets.clear();
        for (auto& u : alive)
            if (u && u->radius_px >= 0)
                planets.push_back(move(u));
    }
};

int main() {
    unsigned int W = 1920, H = 1080;
    RenderWindow window(VideoMode(W, H), "Solar System", Style::Fullscreen);

    Texture bgTexture;
    Sprite bgSprite;
    if (bgTexture.loadFromFile("Assets/10.jpg")) {
        bgTexture.setSmooth(true);
        bgSprite.setTexture(bgTexture);
        bgSprite.setPosition(0.f, 0.f);

        Vector2u texSize = bgTexture.getSize();
        Vector2u winSize = window.getSize();
        float scaleX = static_cast<float>(winSize.x) / static_cast<float>(texSize.x);
        float scaleY = static_cast<float>(winSize.y) / static_cast<float>(texSize.y);
        bgSprite.setScale(scaleX, scaleY);
    }

    SolarSystem system;
    system.createSun(Vector2f(W * 0.5f, H * 0.5f), 60.f);

    Clock clk;
    float accumulator = 0.f;
    const float fixed = 1.f / 60.f;

    while (window.isOpen()) {
        float dt = clk.restart().asSeconds();

        Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == Event::Closed) window.close();
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape)
                window.close();
            if (ev.type == Event::MouseButtonPressed &&
                ev.mouseButton.button == Mouse::Left)
            {
                Vector2i m = Mouse::getPosition(window);
                system.spawnPlanetAt(Vector2f((float)m.x, (float)m.y));
            }
        }

        accumulator += dt;
        while (accumulator >= fixed) {
            system.step(fixed);
            accumulator -= fixed;
        }

        
        window.clear(Color::Black);

        if (bgSprite.getTexture())
            window.draw(bgSprite);

        if (system.sun && system.sun->sprite.getTexture())
            window.draw(system.sun->sprite);
        else if (system.sun)
            window.draw(system.sun->shape);

        for (auto& p : system.planets)
            window.draw(p->orbitRing);

        for (auto& p : system.planets)
            window.draw(p->sprite);

        window.display();
    }

    return 0;
}
