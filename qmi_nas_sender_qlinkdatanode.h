#ifndef QMI_NAS_SENDER_qlinkdatanode_H_INCLUDED
#define QMI_NAS_SENDER_qlinkdatanode_H_INCLUDED

#define RADIO_IF_NO_SVC (0x00) // None (no service)
#define RADIO_IF_CDMA_1X (0x01)
#define RADIO_IF_CDMA_1XEVDO (0x02)
#define RADIO_IF_AMPS (0x03)
#define RADIO_IF_GSM (0x04)
#define RADIO_IF_UMTS (0x05)
#define RADIO_IF_LTE (0x08)

/* Signal level conversion result codes */
#define RSSI_TOOLO_CODE (0)
#define RSSI_TOOHI_CODE (31)
#define RSSI_UNKNOWN_CODE (99)

/* RSSI range conversion */
#define RSSI_MIN (51)   /* per 3GPP 27.007  (negative value) */
#define RSSI_MAX (113)  /* per 3GPP 27.007  (negative value) */
#define RSSI_NO_SIGNAL (125)  /* from CM */
#define RSSI_OFFSET (182.26)
#define RSSI_SLOPE (-100.0/62.0)

#define DSAT_CSQ_MAX_SIGNAL RSSI_TOOHI_CODE

typedef struct {
	unsigned char plmn[3];
}Proto_Pack_Plmn_S_Type;

#endif 
