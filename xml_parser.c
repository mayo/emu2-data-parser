#include <stdio.h>
#include <ctype.h>
#include "xml_parser.h"

//void (*handle_error)(void)
void xml_parser_init(XmlParser *xml_parser, tag_handler handle_tag_cb, data_handler handle_data_cb, error_handler handle_error_cb, void *handler_userdata) {
  xml_parser->state = XML_STATE_Initialized;
  xml_parser->current_token_len = 0;
  xml_parser->is_terminating_tag = 0;
  xml_parser->tag_handler_cb = handle_tag_cb;
  xml_parser->data_handler_cb = handle_data_cb;
  xml_parser->error_handler_cb = handle_error_cb;
  xml_parser->handler_userdata = handler_userdata;
}

static void add_to_token(XmlParser *xml_parser, char c) {
    if (xml_parser->current_token_len < sizeof(xml_parser->current_token)) {
        xml_parser->current_token[xml_parser->current_token_len++] = c;
    }
}

static void xml_parser_to_ParseTag(XmlParser *xml_parser, int is_terminating_tag) {
  xml_parser->state = XML_STATE_ParseTag;
  xml_parser->is_terminating_tag = is_terminating_tag;
}

//add len to handle buffers with \0 in them?
void xml_parser_process(XmlParser *xml_parser, const char *buffer, size_t len) {
  for (const char *end = buffer + len;
        buffer < end && xml_parser->state != XML_STATE_ParseError && xml_parser->state != XML_STATE_EndOfData;
        buffer++) {
    
    int c = (int)*buffer;//fgetc(stdin);

    //End of data
    if (c == -1) {
      switch (xml_parser->state) {
        // case XML_STATE_ParseNumber:
        // case XML_STATE_CheckEndOfString:
        //     printCurrentToken();
        //     parserState = XML_STATE_EndOfData;
        //     break;

        case XML_STATE_Initialized:
        case XML_STATE_FindStartOfTag:
        case XML_STATE_EndOfTag:
          break;

        case XML_STATE_ParseData:
        case XML_STATE_ParseTag:
            // Data ends in the middle of token parsing? No way!
            fprintf(stderr, "Data ended abruptly!\n");
            xml_parser->state = XML_STATE_ParseError;
            break;

        // case XML_STATE_FindStartOfData:
        // case XML_STATE_FindStartOfToken:
        // case XML_STATE_FindDelimiter:
        //     // This is okay, data stream may end while in these states
        //     parserState = XML_STATE_EndOfData;
        //     break;

        case XML_STATE_ParseError:
        case XML_STATE_EndOfData:
            break;
      }
    }

    // Ignore control characters
    if (iscntrl(c) == 1) {
        // printf("iscntrl = %d\n", iscntrl(c));
        continue;
    }

    switch (xml_parser->state) {
      case XML_STATE_Initialized:
        if (c == '<') {
          xml_parser_to_ParseTag(xml_parser, 0);
          break;
        }

        xml_parser->state = XML_STATE_FindStartOfTag;
        break;

      case XML_STATE_FindStartOfTag:
        if (c == '<') {
          xml_parser_to_ParseTag(xml_parser, 0);
        }
        break;

      case XML_STATE_ParseTag:
        if (c == '/') {
          xml_parser->is_terminating_tag = 1;
          break;
        }

        if (c == '>') {
          xml_parser->state = XML_STATE_EndOfTag;

          //TODO: nneed to handle empty tag - should not trigger callbback

          //First handle token
          xml_parser->current_token[xml_parser->current_token_len] = '\0';
          xml_parser->tag_handler_cb(xml_parser->current_token, xml_parser->is_terminating_tag, xml_parser->handler_userdata);
          xml_parser->current_token_len = 0;

          //reset tag state
          xml_parser->is_terminating_tag = 0;

          break;
        }

        add_to_token(xml_parser, c);
        break;

      case XML_STATE_EndOfTag:
        //New tag starting
        if (c == '<') {
          xml_parser_to_ParseTag(xml_parser, 0);
          break;
        }

        //Ignore witespace
        if (c == ' ' || c == '\n') {
          break;
        }

        //Most likely data
        xml_parser->state = XML_STATE_ParseData;
        add_to_token(xml_parser, c);
        break;

      case XML_STATE_ParseData:
        if (c == '<') {
          xml_parser_to_ParseTag(xml_parser, 0);
          
          //Handle Data
          xml_parser->current_token[xml_parser->current_token_len] = '\0';
          xml_parser->data_handler_cb(xml_parser->current_token, xml_parser->handler_userdata);
          xml_parser->current_token_len = 0;
          break;
        }

        if (c == '>') {
          //TODO: This is a soft parse error, so in theory, we should be able to
          //recover by resetting the parser state and look for 
          xml_parser->state = XML_STATE_ParseError;
          break;
        }
        
        add_to_token(xml_parser, c);
        break;

      case XML_STATE_ParseError:
        // Nothing to do
        break;

      case XML_STATE_EndOfData:
        // Nothing to do
        break;
    }

  }
}
