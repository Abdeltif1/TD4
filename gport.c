int dpp_cosq_gport_show(int unit, bcm_gport_t gport, int flags, int verbose) {

    int rv = BCM_E_NONE;
    int cos;
    int base_queue = -1;
    int base_flow = -1;
    int num_cos_levels;
    int mode;
    int weight;
    char *type = NULL;
    int modid = 0;
    int port = 0;
    bcm_gport_t physical_port = 0;
    char *mode_str = NULL;
    uint32 min_size, max_size, flags_2 = 0;
    uint32 kbits_sec_min, kbits_sec_max;

    type = dpp_get_gport_type_string(gport);
    if (strstr(type,"Invalid") != 0) {
        LOG_ERROR(BSL_LS_APPL_SHELL,
                  (BSL_META_U(unit,
                              "ERROR: gport(0x%x) is invalid\n"),gport));
        return (BCM_E_PARAM);
    }
    
    BCM_IF_ERROR_RETURN(bcm_cosq_gport_get(unit, gport, &physical_port, &num_cos_levels, &flags_2));

    modid = BCM_GPORT_MODPORT_MODID_GET(physical_port);

 
    if (BCM_COSQ_GPORT_IS_VOQ_CONNECTOR(gport)) {

        base_flow = BCM_COSQ_GPORT_VOQ_CONNECTOR_ID_GET(gport);

        cli_out("  gp:0x%08x   cos:%d   %32s    flow:%2d)\n",gport, num_cos_levels, type, base_flow);


    }
#ifdef BCM_PETRA_SUPPORT
    else if(BCM_GPORT_IS_SCHEDULER(gport)) {
        base_flow = BCM_GPORT_SCHEDULER_GET(gport);
        if (flags & DPP_DEVICE_COSQ_CL_MASK) {
            cli_out("  gp:0x%08x   cos:%d   %32s    cl se:%2d)\n",gport, num_cos_levels, type, base_flow);
        }
        if (flags & DPP_DEVICE_COSQ_FQ_MASK) {
            cli_out("  gp:0x%08x   cos:%d   %32s    fq se:%2d)\n",gport, num_cos_levels, type, base_flow);
        }
        if (flags & DPP_DEVICE_COSQ_HR_MASK) {
            cli_out("  gp:0x%08x   cos:%d   %32s    hr se:%2d)\n",gport, num_cos_levels, type, base_flow);
        }
    } 
#endif /* BCM_PETRA_SUPPORT */
    else {
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            /* get base_queue and num_cos_levels */ 
            base_queue = BCM_GPORT_UNICAST_QUEUE_GROUP_QID_GET(gport); 
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport)) {
            base_queue = BCM_GPORT_MCAST_QUEUE_GROUP_QID_GET(gport);
        }
        else if (BCM_COSQ_GPORT_IS_ISQ(gport)) {
            base_queue = BCM_COSQ_GPORT_ISQ_QID_GET(gport);
        }

        port  = BCM_GPORT_MODPORT_PORT_GET(physical_port);
        cli_out("  gp:0x%08x   cos:%d   %32s    mod:%2d   port:%2d voq:%4d)\n",gport, num_cos_levels, type, modid, port, base_queue);
        /* get mode and weighted value for each cos level */
        if (verbose == 1) {
            cli_out("               cos       mode           weight    kbps_min   kbps_max  min_size  max_size  flags\n");
            for(cos=0;cos<num_cos_levels;cos++) {
                BCM_IF_ERROR_RETURN(bcm_cosq_gport_sched_get(unit, gport, cos, &mode, &weight));
                BCM_IF_ERROR_RETURN(bcm_cosq_gport_bandwidth_get(unit, gport, cos, &kbits_sec_min, &kbits_sec_max, &flags_2));
                BCM_IF_ERROR_RETURN(bcm_cosq_gport_size_get(unit, gport, cos, &min_size, &max_size));
                mode_str = dpp_get_bw_mode_string(mode);
                cli_out("              %3d    %-22s %d     %-6d      %-6d     %dKB    %dKB %3d  \n",cos,mode_str,weight,kbits_sec_min,kbits_sec_max, min_size/1024, max_size/1024, flags);
            }
        }
    } 

    return rv;
}