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

bcm_error_t setLoopback(int unit, bcm_port_t port)
{
    bcm_field_entry_t entry;
    bcm_field_group_config_t group_config;

    BCM_IF_ERROR_RETURN(bcm_port_loopback_set(unit, port, BCM_PORT_LOOPBACK_MAC));

    if (port == 1 || port == 142 || port == 21 || port == 43 || port == 120)
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
/*function to check for errors*/
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

        printf("Message: %s, Executed Successfully - Test Passed ✔️ !!\n", msg);
    }
}

bcm_error_t add_ecmp_group(int unit, bcm_vrf_t vrf, bcm_ip_t dip, bcm_ip_t mask, int *egress_objects, int egress_count)
{
    bcm_l3_ecmp_member_t ecmp_member_array[10];
    bcm_l3_egress_ecmp_t ecmp_grp;
    int ecmp_member_count = egress_count;
    bcm_if_t ecmp_obj_out;
    int i = 0;
    // loop through egress_objects and add them to ecmp_member_array
    for (i = 0; i < egress_count; i++)
    {
        bcm_l3_ecmp_member_t_init(&ecmp_member_array[i]);
        ecmp_member_array[i].egress_if = egress_objects[i];
    }

    bcm_l3_egress_ecmp_t_init(&ecmp_grp);
    ecmp_grp.max_paths = 64;
    rv = bcm_l3_ecmp_create(unit, 0, &ecmp_grp, ecmp_member_count, ecmp_member_array);
    if (BCM_FAILURE(rv))
    {
        printf("Error executing bcm_l3_ecmp_create(): %s.\n", bcm_errmsg(rv));
        return rv;
    }
    ecmp_obj_out = ecmp_grp.ecmp_intf;

    bcm_l3_route_t route_info;
    /* Install the default route for DIP */
    bcm_l3_route_t_init(&route_info);
    route_info.l3a_flags = BCM_L3_MULTIPATH;
    route_info.l3a_intf = ecmp_obj_out;
    route_info.l3a_subnet = 0;
    route_info.l3a_ip_mask = 0;
    route_info.l3a_vrf = vrf;
    // BCM_IF_ERROR_RETURN(
    bcm_l3_route_add(unit, &route_info);

    /* Install the route for DIP */
    bcm_l3_route_t_init(&route_info);
    route_info.l3a_flags = BCM_L3_MULTIPATH;
    route_info.l3a_intf = ecmp_obj_out;
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

bcm_error_t add_station(int unit, bcm_mac_t l2addr, bcm_vlan_t vlan, bcm_vrf_t vrf)
{
    /* Config my station */
    int station_id;
    bcm_l2_station_t l2_station;
    bcm_l2_station_t_init(&l2_station);
    sal_memcpy(l2_station.dst_mac, l2addr, sizeof(l2addr));
    l2_station.dst_mac_mask = "ff:ff:ff:ff:ff:ff";
    // l2_station.vlan = vlan;
    // l2_station.vlan_mask = 0xfff;
    // l2_station.vfi = vrf;
    BCM_IF_ERROR_RETURN(bcm_l2_station_add(unit, &station_id, &l2_station));

    return BCM_E_NONE;
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
bcm_error_t create_egress_ingress_intf(int unit, bcm_port_t port, bcm_vrf_t vrf, bcm_vlan_t vlan, bcm_mac_t mac, bcm_mac_t mac_out, bcm_if_t ing_if, bcm_if_t egr_if)
{

    int rv;

    rv = create_l3_egress_object(mac, mac_out, egr_if, vlan, port);
    error_handling(rv, "creating l3 egress object");
    rv = configure_l3_ing_obj(unit, port, vlan, vrf, mac, ing_if);
    error_handling(rv, "creating l3 ingress intf");
    rv = add_station(unit, mac, vlan, vrf);
    error_handling(rv, "adding station with vlan");

    return BCM_E_NONE;
}

// loop through all ports and set loopback port size
int i;
for (i = 0; i < sizeof(ports) / sizeof(ports[0]); i++)
{
    rv = setLoopback(unit, ports[i]);
}

void bbshell(int unit, char *str)
{
    printf("B_CM.%d> %s\n", unit, str);
    bshell(unit, str);
}

bbshell(unit, "pw start report +raw +decode +pmd");

/* Automated code*/
