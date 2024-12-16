

int unit = 0;
bcm_error_t rv;
bcm_ip_t mask = 0xfffff000;
int ports[] = {1, 2, 3, 4, 20, 21, 22, 23, 38, 40, 41, 42, 43, 60, 61, 62, 63, 78, 80, 81, 82, 83, 100, 101, 102, 103, 118, 120, 121, 122, 123, 140, 141, 142, 143, 158};
bcm_pbmp_t pbmp, ubmp;

void clr_red() { printf("%c[1;31m", 0x1B); }
void clr_grn() { printf("%c[1;32m", 0x1B); }
void clr_ylw() { printf("%c[1;33m", 0x1B); }
void clr_blu() { printf("%c[1;34m", 0x1B); }
void clr_mag() { printf("%c[1;35m", 0x1B); }
void clr_cyn() { printf("%c[1;36m", 0x1B); }
void clr_noc() { printf("%c[0m", 0x1B); }

bcm_error_t assign_vrf_to_vlan(int unit, int vrf, bcm_vlan_t vid)
{
    bcm_vlan_control_vlan_t vlan_ctrl;
    bcm_vlan_control_vlan_t_init(&vlan_ctrl);
    bcm_vlan_control_vlan_get(unit, vid, &vlan_ctrl);
    vlan_ctrl.vrf = vrf;
    BCM_IF_ERROR_RETURN(bcm_vlan_control_vlan_set(unit, vid, vlan_ctrl));

    return BCM_E_NONE;
}

bcm_error_t add_station(int unit, bcm_mac_t l2addr)
{
    /* Config my station */
    int station_id;
    bcm_l2_station_t l2_station;
    bcm_l2_station_t_init(&l2_station);
    sal_memcpy(l2_station.dst_mac, l2addr, sizeof(l2addr));
    l2_station.dst_mac_mask = "ff:ff:ff:ff:ff:ff";
    // l2_station.vlan = 2;
    // l2_station.vlan_mask = 0xfff;
    BCM_IF_ERROR_RETURN(bcm_l2_station_add(unit, &station_id, &l2_station));

    return BCM_E_NONE;
}

int add_to_l2_station(int UNIT, bcm_mac_t mac, bcm_vlan_t vid)
{
    bcm_l2_station_t l2_station;
    int station_id;
    bcm_error_t rv = BCM_E_NONE;

    bcm_l2_station_t_init(&l2_station);

    sal_memcpy(l2_station.dst_mac, mac, sizeof(mac));

    l2_station.dst_mac_mask = "ff:ff:ff:ff:ff:ff";
    l2_station.vlan = vid;
    l2_station.vlan_mask = 0xfff;
    rv = bcm_l2_station_add(UNIT, &station_id, &l2_station);

    return rv;
}

bcm_error_t add_route(int unit, bcm_if_t egr_obj_id, bcm_vrf_t vrf, bcm_ip_t dip, bcm_ip_t mask)
{
    bcm_l3_route_t route_info;
    /* Install the default route for DIP */
    bcm_l3_route_t_init(&route_info);
    // route_info.l3a_flags = BCM_L3_MULTIPATH;
    route_info.l3a_intf = egr_obj_id;
    route_info.l3a_subnet = 0;
    route_info.l3a_ip_mask = 0;
    route_info.l3a_vrf = vrf;
    // BCM_IF_ERROR_RETURN(
    bcm_l3_route_add(unit, &route_info);

    /* Install the route for DIP */
    bcm_l3_route_t_init(&route_info);
    // route_info.l3a_flags = BCM_L3_MULTIPATH;
    route_info.l3a_intf = egr_obj_id;
    route_info.l3a_subnet = dip;
    route_info.l3a_ip_mask = mask;
    route_info.l3a_vrf = vrf;
    BCM_IF_ERROR_RETURN(bcm_l3_route_add(unit, &route_info));

    return BCM_E_NONE;
}

bcm_error_t configure_l3_ing_obj(int unit, bcm_port_t ingress_port, bcm_vlan_t ingress_vlan, bcm_vrf_t vrf, bcm_mac_t router_mac_in, bcm_if_t ingress_if)
{

    /* Create L3 ingress interface */
    bcm_l3_ingress_t l3_ingress;
    bcm_l3_ingress_t_init(&l3_ingress);
    l3_ingress.flags = BCM_L3_INGRESS_WITH_ID;
    l3_ingress.vrf = vrf;
    BCM_IF_ERROR_RETURN(bcm_l3_ingress_create(unit, &l3_ingress, &ingress_if));

    /* Config vlan_id to l3_iif mapping */
    bcm_vlan_control_vlan_t vlan_ctrl;
    bcm_vlan_control_vlan_t_init(&vlan_ctrl);
    bcm_vlan_control_vlan_get(unit, ingress_vlan, &vlan_ctrl);
    vlan_ctrl.ingress_if = ingress_if;
    BCM_IF_ERROR_RETURN(bcm_vlan_control_vlan_set(unit, ingress_vlan, vlan_ctrl));

    return BCM_E_NONE;
}

bcm_error_t create_l3_egress_object(bcm_mac_t dst_mac, bcm_mac_t router_mac_out, bcm_if_t egr_obj_id, bcm_vlan_t vlan, bcm_port_t port)
{
    /* Create L3 interface */
    bcm_l3_intf_t l3_intf_out;
    bcm_l3_intf_t_init(&l3_intf_out);
    sal_memcpy(l3_intf_out.l3a_mac_addr, router_mac_out, sizeof(router_mac_out));
    l3_intf_out.l3a_vid = vlan;
    // l3_intf_out.l3a_vrf = vrf;
    BCM_IF_ERROR_RETURN(bcm_l3_intf_create(unit, &l3_intf_out));

    /* Create L3 egress object */
    bcm_l3_egress_t l3_egress;

    bcm_l3_egress_t_init(&l3_egress);
    sal_memcpy(l3_egress.mac_addr, dst_mac, sizeof(dst_mac));
    int flags = BCM_L3_WITH_ID;
    l3_egress.port = port;
    l3_egress.intf = l3_intf_out.l3a_intf_id;
    BCM_IF_ERROR_RETURN(bcm_l3_egress_create(unit, flags, &l3_egress, &egr_obj_id));

    return BCM_E_NONE;
}

bcm_error_t setLoopback(int unit, bcm_port_t port)
{
    bcm_field_entry_t entry;
    bcm_field_group_config_t group_config;

    BCM_IF_ERROR_RETURN(bcm_port_loopback_set(unit, port, BCM_PORT_LOOPBACK_MAC));

    if (port == 21 || port == 22 || port == 4 || port == 3 || port == 103)
    {
        bcm_field_group_config_t_init(&group_config);
        BCM_FIELD_QSET_INIT(group_config.qset);
        BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyStageIngress);
        BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyInPort);
        BCM_FIELD_ASET_ADD(group_config.aset, bcmFieldActionCopyToCpu);
        group_config.priority = port;
        BCM_IF_ERROR_RETURN(bcm_field_group_config_create(unit, &group_config));

        BCM_IF_ERROR_RETURN(bcm_field_entry_create(unit, group_config.group, &entry));
        BCM_IF_ERROR_RETURN(bcm_field_qualify_InPort(unit, entry, port, BCM_FIELD_EXACT_MATCH_MASK));
        BCM_IF_ERROR_RETURN(bcm_field_action_add(unit, entry, bcmFieldActionCopyToCpu, 0, 0));

        BCM_IF_ERROR_RETURN(bcm_field_entry_install(unit, entry));
    }

    return BCM_E_NONE;
}

bcm_error_t add_host(int unit, bcm_ip_t dip, bcm_vrf_t vrf, bcm_if_t ecmp_obj_out)
{
    /* Add host entry with Egress object */
    bcm_l3_host_t host;
    bcm_l3_host_t_init(&host);
    host.l3a_intf = ecmp_obj_out;
    host.l3a_ip_addr = dip;
    host.l3a_vrf = vrf;
    rv = bcm_l3_host_add(unit, &host);
    if (BCM_FAILURE(rv))
    {
        printf("Error executing bcm_l3_host_add(): %s.\n", bcm_errmsg(rv));
        return rv;
    }

    return BCM_E_NONE;
}

// error handling
void error_handling(bcm_error_t rv, char *msg)
{
    if (BCM_FAILURE(rv))
    {
        clr_red();
        printf("Message: %s, %s\n", msg, bcm_errmsg(rv));
        clr_noc();
    }
    else if (BCM_SUCCESS(rv))
    {
        printf("Message: %s, Executed Successfully!!\n", msg);
    }
}

// loop through all ports and set loopback port size
int i;
for (i = 0; i < sizeof(ports) / sizeof(ports[0]); i++)
{
    rv = setLoopback(unit, ports[i]);
}

bcm_error_t add_ports_to_vlan(int unit, int vlan, int port)
{
    /* Port VLAN configuration */
    BCM_PBMP_CLEAR(ubmp);
    BCM_PBMP_CLEAR(pbmp);
    BCM_PBMP_PORT_ADD(pbmp, port);
    BCM_IF_ERROR_RETURN(bcm_vlan_port_add(unit, vlan, pbmp, ubmp));
    BCM_IF_ERROR_RETURN(bcm_port_untagged_vlan_set(unit, port, vlan));

    return BCM_E_NONE;
}

void bbshell(int unit, char *str)
{
    printf("B_CM.%d> %s\n", unit, str);
    bshell(unit, str);
}

bcm_error_t createHost(int unit, bcm_mac_t dest_mac, bcm_mac_t router_mac_out, bcm_mac_t router_mac_in, bcm_vrf_t vrf, bcm_if_t ecmp_obj_out, bcm_vlan_t vlan, bcm_port_t port, bcm_ip_t dip, bcm_if_t ing_intf_id)
{
    bcm_error_t rv;
    BCM_IF_ERROR_RETURN(bcm_vlan_create(unit, vlan));
    rv = add_ports_to_vlan(unit, vlan, port);
    error_handling(rv, "adding port to vlan");

    rv = create_l3_egress_object(dest_mac, router_mac_out, ecmp_obj_out, vlan, port);
    error_handling(rv, "creating l3 egress object");

    rv = add_host(unit, dip, vrf, ecmp_obj_out);
    error_handling(rv, "adding host");

    rv = configure_l3_ing_obj(unit, port, vlan, vrf, router_mac_in, ing_intf_id);
    error_handling(rv, "creating l3 ingress intf");
    add_station(unit, router_mac_in);
    error_handling(rv, "adding station with vlan");
    return BCM_E_NONE;
}

bbshell(unit, "pw start report +raw +decode +pmd");

bcm_error_t createLink(
    int unit, bcm_mac_t mac_s1, bcm_mac_t mac_s2, bcm_mac_t mac_out_s1, bcm_mac_t mac_out_s2, bcm_vrf_t vrf_s1, bcm_vrf_t vrf_s2, bcm_if_t ing_s1, bcm_if_t ing_s2, bcm_if_t egr_s1, bcm_if_t egr_s2, bcm_port_t port_s1, bcm_port_t port_s2)
{

    rv = configure_l3_ing_obj(unit, port_s2, vrf_s2, vrf_s2, mac_s1, ing_s2);
    error_handling(rv, "creating l3 ingress intf");
    rv = add_station(unit, mac_s1);
    error_handling(rv, "adding station with vlan");

    rv = create_l3_egress_object(mac_s1, mac_out_s1, egr_s2, vrf_s2, port_s2);
    error_handling(rv, "creating l3 egress object");

    rv = create_l3_egress_object(mac_s2, mac_out_s2, egr_s1, vrf_s1, port_s1);
    error_handling(rv, "creating l3 egress object");

    rv = configure_l3_ing_obj(unit, port_s1, vrf_s1, vrf_s1, mac_1_0, ing_s1);
    error_handling(rv, "creating l3 ingress intf");
    rv = add_station(unit, mac_s2);
    error_handling(rv, "adding station with vlan");

    return BCM_E_NONE;
}

bcm_mac_t mac_0_1 = {0xcf, 0xda, 0x2d, 0x8f, 0x6d, 0x53};
bcm_mac_t mac_1_0 = {0xcf, 0xda, 0x2d, 0x8f, 0x6d, 0x54};

bcm_mac_t router_mac_out_route_0_1 = {0xd0, 0xdf, 0x88, 0xf9, 0x6c, 0x7b};
bcm_mac_t router_mac_out_route_1_0 = {0xd0, 0xdf, 0x88, 0xf9, 0x6c, 0x7a};

bcm_vrf_t vrf_s1 = 4;
bcm_vrf_t vrf_s2 = 5;

bcm_if_t ing_s1 = 11;
bcm_if_t ing_s2 = 12;

bcm_if_t egr_s1 = 100004;
bcm_if_t egr_s2 = 100003;

bcm_port_t port_s1 = 2;
bcm_port_t port_s2 = 122;

// port and vlan / link
BCM_IF_ERROR_RETURN(bcm_vlan_create(unit, 4));
rv = add_ports_to_vlan(unit, 4, 122);
error_handling(rv, "adding port 21 to vlan 2");

BCM_IF_ERROR_RETURN(bcm_vlan_create(unit, 5));
rv = add_ports_to_vlan(unit, 5, 2);
error_handling(rv, "adding port 21 to vlan 2");

createLink(unit, mac_0_1, mac_1_0, router_mac_out_route_0_1, router_mac_out_route_1_0, vrf_s1, vrf_s2, ing_s1, ing_s2, egr_s1, egr_s2, port_s2, port_s1);

// host in A
BCM_IF_ERROR_RETURN(bcm_vlan_create(unit, 10));
rv = add_ports_to_vlan(unit, 10, 3);

bcm_mac_t dest_mac_host_0_0 = {0x71, 0x4f, 0x20, 0xa1, 0xd3, 0xfe};
bcm_mac_t router_mac_out_host_0_0 = {0x21, 0x07, 0xab, 0xa7, 0x11, 0xec};
rv = create_l3_egress_object(dest_mac_host_0_0, router_mac_out_host_0_0, 100005, 10, 3);
error_handling(rv, "creating l3 egress object");
rv = add_host(unit, 0xa0404ce, 4, 100005);

// ingress intf for HA
bcm_mac_t router_mac_in2 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
add_station(unit, router_mac_in2);
rv = configure_l3_ing_obj(unit, 3, 10, 4, router_mac_in2, 14);
error_handling(rv, "creating l3 ingress intf");

// host in B
BCM_IF_ERROR_RETURN(bcm_vlan_create(unit, 15));
rv = add_ports_to_vlan(unit, 15, 4);
error_handling(rv, "adding port 4 to vlan 15");

bcm_mac_t dest_mac_host_1_2 = {0xa1, 0x01, 0x12, 0x14, 0x1a, 0x58};
bcm_mac_t router_mac_out_host_1_2 = {0x9e, 0x5a, 0x9e, 0x69, 0xe3, 0xd5};
rv = create_l3_egress_object(dest_mac_host_1_2, router_mac_out_host_1_2, 100009, 15, 4);
error_handling(rv, "creating l3 egress object");
rv = add_host(unit, 0xa0505ce, 5, 100009);

// ingress intf for BA
bcm_mac_t router_mac_in = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
add_station(unit, router_mac_in);
rv = configure_l3_ing_obj(unit, 4, 15, 5, router_mac_in, 13);
error_handling(rv, "creating l3 ingress intf");

rv = createHost(unit, dest_mac_host_0_0, router_mac_out_host_0_0, router_mac_in, 2, 100005, 10, 3, 0xa0202ce, 200);
error_handling(rv, "creating host in A");
rv = createHost(unit, dest_mac_host_1_2, router_mac_out_host_1_2, router_mac_in2, 3, 100009, 15, 4, 0xa0303ce, 100);
error_handling(rv, "creating host in B");

rv = createHost(unit, dest_mac_host_1_3, router_mac_out_host_1_3, router_mac_in3, 6, 102010, 302, 21, 0xa0606ce, 501);
error_handling(rv, "creating host in C");

