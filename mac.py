import random


class Mac:
    def __init__(self):
        self.used_addresses = set()
        self.counter = 0

    def generate_random_mac(self):
        while True:
            # Generate the MAC address using the counter
            mac_address = [self.counter >> 8 & 0xff, self.counter & 0xff,  # Split counter into bytes
                           random.randint(0x00, 0xff), random.randint(0x00, 0xff),
                           random.randint(0x00, 0xff), random.randint(0x00, 0xff)]
            mac_str = "{{{}}}".format(", ".join(map(lambda x: "0x{:02x}".format(x), mac_address)))

            # Check if the generated MAC address is already used
            if mac_str not in self.used_addresses:
                self.used_addresses.add(mac_str)
                self.counter += 1
                return mac_str

    def reset(self):
        # Reset used_addresses set and the counter
        self.used_addresses = set()
        self.counter = 0
