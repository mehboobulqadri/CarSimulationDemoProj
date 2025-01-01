#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <cmath>
#include <map>
#include <fstream>
#include <ctime>

// All Global Booleans
bool showRestartButton = false;
bool showStats = false;
bool enlargedMinimap = false;
bool musicMenu = false;
bool settingsMenu = false;
bool showEscapeMenu = false;
bool statsToggled = false;
bool minimapToggled = false;
bool musicMenuToggled = false;
bool showCar = false;
bool escapeMenuToggled = false;
bool darkMode = true;
static bool inFuelArea = false;
int escapeCount = 0;

// Constants for car physics and road behavior
const float MAX_SPEED = 120.f;
const float ACCELERATION = 100.f;
const float TURN_RATE = 120.f;
const float FRICTION = 20.f;
const float MIN_TURN_SPEED = 20.f;
const float MENU_TOGGLE_COOLDOWN = 0.1f;
const float MUSIC_CHANGE_COOLDOWN = 0.5f;

// Key Cooldown 
std::map<sf::Keyboard::Key, sf::Clock> keyCooldowns;

// Struct for Car
struct Car {
    sf::RectangleShape shape;
    sf::Sprite sprite;
    sf::Texture texture;
    float speed = 0.f;
    float fuel = 2000.f;
    float angle = 0.f;
    bool hasSprite = false;
    long double mileage = 0;
    sf::Vector2f position;

};

// Function Prototypes
void generateLogFile(const Car& car);
void restrictView(sf::View& view, const sf::Vector2u& mapSize);
void timeDelay(float seconds);
void drawDynamicMinimap(sf::RenderWindow& window, const sf::Sprite& mapSprite, const sf::View& view, const Car& car);
bool isKeyReady(sf::Keyboard::Key key, float cooldownTime);
void handleInput(Car& car, float deltaTime, float handling);
void updateCar(Car& car, float deltaTime, const sf::Image& roadMask);
void drawInteractiveStats(sf::RenderWindow& window, const Car& car, const sf::View& view);
void showMiniMape(sf::RenderWindow& window, const sf::View& view, sf::Sprite map);
void drawEscapeMenu(sf::RenderWindow& window, const sf::View& view,
    std::vector<std::pair<sf::RectangleShape, std::string>>& escapeMenuButtons);
void showMusicMenu(sf::RenderWindow& window, const sf::View& view, sf::Music& backgroundMusic, std::vector<std::string>& songList, int& currentSongIndex, float& volume);
void drawMinimap(sf::RenderWindow& window, sf::RenderTexture& miniMapTexture, const sf::Sprite& mapSprite, const sf::View& view, bool enlarged);

int main()
{
    // Rendering a Window and setting a fps limit
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Car Simulation");
    window.setFramerateLimit(90);

    // Loading the map Texture & Sprite
    sf::Texture mapTexture;
    if (!mapTexture.loadFromFile("main_img.png")) {
        std::cerr << "Error loading map image!" << std::endl;
        return -1;
    }
    sf::Sprite mapSprite(mapTexture);

    sf::Sprite miniMap;
    sf::Texture miniTexture;
    miniTexture.loadFromFile("miniMap.png");
    miniMap.setTexture(miniTexture);
    miniMap.setScale(0.27f, 0.27f);

    sf::Sprite enlargedMiniMap;
    sf::Texture enlargedTexture;
    enlargedTexture.loadFromFile("main_img.png");
    enlargedMiniMap.setTexture(enlargedTexture);

    sf::Image roadMask;
    if (!roadMask.loadFromFile("map_mask.jpeg")) {
        std::cerr << "Error loading road mask!" << std::endl;
        return -1;
    }

    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("basic_music.mp3")) {
        std::cerr << "Error loading music!" << std::endl;
        return -1;
    }

    backgroundMusic.setLoop(true);
    backgroundMusic.play();

    std::vector<std::string> songList = { "basic_music.mp3", "Adele.mp3", "Ainsi-Bas-La-Vida.mp3", "Akela-hon.mp3" };
    int currentSongIndex = 0;
    float volume = 50.f;
    backgroundMusic.setVolume(volume);

    Car car;
    car.shape.setSize(sf::Vector2f(80.f, 40.f));
    car.shape.setOrigin(10000.f, 10000.f);
    car.shape.setFillColor(sf::Color::Blue);

    if (car.texture.loadFromFile("car4.png")) {
        car.sprite.setTexture(car.texture);
        car.sprite.setOrigin(car.texture.getSize().x / 2.f, car.texture.getSize().y / 2.f);
        float scaleX = car.shape.getSize().x / car.texture.getSize().x;
        float scaleY = car.shape.getSize().y / car.texture.getSize().y;
        car.sprite.setScale(scaleX, scaleY);
        car.hasSprite = true;
    }

    car.shape.setPosition(2450.f, 2064.f);
    if (car.hasSprite)
        car.sprite.setPosition(2450.f, 2064.f);

    bool carPlaced = true;
    sf::View view;
    view.setSize(1280.0f, 768.0f);
    sf::RenderTexture miniMapTexture;
    if (!miniMapTexture.create(800, 600)) {
        std::cerr << "Error creating mini map texture!" << std::endl;
        return -1;
    }

    sf::Clock clock;
    ChangeTheme:

    std::vector<std::pair<sf::RectangleShape, std::string>> escapeMenuButtons = {
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Restart"},
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Change Theme"},
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Music"},
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Quit"},
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Generate Log"}
    };

    float buttonStartY = 0; // Adjusted dynamically later
    float buttonSpacing = 70.f;

    for (size_t i = 0; i < escapeMenuButtons.size(); ++i) {
        escapeMenuButtons[i].first.setFillColor(sf::Color(100, 100, 200));
        if (darkMode)
            escapeMenuButtons[i].first.setOutlineColor(sf::Color::White);
        else
            escapeMenuButtons[i].first.setOutlineColor(sf::Color::Black);
        escapeMenuButtons[i].first.setOutlineThickness(2.f);
        escapeMenuButtons[i].first.setPosition(0, buttonStartY + i * buttonSpacing); // Dynamic update later
    }
   
    bool virginity = true;
    
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape && !escapeMenuToggled) {
                    if (musicMenu) {
                        musicMenu = false;
                    }
                    else if (enlargedMinimap) {
                        enlargedMinimap = false;
                    }
                    else {
                        showEscapeMenu = !showEscapeMenu;
                    }
                    escapeMenuToggled = true;
                    if (!virginity && !showCar)
                    {
                        showCar = !showCar;
                        car.shape.setOrigin(40.f, 20.f);
                    }

                }

                // Toggle Stats (Tab Key)
                if (event.key.code == sf::Keyboard::Tab && !statsToggled) {
                    showStats = !showStats;
                    statsToggled = true;
                }

                // Minimap Toggle (M Key)
                if (event.key.code == sf::Keyboard::M && !minimapToggled && !showEscapeMenu && !musicMenu) {
                    enlargedMinimap = !enlargedMinimap;
                    minimapToggled = true;
                    if (showStats)
                        showStats = !showStats;
                }

                // Music Menu from Escape Menu
                if (showEscapeMenu && event.key.code == sf::Keyboard::M && !musicMenuToggled) {
                    showEscapeMenu = false;
                    musicMenu = true;
                    musicMenuToggled = true;
                }

                if (showEscapeMenu && event.key.code == sf::Keyboard::R)
                {
                    car.shape.setPosition(2450.f, 2064.f);
                    if (car.hasSprite)
                        car.sprite.setPosition(2450.f, 2064.f);
                    car.speed = 0;
                    car.angle = 0;
                    car.fuel = 2000;
                    car.mileage = 0;
                    car.speed = 0;
                    showEscapeMenu = !showEscapeMenu;
                }
            }

            if (event.type == sf::Event::KeyReleased) {
                // Reset toggles when keys are released
                if (event.key.code == sf::Keyboard::Tab)
                    statsToggled = false;
                if (event.key.code == sf::Keyboard::Escape)
                {

                    escapeMenuToggled = false;
                    showRestartButton = true;
                    virginity = false;
                }
                if (event.key.code == sf::Keyboard::M)
                    minimapToggled = false;
                musicMenuToggled = false;
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && !showEscapeMenu) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                sf::FloatRect miniMapBounds(view.getCenter().x + view.getSize().x / 2 - 210.f,
                    view.getCenter().y - view.getSize().y / 2 + 10.f,
                    miniTexture.getSize().x * 0.27f,
                    miniTexture.getSize().y * 0.27f);
                if (miniMapBounds.contains(mousePos)) {
                    enlargedMinimap = !enlargedMinimap;
                }
            }
        }

        if (carPlaced && !showEscapeMenu) {
            float deltaTime = clock.restart().asSeconds();
            handleInput(car, deltaTime, TURN_RATE);
            updateCar(car, deltaTime, roadMask);

            view.setCenter(car.shape.getPosition());
            window.setView(view);
        }
        if (carPlaced && !showEscapeMenu) {
            float deltaTime = clock.restart().asSeconds();
            handleInput(car, deltaTime, TURN_RATE);
            updateCar(car, deltaTime, roadMask);

            view.setCenter(car.shape.getPosition());
            restrictView(view, mapTexture.getSize());  // Add this line

            window.setView(view);
        }
        if (carPlaced && !showEscapeMenu) {
            float deltaTime = clock.restart().asSeconds();
            handleInput(car, deltaTime, TURN_RATE);
            updateCar(car, deltaTime, roadMask);

            view.setCenter(car.shape.getPosition());
            restrictView(view, mapTexture.getSize());  // Add this line

            window.setView(view);
        }
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && showEscapeMenu) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

            for (size_t i = 0; i < escapeMenuButtons.size(); ++i) {
                if (escapeMenuButtons[i].first.getGlobalBounds().contains(mousePos)) {
                    if (escapeMenuButtons[i].second == "Restart") {
                        // Reset car properties
                        car.shape.setPosition(2450.f, 2064.f);
                        if (car.hasSprite)
                            car.sprite.setPosition(2450.f, 2064.f);
                        car.speed = 0;
                        car.angle = 0;
                        car.fuel = 2000;
                        car.mileage = 0;
                        car.position.x = 0;
                        car.position.y = 0;
                        showEscapeMenu = false;
                        virginity = false;
                        showCar = true;
                    }
                    else if (escapeMenuButtons[i].second == "Change Theme") {

                        darkMode = !darkMode;
                        timeDelay(0.26f);
                        goto ChangeTheme;
                       
                    }
                    else if (escapeMenuButtons[i].second == "Music") {
                        musicMenu = true;
                        showEscapeMenu = false;
                    }
                    else if (escapeMenuButtons[i].second == "Quit") {
                        window.close();
                    }
                    else if (escapeMenuButtons[i].second == "Generate Log") {

                        generateLogFile(car);
                        timeDelay(0.26f);
                    }
                }
            }
        }
        window.clear();
        window.draw(mapSprite);
        if (!showEscapeMenu && escapeCount == 0)
        {
            showEscapeMenu = !showEscapeMenu;
            auto ptr = &escapeCount;
            std::vector<std::pair<std::string, int >> One[1];
            One->push_back({ "one", 1 });
            for (auto& x : One)
                for (auto& y : x)
                    *ptr += y.second;
        }
        if (car.hasSprite && showCar) {
            window.draw(car.sprite);
        }
        else {
            window.draw(car.shape);
        }

        if (showStats) {
            drawInteractiveStats(window, car, view);
        }

        if (enlargedMinimap) {
            showMiniMape(window, view, enlargedMiniMap);
        }
        else {
            drawDynamicMinimap(window, miniMap, view, car);
        }


        if (showEscapeMenu) {
            // Update button positions dynamically based on the view
            float buttonStartY = view.getCenter().y - 100.f; // Center vertically
            for (size_t i = 0; i < escapeMenuButtons.size(); ++i) {
                escapeMenuButtons[i].first.setPosition(view.getCenter().x - 150.f, buttonStartY + i * 70.f);
            }

            // Draw the escape menu with clickable buttons
            drawEscapeMenu(window, view, escapeMenuButtons);
        }
        if (musicMenu) {
            showMusicMenu(window, view, backgroundMusic, songList, currentSongIndex, volume);
        }

        window.display();
    }

    return 0;
}

bool isKeyReady(sf::Keyboard::Key key, float cooldownTime) {
    // Check if the cooldown has elapsed for the given key
    if (keyCooldowns[key].getElapsedTime().asSeconds() >= cooldownTime) {
        keyCooldowns[key].restart();
        return true;
    }
    return false;
}

void handleInput(Car& car, float deltaTime, float handling) {
    if (car.fuel <= 0)
        return;

    // Define friction and drag coefficients
    const float FRICTION = 26.f;  // Friction opposing movement
    const float DRAG = 0.02f;     // Air resistance

    // Acceleration and braking
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        car.speed += ACCELERATION * deltaTime;
        if (car.speed > MAX_SPEED)
            car.speed = MAX_SPEED;
        car.fuel -= 5 * deltaTime; // Consume more fuel for acceleration
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        car.speed -= ACCELERATION * deltaTime * 0.5f;
        if (car.speed < -MAX_SPEED / 3)
            car.speed = -MAX_SPEED / 3;
        car.fuel -= 2 * deltaTime;
    }


    // Apply drag and friction
    car.speed *= (1 - DRAG * deltaTime);  // Simulates air resistance
    if (car.speed > 0) {
        car.speed -= FRICTION * deltaTime;
        if (car.speed < 0) car.speed = 0;
    }
    else if (car.speed < 0) {
        car.speed += FRICTION * deltaTime;
        if (car.speed > 0) car.speed = 0;
    }

    // Turning with dynamic handling based on speed
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && std::abs(car.speed) > MIN_TURN_SPEED) {
        car.angle -= handling * deltaTime * (car.speed / MAX_SPEED);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && std::abs(car.speed) > MIN_TURN_SPEED) {
        car.angle += handling * deltaTime * (car.speed / MAX_SPEED);
    }
}

void updateCar(Car& car, float deltaTime, const sf::Image& roadMask) {
    if (!escapeMenuToggled && !musicMenu) {
        float angleRadians = car.angle * 3.14159f / 180.f;
        sf::Vector2f movement(std::cos(angleRadians) * car.speed * deltaTime,
            std::sin(angleRadians) * car.speed * deltaTime);

        sf::Vector2f newPosition = car.shape.getPosition() + movement;

        if (newPosition.x < 0 || newPosition.y < 0 ||
            newPosition.x >= roadMask.getSize().x || newPosition.y >= roadMask.getSize().y) {
            car.speed *= 0.5f; // Halve speed on collision with boundaries
            return;
        }

        sf::Color pixelColor = roadMask.getPixel(static_cast<unsigned int>(newPosition.x),
            static_cast<unsigned int>(newPosition.y));
        if (pixelColor == sf::Color::Black) {
            car.speed *= 0.7f; // Reduce speed more significantly on off-road
            return;
        }
        else if (pixelColor == sf::Color::Green)
        {

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::F) && isKeyReady(sf::Keyboard::F, 0.33f))
            {
                if (car.fuel < 1051)
                    car.fuel += 10;

                else
                    car.fuel += (2000 - car.fuel);
            }


        }

        car.shape.move(movement);
        car.shape.setRotation(car.angle);

        if (car.hasSprite) {
            car.sprite.setPosition(car.shape.getPosition());
            car.sprite.setRotation(car.angle);
        }
        float dx = car.speed * cos(car.angle) * deltaTime;
        float dy = car.speed * sin(car.angle) * deltaTime;

        car.position.x += dx;
        car.position.y += dy;

        // Accumulate mileage
        car.mileage += std::sqrt(dx * dx + dy * dy);

    }
}

void drawInteractiveStats(sf::RenderWindow& window, const Car& car, const sf::View& view) {
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Error loading font!" << std::endl;
        return;
    }

    // Button properties
    const float buttonWidth = 180.f;
    const float buttonHeight = 40.f;
    const float buttonSpacing = 10.f;
    const sf::Vector2f startPos(view.getCenter().x - view.getSize().x / 2 + 20.f,
        view.getCenter().y - view.getSize().y / 2 + 20.f);

    // Create reusable button style
    auto createButton = [&](const std::string& label, float yOffset) {
        sf::RectangleShape button(sf::Vector2f(buttonWidth, buttonHeight));
        button.setFillColor(sf::Color(100, 100, 200)); // Blue with slight transparency
        if (darkMode)
            button.setOutlineColor(sf::Color::White);
        else
            button.setOutlineColor(sf::Color::Black);
        button.setOutlineThickness(2.f);
        button.setPosition(startPos.x, startPos.y + yOffset);

        sf::Text buttonText;
        buttonText.setFont(font);
        buttonText.setString(label);
        buttonText.setCharacterSize(14);
        buttonText.setFillColor(sf::Color::White);
        buttonText.setPosition(button.getPosition().x + 10.f, button.getPosition().y + 10.f);

        return std::make_pair(button, buttonText);
        };

    // Create buttons
    std::vector<std::pair<sf::RectangleShape, sf::Text>> buttons;

    // Fuel stat
    buttons.push_back(createButton("Fuel: " + std::to_string(static_cast<int>(car.fuel) / 20) + "%", 0.f));

    // Speed stat
    buttons.push_back(createButton("Speed: " + std::to_string(static_cast<int>(std::abs(car.speed) / 2)) + " km/h",
        (buttonHeight + buttonSpacing) * 1));

    // X-Y position
    buttons.push_back(createButton("Position: (" + std::to_string(static_cast<int>(car.position.x)) + ", " +
        std::to_string(static_cast<int>(car.position.y)) + ")",
        (buttonHeight + buttonSpacing) * 2));

    // Mileage stat
    buttons.push_back(createButton("Mileage: " + std::to_string(static_cast<int>((car.mileage) / 1000) / 2) + " km",
        (buttonHeight + buttonSpacing) * 3));

    // Draw buttons
    for (const auto& button : buttons) {
        window.draw(button.first);  // Draw button background
        window.draw(button.second); // Draw button text
    }
}

void showMiniMape(sf::RenderWindow& window, const sf::View& view, sf::Sprite map) {
    // Define the original resolution of the map sprite
    float originalWidth = 3000.f;
    float originalHeight = 2258.f;

    // Scale the map sprite to your desired scale
    float scaleFactor = 0.27f;  // Example scale factor (10%)

    map.setScale(scaleFactor, scaleFactor);

    // Get the scaled size of the map
    float scaledWidth = originalWidth * scaleFactor;
    float scaledHeight = originalHeight * scaleFactor;

    // Position the map sprite so that its center aligns with the view's center
    map.setPosition(view.getCenter().x - scaledWidth / 2, view.getCenter().y - scaledHeight / 2);

    // Draw the map sprite
    window.draw(map);
}

void drawEscapeMenu(sf::RenderWindow& window, const sf::View& view,
    std::vector<std::pair<sf::RectangleShape, std::string>>& escapeMenuButtons) {
    // Create semi-transparent overlay
    sf::RectangleShape menuOverlay(sf::Vector2f(view.getSize().x, view.getSize().y));
    if (darkMode)
        menuOverlay.setFillColor(sf::Color(0, 0, 0, 150));
    else
        menuOverlay.setFillColor(sf::Color(175, 175, 175, 150));
    menuOverlay.setPosition(view.getCenter().x - view.getSize().x / 2, view.getCenter().y - view.getSize().y / 2);

    // Load font
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Error loading font!" << std::endl;
        return;
    }

    // Set up text for button labels
    sf::Text buttonText;
    buttonText.setFont(font);
    buttonText.setCharacterSize(25);
    buttonText.setFillColor(sf::Color::White);

    // Draw overlay
    window.draw(menuOverlay);

    // Draw buttons and their labels
    for (auto& button : escapeMenuButtons) {
        if (button.second == "Restart" && !showRestartButton) {
            // Change the label to "Continue"
            buttonText.setString("Start");
        }
        else {
            // Use the existing label
            buttonText.setString(button.second);
        }

        // Draw the button
        window.draw(button.first);

        // Center the text on the button
        buttonText.setPosition(
            button.first.getPosition().x + (button.first.getSize().x - buttonText.getLocalBounds().width) / 2.f,
            button.first.getPosition().y + (button.first.getSize().y - buttonText.getLocalBounds().height) / 2.f - 5.f);

        // Draw the button text
        window.draw(buttonText);
    }
}

void showMusicMenu(sf::RenderWindow& window, const sf::View& view, sf::Music& backgroundMusic, std::vector<std::string>& songList, int& currentSongIndex, float& volume) {
    // Create the overlay for the menu
    sf::RectangleShape menuOverlay(sf::Vector2f(view.getSize().x, view.getSize().y));
    if(darkMode)
        menuOverlay.setFillColor(sf::Color(0, 0, 0, 150));
    else
        menuOverlay.setFillColor(sf::Color(175, 175, 175, 150));
    menuOverlay.setPosition(view.getCenter().x - view.getSize().x / 2, view.getCenter().y - view.getSize().y / 2);

    // Load the font
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Error loading font!" << std::endl;
        return;
    }

    // Create the title text
    sf::Text title("Music Menu", font, 24);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold | sf::Text::Underlined);
    title.setPosition(view.getCenter().x - title.getLocalBounds().width / 2.f, view.getCenter().y - 250.f);

    // Create volume text
    sf::Text volumeText("Volume: " + std::to_string(static_cast<int>(volume)) + "%", font, 20);
    volumeText.setFillColor(sf::Color::White);
    volumeText.setPosition(view.getCenter().x - volumeText.getLocalBounds().width / 2.f, view.getCenter().y - 180.f);

    // Create current song text
    sf::Text currentSongText("Current Song: " + songList[currentSongIndex], font, 20);
    currentSongText.setFillColor(sf::Color::White);
    currentSongText.setPosition(view.getCenter().x - currentSongText.getLocalBounds().width / 2.f, view.getCenter().y - 130.f);

    // Create a background box for the text elements
    sf::RectangleShape infoBox(sf::Vector2f(400.f, 180.f)); // Box size (adjusted for all text)
    infoBox.setFillColor(sf::Color(100, 100, 200, 180)); // Semi-transparent black box
    if(darkMode)
        infoBox.setOutlineColor(sf::Color::White); // White outline
    else
        infoBox.setOutlineColor(sf::Color::Black);
    infoBox.setOutlineThickness(2.f); // Outline thickness
    infoBox.setPosition(view.getCenter().x - 200.f, view.getCenter().y - 220.f); // Position the box

    // Draw the overlay and text box
    window.draw(menuOverlay);
    window.draw(infoBox);  // Draw the background box

    // Draw the text elements inside the box
    window.draw(title);
    window.draw(volumeText);
    window.draw(currentSongText);

    // Create buttons for Play/Pause, Next Song, Previous Song, and Back
    std::vector<std::pair<sf::RectangleShape, std::string>> musicMenuButtons = {
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Play/Pause"},
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Next Song"},
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Previous Song"},
        {sf::RectangleShape(sf::Vector2f(300.f, 50.f)), "Back"}
    };

    // Set up button appearance and position
    float buttonStartY = view.getCenter().y - 30.f; // Adjust vertically based on the view
    float buttonSpacing = 70.f;

    for (size_t i = 0; i < musicMenuButtons.size(); ++i) {
        // Set button color and outline
        musicMenuButtons[i].first.setFillColor(sf::Color(100, 100, 200));  // Light gray button color
        if (darkMode)
            musicMenuButtons[i].first.setOutlineColor(sf::Color::White);  // White outline
        else
            musicMenuButtons[i].first.setOutlineColor(sf::Color::Black);
        musicMenuButtons[i].first.setOutlineThickness(2.f);  // Outline thickness
        musicMenuButtons[i].first.setPosition(view.getCenter().x - 150.f, buttonStartY + i * buttonSpacing);

        // Create text for each button
        sf::Text buttonText(musicMenuButtons[i].second, font, 20);
        buttonText.setFillColor(sf::Color::White);  // White text
        buttonText.setPosition(musicMenuButtons[i].first.getPosition().x + (musicMenuButtons[i].first.getSize().x / 2.f) - buttonText.getLocalBounds().width / 2.f,
            musicMenuButtons[i].first.getPosition().y + (musicMenuButtons[i].first.getSize().y / 2.f) - buttonText.getLocalBounds().height / 2.f);

        // Draw button rectangle and textsddd
        window.draw(musicMenuButtons[i].first);  // Draw the button rectangle
        window.draw(buttonText);  // Draw the button text
    }

    // Event handling for button clicks
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window.close();

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

            // Check if any of the main menu buttons were clicked
            for (size_t i = 0; i < musicMenuButtons.size(); ++i) {
                if (musicMenuButtons[i].first.getGlobalBounds().contains(mousePos)) {
                    if (musicMenuButtons[i].second == "Play/Pause") {
                        if (backgroundMusic.getStatus() == sf::Music::Playing) {
                            backgroundMusic.pause();
                        }
                        else {
                            backgroundMusic.play();
                        }
                    }
                    else if (musicMenuButtons[i].second == "Next Song") {
                        currentSongIndex = (currentSongIndex + 1) % songList.size();
                        if (!backgroundMusic.openFromFile(songList[currentSongIndex])) {
                            std::cerr << "Error loading song: " << songList[currentSongIndex] << std::endl;
                        }
                        else {
                            backgroundMusic.play();
                        }
                    }
                    else if (musicMenuButtons[i].second == "Previous Song") {
                        currentSongIndex = (currentSongIndex - 1 + songList.size()) % songList.size();
                        if (!backgroundMusic.openFromFile(songList[currentSongIndex])) {
                            std::cerr << "Error loading song: " << songList[currentSongIndex] << std::endl;
                        }
                        else {
                            backgroundMusic.play();
                        }
                    }
                    else if (musicMenuButtons[i].second == "Back") {
                        musicMenu = false;  // Close the music menu
                        showEscapeMenu = true;  // Show the escape menu
                    }
                }
            }
        }
    }

    // Keyboard shortcuts handling
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::M)) {
        if (backgroundMusic.getStatus() == sf::Music::Playing) {
            backgroundMusic.pause();
        }
        else {
            backgroundMusic.play();
        }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && isKeyReady(sf::Keyboard::Left, 0.2f)) {
        currentSongIndex = (currentSongIndex - 1 + songList.size()) % songList.size();
        if (!backgroundMusic.openFromFile(songList[currentSongIndex])) {
            std::cerr << "Error loading song: " << songList[currentSongIndex] << std::endl;
        }
        else {
            backgroundMusic.play();
        }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && isKeyReady(sf::Keyboard::Right, 0.2f)) {
        currentSongIndex = (currentSongIndex + 1) % songList.size();
        if (!backgroundMusic.openFromFile(songList[currentSongIndex])) {
            std::cerr << "Error loading song: " << songList[currentSongIndex] << std::endl;
        }
        else {
            backgroundMusic.play();
        }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && isKeyReady(sf::Keyboard::Up, 0.2f)) {
        volume = std::min(volume + 5.f, 100.f);
        backgroundMusic.setVolume(volume);
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && isKeyReady(sf::Keyboard::Down, 0.2f)) {
        volume = std::max(volume - 5.f, 0.f);
        backgroundMusic.setVolume(volume);
    }
}

void drawMinimap(sf::RenderWindow& window, sf::RenderTexture& miniMapTexture, const sf::Sprite& mapSprite, const sf::View& view, bool enlarged)
{

    miniMapTexture.clear();
    miniMapTexture.draw(mapSprite);
    miniMapTexture.display();

    sf::Sprite miniMapSprite(miniMapTexture.getTexture());
    miniMapSprite.setScale(0.25f, 0.25f);
    miniMapSprite.setPosition(view.getCenter().x + view.getSize().x / 2 - 210.f, view.getCenter().y - view.getSize().y / 2 + 10.f);
    window.draw(miniMapSprite);
}

void generateLogFile(const Car& car) {
    std::ofstream logFile("car_simulation_log.txt", std::ios::app);
    if (!logFile) {
        std::cerr << "Error opening log file!" << std::endl;
        return;
    }

    // Get the current time

    std::time_t now = std::time(nullptr);
    std::tm timeInfo;

#ifdef _WIN32
    localtime_s(&timeInfo, &now); // Windows-specific thread-safe function
#else
    localtime_r(&now, &timeInfo); // POSIX-specific thread-safe function
#endif

    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeInfo);
    // Write car details to the log file
    logFile << "Log Timestamp: " << timeStr << "\n";
    logFile << "Car Position: (" << car.shape.getPosition().x << ", " << car.shape.getPosition().y << ")\n";
    logFile << "Mileage: " << car.mileage / 2000 << " km\n";
    logFile << "Fuel: " << car.fuel / 20 << " L\n";
    srand(time(0));
    logFile << "Engine Temperature: " << ((rand() % 50) + 50) << "C\n";
    logFile << "Tire Wear: " << int(car.mileage / 1000) << "%\n";
    logFile << "-------------------------------\n";

    logFile.close();
    std::cout << "Log file updated successfully!" << std::endl;
}

void restrictView(sf::View& view, const sf::Vector2u& mapSize) {
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    float halfWidth = size.x / 2.f;
    float halfHeight = size.y / 2.f;

    // Restrict the center position based on the map size
    if (center.x - halfWidth < 0)
        center.x = halfWidth;
    if (center.x + halfWidth > mapSize.x)
        center.x = mapSize.x - halfWidth;
    if (center.y - halfHeight < 0)
        center.y = halfHeight;
    if (center.y + halfHeight > mapSize.y)
        center.y = mapSize.y - halfHeight;

    view.setCenter(center);
}

void timeDelay(float seconds)
{
    sf::Clock clock;
    while (clock.getElapsedTime().asSeconds() < seconds)
    {
        // Optional: Process events or do other tasks during the delay
        sf::sleep(sf::milliseconds(10));  // Sleep to avoid maxing out CPU
    }

}

void drawDynamicMinimap(sf::RenderWindow& window, const sf::Sprite& mapSprite, const sf::View& view, const Car& car) {
    // Define the minimap size and position
    float minimapWidth = 300.f;
    float minimapHeight = 200.f;
    sf::Vector2f minimapPosition(view.getCenter().x + view.getSize().x / 2 - minimapWidth - 10.f,
        view.getCenter().y - view.getSize().y / 2 + 10.f);

    // Draw the minimap background
    sf::RectangleShape minimapBackground(sf::Vector2f(minimapWidth - 5.f, minimapHeight + 6.f));
    minimapBackground.setPosition(minimapPosition);
    minimapBackground.setFillColor(sf::Color(50, 50, 50, 200));
    window.draw(minimapBackground);

    // Scale the map sprite to fit within the minimap
    sf::Sprite scaledMapSprite = mapSprite;
    float scaleX = minimapWidth / mapSprite.getGlobalBounds().width;
    float scaleY = minimapHeight / mapSprite.getGlobalBounds().height;
    scaledMapSprite.setScale(0.099f, 0.099f);
    scaledMapSprite.setPosition(minimapPosition);
    window.draw(scaledMapSprite);

    // Calculate the car's position on the minimap
    sf::Vector2f carPositionOnMap = car.shape.getPosition();
    sf::Vector2f mapSize(mapSprite.getTexture()->getSize());
    sf::Vector2f scaledCarPosition(
        (carPositionOnMap.x / mapSize.x) * minimapWidth + minimapPosition.x,
        (carPositionOnMap.y / mapSize.y) * minimapHeight + minimapPosition.y
    );

    // Draw the car as a dot on the minimap
    sf::CircleShape carDot(3.f);  // Radius of the dot
    carDot.setFillColor(sf::Color::Red);
    carDot.setPosition(scaledCarPosition - sf::Vector2f(carDot.getRadius(), carDot.getRadius() - 20.f));
    window.draw(carDot);
}