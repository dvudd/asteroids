#include <print>
#include <random>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

const bool debug = true;

// Read the current screen resolution.
const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
const unsigned int windowWidth = desktop.size.x;
const unsigned int windowHeight = desktop.size.y;

struct Bullet
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    float radius = 5.f;
    int life = 120;

    void update()
    {
        position += velocity;
        life--;
    }

    bool isDead() const
    {
        return life <= 0;
    }
};

struct Asteroid
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    int size;
    float radius;
    float rotationDirection;
    bool alive = true;
    float angle;

    void update()
    {
        if (position.x > windowWidth + radius)
        {
            position.x = -radius;
        }
        else if (position.x < -radius)
        {
            position.x = windowWidth + radius;
        }

        if (position.y > windowHeight + radius)
        {
            position.y = -radius;
        }
        else if (position.y < -radius)
        {
            position.y = windowHeight + radius;
        }
        position += velocity;
        angle += rotationDirection;
    }

    bool isDead() const
    {
        return !alive;
    }
};

struct Debris
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    int life = 25;

    void update()
    {
        position += velocity;
        life --;
    }

    bool isDead() const
    {
        return life <= 0;
    }
};

/* Returns a random integer between min & max */
int randomInt(int min, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distr(min, max);
    int random = distr(gen);

    return random;
}

/* Rounds the vector values to its closest integrer */
sf::Vector2f round(const sf::Vector2f vector)
{
    return sf::Vector2f{ std::round(vector.x), std::round(vector.y) };
}

/* Returns a random position around the edge of the screen */
sf::Vector2f randomPosition()
{
    // Random spawnpoint generator
    int randomSide = randomInt(1, 4);
    int randomHeight = randomInt(1, windowHeight);
    int randomWidth = randomInt(1, windowWidth);
    float width = windowWidth;
    float height = windowHeight;

    switch (randomSide) {
        case 1:
            width = randomWidth;
            height = 0;
            break;
        case 2:
            width = windowWidth;
            height = randomHeight;
            break;
        case 3:
            width = randomWidth;
            height = windowHeight;
            break;
        case 4:
            width = 0;
            height = randomHeight;
    }

    sf::Vector2f position = {width, height};

    return position;
}

/* Returns a random direction and speed */
sf::Vector2f randomVelocity()
{
    float randomAngle = static_cast<float>(std::rand()) / RAND_MAX;
    float angle = randomAngle * 2.f * sf::priv::pi;


    sf::Vector2f direction{
        std::cos(angle),
        std::sin(angle)
    };

    float randomSpeed = static_cast<float>(std::rand()) / RAND_MAX;
    float minSpeed = 0.2f;
    float maxSpeed = 2.0f;
    float speed = minSpeed + randomSpeed * (maxSpeed - minSpeed);

    sf::Vector2f velocity = direction * speed;

    return velocity;
}

/* Returns a random values between -0.02f and +0.02f */
float randomRotation()
{
    float randomSpeed = static_cast<float>(std::rand()) / RAND_MAX;
    float minSpeed = -0.02f;
    float maxSpeed = 0.02f;
    float speed = minSpeed + randomSpeed * (maxSpeed - minSpeed);
    return speed;
}

/* Spawns a Asteroid at Position with Velocity and the size of (1-3) */
void spawnAsteroid(std::vector<Asteroid>& asteroids, sf::Vector2f position, sf::Vector2f velocity, int size)
{
    float radius;
    float rotation = randomRotation();
    switch (size) {
        case 1:
            radius = 25.f;
            break;
        case 2:
            radius = 35.f;
            break;
        case 3:
            radius = 45.f;
            break;
    }

    asteroids.push_back(Asteroid{position, velocity, size, radius, rotation});
}

/* Spawns three bits of debris at position with random velocity */
void spawnDebris(std::vector<Debris>& debris, sf::Vector2f position)
{
    debris.push_back(Debris{position, randomVelocity()});
    debris.push_back(Debris{position, randomVelocity()});
    debris.push_back(Debris{position, randomVelocity()});
}

/* Spawns two smaller asteroids at the same position. Gives points according to previous size */
int splitAsteroid(std::vector<Asteroid>& asteroids, sf::Vector2f position, int size) {
    // Split the asteroids to smaller pieces, if bigger than small.
    if (size > 1)
    {
        spawnAsteroid(asteroids, position, randomVelocity(), size -1);
        spawnAsteroid(asteroids, position, randomVelocity(), size -1);
    }
    // Give the player points depending on asteroid size.
    int earnedScore;
    switch (size) {
        case 1:
            earnedScore = 10;
            break;
        case 2:
            earnedScore = 20;
            break;
        case 3:
            earnedScore = 30;
            break;
    }
    return earnedScore;
}

int main(int argc, char* argv[])
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    // create a new window of size w*h pixels
    // top-left of the window is (0,0) and bottom-right is (w,h)
    // you will have to read these from the config file

    sf::RenderWindow window(sf::VideoMode({ windowWidth, windowHeight}), "Asteroids", sf::State::Fullscreen);
    window.setFramerateLimit(60);

    // initialize imgui and create a clock used for its internal timing
    if (!ImGui::SFML::Init(window))
    {
        std::print("Could not initialize window\n");
        std::exit(-1);
    }

    /* Clock logic */
    sf::Clock deltaClock;
    sf::Clock weaponCooldown;
    sf::Clock enemyCooldown;
    sf::Clock damageClock;

    // scale the imgui ui and text size by 2
    ImGui::GetStyle().ScaleAllSizes(2.0f);
    ImGui::GetIO().FontGlobalScale = 2.0f;

    // the imgui color {r, g, b} wheel requires floats from 0-1
    // sfml will require instead of uint8_t from 0-255
    // this is the only really annoying conversion between sfml and imgui
    float c[3] = { 0.0f, 1.0f, 1.0f};

    // let's load a font so we can display some text
    sf::Font myFont;

    // attempt to load the font from a file
    if (!myFont.openFromFile("assets/fonts/ubuntu-mono.ttf"))
    {
        // if we can't load the font, print an error to the error console and exit
        std::print("Could not load font!\n");
        std::exit(-1);
    }

    // set up the text object that will be drawn to the screen
    sf::Text text(myFont, "Sample Text", 24);

    // position the top-left corner of the text so that the text alignt on the bottom
    // text character size is in pixels, so move the text up from the bottom by its wHeight
    text.setPosition({ 0, windowHeight - (float)text.getCharacterSize() });

    // set up a character array to set the text
    char displayString[255] = "Sample Text";

    /* PLAYER LOGIC */
    // create the player shape
    sf::CircleShape player(40.f, 60);
    sf::Angle angle = player.getRotation();
    player.setFillColor(sf::Color::Transparent);
    player.setOutlineColor(sf::Color::Blue);
    player.setOutlineThickness(2);
    player.setOrigin({40.f, 40.f});
    player.setPosition({windowWidth / 2.f, windowHeight / 2.f});
    player.setRotation(sf::degrees(270));
    sf::Vector2 playerPosition = player.getPosition();
    sf::Vector2 playerVelocity = {0.f, 0.f};
    int playerHitpoints = 3;
    bool playerInvulnerable = false;

    // Create the player sprite
    sf::Texture playerTexture("assets/textures/spaceship.png");
    sf::Sprite ship(playerTexture);
    ship.setScale({.5f, .5f});
    ship.setOrigin({120.f, 100.f});

    // Create the player engine flame sprite
    sf::Texture flameTexture("assets/textures/engine_flame.png");
    sf::Sprite flame(flameTexture);
    flame.setScale({.5f, .5f});
    flame.setOrigin({120.5f, 29.f});
    float engineOffset = 60.f;

    /* BULLET LOGIC */
    std::vector<Bullet> bullets;
    int bulletCooldown = 500;
    float bulletOffset = 25.f;
    float bulletSpeed = 15.f;

    // Create the bullet sprite
    sf::Texture bulletTexture("assets/textures/bullet.png");
    sf::Sprite bulletSprite(bulletTexture);
    bulletSprite.setScale({.7f, .7f});
    bulletSprite.setOrigin({40.5f, 66.f});

    /* ASTEROID LOGIC */
    std::vector<Asteroid> asteroids;
    // Create the asteroid shape
    sf::Vector2f asteroidPos = {windowWidth / 2.f, windowHeight / 6.f};
    sf::Vector2f asteroidVel = {1.f, 1.f};

    // Create the small asteroid sprite
    sf::Texture smallAsteroidTexture("assets/textures/asteroid_small.png");
    sf::Sprite smallAsteroid(smallAsteroidTexture);
    smallAsteroid.setScale({.5f, .5f});
    smallAsteroid.setOrigin({48.f, 43.5f});

    // Create the medium asteroid sprite
    sf::Texture mediumAsteroidTexture("assets/textures/asteroid_medium.png");
    sf::Sprite mediumAsteroid(mediumAsteroidTexture);
    mediumAsteroid.setScale({.5f, .5f});
    mediumAsteroid.setOrigin({63.f, 64.f});

    // Create the big asteroid sprite
    sf::Texture bigAsteroidTexture("assets/textures/asteroid_big.png");
    sf::Sprite bigAsteroid(bigAsteroidTexture);
    bigAsteroid.setScale({.5f, .5f});
    bigAsteroid.setOrigin({96.f, 97.f});

    /* DEBRIS LOGIC */
    std::vector<Debris> debris;
    sf::Texture debrisTexture("assets/textures/asteroid_small.png");
    sf::Sprite debrisSprite(smallAsteroidTexture);
    debrisSprite.setScale({.2f, .2f});
    debrisSprite.setOrigin({48.f, 43.5f});

    /* PHYSICS LOGIC */
    constexpr float PI = 3.14159265f;
    float thrust = 0.09f;
    float drag = 0.999f;
    float turnAngle = 0.f;
    float turnSpeed = 2.f;
    float turnMax = 4.f;
    float turnDrag = .8f;

    /* GUI LOGIC */
    bool drawText = true;
    bool drawHitbox = false;
    // Score
    int playerScore = 0;
    sf::Text score(myFont, std::to_string(playerScore), 48);
    sf::Vector2f rounded = round(score.getGlobalBounds().size / 2.f + score.getLocalBounds().position);
    score.setOrigin(rounded);
    score.setPosition({static_cast<float>(windowWidth) / 2, 25.f});

    // main loop - continues for each frame while window is open
    // process user input if any exists
    // update internal game simulation
    // render current game screen
    while (window.isOpen())
    {
        // event handling
        while (auto event = window.pollEvent())
        {
            //pass the event to imgui to be parsed
            ImGui::SFML::ProcessEvent(window, *event);

            // this event triggers when the window is closed
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        // update imgui for this frame with the time that the last frame took
        ImGui::SFML::Update(window, deltaClock.restart());

        // draw the UI
        if (debug) {
            ImGui::Begin("Debug Screen");
            ImGui::Text("Window Text!");
            //ImGui::SameLine();
            ImGui::Checkbox("Draw Text", &drawText);
            ImGui::Checkbox("Draw Hitbox", &drawHitbox);
            ImGui::Text("Player Hitpoints: %d", playerHitpoints);
            ImGui::Text("Active Bullets: %zu", bullets.size());
            ImGui::Text("Active Asteroids: %zu", asteroids.size());
            ImGui::Text("Active Debris: %zu", debris.size());
            ImGui::InputText("Text", displayString, 255);
            if (ImGui::Button("Set Text"))
            {
               text.setString(std::to_string(playerScore));
            }
            if (ImGui::Button("Spawn Small Asteroid"))
            {
                spawnAsteroid(asteroids, randomPosition(), randomVelocity(), 1);
            }
            if (ImGui::Button("Spawn Medium Asteroid"))
            {
                spawnAsteroid(asteroids, randomPosition(), randomVelocity(), 2);
            }
            if (ImGui::Button("Spawn Big Asteroid"))
            {
                spawnAsteroid(asteroids, randomPosition(), randomVelocity(), 3);
            }
            ImGui::SameLine();
            ImGui::End();
        };

        // basic rendering function calls
        window.clear();     // clear the window of anything previously drawn
        if (drawText)       // draw the text if the boolean is true
        {
            window.draw(text);
        }

        // Rotate the ship to the left
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        {
            //player.rotate(sf::degrees(-3));
            turnAngle -= turnSpeed;
        }
        // Rotate the ship to the right
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        {
            //player.rotate(sf::degrees(3));
            turnAngle += turnSpeed;
        }
        turnAngle = std::clamp(turnAngle, -turnMax, turnMax);
        turnAngle *= turnDrag;
        player.rotate(sf::degrees(turnAngle));

        // Calculate which way the player is facing.
        float angleFloat = player.getRotation().asRadians();

        sf::Vector2f forward{
            std::cos(angleFloat),
            std::sin(angleFloat)
        };

        // Apply thrust
        bool isThrusting = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
        if (isThrusting)
        {
            playerVelocity += forward * thrust;
        }

        // Apply drag
        playerVelocity *= drag;

        // Update player Velocity
        playerPosition += playerVelocity;

        // Player wrapping around screen
        sf::FloatRect bounds = player.getGlobalBounds();
        float halfWidth = bounds.size.x / 2.f;
        float halfHeight = bounds.size.y / 2.f;

        // X axis
        if (playerPosition.x > windowWidth + halfWidth)
        {
            playerPosition.x = 0 - halfWidth;
        }
        else if (playerPosition.x < 0 - halfWidth)
        {
            playerPosition.x = windowWidth + halfWidth;
        }
        // Y axis
        if (playerPosition.y > windowHeight + halfHeight)
        {
            playerPosition.y = 0 - halfHeight;
        }
        else if (playerPosition.y < 0 - halfHeight)
        {
            playerPosition.y = windowHeight + halfHeight;
        }

        // Update Player Position
        player.setPosition(playerPosition);
        ship.setPosition(playerPosition);
        ship.setRotation(player.getRotation() + sf::degrees(90));

        // FIRE ZE MISSILES
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && weaponCooldown.getElapsedTime().asMilliseconds() >= bulletCooldown)
        {
            sf::Vector2f bulletPosition = playerPosition + forward * bulletOffset;
            sf::Vector2f bulletVelocity = playerVelocity + forward * bulletSpeed;
            bullets.push_back(Bullet{bulletPosition, bulletVelocity});
            weaponCooldown.restart();
        }

        // Update Bullets
        for (auto& bullet : bullets)
        {
            bullet.update();
        }

        // Spawn enemies
        int enemySize = 1;
        int enemySpawnTimer = randomInt(2000, 5000);
        if (enemyCooldown.getElapsedTime().asMilliseconds() >= enemySpawnTimer)
        {
            if (playerScore >= 200)
            {
                enemySize = randomInt(2, 3);
            }
            else if (playerScore >= 100)
            {
                enemySize = randomInt(1, 2);
            }
            spawnAsteroid(asteroids, randomPosition(), randomVelocity(), enemySize);
            enemyCooldown.restart();
        }

        // Update Asteroids
        for (auto& asteroid : asteroids)
        {
            asteroid.update();
        }

        // Bullets Collision detection
        for (auto& bullet : bullets)
        {
            for (auto& asteroid : asteroids)
            {
                sf::Vector2f difference = bullet.position - asteroid.position;
                float distanceSquared =
                    difference.x * difference.x +
                    difference.y * difference.y;

                float combinedRadius = bullet.radius + asteroid.radius;
                if (distanceSquared <= combinedRadius * combinedRadius)
                {
                    // Collision!
                    asteroid.alive = false;
                    bullet.life = 0;
                    break;
                }
            }
        }

        // Remove dead bullets
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(),
                [&](const Bullet& bullet)
                {
                    return bullet.isDead();
                }),
            bullets.end()
        );

        // Asteroid Collision detection
        std::vector<Asteroid> newAsteroids;
        for (auto& asteroid : asteroids)
        {

            // Calculate distance between asteroid and the player.
            sf::Vector2f difference = player.getPosition() - asteroid.position;
            float distanceSquared =
                difference.x * difference.x +
                difference.y * difference.y;

            // Collision
            float combinedRadius = player.getRadius() + asteroid.radius;
            if (distanceSquared <= combinedRadius * combinedRadius)
            {
                if (!playerInvulnerable)
                {
                    playerHitpoints -= 1;
                    playerInvulnerable = true;
                    damageClock.restart();
                }
                asteroid.alive = false;
            }

            // Split dead asteroids and award points
            if (!asteroid.alive)
            {
                // Split the asteroid
                playerScore += splitAsteroid(newAsteroids, asteroid.position, asteroid.size);
                if (asteroid.size <= 1)
                {
                    spawnDebris(debris, asteroid.position);
                }
            }
        }

        // Remove dead asteroids
        asteroids.erase(
            std::remove_if(asteroids.begin(), asteroids.end(),
                [](const Asteroid& asteroid)
                {
                    return asteroid.isDead();
                }),
            asteroids.end()
        );

        // Merge the new asteroids into the asteroids vector
        asteroids.insert(asteroids.end(), newAsteroids.begin(), newAsteroids.end());

        // Update Debris
        for (auto& bits : debris)
        {
            bits.update();
        }

        // Remove dead debris
        debris.erase(
            std::remove_if(debris.begin(), debris.end(),
                [](const Debris& debris)
                {
                    return debris.isDead();
                }),
            debris.end()
        );

        // Draw Bullets
        for (const auto& bullet : bullets)
        {
            float bulletAngle = std::atan2(bullet.velocity.y, bullet.velocity.x);
            bulletSprite.setPosition(bullet.position);
            bulletSprite.setRotation(sf::radians(bulletAngle) + sf::degrees(90));
            window.draw(bulletSprite);
        }

        // Draw Asteroids
        for (const auto& asteroid : asteroids)
        {
            if (asteroid.size == 1)
            {
                smallAsteroid.setPosition(asteroid.position);
                smallAsteroid.setRotation(sf::radians(asteroid.angle));
                window.draw(smallAsteroid);
            }
            else if (asteroid.size == 2) {
                mediumAsteroid.setPosition(asteroid.position);
                mediumAsteroid.setRotation(sf::radians(asteroid.angle));
                window.draw(mediumAsteroid);
            }
            else
            {
                bigAsteroid.setPosition(asteroid.position);
                bigAsteroid.setRotation(sf::radians(asteroid.angle));
                window.draw(bigAsteroid);
            }
        }

        // Draw Debris
        for (auto& bits : debris)
        {
            debrisSprite.setPosition(bits.position);
            window.draw(debrisSprite);
        }

        // Debug - center the tipDot on the player position
        if (drawHitbox)
        {
            window.draw(player);
        }

        // Player damage animation
        if (playerInvulnerable)
        {
            // flash/tint
            int flashIndex = static_cast<int>(damageClock.getElapsedTime().asSeconds() / 0.12f);
            if (flashIndex % 2 == 0)
            {
                ship.setColor(sf::Color(255, 80, 80));
            }
            else
            {
                ship.setColor(sf::Color::White);
            }
            if (damageClock.getElapsedTime().asMilliseconds() >= 1000)
            {
                playerInvulnerable = false;
                ship.setColor(sf::Color::White);
            }
        }

        // Draw the player
        if (isThrusting)
        {
            flame.setPosition(playerPosition - forward * engineOffset);
            flame.setRotation(ship.getRotation());
            window.draw(flame);
        }
        window.draw(ship);

        // Draw the UI last so it's on top
        score.setString(std::to_string(playerScore));
        window.draw(score);
        ImGui::SFML::Render(window);
        window.display();
    }

    return 0;

}
