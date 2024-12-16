#include <bcm/error.h>
#include <bcm/port.h>
#include <bcm/time.h>
#include <bcm/pkt.h>
#include <bcm/cosq.h>
#include <stdio.h>

#define MAX_PORTS 128

typedef struct {
    uint64 max_latency_ns;
    uint64 max_queue_occupancy_cells;
} port_stats_t;

port_stats_t port_stats[MAX_PORTS];

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
    if (COMPILER_64_GT(latency_ns, port_stats[pkt->src_port].max_latency_ns)) {
        port_stats[pkt->src_port].max_latency_ns = latency_ns;
    }

    // Print the maximum latency
    printf("Maximum Latency on Port %d: %llu ns\n", pkt->src_port, port_stats[pkt->src_port].max_latency_ns);
}

void update_queue_occupancy(int unit, bcm_port_t port) {
    int rv;
    bcm_cosq_stat_t stat = bcmCosqStatIngressPortPGSharedBytesCurrent; // or another appropriate stat
    uint64 occupancy;

    rv = bcm_cosq_stat_get(unit, port, -1, stat, &occupancy);
    if (BCM_FAILURE(rv)) {
        printf("Error getting queue occupancy for port %d: %s\n", port, bcm_errmsg(rv));
        return;
    }

    // Update maximum queue occupancy
    if (COMPILER_64_GT(occupancy, port_stats[port].max_queue_occupancy_cells)) {
        port_stats[port].max_queue_occupancy_cells = occupancy;
    }

    // Print the maximum queue occupancy
    printf("Maximum Queue Occupancy on Port %d: %llu cells\n", port, port_stats[port].max_queue_occupancy_cells);
}

void packet_rx_callback(int unit, bcm_pkt_t *pkt, void *cookie) {
    // Call the latency calculation function
    calculate_latency(pkt);

    // Update queue occupancy for the ingress and egress ports
    update_queue_occupancy(unit, pkt->src_port);
    update_queue_occupancy(unit, pkt->dst_port);

    // Indicate that the packet has been processed
    bcm_rx_free(unit, pkt);
}

int main() {
    int unit = 0; // Device unit number
    bcm_port_t ingress_port = 1; // Ingress port number
    bcm_port_t egress_port = 2; // Egress port number
    int rv;

    // Initialize the BCM API
    rv = bcm_init(unit);
    if (BCM_FAILURE(rv)) {
        printf("BCM initialization failed: %s\n", bcm_errmsg(rv));
        return rv;
    }

    // Enable timestamping on the ingress and egress ports
    rv = bcm_port_control_set(unit, ingress_port, bcmPortControlPacketTimeStampInsertRx, 1);
    if (BCM_FAILURE(rv)) {
        printf("Error enabling timestamp on ingress port %d: %s\n", ingress_port, bcm_errmsg(rv));
        return rv;
    }
    rv = bcm_port_control_set(unit, egress_port, bcmPortControlPacketTimeStampInsertTx, 1);
    if (BCM_FAILURE(rv)) {
        printf("Error enabling timestamp on egress port %d: %s\n", egress_port, bcm_errmsg(rv));
        return rv;
    }

    // Register packet receive callback
    rv = bcm_rx_register(unit, "packet_rx_callback", packet_rx_callback, 100, NULL, 0);
    if (BCM_FAILURE(rv)) {
        printf("Error registering RX callback: %s\n", bcm_errmsg(rv));
        return rv;
    }

    printf("BCM Packet Processing Initialized.\n");

    // Main loop to keep the application running
    while (1) {
        // Simulate packet processing (actual packet processing is done via the callback)
    }

    return 0;
}
