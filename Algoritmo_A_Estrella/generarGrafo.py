import osmnx as ox
import networkx as nx
from geopy.distance import geodesic
import json
import sys

arg1_icicio_coord_lat = float(sys.argv[1])
arg2_icicio_coord_lon = float(sys.argv[2])
arg3_destino_coord_lat = float(sys.argv[3])
arg4_destino_coord_lon = float(sys.argv[4])

inicio_coord = (arg1_icicio_coord_lat, arg2_icicio_coord_lon)
destino_coord = (arg3_destino_coord_lat, arg4_destino_coord_lon)

centro_grafo = (
    (inicio_coord[0] + destino_coord[0]) / 2,
    (inicio_coord[1] + destino_coord[1]) / 2
)

distancia = (geodesic(inicio_coord, destino_coord).meters / 2) * 1.5

grafo = ox.graph_from_point(centro_grafo, distancia, network_type="drive")

inicio_nodo = ox.nearest_nodes(grafo, inicio_coord[1], inicio_coord[0])  # lon, lat
destino_nodo = ox.nearest_nodes(grafo, destino_coord[1], destino_coord[0])  # lon, lat

# Función del módulo networkx que devuelve una lista de los nodos del camino más corto usando A*
# path = nx.astar_path(grafo, inicio_nodo, destino_nodo, weight="length")

# Muestra y guarda una imagen del grafo y el camino más corto entre inicio y fin usando el algoritmo A*
# fig, ax = ox.plot_graph_route(grafo, path, route_linewidth=6, route_color='b', node_size=0)
# fig.savefig("grafo.png", dpi=300, bbox_inches="tight")

# Convierte ox.MultiDiGraph a nx.Graph
grafo = nx.Graph(grafo)

grafo_data = {
    "inicio": inicio_nodo,
    "destino": destino_nodo,
    "nodos": {node: {"lat": data["y"], "lon": data["x"]} for node, data in grafo.nodes(data=True)},
    "arcos": [
        {"primero": u, "segundo": v}
        for u, v, data in grafo.edges(data=True)
    ],
    # "path": path
}

# Guardar datos en JSON
with open("grafo_data.json", "w") as f:
    json.dump(grafo_data, f, indent=4)
