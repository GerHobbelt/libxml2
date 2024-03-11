/*
 * parserInternals.c : Internal routines (and obsolete ones) needed for the
 *                     XML and HTML parsers.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXML
#include "libxml.h"

#if defined(_WIN32)
#define XML_DIR_SEP '\\'
#else
#define XML_DIR_SEP '/'
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/entities.h>
#include <libxml/xmlerror.h>
#include <libxml/encoding.h>
#include <libxml/xmlIO.h>
#include <libxml/uri.h>
#include <libxml/dict.h>
#include <libxml/xmlsave.h>
#ifdef LIBXML_CATALOG_ENABLED
#include <libxml/catalog.h>
#endif
#include <libxml/chvalid.h>

#define CUR(ctxt) ctxt->input->cur
#define END(ctxt) ctxt->input->end

#include "private/buf.h"
#include "private/enc.h"
#include "private/error.h"
#include "private/io.h"
#include "private/parser.h"

#define XML_MAX_ERRORS 100

/*
 * XML_MAX_AMPLIFICATION_DEFAULT is the default maximum allowed amplification
 * factor of serialized output after entity expansion.
 */
#define XML_MAX_AMPLIFICATION_DEFAULT 5

/*
 * Various global defaults for parsing
 */

/**
 * xmlCheckVersion:
 * @version: the include version number
 *
 * check the compiled lib version against the include one.
 */
void
xmlCheckVersion(int version) {
    int myversion = LIBXML_VERSION;

    xmlInitParser();

    if ((myversion / 10000) != (version / 10000)) {
	fprintf(stderr,
		"Fatal: program compiled against libxml %d using libxml %d\n",
		(version / 10000), (myversion / 10000));
    } else if ((myversion / 100) < (version / 100)) {
	fprintf(stderr,
		"Warning: program compiled against libxml %d using older %d\n",
		(version / 100), (myversion / 100));
    }
}


/************************************************************************
 *									*
 *		Some factorized error routines				*
 *									*
 ************************************************************************/


/**
 * xmlCtxtSetErrorHandler:
 * @ctxt:  an XML parser context
 * @handler:  error handler
 * @data:  data for error handler
 *
 * Register a callback function that will be called on errors and
 * warnings. If handler is NULL, the error handler will be deactivated.
 *
 * This is the recommended way to collect errors from the parser and
 * takes precedence over all other error reporting mechanisms.
 * These are (in order of precedence):
 *
 * - per-context structured handler (xmlCtxtSetErrorHandler)
 * - per-context structured "serror" SAX handler
 * - global structured handler (xmlSetStructuredErrorFunc)
 * - per-context generic "error" and "warning" SAX handlers
 * - global generic handler (xmlSetGenericErrorFunc)
 * - print to stderr
 *
 * Available since 2.13.0.
 */
void
xmlCtxtSetErrorHandler(xmlParserCtxtPtr ctxt, xmlStructuredErrorFunc handler,
                       void *data)
{
    if (ctxt == NULL)
        return;
    ctxt->errorHandler = handler;
    ctxt->errorCtxt = data;
}

/**
 * xmlCtxtErrMemory:
 * @ctxt:  an XML parser context
 * @domain:  domain
 *
 * Handle an out-of-memory error
 */
void
xmlCtxtErrMemory(xmlParserCtxtPtr ctxt)
{
    xmlStructuredErrorFunc schannel = NULL;
    xmlGenericErrorFunc channel = NULL;
    void *data;

    ctxt->errNo = XML_ERR_NO_MEMORY;
    ctxt->instate = XML_PARSER_EOF; /* TODO: Remove after refactoring */
    ctxt->wellFormed = 0;
    ctxt->disableSAX = 2;

    if (ctxt->errorHandler) {
        schannel = ctxt->errorHandler;
        data = ctxt->errorCtxt;
    } else if ((ctxt->sax->initialized == XML_SAX2_MAGIC) &&
        (ctxt->sax->serror != NULL)) {
        schannel = ctxt->sax->serror;
        data = ctxt->userData;
    } else {
        channel = ctxt->sax->error;
        data = ctxt->userData;
    }

    xmlRaiseMemoryError(schannel, channel, data, XML_FROM_PARSER,
                        &ctxt->lastError);
}

void
xmlCtxtVErr(xmlParserCtxtPtr ctxt, xmlNodePtr node, xmlErrorDomain domain,
            xmlParserErrors code, xmlErrorLevel level,
            const xmlChar *str1, const xmlChar *str2, const xmlChar *str3,
            int int1, const char *msg, va_list ap)
{
    xmlStructuredErrorFunc schannel = NULL;
    xmlGenericErrorFunc channel = NULL;
    void *data = NULL;
    const char *file = NULL;
    int line = 0;
    int col = 0;
    int res;

    if (PARSER_STOPPED(ctxt))
	return;

    if (code == XML_ERR_NO_MEMORY) {
        xmlCtxtErrMemory(ctxt);
        return;
    }

    if (level == XML_ERR_WARNING) {
        if (ctxt->nbWarnings >= XML_MAX_ERRORS)
            return;
        ctxt->nbWarnings += 1;
    } else {
        if (ctxt->nbErrors >= XML_MAX_ERRORS)
            return;
        ctxt->nbErrors += 1;
    }

    if (((ctxt->options & XML_PARSE_NOERROR) == 0) &&
        ((level != XML_ERR_WARNING) ||
         ((ctxt->options & XML_PARSE_NOWARNING) == 0))) {
        if (ctxt->errorHandler) {
            schannel = ctxt->errorHandler;
            data = ctxt->errorCtxt;
        } else if ((ctxt->sax->initialized == XML_SAX2_MAGIC) &&
            (ctxt->sax->serror != NULL)) {
            schannel = ctxt->sax->serror;
            data = ctxt->userData;
        } else if ((domain == XML_FROM_VALID) || (domain == XML_FROM_DTD)) {
            if (level == XML_ERR_WARNING)
                channel = ctxt->vctxt.warning;
            else
                channel = ctxt->vctxt.error;
            data = ctxt->vctxt.userData;
        } else {
            if (level == XML_ERR_WARNING)
                channel = ctxt->sax->warning;
            else
                channel = ctxt->sax->error;
            data = ctxt->userData;
        }
    }

    if (ctxt->input != NULL) {
        xmlParserInputPtr input = ctxt->input;

        if ((input->filename == NULL) &&
            (ctxt->inputNr > 1)) {
            input = ctxt->inputTab[ctxt->inputNr - 2];
        }
        file = input->filename;
        line = input->line;
        col = input->col;
    }

    res = xmlVRaiseError(schannel, channel, data, ctxt, node, domain, code,
                         level, file, line, (const char *) str1,
                         (const char *) str2, (const char *) str3, int1, col,
                         msg, ap);

    if (res < 0) {
        xmlCtxtErrMemory(ctxt);
        return;
    }

    if (level >= XML_ERR_ERROR)
        ctxt->errNo = code;
    if (level == XML_ERR_FATAL) {
        ctxt->wellFormed = 0;
        if (ctxt->recovery == 0)
            ctxt->disableSAX = 1;
    }

    return;
}

void
xmlCtxtErr(xmlParserCtxtPtr ctxt, xmlNodePtr node, xmlErrorDomain domain,
           xmlParserErrors code, xmlErrorLevel level,
           const xmlChar *str1, const xmlChar *str2, const xmlChar *str3,
           int int1, const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    xmlCtxtVErr(ctxt, node, domain, code, level,
                str1, str2, str3, int1, msg, ap);
    va_end(ap);
}

/**
 * xmlErrInternal:
 * @ctxt:  an XML parser context
 * @msg:  the error message
 * @str:  error information
 *
 * Handle an internal error
 */
static void LIBXML_ATTR_FORMAT(2,0)
xmlErrInternal(xmlParserCtxtPtr ctxt, const char *msg, const xmlChar * str)
{
    if (ctxt == NULL)
        return;
    xmlCtxtErr(ctxt, NULL, XML_FROM_PARSER, XML_ERR_INTERNAL_ERROR,
               XML_ERR_FATAL, str, NULL, NULL, 0, msg, str);
}

/**
 * xmlFatalErr:
 * @ctxt:  an XML parser context
 * @error:  the error number
 * @info:  extra information string
 *
 * Handle a fatal parser error, i.e. violating Well-Formedness constraints
 */
void
xmlFatalErr(xmlParserCtxtPtr ctxt, xmlParserErrors error, const char *info)
{
    const char *errmsg;

    errmsg = xmlErrString(error);

    if (info == NULL) {
        xmlCtxtErr(ctxt, NULL, XML_FROM_PARSER, error, XML_ERR_FATAL,
                   NULL, NULL, NULL, 0, "%s\n", errmsg);
    } else {
        xmlCtxtErr(ctxt, NULL, XML_FROM_PARSER, error, XML_ERR_FATAL,
                   (const xmlChar *) info, NULL, NULL, 0,
                   "%s: %s\n", errmsg, info);
    }
}

/**
 * xmlErrEncodingInt:
 * @ctxt:  an XML parser context
 * @error:  the error number
 * @msg:  the error message
 * @val:  an integer value
 *
 * n encoding error
 */
static void LIBXML_ATTR_FORMAT(3,0)
xmlErrEncodingInt(xmlParserCtxtPtr ctxt, xmlParserErrors error,
                  const char *msg, int val)
{
    xmlCtxtErr(ctxt, NULL, XML_FROM_PARSER, error, XML_ERR_FATAL,
               NULL, NULL, NULL, val, msg, val);
}

/**
 * xmlIsLetter:
 * @c:  an unicode character (int)
 *
 * Check whether the character is allowed by the production
 * [84] Letter ::= BaseChar | Ideographic
 *
 * Returns 0 if not, non-zero otherwise
 */
int
xmlIsLetter(int c) {
    return(IS_BASECHAR(c) || IS_IDEOGRAPHIC(c));
}

/************************************************************************
 *									*
 *		Input handling functions for progressive parsing	*
 *									*
 ************************************************************************/

/* we need to keep enough input to show errors in context */
#define LINE_LEN        80

/**
 * xmlHaltParser:
 * @ctxt:  an XML parser context
 *
 * Blocks further parser processing don't override error
 * for internal use
 */
void
xmlHaltParser(xmlParserCtxtPtr ctxt) {
    if (ctxt == NULL)
        return;
    ctxt->instate = XML_PARSER_EOF; /* TODO: Remove after refactoring */
    ctxt->disableSAX = 2;
}

/**
 * xmlParserInputRead:
 * @in:  an XML parser input
 * @len:  an indicative size for the lookahead
 *
 * DEPRECATED: This function was internal and is deprecated.
 *
 * Returns -1 as this is an error to use it.
 */
int
xmlParserInputRead(xmlParserInputPtr in ATTRIBUTE_UNUSED, int len ATTRIBUTE_UNUSED) {
    return(-1);
}

/**
 * xmlParserGrow:
 * @ctxt:  an XML parser context
 *
 * Grow the input buffer.
 *
 * Returns the number of bytes read or -1 in case of error.
 */
int
xmlParserGrow(xmlParserCtxtPtr ctxt) {
    xmlParserInputPtr in = ctxt->input;
    xmlParserInputBufferPtr buf = in->buf;
    ptrdiff_t curEnd = in->end - in->cur;
    ptrdiff_t curBase = in->cur - in->base;
    int ret;

    if (buf == NULL)
        return(0);
    /* Don't grow push parser buffer. */
    if ((ctxt->progressive) && (ctxt->inputNr <= 1))
        return(0);
    /* Don't grow memory buffers. */
    if ((buf->encoder == NULL) && (buf->readcallback == NULL))
        return(0);
    if (buf->error != 0)
        return(-1);

    if (((curEnd > XML_MAX_LOOKUP_LIMIT) ||
         (curBase > XML_MAX_LOOKUP_LIMIT)) &&
        ((ctxt->options & XML_PARSE_HUGE) == 0)) {
        xmlFatalErr(ctxt, XML_ERR_RESOURCE_LIMIT,
                    "Buffer size limit exceeded, try XML_PARSE_HUGE\n");
        xmlHaltParser(ctxt);
	return(-1);
    }

    if (curEnd >= INPUT_CHUNK)
        return(0);

    ret = xmlParserInputBufferGrow(buf, INPUT_CHUNK);
    xmlBufUpdateInput(buf->buffer, in, curBase);

    if (ret < 0) {
        xmlCtxtErrIO(ctxt, buf->error, NULL);
    }

    return(ret);
}

/**
 * xmlParserInputGrow:
 * @in:  an XML parser input
 * @len:  an indicative size for the lookahead
 *
 * DEPRECATED: Don't use.
 *
 * This function increase the input for the parser. It tries to
 * preserve pointers to the input buffer, and keep already read data
 *
 * Returns the amount of char read, or -1 in case of error, 0 indicate the
 * end of this entity
 */
int
xmlParserInputGrow(xmlParserInputPtr in, int len) {
    int ret;
    size_t indx;

    if ((in == NULL) || (len < 0)) return(-1);
    if (in->buf == NULL) return(-1);
    if (in->base == NULL) return(-1);
    if (in->cur == NULL) return(-1);
    if (in->buf->buffer == NULL) return(-1);

    /* Don't grow memory buffers. */
    if ((in->buf->encoder == NULL) && (in->buf->readcallback == NULL))
        return(0);

    indx = in->cur - in->base;
    if (xmlBufUse(in->buf->buffer) > (unsigned int) indx + INPUT_CHUNK) {
        return(0);
    }
    ret = xmlParserInputBufferGrow(in->buf, len);

    in->base = xmlBufContent(in->buf->buffer);
    if (in->base == NULL) {
        in->base = BAD_CAST "";
        in->cur = in->base;
        in->end = in->base;
        return(-1);
    }
    in->cur = in->base + indx;
    in->end = xmlBufEnd(in->buf->buffer);

    return(ret);
}

/**
 * xmlParserShrink:
 * @ctxt:  an XML parser context
 *
 * Shrink the input buffer.
 */
void
xmlParserShrink(xmlParserCtxtPtr ctxt) {
    xmlParserInputPtr in = ctxt->input;
    xmlParserInputBufferPtr buf = in->buf;
    size_t used;

    if (buf == NULL)
        return;
    /* Don't shrink pull parser memory buffers. */
    if (((ctxt->progressive == 0) || (ctxt->inputNr > 1)) &&
        (buf->encoder == NULL) &&
        (buf->readcallback == NULL))
        return;

    used = in->cur - in->base;
    /*
     * Do not shrink on large buffers whose only a tiny fraction
     * was consumed
     */
    if (used > INPUT_CHUNK) {
	size_t res = xmlBufShrink(buf->buffer, used - LINE_LEN);

	if (res > 0) {
            used -= res;
            if ((res > ULONG_MAX) ||
                (in->consumed > ULONG_MAX - (unsigned long)res))
                in->consumed = ULONG_MAX;
            else
                in->consumed += res;
	}
    }

    xmlBufUpdateInput(buf->buffer, in, used);
}

/**
 * xmlParserInputShrink:
 * @in:  an XML parser input
 *
 * DEPRECATED: Don't use.
 *
 * This function removes used input for the parser.
 */
void
xmlParserInputShrink(xmlParserInputPtr in) {
    size_t used;
    size_t ret;

    if (in == NULL) return;
    if (in->buf == NULL) return;
    if (in->base == NULL) return;
    if (in->cur == NULL) return;
    if (in->buf->buffer == NULL) return;

    used = in->cur - in->base;
    /*
     * Do not shrink on large buffers whose only a tiny fraction
     * was consumed
     */
    if (used > INPUT_CHUNK) {
	ret = xmlBufShrink(in->buf->buffer, used - LINE_LEN);
	if (ret > 0) {
            used -= ret;
            if ((ret > ULONG_MAX) ||
                (in->consumed > ULONG_MAX - (unsigned long)ret))
                in->consumed = ULONG_MAX;
            else
                in->consumed += ret;
	}
    }

    if (xmlBufUse(in->buf->buffer) <= INPUT_CHUNK) {
        xmlParserInputBufferRead(in->buf, 2 * INPUT_CHUNK);
    }

    in->base = xmlBufContent(in->buf->buffer);
    if (in->base == NULL) {
        /* TODO: raise error */
        in->base = BAD_CAST "";
        in->cur = in->base;
        in->end = in->base;
        return;
    }
    in->cur = in->base + used;
    in->end = xmlBufEnd(in->buf->buffer);
}

/************************************************************************
 *									*
 *		UTF8 character input and related functions		*
 *									*
 ************************************************************************/

/**
 * xmlNextChar:
 * @ctxt:  the XML parser context
 *
 * DEPRECATED: Internal function, do not use.
 *
 * Skip to the next char input char.
 */

void
xmlNextChar(xmlParserCtxtPtr ctxt)
{
    const unsigned char *cur;
    size_t avail;
    int c;

    if ((ctxt == NULL) || (ctxt->input == NULL))
        return;

    avail = ctxt->input->end - ctxt->input->cur;

    if (avail < INPUT_CHUNK) {
        xmlParserGrow(ctxt);
        if (ctxt->input->cur >= ctxt->input->end)
            return;
        avail = ctxt->input->end - ctxt->input->cur;
    }

    cur = ctxt->input->cur;
    c = *cur;

    if (c < 0x80) {
        if (c == '\n') {
            ctxt->input->cur++;
            ctxt->input->line++;
            ctxt->input->col = 1;
        } else if (c == '\r') {
            /*
             *   2.11 End-of-Line Handling
             *   the literal two-character sequence "#xD#xA" or a standalone
             *   literal #xD, an XML processor must pass to the application
             *   the single character #xA.
             */
            ctxt->input->cur += ((cur[1] == '\n') ? 2 : 1);
            ctxt->input->line++;
            ctxt->input->col = 1;
            return;
        } else {
            ctxt->input->cur++;
            ctxt->input->col++;
        }
    } else {
        ctxt->input->col++;

        if ((avail < 2) || (cur[1] & 0xc0) != 0x80)
            goto encoding_error;

        if (c < 0xe0) {
            /* 2-byte code */
            if (c < 0xc2)
                goto encoding_error;
            ctxt->input->cur += 2;
        } else {
            unsigned int val = (c << 8) | cur[1];

            if ((avail < 3) || (cur[2] & 0xc0) != 0x80)
                goto encoding_error;

            if (c < 0xf0) {
                /* 3-byte code */
                if ((val < 0xe0a0) || ((val >= 0xeda0) && (val < 0xee00)))
                    goto encoding_error;
                ctxt->input->cur += 3;
            } else {
                if ((avail < 4) || ((cur[3] & 0xc0) != 0x80))
                    goto encoding_error;

                /* 4-byte code */
                if ((val < 0xf090) || (val >= 0xf490))
                    goto encoding_error;
                ctxt->input->cur += 4;
            }
        }
    }

    return;

encoding_error:
    /* Only report the first error */
    if ((ctxt->input->flags & XML_INPUT_ENCODING_ERROR) == 0) {
        xmlCtxtErrIO(ctxt, XML_ERR_INVALID_ENCODING, NULL);
        ctxt->input->flags |= XML_INPUT_ENCODING_ERROR;
    }
    ctxt->input->cur++;
    return;
}

/**
 * xmlCurrentChar:
 * @ctxt:  the XML parser context
 * @len:  pointer to the length of the char read
 *
 * DEPRECATED: Internal function, do not use.
 *
 * The current char value, if using UTF-8 this may actually span multiple
 * bytes in the input buffer. Implement the end of line normalization:
 * 2.11 End-of-Line Handling
 * Wherever an external parsed entity or the literal entity value
 * of an internal parsed entity contains either the literal two-character
 * sequence "#xD#xA" or a standalone literal #xD, an XML processor
 * must pass to the application the single character #xA.
 * This behavior can conveniently be produced by normalizing all
 * line breaks to #xA on input, before parsing.)
 *
 * Returns the current char value and its length
 */

int
xmlCurrentChar(xmlParserCtxtPtr ctxt, int *len) {
    const unsigned char *cur;
    size_t avail;
    int c;

    if ((ctxt == NULL) || (len == NULL) || (ctxt->input == NULL)) return(0);

    avail = ctxt->input->end - ctxt->input->cur;

    if (avail < INPUT_CHUNK) {
        xmlParserGrow(ctxt);
        avail = ctxt->input->end - ctxt->input->cur;
    }

    cur = ctxt->input->cur;
    c = *cur;

    if (c < 0x80) {
	/* 1-byte code */
        if (c < 0x20) {
            /*
             *   2.11 End-of-Line Handling
             *   the literal two-character sequence "#xD#xA" or a standalone
             *   literal #xD, an XML processor must pass to the application
             *   the single character #xA.
             */
            if (c == '\r') {
                /*
                 * TODO: This function shouldn't change the 'cur' pointer
                 * as side effect, but the NEXTL macro in parser.c relies
                 * on this behavior when incrementing line numbers.
                 */
                if (cur[1] == '\n')
                    ctxt->input->cur++;
                *len = 1;
                c = '\n';
            } else if (c == 0) {
                if (ctxt->input->cur >= ctxt->input->end) {
                    *len = 0;
                } else {
                    *len = 1;
                    /*
                     * TODO: Null bytes should be handled by callers,
                     * but this can be tricky.
                     */
                    xmlErrEncodingInt(ctxt, XML_ERR_INVALID_CHAR,
                            "Char 0x0 out of allowed range\n", c);
                }
            } else {
                *len = 1;
            }
        } else {
            *len = 1;
        }

        return(c);
    } else {
        int val;

        if (avail < 2)
            goto incomplete_sequence;
        if ((cur[1] & 0xc0) != 0x80)
            goto encoding_error;

        if (c < 0xe0) {
            /* 2-byte code */
            if (c < 0xc2)
                goto encoding_error;
            val = (c & 0x1f) << 6;
            val |= cur[1] & 0x3f;
            *len = 2;
        } else {
            if (avail < 3)
                goto incomplete_sequence;
            if ((cur[2] & 0xc0) != 0x80)
                goto encoding_error;

            if (c < 0xf0) {
                /* 3-byte code */
                val = (c & 0xf) << 12;
                val |= (cur[1] & 0x3f) << 6;
                val |= cur[2] & 0x3f;
                if ((val < 0x800) || ((val >= 0xd800) && (val < 0xe000)))
                    goto encoding_error;
                *len = 3;
            } else {
                if (avail < 4)
                    goto incomplete_sequence;
                if ((cur[3] & 0xc0) != 0x80)
                    goto encoding_error;

                /* 4-byte code */
                val = (c & 0x0f) << 18;
                val |= (cur[1] & 0x3f) << 12;
                val |= (cur[2] & 0x3f) << 6;
                val |= cur[3] & 0x3f;
                if ((val < 0x10000) || (val >= 0x110000))
                    goto encoding_error;
                *len = 4;
            }
        }

        return(val);
    }

encoding_error:
    /* Only report the first error */
    if ((ctxt->input->flags & XML_INPUT_ENCODING_ERROR) == 0) {
        xmlCtxtErrIO(ctxt, XML_ERR_INVALID_ENCODING, NULL);
        ctxt->input->flags |= XML_INPUT_ENCODING_ERROR;
    }
    *len = 1;
    return(0xFFFD); /* U+FFFD Replacement Character */

incomplete_sequence:
    /*
     * An encoding problem may arise from a truncated input buffer
     * splitting a character in the middle. In that case do not raise
     * an error but return 0. This should only happen when push parsing
     * char data.
     */
    *len = 0;
    return(0);
}

/**
 * xmlStringCurrentChar:
 * @ctxt:  the XML parser context
 * @cur:  pointer to the beginning of the char
 * @len:  pointer to the length of the char read
 *
 * DEPRECATED: Internal function, do not use.
 *
 * The current char value, if using UTF-8 this may actually span multiple
 * bytes in the input buffer.
 *
 * Returns the current char value and its length
 */

int
xmlStringCurrentChar(xmlParserCtxtPtr ctxt ATTRIBUTE_UNUSED,
                     const xmlChar *cur, int *len) {
    int c;

    if ((cur == NULL) || (len == NULL))
        return(0);

    /* cur is zero-terminated, so we can lie about its length. */
    *len = 4;
    c = xmlGetUTF8Char(cur, len);

    return((c < 0) ? 0 : c);
}

/**
 * xmlCopyCharMultiByte:
 * @out:  pointer to an array of xmlChar
 * @val:  the char value
 *
 * append the char value in the array
 *
 * Returns the number of xmlChar written
 */
int
xmlCopyCharMultiByte(xmlChar *out, int val) {
    if ((out == NULL) || (val < 0)) return(0);
    /*
     * We are supposed to handle UTF8, check it's valid
     * From rfc2044: encoding of the Unicode values on UTF-8:
     *
     * UCS-4 range (hex.)           UTF-8 octet sequence (binary)
     * 0000 0000-0000 007F   0xxxxxxx
     * 0000 0080-0000 07FF   110xxxxx 10xxxxxx
     * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
     */
    if  (val >= 0x80) {
	xmlChar *savedout = out;
	int bits;
	if (val <   0x800) { *out++= (val >>  6) | 0xC0;  bits=  0; }
	else if (val < 0x10000) { *out++= (val >> 12) | 0xE0;  bits=  6;}
	else if (val < 0x110000)  { *out++= (val >> 18) | 0xF0;  bits=  12; }
	else {
	    xmlErrEncodingInt(NULL, XML_ERR_INVALID_CHAR,
		    "Internal error, xmlCopyCharMultiByte 0x%X out of bound\n",
			      val);
	    return(0);
	}
	for ( ; bits >= 0; bits-= 6)
	    *out++= ((val >> bits) & 0x3F) | 0x80 ;
	return (out - savedout);
    }
    *out = val;
    return 1;
}

/**
 * xmlCopyChar:
 * @len:  Ignored, compatibility
 * @out:  pointer to an array of xmlChar
 * @val:  the char value
 *
 * append the char value in the array
 *
 * Returns the number of xmlChar written
 */

int
xmlCopyChar(int len ATTRIBUTE_UNUSED, xmlChar *out, int val) {
    if ((out == NULL) || (val < 0)) return(0);
    /* the len parameter is ignored */
    if  (val >= 0x80) {
	return(xmlCopyCharMultiByte (out, val));
    }
    *out = val;
    return 1;
}

/************************************************************************
 *									*
 *		Commodity functions to switch encodings			*
 *									*
 ************************************************************************/

static int
xmlDetectEBCDIC(xmlParserInputPtr input, xmlCharEncodingHandlerPtr *hout) {
    xmlChar out[200];
    xmlCharEncodingHandlerPtr handler;
    int inlen, outlen, res, i;

    *hout = NULL;

    /*
     * To detect the EBCDIC code page, we convert the first 200 bytes
     * to EBCDIC-US and try to find the encoding declaration.
     */
    res = xmlLookupCharEncodingHandler(XML_CHAR_ENCODING_EBCDIC, &handler);
    if (res != 0)
        return(res);
    outlen = sizeof(out) - 1;
    inlen = input->end - input->cur;
    res = xmlEncInputChunk(handler, out, &outlen, input->cur, &inlen);
    /*
     * Return the EBCDIC handler if decoding failed. The error will
     * be reported later.
     */
    if (res < 0)
        goto done;
    out[outlen] = 0;

    for (i = 0; i < outlen; i++) {
        if (out[i] == '>')
            break;
        if ((out[i] == 'e') &&
            (xmlStrncmp(out + i, BAD_CAST "encoding", 8) == 0)) {
            int start, cur, quote;

            i += 8;
            while (IS_BLANK_CH(out[i]))
                i += 1;
            if (out[i++] != '=')
                break;
            while (IS_BLANK_CH(out[i]))
                i += 1;
            quote = out[i++];
            if ((quote != '\'') && (quote != '"'))
                break;
            start = i;
            cur = out[i];
            while (((cur >= 'a') && (cur <= 'z')) ||
                   ((cur >= 'A') && (cur <= 'Z')) ||
                   ((cur >= '0') && (cur <= '9')) ||
                   (cur == '.') || (cur == '_') ||
                   (cur == '-'))
                cur = out[++i];
            if (cur != quote)
                break;
            out[i] = 0;
            xmlCharEncCloseFunc(handler);
            res = xmlOpenCharEncodingHandler((char *) out + start, &handler);
            if (res != 0)
                return(res);
            *hout = handler;
            return(0);
        }
    }

done:
    /*
     * Encoding handlers are stateful, so we have to recreate them.
     */
    xmlCharEncCloseFunc(handler);
    res = xmlLookupCharEncodingHandler(XML_CHAR_ENCODING_EBCDIC, &handler);
    if (res != 0)
        return(res);
    *hout = handler;
    return(0);
}

/**
 * xmlSwitchEncoding:
 * @ctxt:  the parser context
 * @enc:  the encoding value (number)
 *
 * Use encoding specified by enum to decode input data. This overrides
 * the encoding found in the XML declaration.
 *
 * This function can also be used to override the encoding of chunks
 * passed to xmlParseChunk.
 *
 * Returns 0 in case of success, -1 otherwise
 */
int
xmlSwitchEncoding(xmlParserCtxtPtr ctxt, xmlCharEncoding enc)
{
    xmlCharEncodingHandlerPtr handler = NULL;
    int ret;
    int res;

    if ((ctxt == NULL) || (ctxt->input == NULL))
        return(-1);

    switch (enc) {
	case XML_CHAR_ENCODING_NONE:
	case XML_CHAR_ENCODING_UTF8:
        case XML_CHAR_ENCODING_ASCII:
            res = 0;
            break;
        case XML_CHAR_ENCODING_EBCDIC:
            res = xmlDetectEBCDIC(ctxt->input, &handler);
            break;
        default:
            res = xmlLookupCharEncodingHandler(enc, &handler);
            break;
    }

    if (res != 0) {
        const char *name = xmlGetCharEncodingName(enc);

        xmlFatalErr(ctxt, res, (name ? name : "<null>"));
        return(-1);
    }

    ret = xmlSwitchInputEncoding(ctxt, ctxt->input, handler);

    if ((ret >= 0) && (enc == XML_CHAR_ENCODING_NONE)) {
        ctxt->input->flags &= ~XML_INPUT_HAS_ENCODING;
    }

    return(ret);
}

/**
 * xmlSwitchEncodingName:
 * @ctxt:  the parser context
 * @encoding:  the encoding name
 *
 * Use specified encoding to decode input data. This overrides the
 * encoding found in the XML declaration.
 *
 * This function can also be used to override the encoding of chunks
 * passed to xmlParseChunk.
 *
 * Available since 2.13.0.
 *
 * Returns 0 in case of success, -1 otherwise
 */
int
xmlSwitchEncodingName(xmlParserCtxtPtr ctxt, const char *encoding) {
    xmlCharEncodingHandlerPtr handler;
    int res;

    if (encoding == NULL)
        return(-1);

    res = xmlOpenCharEncodingHandler(encoding, &handler);
    if (res != 0) {
        xmlFatalErr(ctxt, res, encoding);
        return(-1);
    }

    return(xmlSwitchInputEncoding(ctxt, ctxt->input, handler));
}

/**
 * xmlSwitchInputEncoding:
 * @ctxt:  the parser context
 * @input:  the input stream
 * @handler:  the encoding handler
 *
 * DEPRECATED: Internal function, don't use.
 *
 * Use encoding handler to decode input data.
 *
 * Returns 0 in case of success, -1 otherwise
 */
int
xmlSwitchInputEncoding(xmlParserCtxtPtr ctxt, xmlParserInputPtr input,
                       xmlCharEncodingHandlerPtr handler)
{
    int nbchars;
    xmlParserInputBufferPtr in;

    if ((input == NULL) || (input->buf == NULL)) {
        xmlCharEncCloseFunc(handler);
	return (-1);
    }
    in = input->buf;

    input->flags |= XML_INPUT_HAS_ENCODING;

    /*
     * UTF-8 requires no encoding handler.
     */
    if ((handler != NULL) &&
        (xmlStrcasecmp(BAD_CAST handler->name, BAD_CAST "UTF-8") == 0)) {
        xmlCharEncCloseFunc(handler);
        handler = NULL;
    }

    if (in->encoder == handler)
        return (0);

    if (in->encoder != NULL) {
        /*
         * Switching encodings during parsing is a really bad idea,
         * but Chromium can switch between ISO-8859-1 and UTF-16 before
         * separate calls to xmlParseChunk.
         *
         * TODO: We should check whether the "raw" input buffer is empty and
         * convert the old content using the old encoder.
         */

        xmlCharEncCloseFunc(in->encoder);
        in->encoder = handler;
        return (0);
    }

    in->encoder = handler;

    /*
     * Is there already some content down the pipe to convert ?
     */
    if (xmlBufIsEmpty(in->buffer) == 0) {
        xmlBufPtr buf;
        size_t processed;

        buf = xmlBufCreate();
        if (buf == NULL) {
            xmlCtxtErrMemory(ctxt);
            return(-1);
        }

        /*
         * Shrink the current input buffer.
         * Move it as the raw buffer and create a new input buffer
         */
        processed = input->cur - input->base;
        xmlBufShrink(in->buffer, processed);
        input->consumed += processed;
        in->raw = in->buffer;
        in->buffer = buf;
        in->rawconsumed = processed;

        nbchars = xmlCharEncInput(in);
        xmlBufResetInput(in->buffer, input);
        if (nbchars == XML_ENC_ERR_MEMORY) {
            xmlCtxtErrMemory(ctxt);
        } else if (nbchars < 0) {
            xmlCtxtErrIO(ctxt, in->error, NULL);
            xmlHaltParser(ctxt);
            return (-1);
        }
    }
    return (0);
}

/**
 * xmlSwitchToEncoding:
 * @ctxt:  the parser context
 * @handler:  the encoding handler
 *
 * Use encoding handler to decode input data.
 *
 * This function can be used to enforce the encoding of chunks passed
 * to xmlParseChunk.
 *
 * Returns 0 in case of success, -1 otherwise
 */
int
xmlSwitchToEncoding(xmlParserCtxtPtr ctxt, xmlCharEncodingHandlerPtr handler)
{
    if (ctxt == NULL)
        return(-1);
    return(xmlSwitchInputEncoding(ctxt, ctxt->input, handler));
}

/**
 * xmlDetectEncoding:
 * @ctxt:  the parser context
 *
 * Handle optional BOM, detect and switch to encoding.
 *
 * Assumes that there are at least four bytes in the input buffer.
 */
void
xmlDetectEncoding(xmlParserCtxtPtr ctxt) {
    const xmlChar *in;
    xmlCharEncoding enc;
    int bomSize;
    int autoFlag = 0;

    if (xmlParserGrow(ctxt) < 0)
        return;
    in = ctxt->input->cur;
    if (ctxt->input->end - in < 4)
        return;

    if (ctxt->input->flags & XML_INPUT_HAS_ENCODING) {
        /*
         * If the encoding was already set, only skip the BOM which was
         * possibly decoded to UTF-8.
         */
        if ((in[0] == 0xEF) && (in[1] == 0xBB) && (in[2] == 0xBF)) {
            ctxt->input->cur += 3;
        }

        return;
    }

    enc = XML_CHAR_ENCODING_NONE;
    bomSize = 0;

    switch (in[0]) {
        case 0x00:
            if ((in[1] == 0x00) && (in[2] == 0x00) && (in[3] == 0x3C)) {
                enc = XML_CHAR_ENCODING_UCS4BE;
                autoFlag = XML_INPUT_AUTO_OTHER;
            } else if ((in[1] == 0x3C) && (in[2] == 0x00) && (in[3] == 0x3F)) {
                enc = XML_CHAR_ENCODING_UTF16BE;
                autoFlag = XML_INPUT_AUTO_UTF16BE;
            }
            break;

        case 0x3C:
            if (in[1] == 0x00) {
                if ((in[2] == 0x00) && (in[3] == 0x00)) {
                    enc = XML_CHAR_ENCODING_UCS4LE;
                    autoFlag = XML_INPUT_AUTO_OTHER;
                } else if ((in[2] == 0x3F) && (in[3] == 0x00)) {
                    enc = XML_CHAR_ENCODING_UTF16LE;
                    autoFlag = XML_INPUT_AUTO_UTF16LE;
                }
            }
            break;

        case 0x4C:
	    if ((in[1] == 0x6F) && (in[2] == 0xA7) && (in[3] == 0x94)) {
	        enc = XML_CHAR_ENCODING_EBCDIC;
                autoFlag = XML_INPUT_AUTO_OTHER;
            }
            break;

        case 0xEF:
            if ((in[1] == 0xBB) && (in[2] == 0xBF)) {
                enc = XML_CHAR_ENCODING_UTF8;
                autoFlag = XML_INPUT_AUTO_UTF8;
                bomSize = 3;
            }
            break;

        case 0xFE:
            if (in[1] == 0xFF) {
                enc = XML_CHAR_ENCODING_UTF16BE;
                autoFlag = XML_INPUT_AUTO_UTF16BE;
                bomSize = 2;
            }
            break;

        case 0xFF:
            if (in[1] == 0xFE) {
                enc = XML_CHAR_ENCODING_UTF16LE;
                autoFlag = XML_INPUT_AUTO_UTF16LE;
                bomSize = 2;
            }
            break;
    }

    if (bomSize > 0) {
        ctxt->input->cur += bomSize;
    }

    if (enc != XML_CHAR_ENCODING_NONE) {
        ctxt->input->flags |= autoFlag;
        xmlSwitchEncoding(ctxt, enc);
    }
}

/**
 * xmlSetDeclaredEncoding:
 * @ctxt:  the parser context
 * @encoding:  declared encoding
 *
 * Set the encoding from a declaration in the document.
 *
 * If no encoding was set yet, switch the encoding. Otherwise, only warn
 * about encoding mismatches.
 *
 * Takes ownership of 'encoding'.
 */
void
xmlSetDeclaredEncoding(xmlParserCtxtPtr ctxt, xmlChar *encoding) {
    if (((ctxt->input->flags & XML_INPUT_HAS_ENCODING) == 0) &&
        ((ctxt->options & XML_PARSE_IGNORE_ENC) == 0)) {
        xmlSwitchEncodingName(ctxt, (const char *) encoding);
        ctxt->input->flags |= XML_INPUT_USES_ENC_DECL;
    } else if (ctxt->input->flags & XML_INPUT_AUTO_ENCODING) {
        static const char *allowedUTF8[] = {
            "UTF-8", "UTF8", NULL
        };
        static const char *allowedUTF16LE[] = {
            "UTF-16", "UTF-16LE", "UTF16", NULL
        };
        static const char *allowedUTF16BE[] = {
            "UTF-16", "UTF-16BE", "UTF16", NULL
        };
        const char **allowed = NULL;
        const char *autoEnc = NULL;

        switch (ctxt->input->flags & XML_INPUT_AUTO_ENCODING) {
            case XML_INPUT_AUTO_UTF8:
                allowed = allowedUTF8;
                autoEnc = "UTF-8";
                break;
            case XML_INPUT_AUTO_UTF16LE:
                allowed = allowedUTF16LE;
                autoEnc = "UTF-16LE";
                break;
            case XML_INPUT_AUTO_UTF16BE:
                allowed = allowedUTF16BE;
                autoEnc = "UTF-16BE";
                break;
        }

        if (allowed != NULL) {
            const char **p;
            int match = 0;

            for (p = allowed; *p != NULL; p++) {
                if (xmlStrcasecmp(encoding, BAD_CAST *p) == 0) {
                    match = 1;
                    break;
                }
            }

            if (match == 0) {
                xmlWarningMsg(ctxt, XML_WAR_ENCODING_MISMATCH,
                              "Encoding '%s' doesn't match "
                              "auto-detected '%s'\n",
                              encoding, BAD_CAST autoEnc);
                xmlFree(encoding);
                encoding = xmlStrdup(BAD_CAST autoEnc);
                if (encoding == NULL)
                    xmlCtxtErrMemory(ctxt);
            }
        }
    }

    if (ctxt->encoding != NULL)
        xmlFree((xmlChar *) ctxt->encoding);
    ctxt->encoding = encoding;
}

/************************************************************************
 *									*
 *	Commodity functions to handle entities processing		*
 *									*
 ************************************************************************/

/**
 * xmlFreeInputStream:
 * @input:  an xmlParserInputPtr
 *
 * Free up an input stream.
 */
void
xmlFreeInputStream(xmlParserInputPtr input) {
    if (input == NULL) return;

    if (input->filename != NULL) xmlFree((char *) input->filename);
    if (input->directory != NULL) xmlFree((char *) input->directory);
    if (input->version != NULL) xmlFree((char *) input->version);
    if ((input->free != NULL) && (input->base != NULL))
        input->free((xmlChar *) input->base);
    if (input->buf != NULL)
        xmlFreeParserInputBuffer(input->buf);
    xmlFree(input);
}

/**
 * xmlNewInputStream:
 * @ctxt:  an XML parser context
 *
 * Create a new input stream structure.
 *
 * Returns the new input stream or NULL
 */
xmlParserInputPtr
xmlNewInputStream(xmlParserCtxtPtr ctxt) {
    xmlParserInputPtr input;

    input = (xmlParserInputPtr) xmlMalloc(sizeof(xmlParserInput));
    if (input == NULL) {
        xmlCtxtErrMemory(ctxt);
	return(NULL);
    }
    memset(input, 0, sizeof(xmlParserInput));
    input->line = 1;
    input->col = 1;

    /*
     * If the context is NULL the id cannot be initialized, but that
     * should not happen while parsing which is the situation where
     * the id is actually needed.
     */
    if (ctxt != NULL) {
        if (input->id >= INT_MAX) {
            xmlCtxtErrMemory(ctxt);
            return(NULL);
        }
        input->id = ctxt->input_id++;
    }

    return(input);
}

/**
 * xmlNewIOInputStream:
 * @ctxt:  an XML parser context
 * @input:  an I/O Input
 * @enc:  the charset encoding if known
 *
 * Create a new input stream structure encapsulating the @input into
 * a stream suitable for the parser.
 *
 * Returns the new input stream or NULL
 */
xmlParserInputPtr
xmlNewIOInputStream(xmlParserCtxtPtr ctxt, xmlParserInputBufferPtr input,
	            xmlCharEncoding enc) {
    xmlParserInputPtr inputStream;

    if (input == NULL) return(NULL);
    inputStream = xmlNewInputStream(ctxt);
    if (inputStream == NULL) {
	return(NULL);
    }
    inputStream->filename = NULL;
    inputStream->buf = input;
    xmlBufResetInput(inputStream->buf->buffer, inputStream);

    if (enc != XML_CHAR_ENCODING_NONE) {
        xmlSwitchEncoding(ctxt, enc);
    }

    return(inputStream);
}

/**
 * xmlNewEntityInputStream:
 * @ctxt:  an XML parser context
 * @entity:  an Entity pointer
 *
 * DEPRECATED: Internal function, do not use.
 *
 * Create a new input stream based on an xmlEntityPtr
 *
 * Returns the new input stream or NULL
 */
xmlParserInputPtr
xmlNewEntityInputStream(xmlParserCtxtPtr ctxt, xmlEntityPtr entity) {
    xmlParserInputPtr input;

    if (entity == NULL) {
        xmlErrInternal(ctxt, "xmlNewEntityInputStream entity = NULL\n",
	               NULL);
	return(NULL);
    }
    if (entity->content == NULL) {
	switch (entity->etype) {
            case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
	        xmlErrInternal(ctxt, "Cannot parse entity %s\n",
		               entity->name);
                break;
            case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
            case XML_EXTERNAL_PARAMETER_ENTITY:
		input = xmlLoadExternalEntity((char *) entity->URI,
		       (char *) entity->ExternalID, ctxt);
                if (input != NULL)
                    input->entity = entity;
                return(input);
            case XML_INTERNAL_GENERAL_ENTITY:
	        xmlErrInternal(ctxt,
		      "Internal entity %s without content !\n",
		               entity->name);
                break;
            case XML_INTERNAL_PARAMETER_ENTITY:
	        xmlErrInternal(ctxt,
		      "Internal parameter entity %s without content !\n",
		               entity->name);
                break;
            case XML_INTERNAL_PREDEFINED_ENTITY:
	        xmlErrInternal(ctxt,
		      "Predefined entity %s without content !\n",
		               entity->name);
                break;
	}
	return(NULL);
    }
    input = xmlNewInputStream(ctxt);
    if (input == NULL) {
	return(NULL);
    }
    if (entity->URI != NULL)
	input->filename = (char *) xmlStrdup((xmlChar *) entity->URI);
    input->base = entity->content;
    if (entity->length == 0)
        entity->length = xmlStrlen(entity->content);
    input->cur = entity->content;
    input->length = entity->length;
    input->end = &entity->content[input->length];
    input->entity = entity;
    return(input);
}

/**
 * xmlNewStringInputStream:
 * @ctxt:  an XML parser context
 * @buffer:  an memory buffer
 *
 * Create a new input stream based on a memory buffer.
 * Returns the new input stream
 */
xmlParserInputPtr
xmlNewStringInputStream(xmlParserCtxtPtr ctxt, const xmlChar *buffer) {
    xmlParserInputPtr input;
    xmlParserInputBufferPtr buf;

    if (buffer == NULL) {
        xmlErrInternal(ctxt, "xmlNewStringInputStream string = NULL\n",
	               NULL);
	return(NULL);
    }
    buf = xmlParserInputBufferCreateString(buffer);
    if (buf == NULL) {
	xmlCtxtErrMemory(ctxt);
        return(NULL);
    }
    input = xmlNewInputStream(ctxt);
    if (input == NULL) {
        xmlCtxtErrMemory(ctxt);
	xmlFreeParserInputBuffer(buf);
	return(NULL);
    }
    input->buf = buf;
    xmlBufResetInput(input->buf->buffer, input);
    return(input);
}

/**
 * xmlNewInputFromFile:
 * @ctxt:  an XML parser context
 * @filename:  the filename to use as entity
 *
 * Create a new input stream based on a file or an URL.
 *
 * Returns the new input stream or NULL in case of error
 */
xmlParserInputPtr
xmlNewInputFromFile(xmlParserCtxtPtr ctxt, const char *filename) {
    xmlParserInputBufferPtr buf;
    xmlParserInputPtr inputStream;
    char *directory = NULL;
    xmlChar *URI = NULL;
    int code;

    if ((ctxt == NULL) || (filename == NULL))
        return(NULL);

    code = xmlParserInputBufferCreateFilenameSafe(filename,
                                                  XML_CHAR_ENCODING_NONE, &buf);
    if (buf == NULL) {
        xmlCtxtErrIO(ctxt, code, filename);
	return(NULL);
    }

    inputStream = xmlNewInputStream(ctxt);
    if (inputStream == NULL) {
	xmlFreeParserInputBuffer(buf);
	return(NULL);
    }

    inputStream->buf = buf;
    inputStream = xmlCheckHTTPInput(ctxt, inputStream);
    if (inputStream == NULL)
        return(NULL);

    if (inputStream->filename == NULL)
	URI = xmlStrdup((xmlChar *) filename);
    else
	URI = xmlStrdup((xmlChar *) inputStream->filename);
    directory = xmlParserGetDirectory((const char *) URI);
    if (inputStream->filename != NULL) xmlFree((char *)inputStream->filename);
    inputStream->filename = (char *) xmlCanonicPath((const xmlChar *) URI);
    if (URI != NULL) xmlFree((char *) URI);
    inputStream->directory = directory;

    xmlBufResetInput(inputStream->buf->buffer, inputStream);
    if ((ctxt->directory == NULL) && (directory != NULL))
        ctxt->directory = (char *) xmlStrdup((const xmlChar *) directory);
    return(inputStream);
}

/************************************************************************
 *									*
 *		Commodity functions to handle parser contexts		*
 *									*
 ************************************************************************/

/**
 * xmlInitSAXParserCtxt:
 * @ctxt:  XML parser context
 * @sax:  SAX handlert
 * @userData:  user data
 *
 * Initialize a SAX parser context
 *
 * Returns 0 in case of success and -1 in case of error
 */

static int
xmlInitSAXParserCtxt(xmlParserCtxtPtr ctxt, const xmlSAXHandler *sax,
                     void *userData)
{
    xmlParserInputPtr input;

    if (ctxt == NULL)
        return(-1);

    xmlInitParser();

    if (ctxt->dict == NULL)
	ctxt->dict = xmlDictCreate();
    if (ctxt->dict == NULL)
	return(-1);
    xmlDictSetLimit(ctxt->dict, XML_MAX_DICTIONARY_LIMIT);

    if (ctxt->sax == NULL)
	ctxt->sax = (xmlSAXHandler *) xmlMalloc(sizeof(xmlSAXHandler));
    if (ctxt->sax == NULL)
	return(-1);
    if (sax == NULL) {
	memset(ctxt->sax, 0, sizeof(xmlSAXHandler));
        xmlSAXVersion(ctxt->sax, 2);
        ctxt->userData = ctxt;
    } else {
	if (sax->initialized == XML_SAX2_MAGIC) {
	    memcpy(ctxt->sax, sax, sizeof(xmlSAXHandler));
        } else {
	    memset(ctxt->sax, 0, sizeof(xmlSAXHandler));
	    memcpy(ctxt->sax, sax, sizeof(xmlSAXHandlerV1));
        }
        ctxt->userData = userData ? userData : ctxt;
    }

    ctxt->maxatts = 0;
    ctxt->atts = NULL;
    /* Allocate the Input stack */
    if (ctxt->inputTab == NULL) {
	ctxt->inputTab = (xmlParserInputPtr *)
		    xmlMalloc(5 * sizeof(xmlParserInputPtr));
	ctxt->inputMax = 5;
    }
    if (ctxt->inputTab == NULL)
	return(-1);
    while ((input = inputPop(ctxt)) != NULL) { /* Non consuming */
        xmlFreeInputStream(input);
    }
    ctxt->inputNr = 0;
    ctxt->input = NULL;

    ctxt->version = NULL;
    ctxt->encoding = NULL;
    ctxt->standalone = -1;
    ctxt->hasExternalSubset = 0;
    ctxt->hasPErefs = 0;
    ctxt->html = 0;
    ctxt->external = 0;
    ctxt->instate = XML_PARSER_START;
    ctxt->token = 0;
    ctxt->directory = NULL;

    /* Allocate the Node stack */
    if (ctxt->nodeTab == NULL) {
	ctxt->nodeTab = (xmlNodePtr *) xmlMalloc(10 * sizeof(xmlNodePtr));
	ctxt->nodeMax = 10;
    }
    if (ctxt->nodeTab == NULL)
	return(-1);
    ctxt->nodeNr = 0;
    ctxt->node = NULL;

    /* Allocate the Name stack */
    if (ctxt->nameTab == NULL) {
	ctxt->nameTab = (const xmlChar **) xmlMalloc(10 * sizeof(xmlChar *));
	ctxt->nameMax = 10;
    }
    if (ctxt->nameTab == NULL)
	return(-1);
    ctxt->nameNr = 0;
    ctxt->name = NULL;

    /* Allocate the space stack */
    if (ctxt->spaceTab == NULL) {
	ctxt->spaceTab = (int *) xmlMalloc(10 * sizeof(int));
	ctxt->spaceMax = 10;
    }
    if (ctxt->spaceTab == NULL)
	return(-1);
    ctxt->spaceNr = 1;
    ctxt->spaceMax = 10;
    ctxt->spaceTab[0] = -1;
    ctxt->space = &ctxt->spaceTab[0];
    ctxt->myDoc = NULL;
    ctxt->wellFormed = 1;
    ctxt->nsWellFormed = 1;
    ctxt->valid = 1;
    ctxt->loadsubset = xmlLoadExtDtdDefaultValue;
    if (ctxt->loadsubset) {
        ctxt->options |= XML_PARSE_DTDLOAD;
    }
    ctxt->validate = xmlDoValidityCheckingDefaultValue;
    ctxt->pedantic = xmlPedanticParserDefaultValue;
    if (ctxt->pedantic) {
        ctxt->options |= XML_PARSE_PEDANTIC;
    }
    ctxt->linenumbers = xmlLineNumbersDefaultValue;
    ctxt->keepBlanks = xmlKeepBlanksDefaultValue;
    if (ctxt->keepBlanks == 0) {
	ctxt->sax->ignorableWhitespace = xmlSAX2IgnorableWhitespace;
	ctxt->options |= XML_PARSE_NOBLANKS;
    }

    ctxt->vctxt.flags = XML_VCTXT_USE_PCTXT;
    ctxt->vctxt.userData = ctxt;
    ctxt->vctxt.error = xmlParserValidityError;
    ctxt->vctxt.warning = xmlParserValidityWarning;
    if (ctxt->validate) {
	if (xmlGetWarningsDefaultValue == 0)
	    ctxt->vctxt.warning = NULL;
	else
	    ctxt->vctxt.warning = xmlParserValidityWarning;
	ctxt->vctxt.nodeMax = 0;
        ctxt->options |= XML_PARSE_DTDVALID;
    }
    ctxt->replaceEntities = xmlSubstituteEntitiesDefaultValue;
    if (ctxt->replaceEntities) {
        ctxt->options |= XML_PARSE_NOENT;
    }
    ctxt->record_info = 0;
    ctxt->checkIndex = 0;
    ctxt->inSubset = 0;
    ctxt->errNo = XML_ERR_OK;
    ctxt->depth = 0;
    ctxt->catalogs = NULL;
    ctxt->sizeentities = 0;
    ctxt->sizeentcopy = 0;
    ctxt->input_id = 1;
    ctxt->maxAmpl = XML_MAX_AMPLIFICATION_DEFAULT;
    xmlInitNodeInfoSeq(&ctxt->node_seq);

    if (ctxt->nsdb == NULL) {
        ctxt->nsdb = xmlParserNsCreate();
        if (ctxt->nsdb == NULL) {
            xmlCtxtErrMemory(ctxt);
            return(-1);
        }
    }

    return(0);
}

/**
 * xmlInitParserCtxt:
 * @ctxt:  an XML parser context
 *
 * DEPRECATED: Internal function which will be made private in a future
 * version.
 *
 * Initialize a parser context
 *
 * Returns 0 in case of success and -1 in case of error
 */

int
xmlInitParserCtxt(xmlParserCtxtPtr ctxt)
{
    return(xmlInitSAXParserCtxt(ctxt, NULL, NULL));
}

/**
 * xmlFreeParserCtxt:
 * @ctxt:  an XML parser context
 *
 * Free all the memory used by a parser context. However the parsed
 * document in ctxt->myDoc is not freed.
 */

void
xmlFreeParserCtxt(xmlParserCtxtPtr ctxt)
{
    xmlParserInputPtr input;

    if (ctxt == NULL) return;

    while ((input = inputPop(ctxt)) != NULL) { /* Non consuming */
        xmlFreeInputStream(input);
    }
    if (ctxt->spaceTab != NULL) xmlFree(ctxt->spaceTab);
    if (ctxt->nameTab != NULL) xmlFree((xmlChar * *)ctxt->nameTab);
    if (ctxt->nodeTab != NULL) xmlFree(ctxt->nodeTab);
    if (ctxt->nodeInfoTab != NULL) xmlFree(ctxt->nodeInfoTab);
    if (ctxt->inputTab != NULL) xmlFree(ctxt->inputTab);
    if (ctxt->version != NULL) xmlFree((char *) ctxt->version);
    if (ctxt->encoding != NULL) xmlFree((char *) ctxt->encoding);
    if (ctxt->extSubURI != NULL) xmlFree((char *) ctxt->extSubURI);
    if (ctxt->extSubSystem != NULL) xmlFree((char *) ctxt->extSubSystem);
#ifdef LIBXML_SAX1_ENABLED
    if ((ctxt->sax != NULL) &&
        (ctxt->sax != (xmlSAXHandlerPtr) &xmlDefaultSAXHandler))
#else
    if (ctxt->sax != NULL)
#endif /* LIBXML_SAX1_ENABLED */
        xmlFree(ctxt->sax);
    if (ctxt->directory != NULL) xmlFree((char *) ctxt->directory);
    if (ctxt->vctxt.nodeTab != NULL) xmlFree(ctxt->vctxt.nodeTab);
    if (ctxt->atts != NULL) xmlFree((xmlChar * *)ctxt->atts);
    if (ctxt->dict != NULL) xmlDictFree(ctxt->dict);
    if (ctxt->nsTab != NULL) xmlFree(ctxt->nsTab);
    if (ctxt->nsdb != NULL) xmlParserNsFree(ctxt->nsdb);
    if (ctxt->attrHash != NULL) xmlFree(ctxt->attrHash);
    if (ctxt->pushTab != NULL) xmlFree(ctxt->pushTab);
    if (ctxt->attallocs != NULL) xmlFree(ctxt->attallocs);
    if (ctxt->attsDefault != NULL)
        xmlHashFree(ctxt->attsDefault, xmlHashDefaultDeallocator);
    if (ctxt->attsSpecial != NULL)
        xmlHashFree(ctxt->attsSpecial, NULL);
    if (ctxt->freeElems != NULL) {
        xmlNodePtr cur, next;

	cur = ctxt->freeElems;
	while (cur != NULL) {
	    next = cur->next;
	    xmlFree(cur);
	    cur = next;
	}
    }
    if (ctxt->freeAttrs != NULL) {
        xmlAttrPtr cur, next;

	cur = ctxt->freeAttrs;
	while (cur != NULL) {
	    next = cur->next;
	    xmlFree(cur);
	    cur = next;
	}
    }
    /*
     * cleanup the error strings
     */
    if (ctxt->lastError.message != NULL)
        xmlFree(ctxt->lastError.message);
    if (ctxt->lastError.file != NULL)
        xmlFree(ctxt->lastError.file);
    if (ctxt->lastError.str1 != NULL)
        xmlFree(ctxt->lastError.str1);
    if (ctxt->lastError.str2 != NULL)
        xmlFree(ctxt->lastError.str2);
    if (ctxt->lastError.str3 != NULL)
        xmlFree(ctxt->lastError.str3);

#ifdef LIBXML_CATALOG_ENABLED
    if (ctxt->catalogs != NULL)
	xmlCatalogFreeLocal(ctxt->catalogs);
#endif
    xmlFree(ctxt);
}

/**
 * xmlNewParserCtxt:
 *
 * Allocate and initialize a new parser context.
 *
 * Returns the xmlParserCtxtPtr or NULL
 */

xmlParserCtxtPtr
xmlNewParserCtxt(void)
{
    return(xmlNewSAXParserCtxt(NULL, NULL));
}

/**
 * xmlNewSAXParserCtxt:
 * @sax:  SAX handler
 * @userData:  user data
 *
 * Allocate and initialize a new SAX parser context. If userData is NULL,
 * the parser context will be passed as user data.
 *
 * Returns the xmlParserCtxtPtr or NULL if memory allocation failed.
 */

xmlParserCtxtPtr
xmlNewSAXParserCtxt(const xmlSAXHandler *sax, void *userData)
{
    xmlParserCtxtPtr ctxt;

    ctxt = (xmlParserCtxtPtr) xmlMalloc(sizeof(xmlParserCtxt));
    if (ctxt == NULL)
	return(NULL);
    memset(ctxt, 0, sizeof(xmlParserCtxt));
    if (xmlInitSAXParserCtxt(ctxt, sax, userData) < 0) {
        xmlFreeParserCtxt(ctxt);
	return(NULL);
    }
    return(ctxt);
}

/************************************************************************
 *									*
 *		Handling of node information				*
 *									*
 ************************************************************************/

/**
 * xmlClearParserCtxt:
 * @ctxt:  an XML parser context
 *
 * Clear (release owned resources) and reinitialize a parser context
 */

void
xmlClearParserCtxt(xmlParserCtxtPtr ctxt)
{
  if (ctxt==NULL)
    return;
  xmlClearNodeInfoSeq(&ctxt->node_seq);
  xmlCtxtReset(ctxt);
}


/**
 * xmlParserFindNodeInfo:
 * @ctx:  an XML parser context
 * @node:  an XML node within the tree
 *
 * DEPRECATED: Don't use.
 *
 * Find the parser node info struct for a given node
 *
 * Returns an xmlParserNodeInfo block pointer or NULL
 */
const xmlParserNodeInfo *
xmlParserFindNodeInfo(xmlParserCtxtPtr ctx, xmlNodePtr node)
{
    unsigned long pos;

    if ((ctx == NULL) || (node == NULL))
        return (NULL);
    /* Find position where node should be at */
    pos = xmlParserFindNodeInfoIndex(&ctx->node_seq, node);
    if (pos < ctx->node_seq.length
        && ctx->node_seq.buffer[pos].node == node)
        return &ctx->node_seq.buffer[pos];
    else
        return NULL;
}


/**
 * xmlInitNodeInfoSeq:
 * @seq:  a node info sequence pointer
 *
 * DEPRECATED: Don't use.
 *
 * -- Initialize (set to initial state) node info sequence
 */
void
xmlInitNodeInfoSeq(xmlParserNodeInfoSeqPtr seq)
{
    if (seq == NULL)
        return;
    seq->length = 0;
    seq->maximum = 0;
    seq->buffer = NULL;
}

/**
 * xmlClearNodeInfoSeq:
 * @seq:  a node info sequence pointer
 *
 * DEPRECATED: Don't use.
 *
 * -- Clear (release memory and reinitialize) node
 *   info sequence
 */
void
xmlClearNodeInfoSeq(xmlParserNodeInfoSeqPtr seq)
{
    if (seq == NULL)
        return;
    if (seq->buffer != NULL)
        xmlFree(seq->buffer);
    xmlInitNodeInfoSeq(seq);
}

/**
 * xmlParserFindNodeInfoIndex:
 * @seq:  a node info sequence pointer
 * @node:  an XML node pointer
 *
 * DEPRECATED: Don't use.
 *
 * xmlParserFindNodeInfoIndex : Find the index that the info record for
 *   the given node is or should be at in a sorted sequence
 *
 * Returns a long indicating the position of the record
 */
unsigned long
xmlParserFindNodeInfoIndex(xmlParserNodeInfoSeqPtr seq,
                           xmlNodePtr node)
{
    unsigned long upper, lower, middle;
    int found = 0;

    if ((seq == NULL) || (node == NULL))
        return ((unsigned long) -1);

    /* Do a binary search for the key */
    lower = 1;
    upper = seq->length;
    middle = 0;
    while (lower <= upper && !found) {
        middle = lower + (upper - lower) / 2;
        if (node == seq->buffer[middle - 1].node)
            found = 1;
        else if (node < seq->buffer[middle - 1].node)
            upper = middle - 1;
        else
            lower = middle + 1;
    }

    /* Return position */
    if (middle == 0 || seq->buffer[middle - 1].node < node)
        return middle;
    else
        return middle - 1;
}


/**
 * xmlParserAddNodeInfo:
 * @ctxt:  an XML parser context
 * @info:  a node info sequence pointer
 *
 * DEPRECATED: Don't use.
 *
 * Insert node info record into the sorted sequence
 */
void
xmlParserAddNodeInfo(xmlParserCtxtPtr ctxt,
                     xmlParserNodeInfoPtr info)
{
    unsigned long pos;

    if ((ctxt == NULL) || (info == NULL)) return;

    /* Find pos and check to see if node is already in the sequence */
    pos = xmlParserFindNodeInfoIndex(&ctxt->node_seq, (xmlNodePtr)
                                     info->node);

    if ((pos < ctxt->node_seq.length) &&
        (ctxt->node_seq.buffer != NULL) &&
        (ctxt->node_seq.buffer[pos].node == info->node)) {
        ctxt->node_seq.buffer[pos] = *info;
    }

    /* Otherwise, we need to add new node to buffer */
    else {
        if ((ctxt->node_seq.length + 1 > ctxt->node_seq.maximum) ||
	    (ctxt->node_seq.buffer == NULL)) {
            xmlParserNodeInfo *tmp_buffer;
            unsigned int byte_size;

            if (ctxt->node_seq.maximum == 0)
                ctxt->node_seq.maximum = 2;
            byte_size = (sizeof(*ctxt->node_seq.buffer) *
			(2 * ctxt->node_seq.maximum));

            if (ctxt->node_seq.buffer == NULL)
                tmp_buffer = (xmlParserNodeInfo *) xmlMalloc(byte_size);
            else
                tmp_buffer =
                    (xmlParserNodeInfo *) xmlRealloc(ctxt->node_seq.buffer,
                                                     byte_size);

            if (tmp_buffer == NULL) {
		xmlCtxtErrMemory(ctxt);
                return;
            }
            ctxt->node_seq.buffer = tmp_buffer;
            ctxt->node_seq.maximum *= 2;
        }

        /* If position is not at end, move elements out of the way */
        if (pos != ctxt->node_seq.length) {
            unsigned long i;

            for (i = ctxt->node_seq.length; i > pos; i--)
                ctxt->node_seq.buffer[i] = ctxt->node_seq.buffer[i - 1];
        }

        /* Copy element and increase length */
        ctxt->node_seq.buffer[pos] = *info;
        ctxt->node_seq.length++;
    }
}

/************************************************************************
 *									*
 *		Defaults settings					*
 *									*
 ************************************************************************/
/**
 * xmlPedanticParserDefault:
 * @val:  int 0 or 1
 *
 * DEPRECATED: Use the modern options API with XML_PARSE_PEDANTIC.
 *
 * Set and return the previous value for enabling pedantic warnings.
 *
 * Returns the last value for 0 for no substitution, 1 for substitution.
 */

int
xmlPedanticParserDefault(int val) {
    int old = xmlPedanticParserDefaultValue;

    xmlPedanticParserDefaultValue = val;
    return(old);
}

/**
 * xmlLineNumbersDefault:
 * @val:  int 0 or 1
 *
 * DEPRECATED: The modern options API always enables line numbers.
 *
 * Set and return the previous value for enabling line numbers in elements
 * contents. This may break on old application and is turned off by default.
 *
 * Returns the last value for 0 for no substitution, 1 for substitution.
 */

int
xmlLineNumbersDefault(int val) {
    int old = xmlLineNumbersDefaultValue;

    xmlLineNumbersDefaultValue = val;
    return(old);
}

/**
 * xmlSubstituteEntitiesDefault:
 * @val:  int 0 or 1
 *
 * DEPRECATED: Use the modern options API with XML_PARSE_NOENT.
 *
 * Set and return the previous value for default entity support.
 * Initially the parser always keep entity references instead of substituting
 * entity values in the output. This function has to be used to change the
 * default parser behavior
 * SAX::substituteEntities() has to be used for changing that on a file by
 * file basis.
 *
 * Returns the last value for 0 for no substitution, 1 for substitution.
 */

int
xmlSubstituteEntitiesDefault(int val) {
    int old = xmlSubstituteEntitiesDefaultValue;

    xmlSubstituteEntitiesDefaultValue = val;
    return(old);
}

/**
 * xmlKeepBlanksDefault:
 * @val:  int 0 or 1
 *
 * DEPRECATED: Use the modern options API with XML_PARSE_NOBLANKS.
 *
 * Set and return the previous value for default blanks text nodes support.
 * The 1.x version of the parser used an heuristic to try to detect
 * ignorable white spaces. As a result the SAX callback was generating
 * xmlSAX2IgnorableWhitespace() callbacks instead of characters() one, and when
 * using the DOM output text nodes containing those blanks were not generated.
 * The 2.x and later version will switch to the XML standard way and
 * ignorableWhitespace() are only generated when running the parser in
 * validating mode and when the current element doesn't allow CDATA or
 * mixed content.
 * This function is provided as a way to force the standard behavior
 * on 1.X libs and to switch back to the old mode for compatibility when
 * running 1.X client code on 2.X . Upgrade of 1.X code should be done
 * by using xmlIsBlankNode() commodity function to detect the "empty"
 * nodes generated.
 * This value also affect autogeneration of indentation when saving code
 * if blanks sections are kept, indentation is not generated.
 *
 * Returns the last value for 0 for no substitution, 1 for substitution.
 */

int
xmlKeepBlanksDefault(int val) {
    int old = xmlKeepBlanksDefaultValue;

    xmlKeepBlanksDefaultValue = val;
#ifdef LIBXML_OUTPUT_ENABLED
    if (!val)
        xmlIndentTreeOutput = 1;
#endif
    return(old);
}

