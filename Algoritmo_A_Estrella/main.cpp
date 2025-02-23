#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_set>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <string>

#define M_PI 3.141592
using json = nlohmann::json;

struct Nodo {
    const long long id;
    sf::CircleShape circulo;
    const sf::Vector2f pos;
    std::vector<Nodo*> vecinos;

    // Campos para el algoritmo A*
    Nodo* predecesor;
    float g; // Costo real desde el nodo inicial
    float h; // Costo estimado hasta llegar al nodo final

    Nodo(long long id_, sf::Vector2f pos_);
};

Nodo::Nodo(long long id_, sf::Vector2f pos_) : id(id_), pos(pos_), predecesor(nullptr),
h(0.f), g(std::numeric_limits<float>::infinity()) {
    circulo.setPosition(pos);
    circulo.setFillColor(sf::Color::Red);
    circulo.setRadius(25.f);
    circulo.move(sf::Vector2f(-circulo.getRadius(), -circulo.getRadius()));
}

struct CompararNodo {
    bool operator()(const Nodo* a, const Nodo* b) const {
        return (a->h + a->g) < (b->h + b->g);
    }
};

sf::VertexArray* crearArco(sf::Vector2f pos1_, sf::Vector2f pos2_, const sf::Color& color_) {
    sf::VertexArray* nuevo = new sf::VertexArray(sf::Lines, 2);
    (*nuevo)[0].color = color_;
    (*nuevo)[0].position = pos1_;
    (*nuevo)[1].color = color_;
    (*nuevo)[1].position = pos2_;
    return nuevo;
}

std::vector<Nodo*> nodos;
std::vector<sf::VertexArray*> arcos;
sf::RectangleShape marcoGrafo;

// Variables para el algoritmo A*
Nodo* nodoInicio = nullptr;
Nodo* nodoDestino = nullptr;
std::set<Nodo*, CompararNodo> nodosPorVisitar;
std::unordered_set<Nodo*> nodosVisitados;
std::vector<sf::VertexArray*> arcosRecorridos;
std::vector<sf::VertexArray*> camino;

float distanciaEuclidiana(sf::Vector2f pos1, sf::Vector2f pos2) {
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

        temp_g = n->g + distanciaEuclidiana(it->pos, n->pos);
        if (temp_g < it->g) {
            // Se actualiza solo si encontramos un mejor camino
            nodosPorVisitar.erase(it);  // IMPORTANTE: eliminarlo antes de modificar `g`
            it->g = temp_g;
            it->predecesor = n;
            arcosRecorridos.push_back(crearArco(n->pos, it->pos, sf::Color::Magenta));
            nodosPorVisitar.insert(it);  // Reinsertar con el nuevo costo
        }

        if (it->id == nodoDestino->id) {
            std::cout << "Bingo!" << std::endl;
            return false;  // Terminamos porque encontramos el nodo final
        }
    }

    return true;
}

void liberarMemoriaAlgoritmo() {
    for (auto& it : arcosRecorridos) {
        delete arcosRecorridos.back();
        arcosRecorridos.pop_back();
    }

    for (auto& it : camino) {
        delete camino.back();
        camino.pop_back();
    }
}

void resetear() {
    liberarMemoriaAlgoritmo();
    nodosPorVisitar.clear();
    nodosVisitados.clear();

    for (auto& it : nodos) {
        it->predecesor = nullptr;
        it->g = std::numeric_limits<float>::infinity();
        it->h = 0.f;
    }
}

void generarCamino() {
    Nodo* it = nodoDestino;
    do {
        camino.push_back(crearArco(it->pos, it->predecesor->pos, sf::Color::Magenta));
        it = it->predecesor;
    } while (it->predecesor != nullptr);
}

// Inicializa los valores h de TODOS los nodos del grafo
void inicializarHeuristicos(const sf::Vector2f& nodoFinalPos) {
    for (auto& it : nodos) {
        it->h = distanciaEuclidiana(it->pos, nodoFinalPos);
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

void leerArgs(std::vector<std::string>& args) {
    std::string coord, lat, lon;
    bool primeroLeido = false;

    std::cout << "Inicio: ";
    std::getline(std::cin, coord);
    for (const auto& it : coord) {
        if (!primeroLeido) {
            if (it != ',') {
                lat += it;
            }
            else {
                primeroLeido = true;
            }
        }
        else {
            if (it != ' ') {
                lon += it;
            }
        }
    }
    args[0] = lat;
    args[1] = lon;

    lat.clear();
    lon.clear();
    coord.clear();
    primeroLeido = false;
    std::cout << "Destino: ";
    std::getline(std::cin, coord);
    for (const auto& it : coord) {
        if (!primeroLeido) {
            if (it != ',') {
                lat += it;
            }
            else {
                primeroLeido = true;
            }
        }
        else {
            if (it != ' ') {
                lon += it;
            }
        }
    }
    args[2] = lat;
    args[3] = lon;
}

bool cargarArchivoJSON(json& graph_data, const char nombre[]) {
    std::ifstream file(nombre);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir " << nombre << std::endl;
        return false;
    }

    try {
        file >> graph_data;
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return false;
    }

    file.close();
    return true;
}

bool leerArcos(json& graph_data) {
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
                return false;
            }

            arcos.push_back(crearArco(src_ptr->pos, tgt_ptr->pos, sf::Color(127, 127, 127)));
        }
        catch (std::exception& e) {
            std::cerr << "Error procesando arco: " << e.what() << std::endl;
            return false;
        }
    }
    std::cout << arcos.size() << " arcos generados." << std::endl;
    return true;
}

bool leerNodos(json& graph_data) {
    // Primero nodo se posiciona en el origen de la ventana SFML (0, 0)
    auto primer_nodo_it = graph_data["nodos"].begin();
    double primera_lat = primer_nodo_it.value()["lat"];
    double primera_lon = primer_nodo_it.value()["lon"];

    const double metrosPorGradoLat = 111319.0;
    const double metrosPorGradoLon = 111319.0 * std::cos(primera_lat * M_PI / 180.0);

    long long inicio = graph_data["inicio"], destino = graph_data["destino"];
    bool inicioEncontrado = false, destinoEncontrado = false;

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

            if (!inicioEncontrado) {
                if (inicio == nodos.back()->id) {
                    nodoInicio = nodos.back();
                    inicioEncontrado = true;
                    continue;
                }
            }
            if (!destinoEncontrado) {
                if (destino == nodos.back()->id) {
                    nodoDestino = nodos.back();
                    destinoEncontrado = true;
                    continue;
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << "Error processing node: " << nodo_it.key() << " - " << e.what() << std::endl;
            return false;
        }
    }

    std::cout << nodos.size() << " nodos generados." << std::endl;

    marcoGrafo.setSize(sf::Vector2f((maxX - minX) * 1.1, (maxY - minY) * 1.1)); // 10% más grande
    marcoGrafo.setPosition(minX - ((marcoGrafo.getSize().x - (maxX - minX)) / 2), minY - ((marcoGrafo.getSize().y - (maxY - minY)) / 2));
    marcoGrafo.setOutlineThickness(50);
    marcoGrafo.setOutlineColor(sf::Color::White);
    marcoGrafo.setFillColor(sf::Color::Transparent);
    return true;
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
    /*std::vector<std::string> args(4);
    leerArgs(args);

    char archivoPython[] = "generarGrafo.py";
    std::string command = std::string("python ") + archivoPython + " " + args[0] + " " + args[1] + " " + args[2] + " " + args[3];
    std::cout << "Ejecutando " << archivoPython << "...";
    system(command.c_str());
    std::cout << " listo." << std::endl;*/

    json graph_data;

    if (!cargarArchivoJSON(graph_data, "grafo_data.json")) return -1;
    if (!leerNodos(graph_data)) return -1;
    if (!leerArcos(graph_data)) return -1;

    bool dibujarArcos = false;
    bool algoritmoActivo = false;
    bool algoritmoFinalizado = false;
    const int nroNodosPorLoop = 30; // A mayor valor, al algoritmo se ejecutará más rapido
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

    nodoInicio->circulo.setFillColor(sf::Color::Blue);
    nodoDestino->circulo.setFillColor(sf::Color::Green);

    sf::Clock clock;
    float delta = 0.f;
    float tiempoAlgoritmo = 0.f;
    std::cout << "\nControles:" << std::endl;
    std::cout << "G: mostrar grafo" << std::endl;
    std::cout << "TAB: cambiar algoritmo" << std::endl;
    std::cout << "S: iniciar algoritmo" << std::endl;
    std::cout << "R: reiniciar\n\n";
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
                        seleccionarNodo(window.mapPixelToCoords(sf::Mouse::getPosition(window)), nodoDestino, sf::Color::Green);
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
                    dibujarArcos = true;
                    std::cout << "\nDibujando grafo...";
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
                    if (nodoInicio == nullptr || nodoDestino == nullptr) {
                        std::cout << "Seleccione los nodos de inicio y final." << std::endl;
                    }
                    else {
                        if (usandoHeuristico) {
                            inicializarHeuristicos(nodoDestino->pos);
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
                else if (event.key.code == sf::Keyboard::R) {
                    resetear();
                    algoritmoActivo = false;
                    algoritmoFinalizado = false;
                    tiempoAlgoritmo = 0.f;
                }
            }

        }

        delta = clock.restart().asSeconds();
        if (algoritmoActivo) {
            tiempoAlgoritmo += delta;
        }
        window.clear(sf::Color::Black);
        window.setView(view);

        if (dibujarArcos) {
            if (nroArcosDibujar < arcos.size()) {
                deltaArco += delta;
                if (deltaArco >= tiempoGenerarArco) {
                    nroArcosDibujar += deltaArco / tiempoGenerarArco;
                    if (nroArcosDibujar >= arcos.size()) {
                        if (nroArcosDibujar != arcos.size()) nroArcosDibujar = arcos.size();
                        std::cout << " listo." << std::endl;
                        dibujarArcos = false;
                    }
                    deltaArco = std::fmod(deltaArco, tiempoGenerarArco);
                }
            }
            else {
                dibujarArcos = false;
                std::cout << " grafo generado." << std::endl;
            }
        }

        if (algoritmoActivo) {
            for (int i = nroNodosPorLoop; i != 0; --i) {
                if (!(algoritmoActivo = algoritm_A_Estrella())) {
                    std::cout << "Tiempo de ejecucion: " << tiempoAlgoritmo << std::endl;
                    algoritmoFinalizado = true;
                    generarCamino();
                    break;
                }
            }
        }

        window.draw(marcoGrafo);

        for (int i = 0; i != nroArcosDibujar; i++) {
            window.draw(*arcos[i]);
        }

        if (mostrarNodos) {
            for (const auto& it : nodos) {
                window.draw(it->circulo);
            }
        }

        if (nodoInicio != nullptr) {
            window.draw(nodoInicio->circulo);
        }
        if (nodoDestino != nullptr) {
            window.draw(nodoDestino->circulo);
        }

        if (algoritmoActivo) {
            for (const auto& it : arcosRecorridos) {
                window.draw(*it);
            }
        }

        if (algoritmoFinalizado) {
            for (const auto& it : camino) {
                window.draw(*it);
            }
        }

        window.display();
    }

    return 0;
}