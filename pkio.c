/*    Returns: 0: when packet matched.   else mismatched.  */ /* */
int verify_send_recv_pkt(int unit, int template_index,        /* */
                         pkt_verif_template_t *pvt)
{                                                            /* */
    /* Packet Verification Template data */                  /* */
    char str[2 * 1024];                                      /* */
    pkt_verif_template_t *verif_data = &pvt[template_index]; /* */
    uint64 sn;                                               /* */
    bcm_pktio_pkt_t *pkt;                                    /* */
    int rc, i, j;                                            /* */
    uint8 *buffer;                                           /* */
    uint32 length;                                           /* */
    uint32 src_port;                                         /* */
    int recvd_pkts;                                          /* */
    int port_pkt_matched[MAX_EGRESS_ENTRIES];                /* */
    int port_pkt_mismatched[MAX_EGRESS_ENTRIES];             /* */
    int rx_start = FALSE;                                    /* */
    int total_pkts_recvd;                                    /* */
    int ingress_pkt_recvd;                                   /* */
    int egress_pkts_recvd;                                   /* */
    int egress_entries;                                      /* */
    int status, retcode;                                     /* */
    uint32 dump_flag = BCM_PKTIO_SYNC_RX_PKT_DUMP;           /* */
    pkt_verif_data_t *ingress_entry_ptr;                     /* */
    pkt_verif_data_t *egress_entry_ptr;                      /* */
                                                             /* */
    if (!opt_dump_flag)
        dump_flag = 0;                                                      /* */
                                                                            /* */
    /* If ingress_pkt is NULL for ingress, then it is assumed that user */  /* */
    /* has already sent the packet before calling this. In that case, it */ /* */
    /* is also assumed that user has called bcm_pktio_sync_rx_start() */    /* */
    /* already. */                                                          /* */
    ingress_entry_ptr = &verif_data->ingress_pkt;                           /* */
    if (ingress_entry_ptr->packet != NULL)
    {                                                       /* */
        /* Start sync Rx */                                 /* */
        rc = bcm_pktio_sync_rx_start(unit, dump_flag, &sn); /* */
        if (BCM_E_NONE != rc)
        {                                                                   /* */
            printf("Error(%s) while executing bcm_pktio_sync_rx_start()\n", /* */
                   bcm_errmsg(rc));                                         /* */
            return rc;                                                      /* */
        }                                                                   /* */
        rx_start = TRUE;                                                    /* */
        printf("\n\n%s (port:%d)\n", ingress_entry_ptr->description,        /* */
               *ingress_entry_ptr->expected_port,                           /* */
               port_to_str(ingress_entry_ptr->expected_port));              /* */
        snprintf(str, 2 * 1024, "tx 1 pbm=%d data=0x%s",                    /* */
                 *ingress_entry_ptr->expected_port,                         /* */
                 process_send_pkt(ingress_entry_ptr->packet));              /* */
        bbshell(unit, str);                                                 /* */
        sal_sleep(1);                                                       /* */
    }                                                                       /* */
    if (!verif_data->is_punted_to_cpu)
    {                                                /* */
        ingress_pkt_recvd = 1; /* Assume received */ /* */
    }
    else
    {                          /* */
        ingress_pkt_recvd = 0; /* */
    }                          /* */
    total_pkts_recvd = 0;      /* */
    egress_pkts_recvd = 0;     /* */
    status = 0;
    status = 0;                                                          /* */
    sal_memset(&port_pkt_matched, 0x00, sizeof(port_pkt_matched));       /* */
    sal_memset(&port_pkt_mismatched, 0x00, sizeof(port_pkt_mismatched)); /* */
    while (1)
    { /* */
        if (opt_verbose)
            printf("\n\t Waiting for packet...%d\n",                       /* */
                   total_pkts_recvd + 1);                                  /* */
        rc = bcm_pktio_sync_rx(unit, sn, &pkt, 2 * 1000 * 1000 /*usecs*/); /* */
        if (BCM_E_NONE != rc)
        { /* */
            if (total_pkts_recvd == 0)
            {                                                                 /* */
                printf("Error(%d)(%s) while executing bcm_pktio_sync_rx()\n", /* */
                       rc, bcm_errmsg(rc));                                   /* */
                status = rc;                                                  /* */
            }                                                                 /* */
            break;                                                            /* */
        }                                                                     /* */
        total_pkts_recvd += 1;                                                /* */
        rc = bcm_pktio_pkt_data_get(unit, pkt, (void *)&buffer, &length);     /* */
        if (BCM_E_NONE != rc)
        {                                                                      /* */
            printf("Error(%s) while executing bcm_pktio_pkt_data_get()\n",     /* */
                   bcm_errmsg(rc));                                            /* */
            status = rc;                                                       /* */
            break;                                                             /* */
        }                                                                      /* */
        /* Get source port metadata */                                         /* */
        rc = bcm_pktio_pmd_field_get(unit, pkt, bcmPktioPmdTypeRx,             /* */
                                     BCM_PKTIO_RXPMD_SRC_PORT_NUM, &src_port); /* */
        if (BCM_E_NONE != rc)
        {                                                                   /* */
            printf("Error(%s) while executing bcm_pktio_pmd_field_get()\n", /* */
                   bcm_errmsg(rc));                                         /* */
            status = rc;                                                    /* */
            break;                                                          /* */
        }                                                                   /* */
        /* Print the template entry if 1st egress entry is set to */        /* */
        /* NULL and for Egress ONLY */                                      /* */
        if ((verif_data->egress_pkt[0].packet == NULL) &&                   /* */
            (ingress_pkt_recvd > 0))
        {                                                               /* */
            print_template_entry(src_port,                              /* */
                                 verif_data->egress_pkt[0].description, /* */
                                 buffer, length);                       /* */
        }                                                               /* */
        if (ingress_pkt_recvd != 0)
        { /* Only egress packets */           /* */
            egress_pkts_recvd += 1;           /* */
        }                                     /* */
        egress_entries = 0;                   /* */
        /* Loop through the egress entries */ /* */
        for (i = 0;; i++)
        {                                                               /* */
            egress_entry_ptr = &verif_data->egress_pkt[egress_entries]; /* */
            /* Also expected is ingress copy. Handle it. */             /* */
            if (ingress_pkt_recvd == 0)
            { /* */
                if (src_port == *ingress_entry_ptr->expected_port)
                {                                                             /* */
                    retcode = bytes_vs_string_cmp(buffer, length,             /* */
                                                  ingress_entry_ptr->packet); /* */
                    if (retcode == 0)
                    {                           /* */
                        ingress_pkt_recvd += 1; /* */
                    }
                    else if (retcode < 0)
                    {             /* */
                        exit(-1); /* */
                    }
                    else
                    {                                                               /* */
                        printf("-- Packet arrived on INGRESS PORT %d. ", src_port); /* */
                        printf("    DID NOT match [%s].\n",                         /* */
                               ingress_entry_ptr->description);

                        status = -1; /* */
                        break;       /* */
                    }                /* */
                    continue;        /* */
                }                    /* */
            }                        /* */
            if (egress_entry_ptr->expected_port == NULL)
            {          /* */
                break; /* */
            }          /* */
            if (src_port == *egress_entry_ptr->expected_port)
            { /* */
                if (opt_verbose)
                {                                                        /* */
                    printf("Packet on expected egress PORT %d [%s].\n",  /* */
                           src_port, port_to_str(&src_port));            /* */
                    printf("RCVD PACKET(%d): ", length);                 /* */
                    print_buffer(buffer, length, -1);                    /* */
                }                                                        /* */
                retcode = bytes_vs_string_cmp(buffer, length,            /* */
                                              egress_entry_ptr->packet); /* */
                if (retcode == 0)
                {                                          /* */
                    port_pkt_matched[egress_entries] += 1; /* */
                }
                else if (retcode < 0)
                {             /* */
                    exit(-1); /* */
                }
                else
                {                                                               /* */
                    port_pkt_mismatched[egress_entries] += 1;                   /* */
                    clr_red();                                                  /* */
                    printf("\nVerificaion FAILED for %d [%s] on port %d [%s] ", /* */
                           egress_entries, egress_entry_ptr->description,       /* */
                           src_port, port_to_str(&src_port));                   /* */
                    printf("  for [%s].\n", ingress_entry_ptr->description);    /* */
                    status = -1;                                                /* */
                    clr_noc();                                                  /* */
                }                                                               /* */
            }                                                                   /* */
            egress_entries += 1;                                                /* */
        }                                                                       /* */
        if (status < 0)
        {          /* */
            break; /* */
        }          /* */
    }              /* */
    if (opt_verbose)
    {                                                                      /* */
        printf("Total PACKETS so far          : %d\n", total_pkts_recvd);  /* */
        printf("Total INGRESS packet received : %d\n", ingress_pkt_recvd); /* */
        printf("Total EGRESS packets received for this ingress: %d\n",     /* */
               egress_pkts_recvd);                                         /* */
        printf("Total EGRESS entries: %d\n", egress_entries);              /* */
    }                                                                      /* */
    int times_egress_matched = 0;                                          /* */
    if (ingress_pkt_recvd == 0)
    {                                                                        /* */
        clr_red();                                                           /* */
        printf("  Ingress packet [%s] from port %d [%s] not seen on CPU.\n", /* */
               ingress_entry_ptr->description,                               /* */
               *ingress_entry_ptr->expected_port,                            /* */
               port_to_str(ingress_entry_ptr->expected_port));               /* */
        return -1;                                                           /* */
    }
    for (i = 0; i < egress_entries; ++i)
    { /* */
        if (port_pkt_matched[i])
        {                                                  /* */
            times_egress_matched += port_pkt_matched[i];   /* */
            clr_grn();                                     /* */
            printf("PKT MATCHED for egress pkt [%s].\n",   /* */
                   verif_data->egress_pkt[i].description); /* */
            clr_noc();                                     /* */
        }                                                  /* */
    }                                                      /* */
    printf("\n");                                          /* */
    int match_status = -1;                                 /* */
    if ((verif_data->expected_recv == 0) && (egress_pkts_recvd == 0))
    {                                                              /* */
        match_status = 0; /* For handling DROP case as expected */ /* */
    }
    else if (status == 0)
    { /* */
        if (verif_data->expected_recv == egress_pkts_recvd)
        {                     /* */
            match_status = 0; /* */
        }                     /* */
        else
        {                                                                   /* */
            match_status = 1;                                               /* */
            clr_red();                                                      /* */
            printf("  Receive MORE or LESS packets for ingress entry %d\n", /* */
                   template_index);                                         /* */
            printf("   Number of expected rx: %d vs  received %d.\n",       /* */
                   verif_data->expected_recv, egress_pkts_recvd);           /* */
            clr_noc();                                                      /* */
        }                                                                   /* */
    }                                                                       /* */
    if (rx_start)
    {                                          /* */
        rc = bcm_pktio_sync_rx_stop(unit, sn); /* */
        if (BCM_E_NONE != rc)
        {                                                                   /* */
            printf("Error(%s) while executing bcm_pktio_sync_rx_start()\n", /* */
                   bcm_errmsg(rc));                                         /* */
            return rc;                                                      /* */
        }                                                                   /* */
    }                                                                       /* */
    return match_status;                                                    /* */
}