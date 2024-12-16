class Ingress_Interface:
    last_ingress_interface = 2

    def __init__(self):
        self.ingress_interface = self.generate_unique_ingress_interface()
        Ingress_Interface.last_ingress_interface += 1
        print(f"Created: {self.ingress_interface}")

    def generate_unique_ingress_interface(self):
        return Ingress_Interface.last_ingress_interface

    def get_ingress_interface(self):
        return self.ingress_interface
