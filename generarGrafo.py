import osmnx as ox
import networkx as nx
from geopy.distance import geodesic
import json

inicio_coord = (-12.051392, -77.085341)
destino_coord = (-12.065006, -77.039207)

centro_grafo = (
    (inicio_coord[0] + destino_coord[0]) / 2,
    (inicio_coord[1] + destino_coord[1]) / 2
)

distancia = (geodesic(inicio_coord, destino_coord).meters / 2) * 1.5

grafo = ox.graph_from_point(centro_grafo, distancia, network_type="drive")

inicio_nodo = ox.nearest_nodes(grafo, inicio_coord[1], inicio_coord[0])  # lon, lat
destino_nodo = ox.nearest_nodes(grafo, destino_coord[1], destino_coord[0])  # lon, lat

path = nx.astar_path(grafo, inicio_nodo, destino_nodo, weight="length")

fig, ax = ox.plot_graph_route(grafo, path, route_linewidth=6, route_color='b', node_size=0)
fig.savefig("grafo.png", dpi=300, bbox_inches="tight")

# Convierte ox.MultiDiGraph a nx.Graph
grafo = nx.Graph(grafo)

graph_data = {
    "nodos": {node: {"lat": data["y"], "lon": data["x"]} for node, data in grafo.nodes(data=True)},
    "arcos": [
        {"primero": u, "segundo": v}
        for u, v, data in grafo.edges(data=True)
    ],
    "path": path
}

# Guardar datos en JSON
with open("graph_data_v3.json", "w") as f:
    json.dump(graph_data, f, indent=4)
