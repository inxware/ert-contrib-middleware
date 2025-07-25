/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __ESP_WIFI_TYPES_H__
#define __ESP_WIFI_TYPES_H__

#include "esp_private/esp_wifi_types_private.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MODE_NULL = 0,  /**< null mode */
    WIFI_MODE_STA,       /**< WiFi station mode */
    WIFI_MODE_AP,        /**< WiFi soft-AP mode */
    WIFI_MODE_APSTA,     /**< WiFi station + soft-AP mode */
    WIFI_MODE_MAX
} wifi_mode_t;

typedef enum {
    WIFI_IF_STA = ESP_IF_WIFI_STA,
    WIFI_IF_AP  = ESP_IF_WIFI_AP,
} wifi_interface_t;

#define WIFI_OFFCHAN_TX_REQ      1
#define WIFI_OFFCHAN_TX_CANCEL   0

#define WIFI_ROC_REQ     1
#define WIFI_ROC_CANCEL  0

typedef enum {
    WIFI_COUNTRY_POLICY_AUTO,   /**< Country policy is auto, use the country info of AP to which the station is connected */
    WIFI_COUNTRY_POLICY_MANUAL, /**< Country policy is manual, always use the configured country info */
} wifi_country_policy_t;

/** @brief Structure describing WiFi country-based regional restrictions. */
typedef struct {
    char                  cc[3];   /**< country code string */
    uint8_t               schan;   /**< start channel */
    uint8_t               nchan;   /**< total channel number */
    int8_t                max_tx_power;   /**< This field is used for getting WiFi maximum transmitting power, call esp_wifi_set_max_tx_power to set the maximum transmitting power. */
    wifi_country_policy_t policy;  /**< country policy */
} wifi_country_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,         /**< authenticate mode : open */
    WIFI_AUTH_WEP,              /**< authenticate mode : WEP */
    WIFI_AUTH_WPA_PSK,          /**< authenticate mode : WPA_PSK */
    WIFI_AUTH_WPA2_PSK,         /**< authenticate mode : WPA2_PSK */
    WIFI_AUTH_WPA_WPA2_PSK,     /**< authenticate mode : WPA_WPA2_PSK */
    WIFI_AUTH_WPA2_ENTERPRISE,  /**< authenticate mode : WPA2_ENTERPRISE */
    WIFI_AUTH_WPA3_PSK,         /**< authenticate mode : WPA3_PSK */
    WIFI_AUTH_WPA2_WPA3_PSK,    /**< authenticate mode : WPA2_WPA3_PSK */
    WIFI_AUTH_WAPI_PSK,         /**< authenticate mode : WAPI_PSK */
    WIFI_AUTH_MAX
} wifi_auth_mode_t;

typedef enum {
    WIFI_REASON_UNSPECIFIED                        = 1,
    WIFI_REASON_AUTH_EXPIRE                        = 2,
    WIFI_REASON_AUTH_LEAVE                         = 3,
    WIFI_REASON_ASSOC_EXPIRE                       = 4,
    WIFI_REASON_ASSOC_TOOMANY                      = 5,
    WIFI_REASON_NOT_AUTHED                         = 6,
    WIFI_REASON_NOT_ASSOCED                        = 7,
    WIFI_REASON_ASSOC_LEAVE                        = 8,
    WIFI_REASON_ASSOC_NOT_AUTHED                   = 9,
    WIFI_REASON_DISASSOC_PWRCAP_BAD                = 10,
    WIFI_REASON_DISASSOC_SUPCHAN_BAD               = 11,
    WIFI_REASON_BSS_TRANSITION_DISASSOC            = 12,
    WIFI_REASON_IE_INVALID                         = 13,
    WIFI_REASON_MIC_FAILURE                        = 14,
    WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT             = 15,
    WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT           = 16,
    WIFI_REASON_IE_IN_4WAY_DIFFERS                 = 17,
    WIFI_REASON_GROUP_CIPHER_INVALID               = 18,
    WIFI_REASON_PAIRWISE_CIPHER_INVALID            = 19,
    WIFI_REASON_AKMP_INVALID                       = 20,
    WIFI_REASON_UNSUPP_RSN_IE_VERSION              = 21,
    WIFI_REASON_INVALID_RSN_IE_CAP                 = 22,
    WIFI_REASON_802_1X_AUTH_FAILED                 = 23,
    WIFI_REASON_CIPHER_SUITE_REJECTED              = 24,
    WIFI_REASON_TDLS_PEER_UNREACHABLE              = 25,
    WIFI_REASON_TDLS_UNSPECIFIED                   = 26,
    WIFI_REASON_SSP_REQUESTED_DISASSOC             = 27,
    WIFI_REASON_NO_SSP_ROAMING_AGREEMENT           = 28,
    WIFI_REASON_BAD_CIPHER_OR_AKM                  = 29,
    WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION       = 30,
    WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS        = 31,
    WIFI_REASON_UNSPECIFIED_QOS                    = 32,
    WIFI_REASON_NOT_ENOUGH_BANDWIDTH               = 33,
    WIFI_REASON_MISSING_ACKS                       = 34,
    WIFI_REASON_EXCEEDED_TXOP                      = 35,
    WIFI_REASON_STA_LEAVING                        = 36,
    WIFI_REASON_END_BA                             = 37,
    WIFI_REASON_UNKNOWN_BA                         = 38,
    WIFI_REASON_TIMEOUT                            = 39,
    WIFI_REASON_PEER_INITIATED                     = 46,
    WIFI_REASON_AP_INITIATED                       = 47,
    WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT      = 48,
    WIFI_REASON_INVALID_PMKID                      = 49,
    WIFI_REASON_INVALID_MDE                        = 50,
    WIFI_REASON_INVALID_FTE                        = 51,
    WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED = 67,
    WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED        = 68,

    WIFI_REASON_BEACON_TIMEOUT                     = 200,
    WIFI_REASON_NO_AP_FOUND                        = 201,
    WIFI_REASON_AUTH_FAIL                          = 202,
    WIFI_REASON_ASSOC_FAIL                         = 203,
    WIFI_REASON_HANDSHAKE_TIMEOUT                  = 204,
    WIFI_REASON_CONNECTION_FAIL                    = 205,
    WIFI_REASON_AP_TSF_RESET                       = 206,
    WIFI_REASON_ROAMING                            = 207,
    WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG       = 208,
    WIFI_REASON_SA_QUERY_TIMEOUT                   = 209,
} wifi_err_reason_t;

typedef enum {
    WIFI_SECOND_CHAN_NONE = 0,  /**< the channel width is HT20 */
    WIFI_SECOND_CHAN_ABOVE,     /**< the channel width is HT40 and the secondary channel is above the primary channel */
    WIFI_SECOND_CHAN_BELOW,     /**< the channel width is HT40 and the secondary channel is below the primary channel */
} wifi_second_chan_t;

typedef enum {
    WIFI_SCAN_TYPE_ACTIVE = 0,  /**< active scan */
    WIFI_SCAN_TYPE_PASSIVE,     /**< passive scan */
} wifi_scan_type_t;

/** @brief Range of active scan times per channel */
typedef struct {
    uint32_t min;  /**< minimum active scan time per channel, units: millisecond */
    uint32_t max;  /**< maximum active scan time per channel, units: millisecond, values above 1500ms may
                                          cause station to disconnect from AP and are not recommended.  */
} wifi_active_scan_time_t;

/** @brief Aggregate of active & passive scan time per channel */
typedef struct {
    wifi_active_scan_time_t active;  /**< active scan time per channel, units: millisecond. */
    uint32_t passive;                /**< passive scan time per channel, units: millisecond, values above 1500ms may
                                          cause station to disconnect from AP and are not recommended. */
} wifi_scan_time_t;

/** @brief Parameters for an SSID scan. */
typedef struct {
    uint8_t *ssid;               /**< SSID of AP */
    uint8_t *bssid;              /**< MAC address of AP */
    uint8_t channel;             /**< channel, scan the specific channel */
    bool show_hidden;            /**< enable to scan AP whose SSID is hidden */
    wifi_scan_type_t scan_type;  /**< scan type, active or passive */
    wifi_scan_time_t scan_time;  /**< scan time per channel */
    uint8_t home_chan_dwell_time;/**< time spent at home channel between scanning consecutive channels.*/
} wifi_scan_config_t;

typedef enum {
    WIFI_CIPHER_TYPE_NONE = 0,   /**< the cipher type is none */
    WIFI_CIPHER_TYPE_WEP40,      /**< the cipher type is WEP40 */
    WIFI_CIPHER_TYPE_WEP104,     /**< the cipher type is WEP104 */
    WIFI_CIPHER_TYPE_TKIP,       /**< the cipher type is TKIP */
    WIFI_CIPHER_TYPE_CCMP,       /**< the cipher type is CCMP */
    WIFI_CIPHER_TYPE_TKIP_CCMP,  /**< the cipher type is TKIP and CCMP */
    WIFI_CIPHER_TYPE_AES_CMAC128,/**< the cipher type is AES-CMAC-128 */
    WIFI_CIPHER_TYPE_SMS4,       /**< the cipher type is SMS4 */
    WIFI_CIPHER_TYPE_GCMP,       /**< the cipher type is GCMP */
    WIFI_CIPHER_TYPE_GCMP256,    /**< the cipher type is GCMP-256 */
    WIFI_CIPHER_TYPE_AES_GMAC128,/**< the cipher type is AES-GMAC-128 */
    WIFI_CIPHER_TYPE_AES_GMAC256,/**< the cipher type is AES-GMAC-256 */
    WIFI_CIPHER_TYPE_UNKNOWN,    /**< the cipher type is unknown */
} wifi_cipher_type_t;

/**
  * @brief WiFi antenna
  *
  */
typedef enum {
    WIFI_ANT_ANT0,          /**< WiFi antenna 0 */
    WIFI_ANT_ANT1,          /**< WiFi antenna 1 */
    WIFI_ANT_MAX,           /**< Invalid WiFi antenna */
} wifi_ant_t;

/** @brief Description of a WiFi AP */
typedef struct {
    uint8_t bssid[6];                     /**< MAC address of AP */
    uint8_t ssid[33];                     /**< SSID of AP */
    uint8_t primary;                      /**< channel of AP */
    wifi_second_chan_t second;            /**< secondary channel of AP */
    int8_t  rssi;                         /**< signal strength of AP */
    wifi_auth_mode_t authmode;            /**< authmode of AP */
    wifi_cipher_type_t pairwise_cipher;   /**< pairwise cipher of AP */
    wifi_cipher_type_t group_cipher;      /**< group cipher of AP */
    wifi_ant_t ant;                       /**< antenna used to receive beacon from AP */
    uint32_t phy_11b:1;                   /**< bit: 0 flag to identify if 11b mode is enabled or not */
    uint32_t phy_11g:1;                   /**< bit: 1 flag to identify if 11g mode is enabled or not */
    uint32_t phy_11n:1;                   /**< bit: 2 flag to identify if 11n mode is enabled or not */
    uint32_t phy_lr:1;                    /**< bit: 3 flag to identify if low rate is enabled or not */
    uint32_t wps:1;                       /**< bit: 4 flag to identify if WPS is supported or not */
    uint32_t ftm_responder:1;             /**< bit: 5 flag to identify if FTM is supported in responder mode */
    uint32_t ftm_initiator:1;             /**< bit: 6 flag to identify if FTM is supported in initiator mode */
    uint32_t reserved:25;                 /**< bit: 7..31 reserved */
    wifi_country_t country;               /**< country information of AP */
} wifi_ap_record_t;

typedef enum {
    WIFI_FAST_SCAN = 0,                   /**< Do fast scan, scan will end after find SSID match AP */
    WIFI_ALL_CHANNEL_SCAN,                /**< All channel scan, scan will end after scan all the channel */
}wifi_scan_method_t;

typedef enum {
    WIFI_CONNECT_AP_BY_SIGNAL = 0,        /**< Sort match AP in scan list by RSSI */
    WIFI_CONNECT_AP_BY_SECURITY,          /**< Sort match AP in scan list by security mode */
}wifi_sort_method_t;

/** @brief Structure describing parameters for a WiFi fast scan */
typedef struct {
    int8_t              rssi;             /**< The minimum rssi to accept in the fast scan mode */
    wifi_auth_mode_t    authmode;         /**< The weakest authmode to accept in the fast scan mode */
}wifi_scan_threshold_t;

typedef enum {
    WIFI_PS_NONE,        /**< No power save */
    WIFI_PS_MIN_MODEM,   /**< Minimum modem power saving. In this mode, station wakes up to receive beacon every DTIM period */
    WIFI_PS_MAX_MODEM,   /**< Maximum modem power saving. In this mode, interval to receive beacons is determined by the listen_interval parameter in wifi_sta_config_t */
} wifi_ps_type_t;

#define WIFI_PROTOCOL_11B         1
#define WIFI_PROTOCOL_11G         2
#define WIFI_PROTOCOL_11N         4
#define WIFI_PROTOCOL_LR          8

typedef enum {
    WIFI_BW_HT20 = 1, /* Bandwidth is HT20 */
    WIFI_BW_HT40,     /* Bandwidth is HT40 */
} wifi_bandwidth_t;

/** Configuration structure for Protected Management Frame */
typedef struct {
    bool capable;            /**< Deprecated variable. Device will always connect in PMF mode if other device also advertizes PMF capability. */
    bool required;           /**< Advertizes that Protected Management Frame is required. Device will not associate to non-PMF capable devices. */
} wifi_pmf_config_t;

/** Configuration for SAE PWE derivation */
typedef enum {
    WPA3_SAE_PWE_UNSPECIFIED,
    WPA3_SAE_PWE_HUNT_AND_PECK,
    WPA3_SAE_PWE_HASH_TO_ELEMENT,
    WPA3_SAE_PWE_BOTH,
} wifi_sae_pwe_method_t;

/** @brief Soft-AP configuration settings for the ESP32 */
typedef struct {
    uint8_t ssid[32];           /**< SSID of ESP32 soft-AP. If ssid_len field is 0, this must be a Null terminated string. Otherwise, length is set according to ssid_len. */
    uint8_t password[64];       /**< Password of ESP32 soft-AP. */
    uint8_t ssid_len;           /**< Optional length of SSID field. */
    uint8_t channel;            /**< Channel of ESP32 soft-AP */
    wifi_auth_mode_t authmode;  /**< Auth mode of ESP32 soft-AP. Do not support AUTH_WEP in soft-AP mode */
    uint8_t ssid_hidden;        /**< Broadcast SSID or not, default 0, broadcast the SSID */
    uint8_t max_connection;     /**< Max number of stations allowed to connect in */
    uint16_t beacon_interval;   /**< Beacon interval which should be multiples of 100. Unit: TU(time unit, 1 TU = 1024 us). Range: 100 ~ 60000. Default value: 100 */
    wifi_cipher_type_t pairwise_cipher;   /**< pairwise cipher of SoftAP, group cipher will be derived using this. cipher values are valid starting from WIFI_CIPHER_TYPE_TKIP, enum values before that will be considered as invalid and default cipher suites(TKIP+CCMP) will be used. Valid cipher suites in softAP mode are WIFI_CIPHER_TYPE_TKIP, WIFI_CIPHER_TYPE_CCMP and WIFI_CIPHER_TYPE_TKIP_CCMP. */
    bool ftm_responder;         /**< Enable FTM Responder mode */
} wifi_ap_config_t;

/** @brief STA configuration settings for the ESP32 */
typedef struct {
    uint8_t ssid[32];      /**< SSID of target AP. */
    uint8_t password[64];  /**< Password of target AP. */
    wifi_scan_method_t scan_method;    /**< do all channel scan or fast scan */
    bool bssid_set;        /**< whether set MAC address of target AP or not. Generally, station_config.bssid_set needs to be 0; and it needs to be 1 only when users need to check the MAC address of the AP.*/
    uint8_t bssid[6];     /**< MAC address of target AP*/
    uint8_t channel;       /**< channel of target AP. Set to 1~13 to scan starting from the specified channel before connecting to AP. If the channel of AP is unknown, set it to 0.*/
    uint16_t listen_interval;   /**< Listen interval for ESP32 station to receive beacon when WIFI_PS_MAX_MODEM is set. Units: AP beacon intervals. Defaults to 3 if set to 0. */
    wifi_sort_method_t sort_method;    /**< sort the connect AP in the list by rssi or security mode */
    wifi_scan_threshold_t  threshold;     /**< When sort_method is set, only APs which have an auth mode that is more secure than the selected auth mode and a signal stronger than the minimum RSSI will be used. */
    wifi_pmf_config_t pmf_cfg;    /**< Configuration for Protected Management Frame. Will be advertized in RSN Capabilities in RSN IE. */
    uint32_t rm_enabled:1;        /**< Whether Radio Measurements are enabled for the connection */
    uint32_t btm_enabled:1;       /**< Whether BSS Transition Management is enabled for the connection */
    uint32_t mbo_enabled:1;       /**< Whether MBO is enabled for the connection */
    uint32_t transition_disable:1;      /**< Whether to enable transition disable feature */
    uint32_t reserved:28;         /**< Reserved for future feature set */
    wifi_sae_pwe_method_t sae_pwe_h2e;     /**< Whether SAE hash to element is enabled */
    uint8_t failure_retry_cnt;    /**< Number of connection retries station will do before moving to next AP. scan_method should be set as WIFI_ALL_CHANNEL_SCAN to use this config. Note: Enabling this may cause connection time to increase incase best AP doesn't behave properly. */
} wifi_sta_config_t;

/** @brief Configuration data for ESP32 AP or STA.
 *
 * The usage of this union (for ap or sta configuration) is determined by the accompanying
 * interface argument passed to esp_wifi_set_config() or esp_wifi_get_config()
 *
 */
typedef union {
    wifi_ap_config_t  ap;  /**< configuration of AP */
    wifi_sta_config_t sta; /**< configuration of STA */
} wifi_config_t;

/** @brief Description of STA associated with AP */
typedef struct {
    uint8_t mac[6];  /**< mac address */
    int8_t  rssi;    /**< current average rssi of sta connected */
    uint32_t phy_11b:1;      /**< bit: 0 flag to identify if 11b mode is enabled or not */
    uint32_t phy_11g:1;      /**< bit: 1 flag to identify if 11g mode is enabled or not */
    uint32_t phy_11n:1;      /**< bit: 2 flag to identify if 11n mode is enabled or not */
    uint32_t phy_lr:1;       /**< bit: 3 flag to identify if low rate is enabled or not */
    uint32_t is_mesh_child:1;/**< bit: 4 flag to identify mesh child */
    uint32_t reserved:27;    /**< bit: 5..31 reserved */
} wifi_sta_info_t;

#if CONFIG_IDF_TARGET_ESP32C3
#define ESP_WIFI_MAX_CONN_NUM  (10)       /**< max number of stations which can connect to ESP32C3 soft-AP */
#else
#define ESP_WIFI_MAX_CONN_NUM  (15)       /**< max number of stations which can connect to ESP32/ESP32S3/ESP32S2 soft-AP */
#endif

/** @brief List of stations associated with the ESP32 Soft-AP */
typedef struct {
    wifi_sta_info_t sta[ESP_WIFI_MAX_CONN_NUM]; /**< station list */
    int       num; /**< number of stations in the list (other entries are invalid) */
} wifi_sta_list_t;

typedef enum {
    WIFI_STORAGE_FLASH,  /**< all configuration will store in both memory and flash */
    WIFI_STORAGE_RAM,    /**< all configuration will only store in the memory */
} wifi_storage_t;

/**
  * @brief     Vendor Information Element type
  *
  * Determines the frame type that the IE will be associated with.
  */
typedef enum {
    WIFI_VND_IE_TYPE_BEACON,
    WIFI_VND_IE_TYPE_PROBE_REQ,
    WIFI_VND_IE_TYPE_PROBE_RESP,
    WIFI_VND_IE_TYPE_ASSOC_REQ,
    WIFI_VND_IE_TYPE_ASSOC_RESP,
} wifi_vendor_ie_type_t;

/**
  * @brief     Vendor Information Element index
  *
  * Each IE type can have up to two associated vendor ID elements.
  */
typedef enum {
    WIFI_VND_IE_ID_0,
    WIFI_VND_IE_ID_1,
} wifi_vendor_ie_id_t;

#define WIFI_VENDOR_IE_ELEMENT_ID 0xDD

/**
  * @brief     Operation Phymode
  */
typedef enum
{
    WIFI_PHY_MODE_LR,   /**< PHY mode for Low Rate */
    WIFI_PHY_MODE_11B,  /**< PHY mode for 11b */
    WIFI_PHY_MODE_11G,  /**< PHY mode for 11g */
    WIFI_PHY_MODE_HT20, /**< PHY mode for Bandwidth HT20 */
    WIFI_PHY_MODE_HT40, /**< PHY mode for Bandwidth HT40 */
    WIFI_PHY_MODE_HE20, /**< PHY mode for Bandwidth HE20 */
} wifi_phy_mode_t;

/**
 * @brief Vendor Information Element header
 *
 * The first bytes of the Information Element will match this header. Payload follows.
 */
typedef struct {
    uint8_t element_id;      /**< Should be set to WIFI_VENDOR_IE_ELEMENT_ID (0xDD) */
    uint8_t length;          /**< Length of all bytes in the element data following this field. Minimum 4. */
    uint8_t vendor_oui[3];   /**< Vendor identifier (OUI). */
    uint8_t vendor_oui_type; /**< Vendor-specific OUI type. */
    uint8_t payload[0];      /**< Payload. Length is equal to value in 'length' field, minus 4. */
} vendor_ie_data_t;

/** @brief Received packet radio metadata header, this is the common header at the beginning of all promiscuous mode RX callback buffers */
typedef struct {
    signed rssi:8;                /**< Received Signal Strength Indicator(RSSI) of packet. unit: dBm */
    unsigned rate:5;              /**< PHY rate encoding of the packet. Only valid for non HT(11bg) packet */
    unsigned :1;                  /**< reserved */
    unsigned sig_mode:2;          /**< 0: non HT(11bg) packet; 1: HT(11n) packet; 3: VHT(11ac) packet */
    unsigned :16;                 /**< reserved */
    unsigned mcs:7;               /**< Modulation Coding Scheme. If is HT(11n) packet, shows the modulation, range from 0 to 76(MSC0 ~ MCS76) */
    unsigned cwb:1;               /**< Channel Bandwidth of the packet. 0: 20MHz; 1: 40MHz */
    unsigned :16;                 /**< reserved */
    unsigned smoothing:1;         /**< reserved */
    unsigned not_sounding:1;      /**< reserved */
    unsigned :1;                  /**< reserved */
    unsigned aggregation:1;       /**< Aggregation. 0: MPDU packet; 1: AMPDU packet */
    unsigned stbc:2;              /**< Space Time Block Code(STBC). 0: non STBC packet; 1: STBC packet */
    unsigned fec_coding:1;        /**< Flag is set for 11n packets which are LDPC */
    unsigned sgi:1;               /**< Short Guide Interval(SGI). 0: Long GI; 1: Short GI */
#if CONFIG_IDF_TARGET_ESP32
    signed noise_floor:8;         /**< noise floor of Radio Frequency Module(RF). unit: dBm*/
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3
    unsigned :8;                  /**< reserved */
#endif
    unsigned ampdu_cnt:8;         /**< ampdu cnt */
    unsigned channel:4;           /**< primary channel on which this packet is received */
    unsigned secondary_channel:4; /**< secondary channel on which this packet is received. 0: none; 1: above; 2: below */
    unsigned :8;                  /**< reserved */
    unsigned timestamp:32;        /**< timestamp. The local time when this packet is received. It is precise only if modem sleep or light sleep is not enabled. unit: microsecond */
    unsigned :32;                 /**< reserved */
#if CONFIG_IDF_TARGET_ESP32S2
    unsigned :32;                 /**< reserved */
#elif CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3
    signed noise_floor:8;         /**< noise floor of Radio Frequency Module(RF). unit: dBm*/
    unsigned :24;                 /**< reserved */
    unsigned :32;                 /**< reserved */
#endif
    unsigned :31;                 /**< reserved */
    unsigned ant:1;               /**< antenna number from which this packet is received. 0: WiFi antenna 0; 1: WiFi antenna 1 */
#if CONFIG_IDF_TARGET_ESP32S2
    signed noise_floor:8;         /**< noise floor of Radio Frequency Module(RF). unit: dBm*/
    unsigned :24;                 /**< reserved */
#elif CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3
    unsigned :32;                 /**< reserved */
    unsigned :32;                 /**< reserved */
    unsigned :32;                 /**< reserved */
#endif
    unsigned sig_len:12;          /**< length of packet including Frame Check Sequence(FCS) */
    unsigned :12;                 /**< reserved */
    unsigned rx_state:8;          /**< state of the packet. 0: no error; others: error numbers which are not public */
} wifi_pkt_rx_ctrl_t;

/** @brief Payload passed to 'buf' parameter of promiscuous mode RX callback.
 */
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl; /**< metadata header */
    uint8_t payload[0];       /**< Data or management payload. Length of payload is described by rx_ctrl.sig_len. Type of content determined by packet type argument of callback. */
} wifi_promiscuous_pkt_t;

/**
  * @brief Promiscuous frame type
  *
  * Passed to promiscuous mode RX callback to indicate the type of parameter in the buffer.
  *
  */
typedef enum {
    WIFI_PKT_MGMT,  /**< Management frame, indicates 'buf' argument is wifi_promiscuous_pkt_t */
    WIFI_PKT_CTRL,  /**< Control frame, indicates 'buf' argument is wifi_promiscuous_pkt_t */
    WIFI_PKT_DATA,  /**< Data frame, indiciates 'buf' argument is wifi_promiscuous_pkt_t */
    WIFI_PKT_MISC,  /**< Other type, such as MIMO etc. 'buf' argument is wifi_promiscuous_pkt_t but the payload is zero length. */
} wifi_promiscuous_pkt_type_t;


#define WIFI_PROMIS_FILTER_MASK_ALL         (0xFFFFFFFF)  /**< filter all packets */
#define WIFI_PROMIS_FILTER_MASK_MGMT        (1)           /**< filter the packets with type of WIFI_PKT_MGMT */
#define WIFI_PROMIS_FILTER_MASK_CTRL        (1<<1)        /**< filter the packets with type of WIFI_PKT_CTRL */
#define WIFI_PROMIS_FILTER_MASK_DATA        (1<<2)        /**< filter the packets with type of WIFI_PKT_DATA */
#define WIFI_PROMIS_FILTER_MASK_MISC        (1<<3)        /**< filter the packets with type of WIFI_PKT_MISC */
#define WIFI_PROMIS_FILTER_MASK_DATA_MPDU   (1<<4)        /**< filter the MPDU which is a kind of WIFI_PKT_DATA */
#define WIFI_PROMIS_FILTER_MASK_DATA_AMPDU  (1<<5)        /**< filter the AMPDU which is a kind of WIFI_PKT_DATA */
#define WIFI_PROMIS_FILTER_MASK_FCSFAIL     (1<<6)        /**< filter the FCS failed packets, do not open it in general */

#define WIFI_PROMIS_CTRL_FILTER_MASK_ALL         (0xFF800000)  /**< filter all control packets */
#define WIFI_PROMIS_CTRL_FILTER_MASK_WRAPPER     (1<<23)       /**< filter the control packets with subtype of Control Wrapper */
#define WIFI_PROMIS_CTRL_FILTER_MASK_BAR         (1<<24)       /**< filter the control packets with subtype of Block Ack Request */
#define WIFI_PROMIS_CTRL_FILTER_MASK_BA          (1<<25)       /**< filter the control packets with subtype of Block Ack */
#define WIFI_PROMIS_CTRL_FILTER_MASK_PSPOLL      (1<<26)       /**< filter the control packets with subtype of PS-Poll */
#define WIFI_PROMIS_CTRL_FILTER_MASK_RTS         (1<<27)       /**< filter the control packets with subtype of RTS */
#define WIFI_PROMIS_CTRL_FILTER_MASK_CTS         (1<<28)       /**< filter the control packets with subtype of CTS */
#define WIFI_PROMIS_CTRL_FILTER_MASK_ACK         (1<<29)       /**< filter the control packets with subtype of ACK */
#define WIFI_PROMIS_CTRL_FILTER_MASK_CFEND       (1<<30)       /**< filter the control packets with subtype of CF-END */
#define WIFI_PROMIS_CTRL_FILTER_MASK_CFENDACK    (1<<31)       /**< filter the control packets with subtype of CF-END+CF-ACK */

/** @brief Mask for filtering different packet types in promiscuous mode. */
typedef struct {
    uint32_t filter_mask; /**< OR of one or more filter values WIFI_PROMIS_FILTER_* */
} wifi_promiscuous_filter_t;

#define WIFI_EVENT_MASK_ALL                 (0xFFFFFFFF)  /**< mask all WiFi events */
#define WIFI_EVENT_MASK_NONE                (0)           /**< mask none of the WiFi events */
#define WIFI_EVENT_MASK_AP_PROBEREQRECVED   (BIT(0))      /**< mask SYSTEM_EVENT_AP_PROBEREQRECVED event */

/**
  * @brief Channel state information(CSI) configuration type
  *
  */
typedef struct {
    bool lltf_en;           /**< enable to receive legacy long training field(lltf) data. Default enabled */
    bool htltf_en;          /**< enable to receive HT long training field(htltf) data. Default enabled */
    bool stbc_htltf2_en;    /**< enable to receive space time block code HT long training field(stbc-htltf2) data. Default enabled */
    bool ltf_merge_en;      /**< enable to generate htlft data by averaging lltf and ht_ltf data when receiving HT packet. Otherwise, use ht_ltf data directly. Default enabled */
    bool channel_filter_en; /**< enable to turn on channel filter to smooth adjacent sub-carrier. Disable it to keep independence of adjacent sub-carrier. Default enabled */
    bool manu_scale;        /**< manually scale the CSI data by left shifting or automatically scale the CSI data. If set true, please set the shift bits. false: automatically. true: manually. Default false */
    uint8_t shift;          /**< manually left shift bits of the scale of the CSI data. The range of the left shift bits is 0~15 */
} wifi_csi_config_t;

/**
  * @brief CSI data type
  *
  */
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;/**< received packet radio metadata header of the CSI data */
    uint8_t mac[6];            /**< source MAC address of the CSI data */
    bool first_word_invalid;   /**< first four bytes of the CSI data is invalid or not */
    int8_t *buf;               /**< buffer of CSI data */
    uint16_t len;              /**< length of CSI data */
} wifi_csi_info_t;

/**
  * @brief WiFi GPIO configuration for antenna selection
  *
  */
typedef struct {
    uint8_t gpio_select: 1,           /**< Whether this GPIO is connected to external antenna switch */
            gpio_num: 7;              /**< The GPIO number that connects to external antenna switch */
} wifi_ant_gpio_t;

/**
  * @brief WiFi GPIOs configuration for antenna selection
  *
  */
typedef struct {
    wifi_ant_gpio_t  gpio_cfg[4];  /**< The configurations of GPIOs that connect to external antenna switch */
} wifi_ant_gpio_config_t;

/**
  * @brief WiFi antenna mode
  *
  */
typedef enum {
    WIFI_ANT_MODE_ANT0,          /**< Enable WiFi antenna 0 only */
    WIFI_ANT_MODE_ANT1,          /**< Enable WiFi antenna 1 only */
    WIFI_ANT_MODE_AUTO,          /**< Enable WiFi antenna 0 and 1, automatically select an antenna */
    WIFI_ANT_MODE_MAX,           /**< Invalid WiFi enabled antenna */
} wifi_ant_mode_t;

/**
  * @brief WiFi antenna configuration
  *
  */
typedef struct {
    wifi_ant_mode_t rx_ant_mode;          /**< WiFi antenna mode for receiving */
    wifi_ant_t      rx_ant_default;       /**< Default antenna mode for receiving, it's ignored if rx_ant_mode is not WIFI_ANT_MODE_AUTO */
    wifi_ant_mode_t tx_ant_mode;          /**< WiFi antenna mode for transmission, it can be set to WIFI_ANT_MODE_AUTO only if rx_ant_mode is set to WIFI_ANT_MODE_AUTO */
    uint8_t         enabled_ant0: 4,      /**< Index (in antenna GPIO configuration) of enabled WIFI_ANT_MODE_ANT0 */
                    enabled_ant1: 4;      /**< Index (in antenna GPIO configuration) of enabled WIFI_ANT_MODE_ANT1 */
} wifi_ant_config_t;

/**
  * @brief     The Rx callback function of Action Tx operations
  *
  * @param     hdr pointer to the IEEE 802.11 Header structure
  * @param     payload pointer to the Payload following 802.11 Header
  * @param     len length of the Payload
  * @param     channel channel number the frame is received on
  *
  */
typedef int (* wifi_action_rx_cb_t)(uint8_t *hdr, uint8_t *payload,
                                    size_t len, uint8_t channel);

/**
 * @brief Action Frame Tx Request
 *
 *
 */
typedef struct {
    wifi_interface_t ifx;       /**< WiFi interface to send request to */
    uint8_t dest_mac[6];        /**< Destination MAC address */
    bool no_ack;                /**< Indicates no ack required */
    wifi_action_rx_cb_t rx_cb;  /**< Rx Callback to receive any response */
    uint32_t data_len;          /**< Length of the appended Data */
    uint8_t data[0];            /**< Appended Data payload */
} wifi_action_tx_req_t;

/**
  * @brief FTM Initiator configuration
  *
  */
typedef struct {
    uint8_t resp_mac[6];        /**< MAC address of the FTM Responder */
    uint8_t channel;            /**< Primary channel of the FTM Responder */
    uint8_t frm_count;          /**< No. of FTM frames requested in terms of 4 or 8 bursts (allowed values - 0(No pref), 16, 24, 32, 64) */
    uint16_t burst_period;      /**< Requested time period between consecutive FTM bursts in 100's of milliseconds (0 - No pref) */
} wifi_ftm_initiator_cfg_t;

/**
  * @brief WiFi PHY rate encodings
  *
  */
typedef enum {
    WIFI_PHY_RATE_1M_L      = 0x00, /**< 1 Mbps with long preamble */
    WIFI_PHY_RATE_2M_L      = 0x01, /**< 2 Mbps with long preamble */
    WIFI_PHY_RATE_5M_L      = 0x02, /**< 5.5 Mbps with long preamble */
    WIFI_PHY_RATE_11M_L     = 0x03, /**< 11 Mbps with long preamble */
    WIFI_PHY_RATE_2M_S      = 0x05, /**< 2 Mbps with short preamble */
    WIFI_PHY_RATE_5M_S      = 0x06, /**< 5.5 Mbps with short preamble */
    WIFI_PHY_RATE_11M_S     = 0x07, /**< 11 Mbps with short preamble */
    WIFI_PHY_RATE_48M       = 0x08, /**< 48 Mbps */
    WIFI_PHY_RATE_24M       = 0x09, /**< 24 Mbps */
    WIFI_PHY_RATE_12M       = 0x0A, /**< 12 Mbps */
    WIFI_PHY_RATE_6M        = 0x0B, /**< 6 Mbps */
    WIFI_PHY_RATE_54M       = 0x0C, /**< 54 Mbps */
    WIFI_PHY_RATE_36M       = 0x0D, /**< 36 Mbps */
    WIFI_PHY_RATE_18M       = 0x0E, /**< 18 Mbps */
    WIFI_PHY_RATE_9M        = 0x0F, /**< 9 Mbps */
    WIFI_PHY_RATE_MCS0_LGI  = 0x10, /**< MCS0 with long GI, 6.5 Mbps for 20MHz, 13.5 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS1_LGI  = 0x11, /**< MCS1 with long GI, 13 Mbps for 20MHz, 27 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS2_LGI  = 0x12, /**< MCS2 with long GI, 19.5 Mbps for 20MHz, 40.5 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS3_LGI  = 0x13, /**< MCS3 with long GI, 26 Mbps for 20MHz, 54 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS4_LGI  = 0x14, /**< MCS4 with long GI, 39 Mbps for 20MHz, 81 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS5_LGI  = 0x15, /**< MCS5 with long GI, 52 Mbps for 20MHz, 108 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS6_LGI  = 0x16, /**< MCS6 with long GI, 58.5 Mbps for 20MHz, 121.5 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS7_LGI  = 0x17, /**< MCS7 with long GI, 65 Mbps for 20MHz, 135 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS0_SGI  = 0x18, /**< MCS0 with short GI, 7.2 Mbps for 20MHz, 15 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS1_SGI  = 0x19, /**< MCS1 with short GI, 14.4 Mbps for 20MHz, 30 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS2_SGI  = 0x1A, /**< MCS2 with short GI, 21.7 Mbps for 20MHz, 45 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS3_SGI  = 0x1B, /**< MCS3 with short GI, 28.9 Mbps for 20MHz, 60 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS4_SGI  = 0x1C, /**< MCS4 with short GI, 43.3 Mbps for 20MHz, 90 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS5_SGI  = 0x1D, /**< MCS5 with short GI, 57.8 Mbps for 20MHz, 120 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS6_SGI  = 0x1E, /**< MCS6 with short GI, 65 Mbps for 20MHz, 135 Mbps for 40MHz */
    WIFI_PHY_RATE_MCS7_SGI  = 0x1F, /**< MCS7 with short GI, 72.2 Mbps for 20MHz, 150 Mbps for 40MHz */
    WIFI_PHY_RATE_LORA_250K = 0x29, /**< 250 Kbps */
    WIFI_PHY_RATE_LORA_500K = 0x2A, /**< 500 Kbps */
    WIFI_PHY_RATE_MAX,
} wifi_phy_rate_t;


/** WiFi event declarations */
typedef enum {
    WIFI_EVENT_WIFI_READY = 0,           /**< ESP32 WiFi ready */
    WIFI_EVENT_SCAN_DONE,                /**< ESP32 finish scanning AP */
    WIFI_EVENT_STA_START,                /**< ESP32 station start */
    WIFI_EVENT_STA_STOP,                 /**< ESP32 station stop */
    WIFI_EVENT_STA_CONNECTED,            /**< ESP32 station connected to AP */
    WIFI_EVENT_STA_DISCONNECTED,         /**< ESP32 station disconnected from AP */
    WIFI_EVENT_STA_AUTHMODE_CHANGE,      /**< the auth mode of AP connected by ESP32 station changed */

    WIFI_EVENT_STA_WPS_ER_SUCCESS,       /**< ESP32 station wps succeeds in enrollee mode */
    WIFI_EVENT_STA_WPS_ER_FAILED,        /**< ESP32 station wps fails in enrollee mode */
    WIFI_EVENT_STA_WPS_ER_TIMEOUT,       /**< ESP32 station wps timeout in enrollee mode */
    WIFI_EVENT_STA_WPS_ER_PIN,           /**< ESP32 station wps pin code in enrollee mode */
    WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP,   /**< ESP32 station wps overlap in enrollee mode */

    WIFI_EVENT_AP_START,                 /**< ESP32 soft-AP start */
    WIFI_EVENT_AP_STOP,                  /**< ESP32 soft-AP stop */
    WIFI_EVENT_AP_STACONNECTED,          /**< a station connected to ESP32 soft-AP */
    WIFI_EVENT_AP_STADISCONNECTED,       /**< a station disconnected from ESP32 soft-AP */
    WIFI_EVENT_AP_PROBEREQRECVED,        /**< Receive probe request packet in soft-AP interface */

    WIFI_EVENT_FTM_REPORT,               /**< Receive report of FTM procedure */

    /* Add next events after this only */
    WIFI_EVENT_STA_BSS_RSSI_LOW,         /**< AP's RSSI crossed configured threshold */
    WIFI_EVENT_ACTION_TX_STATUS,         /**< Status indication of Action Tx operation */
    WIFI_EVENT_ROC_DONE,                 /**< Remain-on-Channel operation complete */

    WIFI_EVENT_STA_BEACON_TIMEOUT,       /**< ESP32 station beacon timeout */

    WIFI_EVENT_MAX,                      /**< Invalid WiFi event ID */
} wifi_event_t;

/** @cond **/
/** @brief WiFi event base declaration */
ESP_EVENT_DECLARE_BASE(WIFI_EVENT);
/** @endcond **/

/** Argument structure for WIFI_EVENT_SCAN_DONE event */
typedef struct {
    uint32_t status;          /**< status of scanning APs: 0 — success, 1 - failure */
    uint8_t  number;          /**< number of scan results */
    uint8_t  scan_id;         /**< scan sequence number, used for block scan */
} wifi_event_sta_scan_done_t;

/** Argument structure for WIFI_EVENT_STA_CONNECTED event */
typedef struct {
    uint8_t ssid[32];         /**< SSID of connected AP */
    uint8_t ssid_len;         /**< SSID length of connected AP */
    uint8_t bssid[6];         /**< BSSID of connected AP*/
    uint8_t channel;          /**< channel of connected AP*/
    wifi_auth_mode_t authmode;/**< authentication mode used by AP*/
} wifi_event_sta_connected_t;

/** Argument structure for WIFI_EVENT_STA_DISCONNECTED event */
typedef struct {
    uint8_t ssid[32];         /**< SSID of disconnected AP */
    uint8_t ssid_len;         /**< SSID length of disconnected AP */
    uint8_t bssid[6];         /**< BSSID of disconnected AP */
    uint8_t reason;           /**< reason of disconnection */
    int8_t  rssi;             /**< rssi of disconnection */
} wifi_event_sta_disconnected_t;

/** Argument structure for WIFI_EVENT_STA_AUTHMODE_CHANGE event */
typedef struct {
    wifi_auth_mode_t old_mode;         /**< the old auth mode of AP */
    wifi_auth_mode_t new_mode;         /**< the new auth mode of AP */
} wifi_event_sta_authmode_change_t;

/** Argument structure for WIFI_EVENT_STA_WPS_ER_PIN event */
typedef struct {
    uint8_t pin_code[8];         /**< PIN code of station in enrollee mode */
} wifi_event_sta_wps_er_pin_t;

/** Argument structure for WIFI_EVENT_STA_WPS_ER_FAILED event */
typedef enum {
    WPS_FAIL_REASON_NORMAL = 0,     /**< ESP32 WPS normal fail reason */
    WPS_FAIL_REASON_RECV_M2D,       /**< ESP32 WPS receive M2D frame */
    WPS_FAIL_REASON_MAX
} wifi_event_sta_wps_fail_reason_t;

#define MAX_SSID_LEN        32
#define MAX_PASSPHRASE_LEN  64
#define MAX_WPS_AP_CRED     3

/** Argument structure for WIFI_EVENT_STA_WPS_ER_SUCCESS event */
typedef struct {
    uint8_t ap_cred_cnt;                        /**< Number of AP credentials received */
    struct {
        uint8_t ssid[MAX_SSID_LEN];             /**< SSID of AP */
        uint8_t passphrase[MAX_PASSPHRASE_LEN]; /**< Passphrase for the AP */
    } ap_cred[MAX_WPS_AP_CRED];                 /**< All AP credentials received from WPS handshake */
} wifi_event_sta_wps_er_success_t;

/** Argument structure for WIFI_EVENT_AP_STACONNECTED event */
typedef struct {
    uint8_t mac[6];           /**< MAC address of the station connected to ESP32 soft-AP */
    uint8_t aid;              /**< the aid that ESP32 soft-AP gives to the station connected to  */
    bool is_mesh_child;       /**< flag to identify mesh child */
} wifi_event_ap_staconnected_t;

/** Argument structure for WIFI_EVENT_AP_STADISCONNECTED event */
typedef struct {
    uint8_t mac[6];           /**< MAC address of the station disconnects to ESP32 soft-AP */
    uint8_t aid;              /**< the aid that ESP32 soft-AP gave to the station disconnects to  */
    bool is_mesh_child;       /**< flag to identify mesh child */
} wifi_event_ap_stadisconnected_t;

/** Argument structure for WIFI_EVENT_AP_PROBEREQRECVED event */
typedef struct {
    int rssi;                 /**< Received probe request signal strength */
    uint8_t mac[6];           /**< MAC address of the station which send probe request */
} wifi_event_ap_probe_req_rx_t;

/** Argument structure for WIFI_EVENT_STA_BSS_RSSI_LOW event */
typedef struct {
    int32_t rssi;                 /**< RSSI value of bss */
} wifi_event_bss_rssi_low_t;

/**
  * @brief FTM operation status types
  *
  */
typedef enum {
    FTM_STATUS_SUCCESS = 0,     /**< FTM exchange is successful */
    FTM_STATUS_UNSUPPORTED,     /**< Peer does not support FTM */
    FTM_STATUS_CONF_REJECTED,   /**< Peer rejected FTM configuration in FTM Request */
    FTM_STATUS_NO_RESPONSE,     /**< Peer did not respond to FTM Requests */
    FTM_STATUS_FAIL,            /**< Unknown error during FTM exchange */
} wifi_ftm_status_t;

/** Argument structure for */
typedef struct {
    uint8_t dlog_token;     /**< Dialog Token of the FTM frame */
    int8_t rssi;            /**< RSSI of the FTM frame received */
    uint32_t rtt;           /**< Round Trip Time in pSec with a peer */
    uint64_t t1;            /**< Time of departure of FTM frame from FTM Responder in pSec */
    uint64_t t2;            /**< Time of arrival of FTM frame at FTM Initiator in pSec */
    uint64_t t3;            /**< Time of departure of ACK from FTM Initiator in pSec */
    uint64_t t4;            /**< Time of arrival of ACK at FTM Responder in pSec */
} wifi_ftm_report_entry_t;

/** Argument structure for WIFI_EVENT_FTM_REPORT event */
typedef struct {
    uint8_t peer_mac[6];                        /**< MAC address of the FTM Peer */
    wifi_ftm_status_t status;                   /**< Status of the FTM operation */
    uint32_t rtt_raw;                           /**< Raw average Round-Trip-Time with peer in Nano-Seconds */
    uint32_t rtt_est;                           /**< Estimated Round-Trip-Time with peer in Nano-Seconds */
    uint32_t dist_est;                          /**< Estimated one-way distance in Centi-Meters */
    wifi_ftm_report_entry_t *ftm_report_data;   /**< Pointer to FTM Report with multiple entries, should be freed after use */
    uint8_t ftm_report_num_entries;             /**< Number of entries in the FTM Report data */
} wifi_event_ftm_report_t;

#define WIFI_STATIS_BUFFER    (1<<0)
#define WIFI_STATIS_RXTX      (1<<1)
#define WIFI_STATIS_HW        (1<<2)
#define WIFI_STATIS_DIAG      (1<<3)
#define WIFI_STATIS_PS        (1<<4)
#define WIFI_STATIS_ALL       (-1)

/** Argument structure for WIFI_EVENT_ACTION_TX_STATUS event */
typedef struct {
    wifi_interface_t ifx;     /**< WiFi interface to send request to */
    uint32_t context;         /**< Context to identify the request */
    uint8_t da[6];            /**< Destination MAC address */
    uint8_t status;           /**< Status of the operation */
} wifi_event_action_tx_status_t;

/** Argument structure for WIFI_EVENT_ROC_DONE event */
typedef struct {
    uint32_t context;         /**< Context to identify the request */
} wifi_event_roc_done_t;

#ifdef __cplusplus
}
#endif

#endif /* __ESP_WIFI_TYPES_H__ */
