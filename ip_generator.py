import random

class IPGenerator:
    def __init__(self, ip_address, subnet_mask):
        self.ip_address = ip_address
        self.subnet_mask = subnet_mask
        self.generated_ips = set()

    def generate_ip_within_range(self):
        ip_octets = list(map(int, self.ip_address.split(".")))
        subnet_octets = list(map(int, self.subnet_mask.split(".")))
        
        while True:
            generated_ip_octets = []
            for ip_octet, subnet_octet in zip(ip_octets, subnet_octets):
                if subnet_octet == 255:
                    generated_ip_octets.append(ip_octet)
                else:
                    min_ip = (ip_octet & subnet_octet)
                    max_ip = min_ip | (255 - subnet_octet)
                    generated_ip_octets.append(random.randint(min_ip, max_ip))
            
            generated_ip = ".".join(map(str, generated_ip_octets))
            if generated_ip not in self.generated_ips:
                self.generated_ips.add(generated_ip)
                return generated_ip
