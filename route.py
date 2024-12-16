
class Route:
    def __init__(self, router_mac_out, router_mac_destination, router_mac_in, vrf, vlan, ip, egress_id, port, ing_intf_id):
        self.router_mac_out = router_mac_out
        self.router_mac_destination = router_mac_destination
        self.router_mac_in = router_mac_in
        self.vrf = vrf #node to be sent to. 
        self.vlan = vlan
        self.ip = ip
        self.egress_id= egress_id
        self.port = port
        self.ing_intf_id = ing_intf_id
        self.is_created = False
        #log this route into routes.txt in this order: VRF VLAN PORT ROUTER_MAC_OUT ROUTER_MAC_IN ROUTER_MAC_DEST. Egress_ID Ingress_ID IP
        with open("routes.txt", "a") as file:
            if file.tell() == 0:  # Check if file is empty
                # Write column titles if the file is empty
                file.write("VRF VLAN PORT ROUTER_MAC_OUT ROUTER_MAC_IN ROUTER_MAC_DEST Egress_ID Ingress_ID\n")
            
            formatted_data = f"{self.vrf:<4} {self.vlan:<4} {self.port:<4} {self.router_mac_out:<18} {self.router_mac_in:<18} {self.router_mac_destination:<18} {self.egress_id:<10} {self.ing_intf_id} {self.ip}\n"
            file.write(formatted_data)

    #function to set route as created
    def set_created(self):
        self.is_created = True

    def get_is_created(self):
        return self.is_created

    def __str__(self):
        return f"Route with MAC {self.router_mac_out} and IP {self.ip} and VLAN {self.vlan} and port {self.port} connected to router with MAC {self.router_mac_in} and VRF {self.vrf} and egress id {self.egress_id} and ing_intf_id {self.ing_intf_id} and router_mac_destination {self.router_mac_destination}"
        

    