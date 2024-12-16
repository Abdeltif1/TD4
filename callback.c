   uint32 dump_flag = BCM_PKTIO_SYNC_RX_PKT_DUMP;
    uint64 sn;
    bcm_pktio_pkt_t *pkt;
    uint8 *buffer;
    uint32 length;
    uint32 src_port;
    int sent = 0; 
    uint32 count; // Variable to store the queue size
    uint32 max_count = 0;

    // Start synchronous packet reception
    rv = bcm_pktio_sync_rx_start(unit, dump_flag, &sn);
    if (BCM_E_NONE != rv) {
        printf("Error(%s) while executing bcm_pktio_sync_rx_start()\n", bcm_errmsg(rv));
        return rv;
    }


        
        rv = bcm_pktio_sync_rx(unit, sn, &pkt, 60 * 1000 * 1000); // 2 seconds in microseconds
        if (BCM_E_NONE != rv) {
            printf("Error(%s) while executing bcm_pktio_sync_rx()\n", bcm_errmsg(rv));
            break;
        }

        // Get packet data
        rv = bcm_pktio_pkt_data_get(unit, pkt, (void *)&buffer, &length);
        if (BCM_E_NONE != rv) {
            printf("Error(%s) while executing bcm_pktio_pkt_data_get()\n", bcm_errmsg(rv));
            break;
        }

        // Get source port metadata
        rv = bcm_pktio_pmd_field_get(unit, pkt, bcmPktioPmdTypeRx, BCM_PKTIO_RXPMD_SRC_PORT_NUM, &src_port);
        if (BCM_E_NONE != rv) {
            printf("Error(%s) while executing bcm_pktio_pmd_field_get()\n", bcm_errmsg(rv));
            break;
        }


        bcm_port_queued_count_get(unit, src_port, &count); // Get queue size for each port
        
        if (count > max_count) {
            max_count = count;
        }





