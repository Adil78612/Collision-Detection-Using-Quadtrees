#include "../hpp/MainScreen.hpp"
#include "../hpp/Common.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>

extern float particleSpeed;

MainScreen::MainScreen(Game* game) {
    this->game = game;
    pause = false;
    useQuadTree = false;
    boundary = sf::FloatRect(10, 10, game->width * 0.75f, game->height - 20);
    objectNum = 50;
    radius = 2.0f;

    font = new sf::Font();
#if defined(_WIN32) || defined(_WIN64)
    font->loadFromFile("assets\\fonts\\RobotoMono-Regular.ttf");
#else
    font->loadFromFile("assets/fonts/RobotoMono-Regular.ttf");
#endif

    for (int i = 0; i < 3; i++) {
        textboxes.emplace_back(*font);
        textboxes[i].setBorder(2, sf::Color::Black, sf::Color::Black, sf::Color::White);
        textboxes[i].allowNumberOnly();
        textboxes[i].setBackgroundColor(sf::Color(230, 230, 230));
        labels.emplace_back();
        labels[i].setFont(*font);
    }

    labels[0].setString("OBJECTS:");
    labels[1].setString("RADIUS:");
    labels[2].setString("SPEED:");

    textboxes[0].setString("50");
    textboxes[1].setString("2");
    textboxes[2].setString("100");

    buttons.emplace_back(*font);
    buttons[0].setString("APPLY");
    buttons[0].setOnAction([&]() {
        if (!textboxes[0].empty()) objectNum = std::stoi(textboxes[0].getString());
        if (!textboxes[1].empty()) radius = std::stof(textboxes[1].getString());
        if (!textboxes[2].empty()) particleSpeed = std::stof(textboxes[2].getString());
        initializeObjects();
    });

    buttons.emplace_back(*font);
    buttons[1].setString("PAUSE");
    buttons[1].setOnAction([&]() {
        pause = !pause;
        std::cout << (pause ? "Paused\n" : "Resumed\n");
    });

    buttons.emplace_back(*font);
    buttons[2].setString("MODE: BRUTE");
    buttons[2].setOnAction([&]() {
        useQuadTree = !useQuadTree;
        buttons[2].setString(useQuadTree ? "MODE: QUAD" : "MODE: BRUTE");
    });

    fpsLabel.setFont(*font);
    fpsLabel.setCharacterSize(20);
    fpsLabel.setFillColor(sf::Color::Black);
    fpsLabel.setPosition(15.f, 10.f);
    fpsLabel.setString("FPS: 0");

    layoutUI();
    initializeObjects();
}

void MainScreen::layoutUI() {
    float marginRight = game->width / 100;
    float charSize = std::min(50.f, std::max(25.f, game->width / 30.f));
    float textBoxWidth = std::min(100.f, std::max(50.f, game->width / 10.f));

    for (int i = 0; i < 3; i++) {
        textboxes[i].setTextFormat(sf::Color::Black, charSize);
        textboxes[i].setSize(sf::Vector2f(textBoxWidth, charSize));
        textboxes[i].setPosition(sf::Vector2f(game->width - textBoxWidth - marginRight, game->height * 0.075f * i));
        labels[i].setCharacterSize(charSize);
        labels[i].setFillColor(sf::Color::Black);
        labels[i].setPosition({
            textboxes[i].getGlobalBounds().left - labels[i].getGlobalBounds().width - 10.f,
            textboxes[i].getPosition().y
        });
    }

    for (int i = 0; i < buttons.size(); i++) {
        buttons[i].setBorder(sf::Color::Black, 2);
        buttons[i].setFont(*font);
        buttons[i].setCharacterSize(charSize);
        buttons[i].setTextColor(sf::Color::Black);
        buttons[i].setBackgroundColor(sf::Color(220, 220, 220));
        buttons[i].setPosition({
            game->width - marginRight - buttons[i].getGlobalBounds().width / 2,
            game->height * 0.35f + (game->height * 0.1f * (i + 1))
        });
    }
}

void MainScreen::initializeObjects() {
    myObjects.clear();
    quadTree.setData(boundary, 4);

    for (int i = 0; i < objectNum; i++) {
        Particle particle(radius);
        particle.setPosition(sf::Vector2f(
            rand() % static_cast<int>(boundary.width),
            rand() % static_cast<int>(boundary.height)));
        particle.setVelocity(sf::Vector2f(
            rand() % static_cast<int>(particleSpeed) - particleSpeed / 2,
            rand() % static_cast<int>(particleSpeed) - particleSpeed / 2));
        particle.setColor(sf::Color::Green);
        myObjects.push_back(particle);
    }
}

void MainScreen::update(const float dt) {
    for (auto& textbox : textboxes)
        textbox.update(game->window);

    if (!pause) {
        quadTree.reset();

        for (auto& p : myObjects) {
            p.update(dt, boundary);
            p.setColor(sf::Color::Green);
            if (useQuadTree)
                quadTree.insert(&p);
        }

        if (useQuadTree) {
            std::vector<Particle*> found;
            for (auto& p : myObjects) {
                found.clear();
                quadTree.query(p.getGlobalBounds(), found);
                for (auto* other : found) {
                    if (&p != other && Collision::ParticleCollision(p, *other)) {
                        p.setColor(sf::Color::Red);
                        other->setColor(sf::Color::Red);
                    }
                }
            }
        } else {
            for (size_t i = 0; i < myObjects.size(); i++) {
                for (size_t j = i + 1; j < myObjects.size(); j++) {
                    if (Collision::ParticleCollision(myObjects[i], myObjects[j])) {
                        myObjects[i].setColor(sf::Color::Red);
                        myObjects[j].setColor(sf::Color::Red);
                    }
                }
            }
        }
    }

    frameCounter++;
    if (fpsTimer.getElapsedTime().asSeconds() >= 1.0f) {
        fpsLabel.setString("FPS: " + std::to_string(frameCounter));
        frameCounter = 0;
        fpsTimer.restart();
    }
}

void MainScreen::draw() {
    for (auto& p : myObjects)
        p.render(game->window);

    for (size_t i = 0; i < textboxes.size(); i++) {
        game->window->draw(labels[i]);
        textboxes[i].draw(game->window);
    }

    for (auto& button : buttons)
        button.render(game->window);

    game->window->draw(fpsLabel);
}

void MainScreen::handleInput() {
    sf::Event event;
    while (game->window->pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            game->window->close();

        for (auto& textbox : textboxes)
            textbox.handleInput(event);

        for (auto& button : buttons) {
            button.handleInput(event);
            button.update(game->window);

            if (button.isMouseOver() &&
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                button.getOnAction()();
            }
        }
    }
}
