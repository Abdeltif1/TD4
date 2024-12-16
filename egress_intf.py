class Egress_Interface:
    last_egress_interface = 100002
    egress_interfaces = []
    def __init__(self):
        self.egress_interface = self.generate_unique_egress_interface()
        Egress_Interface.egress_interfaces.append(Egress_Interface.last_egress_interface)
        Egress_Interface.last_egress_interface += 1

    def generate_unique_egress_interface(self):
        return Egress_Interface.last_egress_interface

    def get_egress_interface(self):
        return self.egress_interface

    @classmethod
    def reset(cls):
        cls.last_egress_interface = 100002



