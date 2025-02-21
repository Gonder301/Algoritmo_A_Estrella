#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <queue>
#include <set>
#include <unordered_set>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

#include <conio.h>

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
h(0.f), g(std::numeric_limits<float>::infinity()) {
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
    sf::VertexArray linea;

    Arco(sf::Vector2f pos1_, sf::Vector2f pos2_, const sf::Color& color_);
};

Arco::Arco(sf::Vector2f pos1_, sf::Vector2f pos2_, const sf::Color& color_) : linea(sf::Lines, 2) {
    linea[0].color = color_;
    linea[0].position = pos1_;
    linea[1].color = color_;
    linea[1].position = pos2_;
}

std::vector<Nodo*> nodos;
std::vector<Arco*> arcos;
sf::RectangleShape marcoGrafo;

// Variables para el algoritmo A*
Nodo* nodoInicio = nullptr;
Nodo* nodoFinal = nullptr;
std::set<Nodo*, CompararNodo> nodosPorVisitar;
std::unordered_set<Nodo*> nodosVisitados;
std::vector<Arco*> camino;

float dist(sf::Vector2f pos1, sf::Vector2f pos2) {
    return std::sqrt(((pos2.x - pos1.x) * (pos2.x - pos1.x)) + ((pos2.y - pos1.y) * (pos2.y - pos1.y)));
}

bool algoritm_A_Estrella() {
    if (nodosPorVisitar.empty()) {
        std::cout << "No se encontró ningún camino entre los nodos." << std::endl;
        return false;
    }

    Nodo* n = *nodosPorVisitar.begin();  // Tomar el nodo con menor costo f = g + h
    nodosPorVisitar.erase(nodosPorVisitar.begin());  // Eliminarlo del conjunto
    nodosVisitados.insert(n);  // Marcarlo como visitado

    float temp_g = 0.f;
    for (auto& it : n->vecinos) {
        if (nodosVisitados.count(it)) continue;  // Si ya fue visitado, ignorarlo

        temp_g = n->g + dist(it->pos, n->pos);
        if (temp_g < it->g) {
            // Se actualiza solo si encontramos un mejor camino
            nodosPorVisitar.erase(it);  // IMPORTANTE: eliminarlo antes de modificar `g`
            it->g = temp_g;
            it->predecesor = n;
            camino.push_back(new Arco(n->pos, it->pos, sf::Color::Magenta));
            nodosPorVisitar.insert(it);  // Reinsertar con el nuevo costo
        }

        if (it->id == nodoFinal->id) {
            std::cout << "Bingo!" << std::endl;
            return false;  // Terminamos porque encontramos el nodo final
        }
    }

    return true;
}

// Inicializa los valores h de TODOS los nodos del grafo. No recomendado...
void inicializarHeuristicos(const sf::Vector2f& nodoFinalPos) {
    for (auto& it : nodos) {
        it->h = dist(it->pos, nodoFinalPos);
    }
}

bool mouseEnCircle(const sf::CircleShape& circle, sf::Vector2f mousePos) {
    sf::Vector2f circleCenter = circle.getPosition() + sf::Vector2f(circle.getRadius(), circle.getRadius());

    float dx = mousePos.x - circleCenter.x;
    float dy = mousePos.y - circleCenter.y;
    float squaredDistance = dx * dx + dy * dy;

    float squaredRadius = circle.getRadius() * circle.getRadius();

    return squaredDistance <= squaredRadius;
}

void seleccionarNodo(sf::Vector2f mousePos, Nodo*& nodoPtr, const sf::Color& color) {
    for (const auto& it : nodos) {
        if (mouseEnCircle(it->circulo, mousePos)) {
            nodoPtr = it;
            nodoPtr->circulo.setFillColor(color);
            return;
        }
    }
    std::cout << "No se selecciono ningun nodo." << std::endl;
}

void cargarArchivoJSON(json& graph_data, const char nombre[]) {
    std::ifstream file(nombre);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir " << nombre << std::endl;
        return;
    }

    try {
        file >> graph_data;
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return;
    }

    file.close();
}

void leerArcos(json& graph_data) {
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
                return;
            }

            arcos.push_back(new Arco(src_ptr->pos, tgt_ptr->pos, sf::Color(127, 127, 127)));
        }
        catch (std::exception& e) {
            std::cerr << "Error processing edge: " << e.what() << std::endl;
            return;
        }
    }
    std::cout << arcos.size() << " arcos generados." << std::endl;
}

void leerNodos(json& graph_data) {

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
            return;
        }
    }
    std::cout << nodos.size() << " nodos generados." << std::endl;

    marcoGrafo.setSize(sf::Vector2f((maxX - minX) * 1.1, (maxY - minY) * 1.1)); // 10% más grande
    marcoGrafo.setPosition(minX - ((marcoGrafo.getSize().x - (maxX - minX)) / 2), minY - ((marcoGrafo.getSize().y - (maxY - minY)) / 2));
    marcoGrafo.setOutlineThickness(50);
    marcoGrafo.setOutlineColor(sf::Color::White);
    marcoGrafo.setFillColor(sf::Color::Transparent);

}

void inicializarView(sf::RenderWindow& window, sf::RectangleShape marcoGrafo) {
    sf::Vector2f viewSize(marcoGrafo.getSize().x * 1.1f, marcoGrafo.getSize().y * 1.1f);

    float aspectRatio = viewSize.x / viewSize.y;
    float windowAspect = 800.f / 600.f;
    if (aspectRatio > windowAspect) {
        viewSize.y = viewSize.x / windowAspect;
    }
    else {
        viewSize.x = viewSize.y * windowAspect;
    }
    window.setView(sf::View(sf::Vector2f(marcoGrafo.getPosition().x + (marcoGrafo.getSize().x / 2), marcoGrafo.getPosition().y + (marcoGrafo.getSize().y / 2)), viewSize));

}

int main() {
    json graph_data;

    cargarArchivoJSON(graph_data, "graph_data_v3.json");
    leerNodos(graph_data);
    leerArcos(graph_data);

    bool generandoArcos = false;
    bool algoritmoActivo = false;
    const int nroNodosPorLoop = 10;
    bool ctrlPresionado = false;
	bool altPresionado = false;
    bool usandoHeuristico = true;
    bool mostrarNodos = false;

    int nroArcosDibujar = 0;
    const float tiempoGenerarGrafo = 2.f;
    float tiempoGenerarArco = tiempoGenerarGrafo / arcos.size();
    float deltaArco = 0.f;

    sf::RenderWindow window(sf::VideoMode(800, 600), "Grafo");
	inicializarView(window, marcoGrafo);

    // Variables para ajustar la vista al arrastrar el puntero.
    bool moving = false;
    sf::Vector2f oldPos;
    sf::View view = window.getView();
    sf::Vector2f lastMousePos;

    // temp
    nodoInicio = nodos[100];
    nodoInicio->circulo.setFillColor(sf::Color::Blue);
    nodoFinal = nodos[10];
    nodoFinal->circulo.setFillColor(sf::Color::Green);

    sf::Clock clock;
    float delta = 0.f;
    float tiempoAlgoritmo = 0.f;
    std::cout << "Algoritmo activo: " << ((usandoHeuristico) ? "A*" : "Dijkstra") << std::endl;

    while (window.isOpen()) {
        sf::Event event;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
            ctrlPresionado = true;
        }
        else {
            ctrlPresionado = false;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) {
            altPresionado = true;
        }
        else {
            altPresionado = false;
        }

        while (window.pollEvent(event)) {

            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;

            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == 0) {
                    if (ctrlPresionado) {
                        seleccionarNodo(window.mapPixelToCoords(sf::Mouse::getPosition(window)), nodoInicio, sf::Color::Blue);
                    }
                    else if (altPresionado) {
                        seleccionarNodo(window.mapPixelToCoords(sf::Mouse::getPosition(window)), nodoFinal, sf::Color::Green);
                    }
                    else {
                        moving = true;
                        oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
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
                else if (event.key.code == sf::Keyboard::N) {
                    mostrarNodos = !mostrarNodos;
                    if (mostrarNodos) {
                        std::cout << "Mostrando todos los nodos." << std::endl;
                    }
                    else {
                        std::cout << "Ocultando todos los nodos." << std::endl;
                    }
                }
                else if (event.key.code == sf::Keyboard::S) {
                    if (nodoInicio == nullptr || nodoFinal == nullptr) {
                        std::cout << "Seleccione los nodos de inicio y final." << std::endl;
                    }
                    else {
                        if (usandoHeuristico) {
                            inicializarHeuristicos(nodoFinal->pos);
                        }
                        nodoInicio->g = 0;
                        nodosPorVisitar.insert(nodoInicio);
                        algoritmoActivo = true;
                    }
                }
                else if (event.key.code == sf::Keyboard::Tab) {
                    usandoHeuristico = !usandoHeuristico;
                    std::cout << "Algoritmo activo: " << ((usandoHeuristico) ? "A*" : "Dijkstra") << std::endl;
                }
            }

        }

        delta = clock.restart().asSeconds();
        if (algoritmoActivo) {
            tiempoAlgoritmo += delta;
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

        if (algoritmoActivo) {
            for (int i = nroNodosPorLoop; i != 0; --i) {
                if (!(algoritmoActivo = algoritm_A_Estrella())) {
                    std::cout << "Tiempo de ejecucion: " << tiempoAlgoritmo << std::endl;
                    break;
                }
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

        if (nodoInicio != nullptr) {
            window.draw(nodoInicio->circulo);
        }
        if (nodoFinal != nullptr) {
            window.draw(nodoFinal->circulo);
        }

        for (const auto& it : camino) {
            window.draw(it->linea);
        }

        window.display();
    }

    return 0;
}
