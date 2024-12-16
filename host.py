import os

# Class host to represent a host
class Host:
    all_hosts = []

    def __init__(self, name, port, host_mac, router_mac_out, router_mac_in, ip, vlan, vrf, egress_id, ingress_id):
        self.name = name
        self.host_mac = host_mac
        self.router_mac_out = router_mac_out
        self.router_mac_in = router_mac_in
        self.ip = ip
        self.vlan = vlan
        self.egress_id = egress_id
        self.port = port
        self.vrf = vrf
        self.ing_intf_id = ingress_id
        self.__class__.all_hosts.append(self)
        
        # Log the creation of the host
        self.log_creation()

    def log_creation(self):
        log_dir = "host_logs"
        if not os.path.exists(log_dir):
            os.makedirs(log_dir)

        log_file = os.path.join(log_dir, f"{self.name}_log.txt")
        with open(log_file, "a") as f:
            f.write(f"Host {self.name} created.\n")
            f.write(f"Host MAC: {self.host_mac}\n")
            f.write(f"Router MAC (out): {self.router_mac_out}\n")
            f.write(f"Router MAC (in): {self.router_mac_in}\n")
            f.write(f"IP: {self.ip}\n")
            f.write(f"VLAN: {self.vlan}\n")
            f.write(f"Egress ID: {self.egress_id}\n")
            f.write(f"Port: {self.port}\n")
            f.write(f"VRF: {self.vrf}\n")
            f.write(f"Ingress Interface ID: {self.ing_intf_id}\n")
            f.write("\n")

    @classmethod
    def save_all_macs(cls):
        mac_file = "all_macs.txt"
        with open(mac_file, "w") as f:
            for host in cls.all_hosts:
                f.write(f"{host.router_mac_in}\n")

    #function to save all host ports and vlan in one file to be later retrieved as an array
    @classmethod
    def save_all_ports_vlans(cls):
        port_vlan_file = "all_ports_vlans.txt"
        with open(port_vlan_file, "w") as f:
            for host in cls.all_hosts:
                f.write(f"{host.port} {host.vlan}\n")
        

    def __str__(self):
        return f"Host {self.name} with MAC {self.host_mac} and IP {self.ip} and VLAN {self.vlan} and port {self.port} connected to router with MAC {self.router_mac_out}"
