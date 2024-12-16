

int unit = 0;
bcm_error_t rv;
bcm_ip_t mask = 0xfffff000;
int ports[] = {1, 2, 3, 4, 20, 21, 22, 23, 38, 40, 41, 42, 43, 60, 61, 62, 63, 78, 80, 81, 82, 83, 100, 101, 102, 103, 118, 120, 121, 122, 123, 140, 141, 142, 143, 158};
bcm_pbmp_t pbmp, ubmp;
int opaque_object = 2;
uint32_t flexctr_action_id;
bcm_flexctr_counter_value_t counter_value;
uint32 num_entries = 1;
uint32 counter_idx = 0;

// int entries = 108;
int entries = 16;
uint32 stat_id[entries];

void clr_red() { printf("%c[1;31m", 0x1B); }
void clr_grn() { printf("%c[1;32m", 0x1B); }
void clr_ylw() { printf("%c[1;33m", 0x1B); }
void clr_blu() { printf("%c[1;34m", 0x1B); }
void clr_mag() { printf("%c[1;35m", 0x1B); }
void clr_cyn() { printf("%c[1;36m", 0x1B); }
void clr_noc() { printf("%c[0m", 0x1B); }

bcm_error_t random_ecmp_config(int unit)
{
    bcm_error_t rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchECMPLevel1RandomSeed, 0xff));
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchECMPLevel2RandomSeed, 0xf011));

    return rv;
}

int dfp_stat_create(int unit)
{
    bcm_error_t rv;
    bcm_flexctr_action_t flexctr_action;
    bcm_flexctr_index_operation_t *index_op = NULL;
    bcm_flexctr_value_operation_t *value_a_op = NULL;
    bcm_flexctr_value_operation_t *value_b_op = NULL;
    int options = 0;

    bcm_flexctr_action_t_init(&flexctr_action);

    flexctr_action.source = bcmFlexctrSourceFieldDestination;
    flexctr_action.mode = bcmFlexctrCounterModeNormal;
    /* Mode can be to count dropped packets or Non drop packets or count all packets */
    flexctr_action.drop_count_mode = bcmFlexctrDropCountModeNoDrop;
    /* Total number of counters */
    flexctr_action.index_num = 1;

    index_op = &flexctr_action.index_operation;
    index_op->object[0] = bcmFlexctrObjectIngDstFpFlexCtrIndex;
    index_op->mask_size[0] = 16;
    index_op->shift[0] = 0;
    index_op->object[1] = bcmFlexctrObjectConstZero;
    index_op->mask_size[1] = 1;
    index_op->shift[1] = 0;

    /* Increase counter per packet. */
    value_a_op = &flexctr_action.operation_a;
    /* Increase counter per packet. */
    value_a_op->select = bcmFlexctrValueSelectCounterValue;
    value_a_op->object[0] = bcmFlexctrObjectConstOne;
    value_a_op->mask_size[0] = 15;
    value_a_op->shift[0] = 0;
    value_a_op->object[1] = bcmFlexctrObjectConstZero;
    value_a_op->mask_size[1] = 1;
    value_a_op->shift[1] = 0;
    value_a_op->type = bcmFlexctrValueOperationTypeInc;

    /* Increase counter per packet bytes. */
    value_b_op = &flexctr_action.operation_b;
    value_b_op->select = bcmFlexctrValueSelectPacketLength;
    value_b_op->type = bcmFlexctrValueOperationTypeInc;

    rv = bcm_flexctr_action_create(unit, options, &flexctr_action, &flexctr_action_id);
    if (BCM_FAILURE(rv))
    {
        printf("\nError in flex counter action create: %s.\n", bcm_errmsg(rv));
        return rv;
    }
    printf("Stat Counter Id %d\n", flexctr_action_id);

    return BCM_E_NONE;
}

int testVerify(int unit)
{
    char str[512];
    int rv;
    bcm_flexctr_counter_value_t counter_value;
    uint32 num_entries = 1;

    int i;
    int total_recieved = 0;

    /* Get counter value. */
    for (i = 0; i < entries; i++)
    {
        sal_memset(&counter_value, 0, sizeof(counter_value));
        rv = bcm_flexctr_stat_get(unit, flexctr_action_id, num_entries, &stat_id[i],
                                  &counter_value);
        if (BCM_FAILURE(rv))
        {
            printf("flexctr stat get failed: [%s]\n", bcm_errmsg(rv));
            return rv;
        }
        else
        {
            printf("FlexCtr Get for entry fetching counter index %d : %d packets / %d bytes\n",
                   stat_id[i],
                   COMPILER_64_LO(counter_value.value[0]),
                   COMPILER_64_LO(counter_value.value[1]));
            total_recieved += COMPILER_64_LO(counter_value.value[0]);
        }
    }

    // print total
    printf("Total packets recieved : %d\n", total_recieved);

    return BCM_E_NONE;
}

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

    /* Create L3 interface */
    bcm_l3_intf_t l3_intf_in;
    bcm_l3_intf_t_init(&l3_intf_in);
    sal_memcpy(l3_intf_in.l3a_mac_addr, router_mac_in, sizeof(router_mac_in));
    l3_intf_in.l3a_vid = ingress_vlan;
    BCM_IF_ERROR_RETURN(bcm_l3_intf_create(unit, &l3_intf_in));

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

bcm_error_t add_ecmp_group(int unit, bcm_vrf_t vrf, bcm_ip_t dip, bcm_ip_t mask, int *egress_objects, int egress_count)
{
    int dynamic_age = 256;   /* Dynamic age in usec */
    int dynamic_size = 1024; /* Flowset table size */
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
        // BCM_IF_ERROR_RETURN(bcm_l3_egress_ecmp_member_status_set(unit, ecmp_member_array[i].egress_if, BCM_L3_ECMP_DYNAMIC_MEMBER_HW));
    }

    bcm_l3_egress_ecmp_t_init(&ecmp_grp);
    // ecmp_grp.ecmp_group_flags = BCM_L3_ECMP_PATH_NO_SORTING;
    ecmp_grp.dynamic_mode = BCM_L3_ECMP_DYNAMIC_MODE_RANDOM;
    // ecmp_grp.dynamic_size = dynamic_size;
    // ecmp_grp.dynamic_age = dynamic_age;
    // max path
    ecmp_grp.max_paths = 8;
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

bcm_error_t global_params_configuration(int uint)
{
    /*** DLB ECMP Global Parameters configuration ****/
    printf("[+] DLB Global parameters set\n");
    /* DLB ECMP Sampling Rate */
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicSampleRate, 62500)); /* 16 us*/
    /* Create Match ID */
    /* DLB ECMP Quantization parameters */
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicEgressBytesExponent, 3));
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicEgressBytesDecreaseReset, 0));
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicQueuedBytesExponent, 3));
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicQueuedBytesDecreaseReset, 0));
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicPhysicalQueuedBytesExponent, 3));
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicPhysicalQueuedBytesDecreaseReset, 0));

    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicEgressBytesMinThreshold, 1000));           /* 10% of 10Gbs, unit Mbs */
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicEgressBytesMaxThreshold, 10000));          /* 10 Gbs, unit Mbs */
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicQueuedBytesMinThreshold, 0x5D44));         /* in bytes, 0x5E cells x 254 Bytes */
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicQueuedBytesMaxThreshold, 0xFE00));         /* 254 x 0x100 cells */
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicPhysicalQueuedBytesMinThreshold, 0x2EA2)); /* 254 x 0x2F cells */
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicPhysicalQueuedBytesMaxThreshold, 0x7F00)); /* 254 x 0x80 cells */

    /* Set the DLB ECMP random seed */
    BCM_IF_ERROR_RETURN(bcm_switch_control_set(unit, bcmSwitchEcmpDynamicRandomSeed, 0x5555));
}

bcm_error_t setLoopback(int unit, bcm_port_t port)
{
    bcm_field_entry_t entry;
    bcm_field_group_config_t group_config;

    BCM_IF_ERROR_RETURN(bcm_port_loopback_set(unit, port, BCM_PORT_LOOPBACK_MAC));
    // if (port == 4)
    // {
    //     rv = ifp_config(unit, port);
    //     if (BCM_FAILURE(rv))
    //     {
    //         printf("Error in IFP config %s\n", bcm_errmsg(rv));
    //         return rv;
    //     }
    // }
    BCM_IF_ERROR_RETURN(
        bcm_port_control_set(unit, port, bcmPortControlPacketTimeStampInsertRx, 1));
    BCM_IF_ERROR_RETURN(
        bcm_port_control_set(unit, port, bcmPortControlPacketTimeStampRxId, 0x1111));
    BCM_IF_ERROR_RETURN(
        bcm_port_control_set(unit, port, bcmPortControlPacketTimeStampInsertTx, 1));
    BCM_IF_ERROR_RETURN(bcm_port_control_set(unit, port, bcmPortControlPacketTimeStampTxId, 0xffff));

    return BCM_E_NONE;
}

int field_stat_create(int unit, bcm_field_group_t group_id)
{
    bcm_error_t rv;
    bcm_flexctr_action_t flexctr_action;
    bcm_flexctr_index_operation_t *index_op = NULL;
    bcm_flexctr_value_operation_t *value_a_op = NULL;
    bcm_flexctr_value_operation_t *value_b_op = NULL;
    int options = 0;

    bcm_flexctr_action_t_init(&flexctr_action);

    flexctr_action.source = bcmFlexctrSourceIfp;
    flexctr_action.mode = bcmFlexctrCounterModeNormal;
    flexctr_action.hint = group_id;
    flexctr_action.hint_type = bcmFlexctrHintFieldGroupId;
    /* Mode can be to count dropped packets or Non drop packets or count all packets */
    flexctr_action.drop_count_mode = bcmFlexctrDropCountModeDrop;
    /* Total number of counters */
    flexctr_action.index_num = 8192;

    index_op = &flexctr_action.index_operation;
    index_op->object[0] = bcmFlexctrObjectIngIfpOpaqueObj0;
    index_op->mask_size[0] = 16;
    index_op->shift[0] = 0;
    index_op->object[1] = bcmFlexctrObjectConstZero;
    index_op->mask_size[1] = 16;
    index_op->shift[1] = 0;

    /* Increase counter per packet. */
    value_a_op = &flexctr_action.operation_a;
    value_a_op->select = bcmFlexctrValueSelectCounterValue;
    value_a_op->object[0] = bcmFlexctrObjectConstOne;
    value_a_op->mask_size[0] = 16;
    value_a_op->shift[0] = 0;
    value_a_op->object[1] = bcmFlexctrObjectConstZero;
    value_a_op->mask_size[1] = 16;
    value_a_op->shift[1] = 0;
    value_a_op->type = bcmFlexctrValueOperationTypeInc;

    /* Increase counter per packet bytes. */
    value_b_op = &flexctr_action.operation_b;
    value_b_op->select = bcmFlexctrValueSelectPacketLength;
    value_b_op->type = bcmFlexctrValueOperationTypeInc;

    rv = bcm_flexctr_action_create(unit, options, &flexctr_action, &flexctr_action_id);
    if (BCM_FAILURE(rv))
    {
        printf("\nError in flex counter action create: %s.\n", bcm_errmsg(rv));
        return rv;
    }

    return BCM_E_NONE;
}

int testConfigure(int unit)
{
    bcm_field_group_config_t group_config;
    bcm_field_entry_t entry;
    int i;
    bcm_error_t rv;
    bcm_field_flexctr_config_t flexctr_cfg;
    bcm_field_hint_t hint;
    bcm_field_hintid_t hint_id;

    bcm_port_t ports[entries] = {20, 20, 83, 2, 60, 123, 20, 120, 40, 140, 22, 100, 40, 102, 121, 40};
    bcm_vlan_t vlans[entries] = {3041, 541, 3621, 977, 1132, 1787, 3949, 2887, 2335, 1152, 3132, 3393, 1993, 3003, 1359, 166};

    /* create hint first */
    print bcm_field_hints_create(unit, &hint_id);
    /* configuring hint type, and opaque object to be used */
    bcm_field_hint_t_init(&hint);
    hint.hint_type = bcmFieldHintTypeFlexCtrOpaqueObjectSel;
    hint.value = bcmFlexctrObjectIngIfpOpaqueObj0;
    /* Associating the above configured hints to hint id */
    rv = bcm_field_hints_add(unit, hint_id, &hint);
    if (BCM_FAILURE(rv))
    {
        printf("Failed to create hints for opq_obj:%d, error:%s\n", hint.value, bcm_errmsg(rv));
        return rv;
    }

    bcm_field_group_config_t_init(&group_config);
    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyStageIngress);
    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyInPort);
    BCM_FIELD_ASET_ADD(group_config.aset, bcmFieldActionStatGroup);
    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyOuterVlan);
    BCM_FIELD_ASET_ADD(group_config.aset, bcmFieldActionCopyToCpu);
    group_config.hintid = hint_id;

    BCM_IF_ERROR_RETURN(bcm_field_group_config_create(unit, &group_config));
    rv = field_stat_create(unit, group_config.group);
    if (BCM_FAILURE(rv))
    {
        printf("Error in field stat create %s\n", bcm_errmsg(rv));
        return rv;
    }
    flexctr_cfg.flexctr_action_id = flexctr_action_id;

    for (i = 1; i <= entries; i++)
    {
        stat_id[i - 1] = i;
        BCM_IF_ERROR_RETURN(bcm_field_entry_create_id(unit, group_config.group, i));
        BCM_IF_ERROR_RETURN(bcm_field_qualify_InPort(unit, i, ports[i - 1], 0xFF));
        BCM_IF_ERROR_RETURN(bcm_field_qualify_OuterVlan(unit, i, vlans[i - 1], vlans[i - 1]));
        flexctr_cfg.counter_index = stat_id[i - 1];
        BCM_IF_ERROR_RETURN(bcm_field_entry_flexctr_attach(unit, i, &flexctr_cfg));
        BCM_IF_ERROR_RETURN(bcm_field_action_add(unit, i, bcmFieldActionCopyToCpu, 1, i));
        BCM_IF_ERROR_RETURN(bcm_field_entry_install(unit, i));
    }

    return BCM_E_NONE;
}

int ifp_config(int unit)
{
    bcm_field_entry_t entry;
    bcm_field_group_config_t group_config;
    bcm_field_flexctr_config_t flexctr_cfg;

    bcm_port_t ports[entries] = {62, 81, 62, 4, 20, 62, 102, 143, 102};
    bcm_port_t vlans[entries] = {3887, 1490, 1643, 2948, 3501, 554, 861, 801, 3149};

    /* IFP Group Configuration and Creation */
    bcm_field_group_config_t_init(&group_config);

    BCM_FIELD_QSET_INIT(group_config.qset);
    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyStageIngress);
    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyInPort);
    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyOuterVlan);
    BCM_FIELD_ASET_INIT(group_config.aset);
    BCM_FIELD_ASET_ADD(group_config.aset, bcmFieldActionCopyToCpu);
    BCM_FIELD_ASET_ADD(group_config.aset, bcmFieldActionAssignOpaqueObject2);

    group_config.flags |= BCM_FIELD_GROUP_CREATE_WITH_MODE;

    /* Associating hints wth IFP Group */
    group_config.mode = bcmFieldGroupModeAuto;

    BCM_IF_ERROR_RETURN(bcm_field_group_config_create(unit, &group_config));

    for (i = 1; i <= entries; i++)
    {
        stat_id[i - 1] = i;
        /* IFP Entry Configuration and Creation */
        BCM_IF_ERROR_RETURN(bcm_field_entry_create_id(unit, group_config.group, i));

        BCM_IF_ERROR_RETURN(bcm_field_qualify_InPort(unit, i, ports[i - 1], 0xFF));
        BCM_IF_ERROR_RETURN(bcm_field_qualify_OuterVlan(unit, i, vlans[i - 1], vlans[i - 1]));
        BCM_IF_ERROR_RETURN(bcm_field_action_add(unit, i, bcmFieldActionAssignOpaqueObject2, opaque_object, 0));
        BCM_IF_ERROR_RETURN(bcm_field_action_add(unit, i, bcmFieldActionCopyToCpu, 1, 1));

        /* Installing FP Entry to FP TCAM */
        BCM_IF_ERROR_RETURN(bcm_field_entry_install(unit, i));
    }

    return BCM_E_NONE;
}

int dfp_config(int unit)
{
    bcm_field_destination_match_t dest_fp_match;
    bcm_field_destination_action_t dest_fp_action;
    uint32_t flexctr_obj_val;
    bcm_error_t rv;

    bcm_field_destination_match_t_init(&dest_fp_match);
    dest_fp_match.opaque_object_2 = opaque_object;
    dest_fp_match.opaque_object_2_mask = 0xffff;
    dest_fp_match.priority = 100;

    bcm_field_destination_action_t_init(&dest_fp_action);
    // dest_fp_action.flags |= BCM_FIELD_DESTINATION_ACTION_REDIRECT;
    //   dest_fp_action.destination_type = bcmFieldRedirectDestinationTrunk;
    //   dest_fp_action.gport = redirect_trunk_gport;
    dest_fp_action.copy_to_cpu = 1;

    rv = bcm_field_destination_entry_add(unit, 0, &dest_fp_match, &dest_fp_action);
    if (BCM_FAILURE(rv))
    {
        printf("DFP entry add failed %s\n", bcm_errmsg(rv));
        return rv;
    }

    /* Create the flex counter action instance for the field destination policy. */
    print dfp_stat_create(unit);
    printf("Stat counter id 0x%x\n", flexctr_action_id);

    /* Attach the flex counter ID to the field destination policy. */
    rv = bcm_field_destination_stat_attach(unit, &dest_fp_match, flexctr_action_id);
    if (BCM_FAILURE(rv))
    {
        printf("DFP stat attach failed %s\n", bcm_errmsg(rv));
        return rv;
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
bcm_pbmp_t pbmp, ubmp;
BCM_PBMP_CLEAR(ubmp);
BCM_PBMP_CLEAR(pbmp);
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

bcm_error_t getStats(int unit)
{

    /* Get counter value. */
    sal_memset(&counter_value, 0, sizeof(counter_value));
    rv = bcm_flexctr_stat_get(unit, flexctr_action_id, num_entries, &counter_idx,
                              &counter_value);
    if (BCM_FAILURE(rv))
    {
        printf("flexctr stat get failed: [%s]\n", bcm_errmsg(rv));
        return rv;
    }
    else
    {
        printf("Flex Counters collected on Dest FP Packets / Bytes : 0x%08x / 0x%08x \n",
               COMPILER_64_LO(counter_value.value[0]),
               COMPILER_64_LO(counter_value.value[1]));
    }
}

bcm_error_t createLink(
    int unit, bcm_mac_t mac_s1, bcm_mac_t mac_s2, bcm_mac_t mac_out_s1, bcm_mac_t mac_out_s2, bcm_vrf_t vrf_s1, bcm_vrf_t vrf_s2, bcm_if_t ing_s1, bcm_if_t ing_s2, bcm_if_t egr_s1, bcm_if_t egr_s2, bcm_port_t port_s1, bcm_port_t port_s2)
{

    rv = configure_l3_ing_obj(unit, port_s2, vrf_s2, vrf_s2, mac_s1, ing_s2);
    error_handling(rv, "creating l3 ingress intf");
    rv = add_station(unit, mac_s1);
    error_handling(rv, "adding station with vlan");

    rv = create_l3_egress_object(mac_s1, mac_out_s1, egr_s2, vrf_s2, port_s2);
    error_handling(rv, "creating l3 egress object");

    rv = configure_l3_ing_obj(unit, port_s1, vrf_s1, vrf_s1, mac_s2, ing_s1);
    error_handling(rv, "creating l3 ingress intf");
    rv = add_station(unit, mac_s2);
    error_handling(rv, "adding station with vlan");

    rv = create_l3_egress_object(mac_s2, mac_out_s2, egr_s1, vrf_s1, port_s1);
    error_handling(rv, "creating l3 egress object");

    return BCM_E_NONE;
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

void bbshell(int unit, char *str)
{
    bshell(unit, str);
}
bcm_error_t Link(int unit, bcm_vlan_t v1, bcm_vlan_t v2, bcm_port_t p1, bcm_port_t p2, bcm_mac_t router_mac_in_0_1, bcm_mac_t router_mac_out_route_0_1, bcm_mac_t router_mac_in_1_0, bcm_mac_t router_mac_out_route_1_0, bcm_if_t ing1, bcm_if_t ing2, bcm_if_t egr1, bcm_if_t egr2, bcm_ip_t ip_a_to_b, bcm_ip_t ip_b_to_a)
{
    rv = configure_l3_ing_obj(unit, p1, v1, v1, router_mac_in_0_1, ing1);
    error_handling(rv, "creating l3 ingress intf");
    rv = add_station(unit, router_mac_in_0_1);
    error_handling(rv, "adding station with vlan");

    rv = create_l3_egress_object(router_mac_in_0_1, router_mac_out_route_0_1, egr1, v1, p1);
    error_handling(rv, "creating l3 egress object");

    rv = add_route(unit, egr1, v2, ip_a_to_b, mask);
    error_handling(rv, "adding route_0_1");

    rv = create_l3_egress_object(router_mac_in_1_0, router_mac_out_route_1_0, egr2, v2, p2);
    error_handling(rv, "creating l3 egress object");

    rv = configure_l3_ing_obj(unit, p2, v2, v2, router_mac_in_1_0, ing2);
    error_handling(rv, "creating l3 ingress intf");
    rv = add_station(unit, router_mac_in_1_0);
    error_handling(rv, "adding station with vlan");

    rv = add_route(unit, egr2, v1, ip_b_to_a, mask);
    error_handling(rv, "adding route_0_1");

    return BCM_E_NONE;
}

//bbshell(unit, "pw start report +raw +decode +pmd");
rv = testConfigure(unit);
error_handling(rv, "configuring test");

/* Automated code */
