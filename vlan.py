import random
import os



class VLAN:
    # Class attribute to keep track of used VLAN IDs
    used_vlan_ids = set()
    vlans = []
    def __init__(self, vlan_id=None):
        if vlan_id is None:
            self.vlan_id = self.generate_unique_vlan_id()
        else:
            self.vlan_id = vlan_id
        self.ports = []
        VLAN.vlans.append(self)
        # Log the creation of the VLAN
        self.log_creation()
    def generate_unique_vlan_id(self):
        vlan_id = random.randint(1, 4095)
        while vlan_id in VLAN.used_vlan_ids:
            vlan_id = random.randint(1, 4095)
        VLAN.used_vlan_ids.add(vlan_id)
        return vlan_id

    def add_port(self, port):
            self.ports.append(port)
            self.log_port_addition(port)
            # print(f"Port {port} added to VLAN {self.vlan_id}")


    def delete_port(self, port):
        if port in self.ports:
            self.ports.remove(port)
            print(f"Port {port} removed from VLAN {self.vlan_id}")
        else:
            print(f"Port {port} is not a member of VLAN {self.vlan_id}")

    def display_ports(self):
        print(f"Ports belonging to VLAN {self.vlan_id}:")
        for port in self.ports:
            print(port)

    def get_ports(self):
        return self.ports
    
    def getVlanbyId(self, id):
        for vlan in VLAN.vlans:
            if vlan.vlan_id == id:
                return vlan
        return None
    
    #function to log the creation of the vlan and its ports
    def log_creation(self):
        log_dir = "vlan_logs"
        if not os.path.exists(log_dir):
            os.makedirs(log_dir)

        log_file = os.path.join(log_dir, f"{self.vlan_id}_log.txt")
        with open(log_file, "a") as f:
            f.write(f"VLAN {self.vlan_id} created.\n")

    #log addition of a port to a vlan 
    def log_port_addition(self, port):
        log_dir = "vlan_logs"
        log_file = os.path.join(log_dir, f"{self.vlan_id}_log.txt")
        with open(log_file, "a") as f:
            f.write(f"Port {port}\n")