#ifndef XML_PARSER
#define XML_PARSER

#include <stdlib.h>

typedef enum {
  XML_STATE_Initialized,

  XML_STATE_FindStartOfTag,
  XML_STATE_ParseTag,
  XML_STATE_EndOfTag,

  XML_STATE_ParseData,

  XML_STATE_ParseError,
  XML_STATE_EndOfData
} XmlParserState;

typedef void (*tag_handler)(char *tag, int is_terminating, void *userdata);

typedef void (*data_handler)(char *data, void *userdata);

typedef void (*error_handler)(void);

typedef struct {
  char current_token[4096];
  size_t current_token_len;
  int is_terminating_tag;
  XmlParserState state;

  tag_handler tag_handler_cb;
  data_handler data_handler_cb;
  error_handler error_handler_cb;

  void *handler_userdata;

} XmlParser;

void xml_parser_init(XmlParser *xml_parser, tag_handler handle_tag_cb, data_handler handle_data_cb, error_handler handle_error_cb, void *handler_userdata);

void xml_parser_process(XmlParser *xml_parser, const char *buffer, size_t len);

#endif