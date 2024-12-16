




uint32_t flexctr_action_id;
int entries = 5;
uint32 stat_id[entries];

bcm_port_t ports[5] = {4, 81, 60, 61, 140};








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
        BCM_IF_ERROR_RETURN(bcm_field_qualify_OuterVlan(unit, i, 1663, 1663));
        flexctr_cfg.counter_index = stat_id[i - 1];
        BCM_IF_ERROR_RETURN(bcm_field_entry_flexctr_attach(unit, i, &flexctr_cfg));
        BCM_IF_ERROR_RETURN(bcm_field_action_add(unit, i, bcmFieldActionCopyToCpu, 1, i));
        BCM_IF_ERROR_RETURN(bcm_field_entry_install(unit, i));
    }

    return BCM_E_NONE;
}


int testVerify(int unit)
{
    char str[512];
    int rv;
    bcm_flexctr_counter_value_t counter_value;
    uint32 num_entries = 1;
    int count = 5;
    int i;

    /* Get counter value. */
    for (i = 0; i < 5; i++)
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
        }
    }

    return BCM_E_NONE;
}




/*
 * Cleanup test setup
 */
int testCleanup(int unit)
{
    bcm_error_t rv;
    int i;
    bcm_field_flexctr_config_t flexctr_cfg;

    flexctr_cfg.flexctr_action_id = flexctr_action_id;
    /* Detach counter action. */
    for (i = 1; i <= entries; i++)
    {
        flexctr_cfg.counter_index = stat_id[i - 1];
        rv = bcm_field_entry_flexctr_detach(unit, i, &flexctr_cfg);
        if (BCM_FAILURE(rv))
        {
            printf("bcm_field_stat_detach %s\n", bcm_errmsg(rv));
            return rv;
        }
    }

    /* Destroy counter action. */
    rv = bcm_flexctr_action_destroy(unit, flexctr_action_id);
    if (BCM_FAILURE(rv))
    {
        printf("bcm_flexctr_action_destroy %s\n", bcm_errmsg(rv));
        return rv;
    }

    return BCM_E_NONE;
}
