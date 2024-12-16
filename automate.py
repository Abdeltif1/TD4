import copy
import random
import ipaddress
from vlan import VLAN
from host import Host
from ip_generator import IPGenerator
from mac import Mac
from route import Route
from ingress_intf import Ingress_Interface
from egress_intf import Egress_Interface
from config import MAX_PORT_144, PORT_NUMBERS

class NetworkConfigurator:
    def __init__(self, adjacency_matrix_file):
        self.adjacency_matrix = self.read_adjacency_matrix(adjacency_matrix_file)
        self.mac_generator = Mac()
        
    @staticmethod
    def ip_to_hex(ip_address_str):
        ip_address = ipaddress.IPv4Address(ip_address_str)
        return hex(int(ip_address))

    @staticmethod
    def read_adjacency_matrix(file_path):
        print("Reading adjacency matrix from file...")
        with open(file_path, "r") as file:   
            return [[int(x) for x in line.split()] for line in file]

    def write_to_file(self, code, file_path):
        with open(file_path, "a") as file:
            file.write(code)

    def create_vlans_for_switches(self):
        for i in range(1, len(self.adjacency_matrix) + 1):
            VLAN(i + 1)
            print(f"Vlan {i + 1} created")

    def assign_ports_to_nodes(self):
        matrix = copy.deepcopy(self.adjacency_matrix)
        total_links = len(matrix)
        port_index = 0
        
        for i in range(len(matrix)):
            for j in range(len(matrix[i])):
                if matrix[i][j] == 1:
                    matrix[i][j] = PORT_NUMBERS[port_index]
                    port_index = (port_index + 1) % len(PORT_NUMBERS)
        return matrix

    def add_links_to_switches(self, matrix_with_ports):
        for i in range(len(matrix_with_ports)):
            for j in range(len(matrix_with_ports[i])):
                if matrix_with_ports[i][j] != 0 and not isinstance(matrix_with_ports[i][j], Route):
                    self._create_route_pair(matrix_with_ports, i, j)
        return matrix_with_ports

    def _create_route_pair(self, matrix, i, j):
        port1, port2 = matrix[j][i], matrix[i][j]
        
        # First switch
        route1 = self._create_single_route(j + 2, i + 2, port2)
        matrix[i][j] = route1

        # Second switch
        route2 = self._create_single_route(i + 2, j + 2, port1, route1.router_mac_destination)
        matrix[j][i] = route2

    def _create_single_route(self, vrf, other_vrf, port, dest_mac=None):
        vlan = self._get_vlan_by_id(vrf)
        vlan.add_port(port)
        
        router_mac_out = self.mac_generator.generate_random_mac()
        router_mac_destination = dest_mac or self.mac_generator.generate_random_mac()
        router_mac_in = self.mac_generator.generate_random_mac()
        
        subnet_ip = f"10.{vrf}.{vrf}.1"
        ingress_intf = Ingress_Interface().last_ingress_interface
        egress_intf = Egress_Interface().last_egress_interface
        
        return Route(router_mac_out, router_mac_destination, router_mac_in, 
                    vrf, other_vrf, subnet_ip, egress_intf, port, ingress_intf)

    @staticmethod
    def _get_vlan_by_id(vlan_id):
        return next((vlan for vlan in VLAN.vlans if vlan.vlan_id == vlan_id), None)

    def create_l3_ingress_intf(self, matrix_with_routes):
        code = ""
        for i in range(len(matrix_with_routes)):
            for j in range(len(matrix_with_routes[i])):
                if isinstance(matrix_with_routes[i][j], Route) and not matrix_with_routes[i][j].is_created:
                    code += self._generate_route_code(matrix_with_routes, i, j)
        self.write_to_file(code, "automate.c")

    def _generate_route_code(self, matrix, i, j):
        route1, route2 = matrix[i][j], matrix[j][i]
        code = self._generate_mac_declarations(i, j, route1, route2)
        code += self._generate_route_configuration(route1, route2)
        route1.set_created()
        route2.set_created()
        return code

def main():
    network = NetworkConfigurator("adjacency_matrix.txt")
    network.create_vlans_for_switches()
    
    matrix_with_ports = network.assign_ports_to_nodes()
    matrix_with_routes = network.add_links_to_switches(matrix_with_ports)
    
    # Configure network components
    network.create_l3_ingress_intf(matrix_with_routes)
    
    # Create and configure hosts
    create_hosts(network.mac_generator)
    add_hosts_to_switches()
    
    # Save configurations
    Host.save_all_macs()
    Host.save_all_ports_vlans()
    create_egress_ids_file()

if __name__ == "__main__":
    main()
