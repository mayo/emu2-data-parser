#ifndef EMU2_PARSER
#define EMU2_PARSER

#include <stdint.h>
#include "emu2_tags.h"

typedef enum {
  EMU2_METRIC_InstantaneousDemand = EMU2_TAG_InstantaneousDemand,
  EMU2_METRIC_CurrentSummationDelivered = EMU2_TAG_CurrentSummationDelivered,
  EMU2_METRIC_PriceCluster = EMU2_TAG_PriceCluster,
  EMU2_METRIC_ConnectionStatus = EMU2_TAG_ConnectionStatus,
} Emu2MetricTag;

typedef enum {
    EMU2_BOOL_false,
    EMU2_BOOL_true
} Emu2Bool;

typedef struct {
  //TODO: device_mac_id, meter_mac_id
  uint8_t digits_left;
  uint8_t digits_right;
  uint32_t demand;
  uint32_t multiplier;
  uint32_t divisor;
  uint32_t timestamp;
  Emu2Bool suppress_leading_zero;
} Emu2InstantaneousDemand;

typedef struct  {
  //TODO: device_mac_id, meter_mac_id
  uint8_t digits_left;
  uint8_t digits_right;
  uint32_t multiplier;
  uint32_t divisor;
  uint32_t timestamp;
  uint64_t summation_delivered;
  uint64_t summation_received;
  Emu2Bool suppress_leading_zero;
} Emu2CurrentSummationDelivered;

typedef struct {
  //TODO: device_mac_id, meter_mac_id
  uint8_t trailing_digits;
  uint8_t tier;
  uint16_t currency;
  uint16_t duration;
  uint32_t price;
  uint32_t start_time;
  uint32_t timestamp;
  char rate_label[128];
} Emu2PriceCluster;

typedef enum {
    EMU2ConnectionStatus_Rejoining,
    EMU2ConnectionStatus_Connected,

    EMU2ConnectionStatus_Unknown
} Emu2ConnectionStatus_status;

typedef struct {
  //TODO: uint64_t device_mac_id, meter_mac_id
  uint8_t link_strength;
  uint8_t channel;
  uint16_t short_addr;
  Emu2ConnectionStatus_status status;
  uint64_t ext_pan_id;
  //description - just a long worded version of "status"
} Emu2ConnectionStatus;

typedef struct {
    Emu2MetricTag tag;
    union {
      Emu2InstantaneousDemand instantaneous_demand;
      Emu2CurrentSummationDelivered current_summation_delivered;
      Emu2PriceCluster price_cluster;
      Emu2ConnectionStatus connection_status;
    };
    uint64_t device_mac_id;
    uint64_t meter_mac_id;
} Emu2Metric;

typedef enum {
  EMU2_STATE_Idle,
  EMU2_STATE_CollectingRootData,
  EMU2_STATE_ParseError,
} Emu2ParserState;

typedef void (*metric_handler)(Emu2Metric metric, void *userdata);

typedef void (*error_handler)(void);

typedef struct {
  Emu2Tag current_tag;
  Emu2ParserState state;
  Emu2Metric metric;

  metric_handler metric_handler_cb;
  error_handler error_handler_cb;

  void *handler_userdata;
} Emu2Parser;

int emu2_is_root_tag(Emu2Tag tag);

void emu2_parser_init(Emu2Parser *emu2_parser, metric_handler handle_metric_cb, error_handler handle_error_cb, void *handler_userdata);

void emu2_parser_process_tag(Emu2Parser *emu2_parser, char *tag, int is_terminating);

void emu2_parser_process_data(Emu2Parser *emu2_parser, char *data);

#endif