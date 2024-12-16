import ipaddress
import random
from vlan import VLAN
from ingress_intf import Ingress_Interface
from egress_intf import Egress_Interface
from mac import Mac



ports = [1, 2, 3, 4, 20, 21, 22, 23, 40, 41, 42, 43, 60, 61, 62, 63, 80, 81, 82, 83, 100, 101, 102, 103, 120, 121, 122, 123, 140, 141, 142, 143]




def read_adjacency_matrix(file_path):
    print("Reading adjacency matrix from file...")
    adjacency_matrix = []
    with open(file_path, "r") as file:   
        for line in file:
            line = line.split()
            #turn the string into a list of integers
            line = [int(x) for x in line]
            adjacency_matrix.append(line)
    return adjacency_matrix

def assign_ports_to_adjacency_matrix(adjacency_matrix):
    print("Assigning ports to adjacency matrix...")
    port_index = 0
    for i in range(len(adjacency_matrix)):
        for j in range(len(adjacency_matrix[i])):
            if adjacency_matrix[i][j] == 1:
                adjacency_matrix[i][j] = ports[port_index]
                port_index += 1
                if port_index == len(ports):
                    port_index = 0
    return adjacency_matrix


#function to write the generated code to a file
def write_to_file(code, file_path):
    with open(file_path, "a") as file:
        file.write(code)


def structure_matrix(adjacency_matrix, mac_generator):
    #loop through the adjacency matrix
    port_index = 0
    for i in range(len(adjacency_matrix)):
        for j in range(len(adjacency_matrix[i])):
            #if the value is 1, then add the code to the file
            if adjacency_matrix[i][j] == 1:
                if(i > j):
                    adjacency_matrix[i][j] = ("Clone", ports[port_index], j+2+len(adjacency_matrix), Egress_Interface().last_egress_interface, Ingress_Interface().last_ingress_interface, mac_generator.generate_random_mac(), mac_generator.generate_random_mac())
                else:
                    adjacency_matrix[i][j] = ("Real", ports[port_index], j+2,                        Egress_Interface().last_egress_interface, Ingress_Interface().last_ingress_interface, mac_generator.generate_random_mac(), mac_generator.generate_random_mac())
                port_index += 1
                if port_index == len(ports):
                    port_index = 0   
    return adjacency_matrix

# add vlan code
def add_vlan_code():
    code = "/* Creating VLANs & adding ports to them...*/\n"
    #loop through all vlans
    for vlan in VLAN.vlans:
        code += f"""/* vlan creation */\nBCM_IF_ERROR_RETURN(bcm_vlan_create(unit, {vlan.vlan_id}));\n"""
        for port in vlan.ports:
            code += f"rv = add_ports_to_vlan(unit, {vlan.vlan_id}, {port});\n error_handling(rv, \"adding port {port} to vlan {vlan.vlan_id}\");\n"
        # code += f"rv = create_ingress(unit, {vlan.vlan_id}, {vlan.vlan_id}, {10 + vlan.vlan_id});"
        # code += f" error_handling(rv, \"creating ingress intf\");\n"
    write_to_file(code, "unfold.c")

#function to add egress and ingress code
def add_egress_ingress_code(structured_matrix):
    code = "/* egress and ingress creation */\n"
    #loop through the adjacency matrix
    for i in range(len(structured_matrix)):
        for j in range(len(structured_matrix[i])):
            #if the value is 1, then add the code to the file
            if structured_matrix[i][j] != 0:
                destination_node = structured_matrix[i][j]
                code += f"bcm_mac_t mac_{i}_{j} = {destination_node[5]};\n"
                code += f"bcm_mac_t mac_out_{i}_{j} = {destination_node[6]};\n"
                code += f"create_egress_ingress_intf(unit, {destination_node[1]}, {destination_node[2]}, {destination_node[2]}, mac_{i}_{j}, mac_out_{i}_{j}, {destination_node[4]}, {destination_node[3]});\n"
    write_to_file(code, "unfold.c")

#function to return the vlan object given the vlan id
def getVlanbyId(id):
    for vlan in VLAN.vlans:
        if vlan.vlan_id == id:
            return vlan
    return None

def ip_to_hex(ip_address_str):
    # Parse the IP address string
    ip_address = ipaddress.IPv4Address(ip_address_str)
    
    # Convert the IP address to its integer representation
    ip_int = int(ip_address)
    
    # Convert the integer IP address to hexadecimal
    hex_ip = hex(ip_int)
    
    # Return the hexadecimal representation
    return hex_ip


def add_ecmp_egress_code():
    code = "/* ecmp egress creation */\n"
    for i in range(len(VLAN.vlans)):
        vlan_id = VLAN.vlans[i].vlan_id
        ip = f"10.{random.randint(2, 13)}.{random.randint(0, 12)}.1"
        # pick 3 - 5 random egress ids from the ones created
        egress_ids = Egress_Interface.egress_interfaces
        ecmp_egress_ids = random.sample(egress_ids, random.randint(3, 5))
        code += f"int ecmp_member_count_{vlan_id} = {len(ecmp_egress_ids)};\n"
        # insert all the ids into an array in c code
        code += f"bcm_if_t ecmp_member_array_{vlan_id}[ecmp_member_count_{vlan_id}] = "
        code += "{" + ", ".join(map(str, ecmp_egress_ids)) + "};\n"
        code += f"rv = add_ecmp_group(unit, {vlan_id}, {ip_to_hex(ip)}, mask, ecmp_member_array_{vlan_id}, ecmp_member_count_{vlan_id});\n"
        code += f"error_handling(rv, \"adding ecmp group_{vlan_id}\");\n"
    write_to_file(code, "unfold.c")

#functiont to add routes
def add_route_code(structured_matrix):
    code = "/* route creation */\n"
    for i in range(len(structured_matrix)):
        for j in range(len(structured_matrix[i])):
            if structured_matrix[i][j] != 0:
                ip = f"10.{j+2}.{j+2}.1"
                #if it's main vrf is going to be j + 2
                if structured_matrix[i][j][0] == "Real":
                    code += f"rv = add_route(unit, {structured_matrix[i][j][3]}, {i+2}, {ip_to_hex(ip)}, mask);\n error_handling(rv, \"adding route to {j+2}\");\n"
                #if it's clone vrf is going to be j + 2 + len(structured_matrix)
                else:
                    code += f"rv = add_route(unit, {structured_matrix[i][j][3]}, {i+2}, {ip_to_hex(ip)}, mask);\n error_handling(rv, \"adding route to {j+2 + len(structured_matrix)}\");\n"
    write_to_file(code, "unfold.c")
#function to add ports to apropiate VLANs
def add_ports_to_vlans(adjacency_matrix):
    #loop through the adjacency matrix
    for i in range(len(adjacency_matrix)):
        for j in range(len(adjacency_matrix[i])):
            #if the value is 1, then add the code to the file
            if adjacency_matrix[i][j] != 0:
                if adjacency_matrix[i][j][0] == "Real":
                    getVlanbyId(j+2).add_port(adjacency_matrix[i][j][1])
                elif adjacency_matrix[i][j][0] == "Clone":
                    getVlanbyId(j+2+len(adjacency_matrix)).add_port(adjacency_matrix[i][j][1])



#function to create all the vlans
def create_vlans(adjacency_matrix):
    #loop through the adjacency matrix
    for i in range(2,len(adjacency_matrix) + 2):
        VLAN(i) #REAL VLAN
        VLAN(i+len(adjacency_matrix)) #CLONE VLAN

##################### Structuring & Creating Objects #####################

# Tuple Structure: (Type, Port, (Destination) VLAN/VRF, Egress_ID, Ingress_ID, MAC, MAC_OUT)
mac_generator = Mac()
# Read the adjacency matrix from the file
adjacency_matrix = read_adjacency_matrix("adjacency.txt")
structured_matrix = structure_matrix(adjacency_matrix, mac_generator)
print(structured_matrix)
print("Creating VLANs...")
create_vlans(structured_matrix)
print("Creating Egress and Ingress Interfaces...")
add_ports_to_vlans(structured_matrix)






##################### Adding code #####################
add_vlan_code()
add_egress_ingress_code(structured_matrix)
add_route_code(structured_matrix)
print(Egress_Interface.egress_interfaces)
# add_ecmp_egress_code()