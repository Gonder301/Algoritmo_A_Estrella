#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <queue>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

#define M_PI 3.141592
using json = nlohmann::json;

struct Nodo {
    const long long id;
    sf::CircleShape circulo;
    const sf::Vector2f pos;
    std::vector<Nodo*> vecinos;

    // Campos para el algoritmo A*
    Nodo* predecesor;
    float h; // Costo real desde el nodo inicial
    float g; // Costo estimado hasta llegar al nodo final

    Nodo(long long id_, sf::Vector2f pos_);
};

Nodo::Nodo(long long id_, sf::Vector2f pos_) : id(id_), pos(pos_), predecesor(nullptr),
h(std::numeric_limits<float>::infinity()), g(0.f) {
    circulo.setPosition(pos);
    circulo.setFillColor(sf::Color::Red);
    circulo.setRadius(5.f);
    circulo.move(sf::Vector2f(-circulo.getRadius(), -circulo.getRadius()));
}


struct CompararNodo {
    bool operator()(const Nodo* a, const Nodo* b) const {
        return (a->h + a->g) < (b->h + b->g);
    }
};

struct Arco {
    const float distancia;
    sf::VertexArray linea;

    Arco(sf::Vector2f pos1_, sf::Vector2f pos2_, float distancia_);
};

Arco::Arco(sf::Vector2f pos1_, sf::Vector2f pos2_, float distancia_) : distancia(distancia_), linea(sf::Lines, 2) {
    linea[0].color = sf::Color(127, 127, 127);
    linea[0].position = pos1_;
    linea[1].color = sf::Color(127, 127, 127);
    linea[1].position = pos2_;
}

std::vector<Nodo*> nodos;
std::vector<Arco*> arcos;
std::priority_queue<Nodo*, std::vector<Nodo*>, CompararNodo> nodosPorVisitar;
std::vector<Nodo*> nodosVisitados; // ???

bool mouseDentroDeCirculo(const sf::CircleShape& circle, sf::Vector2f mousePosition) {
    sf::Vector2f circleCenter = circle.getPosition() + sf::Vector2f(circle.getRadius(), circle.getRadius());

    float dx = mousePosition.x - circleCenter.x;
    float dy = mousePosition.y - circleCenter.y;
    float squaredDistance = dx * dx + dy * dy;

    // Calculate the squared radius of the circle
    float squaredRadius = circle.getRadius() * circle.getRadius();

    return squaredDistance <= squaredRadius;
}

void mostrarNodoYVecinos(sf::Vector2f mousePos, std::vector<Nodo*>& nodosMostrar_) {
    if (!nodosMostrar_.empty()) {
        nodosMostrar_[0]->circulo.setFillColor(sf::Color::Red);
        nodosMostrar_.clear();
    }

    for (const auto& it : nodos) {
        if (mouseDentroDeCirculo(it->circulo, mousePos)) {
            it->circulo.setFillColor(sf::Color::Blue);
            std::cout << "Nodo seleccionado: " << it->id << std::endl;
            nodosMostrar_.push_back(it);
            break;
        }
    }
    std::vector<Nodo*> vecinosNodo;
    if (!nodosMostrar_.empty()) {
        vecinosNodo = nodosMostrar_.back()->vecinos;
        for (const auto& it : vecinosNodo) {
            nodosMostrar_.push_back(it);
        }
    }
    else {
        std::cout << "No se selecciono ningun nodo" << std::endl;
    }
}

int main() {
    std::ifstream file("graph_data_v3.json");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open graph_data.json" << std::endl;
        return 1;
    }

    json graph_data;

    try {
        file >> graph_data;
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return 1;
    }
    file.close();

    // Primero nodo se posiciona en el origen de la ventana SFML (0, 0)
    auto primer_nodo_it = graph_data["nodos"].begin();
    double primera_lat = primer_nodo_it.value()["lat"];
    double primera_lon = primer_nodo_it.value()["lon"];

    const double metrosPorGradoLat = 111319.0;
    const double metrosPorGradoLon = 111319.0 * std::cos(primera_lat * M_PI / 180.0);

    std::cout << "Generando nodos... ";
    // Variables para crear el marco del grafo
    float minX = 0.f, minY = 0.f, maxX = 0.f, maxY = 0.f; // Como el primer nodos estará en (0, 0), todos estos valores se inicializan a 0.

    for (auto nodo_it = graph_data["nodos"].begin(); nodo_it != graph_data["nodos"].end(); ++nodo_it) {
        try {
            long long node_id = stoll(nodo_it.key());
            double lon = nodo_it.value()["lon"];
            double lat = nodo_it.value()["lat"];
            double x = (lon - primera_lon) * metrosPorGradoLon;
            double y = -(lat - primera_lat) * metrosPorGradoLat;  // Invertir coordenada 'y'. En SFML 'y' va de arriba hacia abajo.

            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;

            nodos.push_back(new Nodo(node_id, sf::Vector2f(x, y)));
        }
        catch (std::exception& e) {
            std::cerr << "Error processing node: " << nodo_it.key() << " - " << e.what() << std::endl;
            return 1;
        }
    }
    std::cout << nodos.size() << " nodos generados." << std::endl;

    sf::RectangleShape marcoGrafo(sf::Vector2f((maxX - minX) * 1.1, (maxY - minY) * 1.1)); // 10% más grande
    marcoGrafo.setPosition(minX - ((marcoGrafo.getSize().x - (maxX - minX)) / 2), minY - ((marcoGrafo.getSize().y - (maxY - minY)) / 2));
    marcoGrafo.setOutlineThickness(50);
    marcoGrafo.setOutlineColor(sf::Color::White);
    marcoGrafo.setFillColor(sf::Color::Transparent);
    sf::Vector2f marcoGrafoCentro(marcoGrafo.getPosition().x + (marcoGrafo.getSize().x / 2), marcoGrafo.getPosition().y + (marcoGrafo.getSize().y / 2));

    std::cout << "Generando arcos... ";
    for (auto& arco_it : graph_data["arcos"]) {
        try {
            Nodo* src_ptr = nullptr;
            long long src_id = arco_it["primero"];

            Nodo* tgt_ptr = nullptr;
            long long tgt_id = arco_it["segundo"];

            for (auto const& it : nodos) {
                if (!src_ptr) {
                    if (src_id == it->id) {
                        src_ptr = it;
                    }
                }

                if (!tgt_ptr) {
                    if (tgt_id == it->id) {
                        tgt_ptr = it;
                    }
                }

                if (src_ptr && tgt_ptr) break;
            }
            // Se añaden como vecinos entre si
            if (src_ptr != nullptr && tgt_ptr != nullptr) {
                src_ptr->vecinos.push_back(tgt_ptr);
                tgt_ptr->vecinos.push_back(src_ptr);
            }
            else {
                std::cout << "Error: No se encontraron los nodos al crear arco." << std::endl;
                return -1;
            }

            // Calcula la distancia Euclideana entre ambos nodos en SFML
            float dx = tgt_ptr->pos.x - src_ptr->pos.x;
            float dy = tgt_ptr->pos.y - src_ptr->pos.y;
            float distancia = sqrt(dx * dx + dy * dy);

            arcos.push_back(new Arco(src_ptr->pos, tgt_ptr->pos, distancia));
        }
        catch (std::exception& e) {
            std::cerr << "Error processing edge: " << e.what() << std::endl;
            return 1;
        }
    }
    std::cout << arcos.size() << " arcos generados." << std::endl;

    bool generandoArcos = false;

    bool ctrlPresionado = false;
    std::vector<Nodo*> nodosMostrar;

    bool mostrarNodos = false;
    int nroArcosDibujar = 0;
    const float tiempoGenerarGrafo = 4.f;
    float tiempoGenerarArco = tiempoGenerarGrafo / arcos.size();
    float deltaArco = 0.f;

    sf::RenderWindow window(sf::VideoMode(800, 600), "Grafo");
    sf::Vector2f viewSize(marcoGrafo.getSize().x * 1.1f, marcoGrafo.getSize().y * 1.1f);

    float aspectRatio = viewSize.x / viewSize.y;
    float windowAspect = 800.f / 600.f;
    if (aspectRatio > windowAspect) {
        viewSize.y = viewSize.x / windowAspect;
    }
    else {
        viewSize.x = viewSize.y * windowAspect;
    }
    window.setView(sf::View(marcoGrafoCentro, viewSize));

    bool moving = false;
    sf::Vector2f oldPos;
    sf::View view = window.getView();
    sf::Vector2f lastMousePos;
    float viewVelocidad = 300.f;

    sf::Clock clock;
    float delta = 0.f;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {

            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;

            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == 0) {
                    if (!ctrlPresionado) {
                        moving = true;
                        oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                    }
                    else {
                        mostrarNodoYVecinos(window.mapPixelToCoords(sf::Mouse::getPosition(window)), nodosMostrar);
                    }
                }
                break;

            case sf::Event::MouseButtonReleased:
                if (event.mouseButton.button == 0) {
                    moving = false;
                }
                break;

            case sf::Event::MouseMoved:
            {
                if (!moving) break;

                // Determina la nueva posición en las coordenadas de la ventana.
                const sf::Vector2f newPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
                // Determina el cambio de posición del cursor.
                // Multiplicar por -1 si se quiere invertir la dirección del cambio.
                const sf::Vector2f deltaPos = oldPos - newPos;

                view.setCenter(view.getCenter() + deltaPos);
                window.setView(view);

                // Se recalcula la nueva posición ya que se cambió view y se guarda como la anterior.
                oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
                break;
            }

            case sf::Event::MouseWheelScrolled:
                if (event.mouseWheelScroll.delta > 0)
                    view.zoom(0.9f);  // Zoom in
                else
                    view.zoom(1.1f);  // Zoom out
                break;

            case sf::Event::KeyReleased:
                if (event.key.code == sf::Keyboard::G) {
                    generandoArcos = true;
                    std::cout << "\nGenerando grafo...";
                    nroArcosDibujar = 0;
                }
                if (event.key.code == sf::Keyboard::N) {
                    mostrarNodos = !mostrarNodos;
                }
            }

        }

        delta = clock.restart().asSeconds();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
            ctrlPresionado = true;
        }
        else {
            ctrlPresionado = false;
        }

        window.clear(sf::Color::Black);
        window.setView(view);

        if (generandoArcos) {
            if (nroArcosDibujar < arcos.size()) {
                deltaArco += delta;
                if (deltaArco >= tiempoGenerarArco) {
                    nroArcosDibujar += deltaArco / tiempoGenerarArco;
                    if (nroArcosDibujar >= arcos.size()) {
                        if (nroArcosDibujar != arcos.size()) nroArcosDibujar = arcos.size();
                        std::cout << " grafo generado." << std::endl;
                        generandoArcos = false;
                    }
                    deltaArco = std::fmod(deltaArco, tiempoGenerarArco);
                }
            }
            else {
                generandoArcos = false;
                std::cout << " grafo generado." << std::endl;
            }
        }

        window.draw(marcoGrafo);

        for (int i = 0; i != nroArcosDibujar; i++) {
            window.draw(arcos[i]->linea);
        }

        if (mostrarNodos) {
            for (const auto& it : nodos) {
                window.draw(it->circulo);
            }
        }

        if (!nodosMostrar.empty()) {
            for (const auto& it : nodosMostrar) {
                window.draw(it->circulo);
            }
        }

        window.display();
    }

    return 0;
}
