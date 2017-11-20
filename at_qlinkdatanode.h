#ifndef AT_qlinkdatanode_H_INCLUDED
#define AT_qlinkdatanode_H_INCLUDED

	#include "event_qlinkdatanode.h"
	#include "common_qlinkdatanode.h" // Net_Config
//	#include "protocol_qlinkdatanode.h"
	#include "feature_macro_qlinkdatanode.h"
	
	#define DEFAULT_MFG_INF_BUF_SIZE (1024)
	#define DEFAULT_RETRY_EXEC_AT_CMD_INTERVAL_TIME (1500*1000) // Unit: us
	
	#define DEFAULT_PDP_CTX_INF_BUF_SIZE (1024)

typedef struct _At_Config_Tag{
  int serial_fd;
  Evt AT_event;
  Evt BootInfo_evt;
  Server_Config net_config;
}At_Config;

typedef struct {
//Description:
//Once app recved an APDU, its content will be stored into a dynamic memory buffer which is pointed
//by pointer APDU_cache. And it will be updated if APDU is tried to send out.
  void *data;
  int data_len;
}APDU_Cache_S_Type;


#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

#define QMI_UIM_AUTHENTICATE_DATA_MAX_V01 1024
#if 1
typedef enum {
  TEST_UIM_AUTH_CONTEXT_ENUM_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  TEST_UIM_AUTH_CONTEXT_RUN_GSM_ALG_V01 = 0x00, 
  TEST_UIM_AUTH_CONTEXT_RUN_CAVE_ALG_V01 = 0x01, 
  TEST_UIM_AUTH_CONTEXT_GSM_SEC_V01 = 0x02, 
  TEST_UIM_AUTH_CONTEXT_3G_SEC_V01 = 0x03, 
  TEST_UIM_AUTH_CONTEXT_VGCS_VBS_SEC_V01 = 0x04, 
  TEST_UIM_AUTH_CONTEXT_GBA_SEC_BOOTSTRAPPING_V01 = 0x05, 
  TEST_UIM_AUTH_CONTEXT_GBA_SEC_NAF_DERIVATION_V01 = 0x06, 
  TEST_UIM_AUTH_CONTEXT_MBMS_SEC_MSK_UPDATE_V01 = 0x07, 
  TEST_UIM_AUTH_CONTEXT_MBMS_SEC_MTK_GENERATION_V01 = 0x08, 
  TEST_UIM_AUTH_CONTEXT_MBMS_SEC_MSK_DELETION_V01 = 0x09, 
  TEST_UIM_AUTH_CONTEXT_MBMS_SEC_MUK_DELETION_V01 = 0x0A, 
  TEST_UIM_AUTH_CONTEXT_IMS_AKA_SEC_V01 = 0x0B, 
  TEST_UIM_AUTH_CONTEXT_HTTP_DIGEST_SEC_V01 = 0x0C, 
  TEST_UIM_AUTH_CONTEXT_COMPUTE_IP_CHAP_V01 = 0x0D, 
  TEST_UIM_AUTH_CONTEXT_COMPUTE_IP_MN_HA_V01 = 0x0E, 
  TEST_UIM_AUTH_CONTEXT_COMPUTE_IP_MIP_RRQ_HASH_V01 = 0x0F, 
  TEST_UIM_AUTH_CONTEXT_COMPUTE_IP_MN_AAA_V01 = 0x10, 
  TEST_UIM_AUTH_CONTEXT_COMPUTE_IP_HRPD_ACCESS_V01 = 0x11, 
  TEST_UIM_AUTH_CONTEXT_ENUM_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}test_uim_auth_context_enum_v01;

#endif
typedef struct {


  test_uim_auth_context_enum_v01 context;
  /**<   Authenticate context. Valid values:\n
        - 0  -- Runs the GSM alogrithm (valid only on a 2G SIM card, as
                specified in \hyperref[S8]{[S8]})\n
        - 1  -- Runs the CAVE algorithm (valid only on a RUIM card, as
                specified in \hyperref[S9]{[S9]})\n
        - 2  -- GSM security context (valid only on a USIM application, as
                specified in \hyperref[S4]{[S4]})\n
        - 3  -- 3G security context (valid only on a USIM application, as
                specified in \hyperref[S4]{[S4]})\n
        - 4  -- VGCS/VBS security context (valid only on a USIM application, as
                specified in \hyperref[S4]{[S4]})\n
        - 5  -- GBA security context, Bootstrapping mode (valid only on
                a USIM or ISIM application, as
                specified in \hyperref[S4]{[S4]} and
                \hyperref[S10]{[S10]})\n
        - 6  -- GBA security context, NAF Derivation mode (valid only on
                a USIM or ISIM application, as
                specified in \hyperref[S4]{[S4]} and
                \hyperref[S10]{[S10]})\n
        - 7  -- MBMS security context, MSK Update mode (valid only on
                a USIM application, as
                specified in \hyperref[S4]{[S4]})\n
        - 8  -- MBMS security context, MTK Generation mode (valid only
                on a USIM application, as
                specified in \hyperref[S4]{[S4]})\n
        - 9  -- MBMS security context, MSK Deletion mode (valid only on
                a USIM application, as specified in
                \hyperref[S4]{[S4]})\n
        - 10 -- MBMS security context, MUK Deletion mode (valid only on
                a USIM application, as specified in
                \hyperref[S4]{[S4]})\n
        - 11 -- IMS AKA security context (valid only on a ISIM application, as
                specified in \hyperref[S10]{[S10]})\n
        - 12 -- HTTP-digest security context valid only on an ISIM
                application, as specified in
                \hyperref[S10]{[S10]})\n
        - 13 -- Compute IP authentication, CHAP (valid only on RUIM or CSIM, as
                specified in \hyperref[S9]{[S9]} and
                \hyperref[S11]{[S11]})\n

                 - 14 -- Compute IP authentication, MN-HA authenticator (valid only on
                RUIM or CSIM, as specified in
                \hyperref[S9]{[S9]} and
                \hyperref[S11]{[S11]})\n
        - 15 -- Compute IP authentication, MIP-RRQ hash (valid only on RUIM
                or CSIM, as specified in
                \hyperref[S9]{[S9]} and
                \hyperref[S11]{[S11]})\n
        - 16 -- Compute IP authentication, MN-AAA authenticator (valid only
                on RUIM or CSIM, as specified in
                \hyperref[S9]{[S9]} and
                \hyperref[S11]{[S11]})\n
        - 17 -- Compute IP authentication, HRPD access authenticator (valid
                only on RUIM or CSIM, as specified in
                \hyperref[S9]{[S9]} and
                \hyperref[S11]{[S11]})\n
       Other values are possible and reserved for future use.
  */

  uint32_t data_len;  /**< Must be set to # of elements in data */
  uint8_t data[QMI_UIM_AUTHENTICATE_DATA_MAX_V01];
  /**<   Authenticate data.*/
}test_uim_authentication_data_type_v01; 


typedef unsigned char BYTE;
// define SEQMS type for checking if SQN is available, this struct should be stored in usim 

typedef struct
	{
	BYTE seq[6];
	}SEQTYPE;

#define delta 0x10000000
#define limit 0x100000


/* This enum defines the first status word                                   */
typedef enum {
  SW1_NORMAL_END = 0x90,  /* Normal ending of the command                    */
  SW1_END_FETCH  = 0x91,  /* Normal ending of the command with extra command */
  SW1_DLOAD_ERR  = 0x9E,  /* SIM data download error */
  SW1_END_RESP   = 0x9F,  /* Normal ending of the command with a response    */
  SW1_BUSY       = 0x93,  /* SIM App toolkit is busy                         */
  SW1_END_RETRY  = 0x92,  /* Command successful with update retries or memory
                             error                                           */
  SW1_REFERENCE  = 0x94,  /* A reference management problem                  */
  SW1_SECURITY   = 0x98,  /* A security problem                              */
  SW1_NULL       = 0x60,  /* NULL procedure byte                             */
  SW1_P3_BAD     = 0x67,  /* Incorrect parameter P3                          */
  SW1_P1_P2_BAD  = 0x6B,  /* Incorrect parameter P1 or P2                    */
  SW1_INS_BAD    = 0x6D,  /* Unknown instruction code                        */
  SW1_CLA_BAD    = 0x6E,  /* Wrong instruction class given in command        */
  SW1_PROBLEM    = 0x6F,  /* Technical problem; no diagnostic given          */
  SW1_SIG_ERROR1 = 0xFF,  /* This is an indication of an error signal from the
                             transmitted byte                                */
  SW1_SIG_ERROR2 = 0xFE,  /* This is an indication of an error signal from the
                             transmitted byte                                */
  SW1_NO_STATUS  = 0x00,   /* This status word is used when a status word cannot
                             be obtained from the card.  It offers to status
                             information */
  SW1_WARNINGS1 = 0x62,     /* Warnings                                      */
  SW1_WARNINGS2 = 0x63,     /* Warnings                                      */
  SW1_EXECUTION_ERR2    = 0x65, /* No info given, state of NV memory changed */
  SW1_CMD_NOT_ALLOWED   = 0x69, /* Command not allowed                       */
  SW1_WRONG_PARAMS      = 0x6A, /* Wrong parameters                          */

  SW1_USIM_RESEND_APDU  = 0x6C, /* Re-send the APDU header                   */
  SW1_EXECUTION_ERR1    = 0x64, /* No info given, NV memory unchanged        */
  SW1_CLA_NOT_SUPPORTED = 0x68, /* functions in CLA not supported            */
  SW1_USIM_END_RESP     = 0x61, /* Normal ending of a command                */

  SW1_UTIL_END_RESP     = 0x61, /* Normal ending of a command                */
  SW1_LENGTH_ERROR      = 0x6C, /* Wrong length Le                           */
  SW1_SE_FAIL           = 0x66, /* Security Environment error/not set         */
  SW1_95_NFC            = 0x95, /* SW1 = 0x95 used for NFC                    */
  SW1_96_NFC            = 0x96, /* SW1 = 0x96 used for NFC                    */
  SW1_97_NFC            = 0x97, /* SW1 = 0x97 used for NFC                    */
  SW1_99_NFC            = 0x99, /* SW1 = 0x99 used for NFC                    */
  SW1_9A_NFC            = 0x9A, /* SW1 = 0x9A used for NFC                    */
  SW1_9B_NFC            = 0x9B, /* SW1 = 0x9B used for NFC                    */
  SW1_9C_NFC            = 0x9C, /* SW1 = 0x9C used for NFC                    */
  SW1_9D_NFC            = 0x9D  /* SW1 = 0x9D used for NFC                    */
} uim_sw1_type;



/* This enum defines the second status word                                  */
typedef enum {
  SW2_NORMAL_END    = 0x00,  /* Normal ending of the command                 */
  SW2_MEM_PROBLEM   = 0x40,  /* Memory problem                               */
  SW2_NO_EF         = 0x00,  /* No EF selected                               */
  SW2_OUT_OF_RANGE  = 0x02,  /* Out of range (invalid address)               */
  SW2_NOT_FOUND     = 0x04,  /* File ID/pattern not found                    */
  SW2_INCONSISTENT  = 0x08,  /* File inconsistent with command               */
  SW2_NO_CHV        = 0x02,  /* No CHV initialized                           */
  SW2_NOT_FULFILLED = 0x04,  /* Access condition not fulfilled               */
  SW2_CONTRADICTION = 0x08,  /* In contradiction with CHV status             */
  SW2_INVALIDATION  = 0x10,  /* In contradiction with invalidation status    */
  SW2_SEQ_PROBLEM   = 0x34,  /* Problem in the sequence of the command       */
  SW2_BLOCKED       = 0x40,  /* Access blocked                               */
  SW2_MAX_REACHED   = 0x50,  /* Increase cannot be performed; Max value
                                reached                                      */
  SW2_ALGORITHM_NOT_SUPPORTED   = 0x84,
  SW2_INVALID_KEY_CHECK_VALUE   = 0x85,
  SW2_PIN_BLOCKED               = 0x83,
  SW2_NO_EF_SELECTED            = 0x86,
  SW2_FILE_NOT_FOUND            = 0x82,
  SW2_REF_DATA_NOT_FOUND        = 0x88,
  SW2_NV_STATE_UNCHANGED        = 0x00,
  SW2_CMD_SUCCESS_INT_X_RETRIES = 0xC0,
  SW2_MEMORY_PROBLEM            = 0x81,
  SW2_SECURITY_NOT_SATISFIED    = 0x82,
  SW2_NO_INFO_GIVEN             = 0x00,
  SW2_LOGICAL_CHAN_NOT_SUPPORTED= 0x81,
  SW2_SECURE_MSG_NOT_SUPPORTED  = 0x82,
  SW2_FILE_STRUCT_INCOMPATIBLE  = 0x81,
  SW2_REF_DATA_INVALIDATED      = 0x84,
  SW2_CONDITIONS_NOT_SATISFIED  = 0x85,
  SW2_BAD_PARAMS_IN_DATA_FIELD  = 0x80,
  SW2_FUNCTION_NOT_SUPPORTED    = 0x81,
  SW2_RECORD_NOT_FOUND          = 0x83,
  SW2_INCONSISTENT_LC           = 0x85,
  SW2_BAD_PARAMS_P1_P2          = 0x86,
  SW2_LC_INCONSISTENT_WITH_P1_P2= 0x87,
  SW2_NOT_ENOUGH_MEMORY_SPACE   = 0x84,
  SW2_NV_STATE_CHANGED          = 0x00,
  SW2_MORE_DATA_AVAILABLE       = 0x10,

  SW2_RET_DATA_MAY_BE_CORRUPTED = 0x81,
  SW2_EOF_REACHED               = 0x82,
  SW2_SEL_EF_INVALIDATED        = 0x83,
  SW2_INCORRECT_FCI_FORMAT      = 0x84,

  SW2_FILE_FILLED_BY_LAST_WRITE = 0x81,

  SW2_APPLN_SPECIFIC_AUTH_ERR   = 0x62,

  SW2_PIN_STATUS_ERR     = 0x85, /* In contradiction with PIN status */
  SW2_PREMSTR_NOT_RDY    = 0x85, /* Pre-master secret not ready */
  SW2_INTRNL_NOT_RDY     = 0x85, /* Internal data not ready */
  SW2_INCORRECT_TAG      = 0x80, /* Incorrect data or tag */
  SW2_UNDEF_99           = 0x99,
  SW2_MORE_DATA          = 0xF1,
  SW2_AUTH_RSP_DATA      = 0xF3, /* Authentication Response Data Available*/

  SW2_UCAST_AUTH_KEY_NOT_INIT                = 0x01,   /* sw1 = 0x69 */
  SW2_UCAST_DATA_BLOCK_NOT_INIT              = 0x02,   /* sw1 = 0x69 */
  SW2_VERIFY_UCAST_FAILED                    = 0x04,   /* sw1 = 0x69 */
  SW2_LOCKED_INIT_PARAM                      = 0x05,   /* sw1 = 0x69 */
  SW2_APPLET_NOT_INIT                        = 0x06,   /* sw1 = 0x69 */
  SW2_PUBLIC_KEY_OR_CERT_NOT_INIT            = 0x07,   /* sw1 = 0x69 */
  SW2_WARNING_NFC_C1                         = 0xC1,   /* sw1 = 0x63 */
  SW2_SW1_PROBLEM                            = 0x00    /* sw1 = 0x6F */

} uim_sw2_type;


#endif

#endif // AT_qlinkdatanode_H_INCLUDED