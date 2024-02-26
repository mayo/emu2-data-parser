#include <stdio.h>
#include <stdlib.h>
#include "xml_parser.h"
#include "emu2_parser.h"

// void foo() {
//     static const Emu2MetricData IDMetric = {
//       .tag = EMU2_TAG_InstantaneousDemand,
//       .members = {EMU2_TAG_DeviceMacId, EMU2_TAG_MeterMacId, EMU2_TAG_TimeStamp, EMU2_TAG_Demand, EMU2_TAG_Multiplier, EMU2_TAG_Divisor, EMU2_TAG_DigitsRight, EMU2_TAG_DigitsLeft, EMU2_TAG_SuppressLeadingZero }
//     }
// };

static void handle_tag(char *tag, int is_terminating, void *userdata) {
  Emu2Parser *emu2_parser = (Emu2Parser *)userdata;
  // uint32_t hash = murmur3_32(tag, strlen(tag), 42);
  // printf("Tag: %s, hash: %08x, end: %d\n", tag, hash, is_terminating);

  emu2_parser_process_tag(emu2_parser, tag, is_terminating);
}

static void handle_data(char *data, void *userdata) {
  Emu2Parser *emu2_parser = (Emu2Parser *)userdata;

  // printf("Data: %s\n", data);
  emu2_parser_process_data(emu2_parser, data);
}

static void handle_metric(Emu2Metric metric, void *userdata) {
    printf("Metric:\n  meter_mac_id = %#16llx\n  device_mac_id = %#16llx\n  type = %d\n", metric.meter_mac_id,  metric.device_mac_id, metric.tag);
     switch (metric.tag) {
      case EMU2_METRIC_InstantaneousDemand:
        printf("InstantaneousDemand:\n  demand = %d\n  multiplier = %d\n  divisor = %d\n  digits_left = %d\n  digits_right = %d\n  suppress_leading_zero = %d\n",
          metric.instantaneous_demand.demand,
          metric.instantaneous_demand.multiplier,
          metric.instantaneous_demand.divisor,
          metric.instantaneous_demand.digits_left,
          metric.instantaneous_demand.digits_right,
          metric.instantaneous_demand.suppress_leading_zero);
        break;

      case EMU2_METRIC_CurrentSummationDelivered:
        printf("CurrentSummationDelivered:\n  summation_delivered = %lld\n  summation_received = %lld\n  multiplier = %d\n  divisor = %d\n  digits_left = %d\n  digits_right = %d\n  suppress_leading_zero = %d\n",
          metric.current_summation_delivered.summation_delivered,
          metric.current_summation_delivered.summation_received,
          metric.current_summation_delivered.multiplier,
          metric.current_summation_delivered.divisor,
          metric.current_summation_delivered.digits_left,
          metric.current_summation_delivered.digits_right,
          metric.current_summation_delivered.suppress_leading_zero);
        break;

      case EMU2_METRIC_PriceCluster:
        printf("PriceCluster:\n  price = %d\n  trailing_digits = %d\n  currency = %d\n  tier = %d\n  start_time = %d\n  duration = %d\n  rate_label = %s\n",
          metric.price_cluster.price,
          metric.price_cluster.trailing_digits,
          metric.price_cluster.currency,
          metric.price_cluster.tier,
          metric.price_cluster.start_time,
          metric.price_cluster.duration,
          metric.price_cluster.rate_label);
        break;

      case EMU2_METRIC_ConnectionStatus:
        printf("ConnectionStatus:\n  status = %d\n  link_strength = %d\n  channel = %d\n  short_addr = %d\n  ext_pan_id = %lld\n",
          metric.connection_status.status,
          metric.connection_status.link_strength,
          metric.connection_status.channel,
          metric.connection_status.short_addr,
          metric.connection_status.ext_pan_id);
        break;
    }
}

int main(void) {
  static XmlParser xml_parser;
  static Emu2Parser emu2_parser;
  
  emu2_parser_init(&emu2_parser, handle_metric, NULL, NULL);
  xml_parser_init(&xml_parser, handle_tag, handle_data, NULL, &emu2_parser);

  char buffer[10];
  size_t num_read;

  do {
    num_read = fread(buffer, sizeof(char), sizeof(buffer), stdin);
    xml_parser_process(&xml_parser, buffer, num_read);

  } while(num_read > 0);

  return EXIT_SUCCESS;
}
