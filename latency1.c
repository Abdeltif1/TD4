uint64 tmp_hack_sec;
int tmp_hack_sec_lower = 2000;
uint64 tmp_hack_nsec;
int tmp_hack_nsec_lower = 3000;
bcm_port_t ingress_port = 63; // Ingress port number
bcm_port_t egress_port = 20; // Egress port number
int rv ; 
bcm_error_t
ConfigurePktTimestamp(int unit)
{
    bcm_time_interface_t intf;
    intf.id = 0;
    intf.flags = BCM_TIME_ENABLE | BCM_TIME_WITH_ID;
    rv = bcm_time_interface_add(0,intf);
    if( rv != BCM_E_NONE )
    {
        printf("bcm_time_interface_add failed with %d\n", bcm_errmsg(rv));
        return rv;
    }
    bcm_time_capture_t time = {0};

    time.flags = BCM_TIME_CAPTURE_IMMEDIATE;
    print bcm_time_capture_get(0, 0, &time);
    bcm_time_ts_counter_t counter;
    
    COMPILER_64_SET(tmp_hack_sec, 0, tmp_hack_sec_lower);
    COMPILER_64_SET(tmp_hack_nsec, 0, tmp_hack_nsec_lower);
    
    if( 1 )
    {
        printf("Doing port control PacketTimeStampInsertRx...\n");
        BCM_IF_ERROR_RETURN( bcm_port_control_set(unit, ingress_port, bcmPortControlPacketTimeStampInsertRx, 1) );
        printf("Doing port control PacketTimeStampRxId...\n");
        BCM_IF_ERROR_RETURN( bcm_port_control_set(unit, ingress_port, bcmPortControlPacketTimeStampRxId, 0xaa) );
        
        printf("Doing port control PacketTimeStampInsertTx...\n");
        BCM_IF_ERROR_RETURN( bcm_port_control_set(unit, egress_port, bcmPortControlPacketTimeStampInsertTx, 1) );
        printf("Doing port control TimeStampTxId...\n");
        BCM_IF_ERROR_RETURN( bcm_port_control_set(unit, egress_port, bcmPortControlPacketTimeStampTxId, 0xbb) );
    }
    
    return BCM_E_NONE;
}


void calculate_latency(bcm_pkt_t *pkt) {
    bcm_port_timesync_tx_info_t timestamp_ingress, timestamp_egress;
    uint64 latency_ns;
    int rv;

    // Get ingress timestamp
    rv = bcm_port_timesync_tx_info_get(pkt->unit, pkt->src_port, &timestamp_ingress);
    if (BCM_FAILURE(rv)) {
        printf("Error getting ingress timestamp: %s\n", bcm_errmsg(rv));
        return;
    }

    // Get egress timestamp
    rv = bcm_port_timesync_tx_info_get(pkt->unit, pkt->dst_port, &timestamp_egress);
    if (BCM_FAILURE(rv)) {
        printf("Error getting egress timestamp: %s\n", bcm_errmsg(rv));
        return;
    }

    // Calculate latency
    COMPILER_64_COPY(latency_ns, timestamp_egress.timestamp);
    COMPILER_64_SUB_64(latency_ns, timestamp_ingress.timestamp);

    // Print the latency
    printf("Latency: %llu ns\n", latency_ns);

    // Update maximum latency
    if (COMPILER_64_GT(latency_ns, max_latency_ns[pkt->src_port])) {
        max_latency_ns[pkt->src_port] = latency_ns;
    }

    // Print the maximum latency
    printf("Maximum Latency on Port %d: %llu ns\n", pkt->src_port, max_latency_ns[pkt->src_port]);
}

void packet_rx_callback(int unit, bcm_pkt_t *pkt, void *cookie) {
    // Call the latency calculation function
    calculate_latency(pkt);

    // Indicate that the packet has been processed

    // Indicate that the packet has been processed
    return BCM_PKTIO_RX_NOT_HANDLED;
}



    //    BCM_IF_ERROR_RETURN(
    //      bcm_port_control_set(unit, ingress_port, bcmPortControlPacketTimeStampInsertRx, 1));
    // BCM_IF_ERROR_RETURN(
    //      bcm_port_control_set(unit, ingress_port, bcmPortControlPacketTimeStampRxId, 0x1111));
    // BCM_IF_ERROR_RETURN(
    //      bcm_port_control_set(unit, egress_port, bcmPortControlPacketTimeStampInsertTx, 1));
    // BCM_IF_ERROR_RETURN(bcm_port_control_set(unit, egress_port, bcmPortControlPacketTimeStampTxId, 0xffff));

    // Register packet receive callback
    rv = bcm_pktio_rx_register(unit, "packet_rx_callback", packet_rx_callback, 100, NULL, 0);
    if (BCM_FAILURE(rv)) {
        printf("Error registering RX callback: %s\n", bcm_errmsg(rv));
        return rv;
    }

     if( (rv = ConfigurePktTimestamp(unit)) != BCM_E_NONE )
    {
        printf("Configuring Packettime stamping in IFP failed with %d\n", rrv);
        return rrv;
    }
