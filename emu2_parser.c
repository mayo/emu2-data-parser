#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "emu2_parser.h"
#include "murmur3.h"

static const Emu2Metric EmptyEmu2Metric;

static unsigned long long parse_ull(const char *data, int *ok) {
    char *end = NULL;
    errno=0;

    if (ok != NULL) {
        *ok = 0;
    }

    unsigned long long value = strtoull(data, &end, 16);

    if (errno == EINVAL || errno == ERANGE) {
        //TODO: how to pass errors around?
        printf("Conversion error occurred: %s\n", strerror(errno));
        return 0;
    }

    if(*end != '\0') {
        //TODO: how to pass errors around?
        printf("Left over data after conversion: '%s'\n", end);
        return 0;
    }

    if (ok != NULL) {
        *ok = 1;
    }

    return value;
}

static int is_root_tag(Emu2Tag tag) {
  if (tag == EMU2_TAG_InstantaneousDemand ||
      tag == EMU2_TAG_CurrentSummationDelivered ||
      tag == EMU2_TAG_PriceCluster ||
      tag == EMU2_TAG_ConnectionStatus) {
    return 1;
  }

  return 0;
}

static void parser_to_Idle(Emu2Parser *emu2_parser) {
  emu2_parser->state = EMU2_STATE_Idle;
  emu2_parser->current_tag = EMU2_TAG_None;
  emu2_parser->metric = EmptyEmu2Metric;
}

static void parser_to_CollectingRootData(Emu2Parser *emu2_parser, int tag_id) {
  // printf("Root: %d\n", tag_id);
  emu2_parser->state = EMU2_STATE_CollectingRootData;
  emu2_parser->current_tag = EMU2_TAG_None;
  emu2_parser->metric.tag = tag_id;
}

void emu2_parser_init(Emu2Parser *emu2_parser, metric_handler handle_metric_cb, error_handler handle_error_cb, void *handler_userdata) {
  emu2_parser->state = EMU2_STATE_Idle;
  emu2_parser->current_tag = EMU2_TAG_None;
  emu2_parser->metric = EmptyEmu2Metric;
  emu2_parser->metric_handler_cb = handle_metric_cb;
  emu2_parser->error_handler_cb = handle_error_cb;
  emu2_parser->handler_userdata = handler_userdata;
}

void emu2_parser_process_tag(Emu2Parser *emu2_parser, char *tag, int is_terminating) {
  int tag_id = murmur3_32(tag, strlen(tag), 42);
  int is_root = is_root_tag(tag_id);

/*
  switch(emu2_parser->state) {
    case EMU2_STATE_Idle:
      //non-root tags or terminating root tags aren't useful when we're not looking for data
      if (!is_root || is_terminating) {
        break;
      }

      emu2_parser_to_CollectingRootData(emu2_parser, tag_id);
      break;

    case EMU2_STATE_CollectingRootData:
      if (is_root) {
        //If we're terminating current root
        if (is_terminating && tag_id == emu2_parser->current_root) {
          printf("Finished root: %d\n", emu2_parser->current_root);
          emu2_parser_to_Idle(emu2_parser);
          break;

        } else {
          //Not terminating current root.

          if (is_terminating) {
            emu2_parser_to_Idle(emu2_parser);
            break;
          
          } else {
            emu2_parser_to_CollectingRootData(emu2_parser, tag_id);
            break;
          }
        }

      //Not a root tag
      } else {
        //case where we don't have tag
        //case where we're terminating 
        if (is_terminating) {

        } else {

        }
      }
  }
*/

  // printf("processing tag: %s, end: %d; tag_id: %d, root: %d, state: %d\n", tag, is_terminating, tag_id, is_root, emu2_parser->state);

  if (is_terminating == 0 && is_root && emu2_parser->state == EMU2_STATE_Idle) {
    parser_to_CollectingRootData(emu2_parser, tag_id);
    return;
  }

  if (is_terminating == 1 && tag_id == emu2_parser->metric.tag && emu2_parser->state == EMU2_STATE_CollectingRootData) {
    // printf("Finished root: %d\n", emu2_parser->metric.tag);
    emu2_parser->metric_handler_cb(emu2_parser->metric, emu2_parser->handler_userdata);
    parser_to_Idle(emu2_parser);
    return;
  }

  if (is_root && tag_id != emu2_parser->metric.tag && emu2_parser->state == EMU2_STATE_CollectingRootData) {
    printf("Unterminated root, starting anew\n");

    if (is_terminating) {
      parser_to_Idle(emu2_parser);
      return;
    } else {
      parser_to_CollectingRootData(emu2_parser, tag_id);
      return;
    }
  }

  //Known non-root tag
  if (!is_root && tag_id != EMU2_TAG_None && emu2_parser->state == EMU2_STATE_CollectingRootData) {
    //Opening tag. If there is a tag in progress, reset
    if (!is_terminating) {
      if (emu2_parser->current_tag == EMU2_TAG_None) {
        // printf("start sub-tag: %s, end: %d\n", tag, is_terminating);

        emu2_parser->current_tag = tag_id;
        return;

      } else {
        //tag mismatch, start over
        parser_to_Idle(emu2_parser);
        return;
      }

    //Closing tag
    } else {
      //is mismatched, start over
      if (tag_id != emu2_parser->current_tag) {
        parser_to_Idle(emu2_parser);
        printf("Mismatched tags!");
        return;
      }

      emu2_parser->current_tag = EMU2_TAG_None;
    }
  }

}

static void fill_uint8_field(Emu2Tag current_tag, Emu2Tag field_tag, uint8_t *field, char *data) {

  if (current_tag != field_tag) {
    return;
  }

  int ok = 0;
  *field = (uint8_t)parse_ull(data, &ok);
}

static void fill_uint16_field(Emu2Tag current_tag, Emu2Tag field_tag, uint16_t *field, char *data) {

  if (current_tag != field_tag) {
    return;
  }

  int ok = 0;
  *field = (uint16_t)parse_ull(data, &ok);
}

static void fill_uint32_field(Emu2Tag current_tag, Emu2Tag field_tag, uint32_t *field, char *data) {

  if (current_tag != field_tag) {
    return;
  }

  int ok = 0;
  *field = (uint32_t)parse_ull(data, &ok);
}

static void fill_uint64_field(Emu2Tag current_tag, Emu2Tag field_tag, uint64_t *field, char *data) {

  if (current_tag != field_tag) {
    return;
  }

  int ok = 0;
  *field = (uint64_t)parse_ull(data, &ok);
}

static void fill_bool_field(Emu2Tag current_tag, Emu2Tag field_tag, Emu2Bool *field, char *data) {

  if (current_tag != field_tag) {
    return;
  }

  if (*data == 'Y') {
    *field = EMU2_BOOL_true;
  } else if (*data == 'N') {
    *field = EMU2_BOOL_false;
  } else {
    *field = -1;
  }
}

static void fill_char_field(Emu2Tag current_tag, Emu2Tag field_tag, char *field, int size, char *data) {

  if (current_tag != field_tag) {
    return;
  }

  int data_len = strlen(data);;
  strncpy(field, data, (data_len < size ? data_len : size));
  field[size] = '\0';
}

static void fill_connection_status_field(Emu2Tag current_tag, Emu2Tag field_tag, Emu2ConnectionStatus_status *field, char *data) {

  if (current_tag != field_tag) {
    return;
  }

  switch(*data) {
    case 'R':
        *field = EMU2ConnectionStatus_Rejoining;
        break;

    case 'C':
        *field = EMU2ConnectionStatus_Connected;
        break;

    default:
        *field = EMU2ConnectionStatus_Unknown;
        break;
  }
}

void emu2_parser_process_data(Emu2Parser *emu2_parser, char *data) {

  fill_uint64_field(emu2_parser->current_tag, EMU2_TAG_MeterMacId, &(emu2_parser->metric.meter_mac_id), data);
  fill_uint64_field(emu2_parser->current_tag, EMU2_TAG_DeviceMacId, &(emu2_parser->metric.device_mac_id), data);

  switch(emu2_parser->metric.tag) {
    case EMU2_TAG_InstantaneousDemand:
      fill_uint8_field(emu2_parser->current_tag, EMU2_TAG_DigitsLeft, &(emu2_parser->metric.instantaneous_demand.digits_left), data);
      fill_uint8_field(emu2_parser->current_tag, EMU2_TAG_DigitsRight, &(emu2_parser->metric.instantaneous_demand.digits_right), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_Demand, &(emu2_parser->metric.instantaneous_demand.demand), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_Multiplier, &(emu2_parser->metric.instantaneous_demand.multiplier), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_Divisor, &(emu2_parser->metric.instantaneous_demand.divisor), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_TimeStamp, &(emu2_parser->metric.instantaneous_demand.timestamp), data);
      fill_bool_field(emu2_parser->current_tag, EMU2_TAG_SuppressLeadingZero, &(emu2_parser->metric.instantaneous_demand.suppress_leading_zero), data);

      break;

    case EMU2_TAG_CurrentSummationDelivered:
      fill_uint8_field(emu2_parser->current_tag, EMU2_TAG_DigitsLeft, &(emu2_parser->metric.current_summation_delivered.digits_left), data);
      fill_uint8_field(emu2_parser->current_tag, EMU2_TAG_DigitsRight, &(emu2_parser->metric.current_summation_delivered.digits_right), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_Multiplier, &(emu2_parser->metric.current_summation_delivered.multiplier), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_Divisor, &(emu2_parser->metric.current_summation_delivered.divisor), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_TimeStamp, &(emu2_parser->metric.current_summation_delivered.timestamp), data);
      fill_uint64_field(emu2_parser->current_tag, EMU2_TAG_SummationDelivered, &(emu2_parser->metric.current_summation_delivered.summation_delivered), data);
      fill_uint64_field(emu2_parser->current_tag, EMU2_TAG_SummationReceived, &(emu2_parser->metric.current_summation_delivered.summation_received), data);
      fill_bool_field(emu2_parser->current_tag, EMU2_TAG_SuppressLeadingZero, &(emu2_parser->metric.current_summation_delivered.suppress_leading_zero), data);

      break;

    case EMU2_TAG_PriceCluster:
       fill_uint8_field(emu2_parser->current_tag, EMU2_TAG_TrailingDigits, &(emu2_parser->metric.price_cluster.trailing_digits), data);
      fill_uint8_field(emu2_parser->current_tag, EMU2_TAG_Tier, &(emu2_parser->metric.price_cluster.tier), data);
      fill_uint16_field(emu2_parser->current_tag, EMU2_TAG_Currency, &(emu2_parser->metric.price_cluster.currency), data);
      fill_uint16_field(emu2_parser->current_tag, EMU2_TAG_Duration, &(emu2_parser->metric.price_cluster.duration), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_Price, &(emu2_parser->metric.price_cluster.price), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_StartTime, &(emu2_parser->metric.price_cluster.start_time), data);
      fill_uint32_field(emu2_parser->current_tag, EMU2_TAG_TimeStamp, &(emu2_parser->metric.current_summation_delivered.timestamp), data);
      fill_char_field(emu2_parser->current_tag, EMU2_TAG_RateLabel, (emu2_parser->metric.price_cluster.rate_label), sizeof(emu2_parser->metric.price_cluster.rate_label), data);

      break;

    case EMU2_TAG_ConnectionStatus:
      fill_uint8_field(emu2_parser->current_tag, EMU2_TAG_LinkStrength, &(emu2_parser->metric.connection_status.link_strength), data);
      fill_uint8_field(emu2_parser->current_tag, EMU2_TAG_Channel, &(emu2_parser->metric.connection_status.channel), data);
      fill_uint16_field(emu2_parser->current_tag, EMU2_TAG_ShortAddr, &(emu2_parser->metric.connection_status.short_addr), data);
      fill_uint64_field(emu2_parser->current_tag, EMU2_TAG_ExtPanId, &(emu2_parser->metric.connection_status.ext_pan_id), data);
      fill_connection_status_field(emu2_parser->current_tag, EMU2_TAG_Status, &(emu2_parser->metric.connection_status.status), data);

      break;
  }

  switch(emu2_parser->current_tag) {
    case EMU2_TAG_Status:
      printf("status: value: %s\n", data);
      break;
    
    default:
      break;
  }
}