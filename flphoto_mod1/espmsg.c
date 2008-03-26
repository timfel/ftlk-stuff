/*
 * "$Id: espmsg.c 321 2005-01-23 03:52:44Z easysw $"
 *
 * ESP internationalization utility.
 *
 * Copyright 1997-2005 by Easy Software Products.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contents:
 *
 *   main()               - Compile/merge/prune/translate message files.
 *   add_message()        - Add a message.
 *   compare_messages()   - Compare two messages...
 *   compile_messages()   - Compile the message catalog for a language.
 *   find_message()       - Find a message.
 *   load_messages()      - Load messages from the named file.
 *   prune_messages()     - Prune any messages that are no longer used.
 *   save_messages()      - Save messages to the named file.
 *   scan_file()          - Scan for message strings.
 *   translate_messages() - Translate messages using Google.
 */

/*
 * Include necessary headers...
 */

#include "espmsg.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <io.h>
#else
#  include <unistd.h>
#endif /* WIN32 && !__CYGWIN__ */
#include <ctype.h>
#include <string.h>
#include <errno.h>

/*
 * See if we have CUPS installed; if so, we can enable the Google
 * translation stuff...
 */

#include "config.h"
#ifdef HAVE_LIBCUPS
#  include <cups/http.h>
#endif /* HAVE_LIBCUPS */


/*
 * Constants...
 */

#define MAX_MESSAGES	5000


/*
 * Message data structure...
 */

typedef struct
{
  char	*id;					/* msgid string */
  char	*str;					/* msgstr string */
  int	ref_count;				/* reference count */
} message_t;


/*
 * Local globals...
 */

int		num_messages;
message_t	messages[MAX_MESSAGES];


/*
 * Local functions...
 */

message_t	*add_message(const char *m);
int		compare_messages(const void *a, const void *b);
void		compile_messages(const char *filename);
message_t	*find_message(const char *m);
void		load_messages(const char *filename);

void		prune_messages(void);
void		save_messages(const char *filename);
void		scan_file(const char *filename);

#ifdef HAVE_LIBCUPS
void		translate_messages(const char *language);
#endif /* HAVE_LIBCUPS */


/*
 * 'main()' - Compile/merge/prune/translate message files.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  int	i;				/* Looping var */


  if (argc < 3 ||
      (strcmp(argv[2], "scan") && strcmp(argv[2], "prune") &&
       strcmp(argv[2], "compile") && strcmp(argv[2], "translate")))
  {
    puts("Usage:");
    puts("");
    puts("    espmsg filename.po compile");
    puts("    espmsg filename.po prune filename1 filename2 ... filenameN");
    puts("    espmsg filename.po scan filename1 filename2 ... filenameN");
    puts("    espmsg filename.po translate {de,es,fr,it,pt}");
    return (1);
  }

  load_messages(argv[1]);

  if (!strcmp(argv[2], "compile"))
  {
   /*
    * Compile the message catalog...
    */

    compile_messages(argv[1]);
  }
  else if (!strcmp(argv[2], "translate"))
  {
   /*
    * Translate using google...
    */

    if (argc != 4)
    {
      puts("Usage: espmsg filename.po translate {de,es,fr,it,pt}");
      return (1);
    }

#ifdef HAVE_LIBCUPS
    translate_messages(argv[3]);
    save_messages(argv[1]);
#else
    puts("Sorry, the translate command was not compiled into espmsg!");
    return (1);
#endif /* HAVE_LIBCUPS */
  }
  else
  {
   /*
    * Scan or prune...
    */

    for (i = 3; i < argc; i ++)
      scan_file(argv[i]);

    if (strcmp(argv[2], "prune") == 0)
      prune_messages();

    save_messages(argv[1]);
  }

  return (0);
}


/*
 * 'add_message()' - Add a message.
 */

message_t *				/* O - Pointer to message data */
add_message(const char *m)		/* I - Message text */
{
  message_t	*temp;			/* Pointer to message data */


  if ((temp = find_message(m)) != NULL)
    return (temp);

  temp = messages + num_messages;
  num_messages ++;

  memset(temp, 0, sizeof(message_t));

  temp->id = strdup(m);

  if (num_messages > 1)
  {
    qsort(messages, num_messages, sizeof(message_t), compare_messages);
    return (find_message(m));
  }
  else
    return (temp);
}


/*
 * 'compare_messages()' - Compare two messages...
 */

int					/* O - Result of comparison */
compare_messages(const void *a,		/* I - First message */
                 const void *b)		/* I - Second message */
{
  return (strcmp(((message_t *)a)->id, ((message_t *)b)->id));
}


/*
 * 'unquote()' - Unquote characters in strings...
 */

void
unquote(char       *d,			/* O - Unquoted strings */
        const char *s)			/* I - Original strings */
{
  while (*s)
  {
    if (*s == '\\')
    {
      s ++;
      if (isdigit(*s))
      {
	*d = 0;

	while (isdigit(*s))
	{
	  *d = *d * 8 + *s - '0';
	  s ++;
	}
      }
      else
      {
	if (*s == 'n')
	  *d ++ = '\n';
	else if (*s == 'r')
	  *d ++ = '\r';
	else if (*s == 't')
	  *d ++ = '\t';
	else
	  *d++ = *s;

	s ++;
      }
    }
    else
      *d++ = *s++;
  }

  *d = '\0';
}


/*
 * 'compile_messages()' - Compile the message catalog for a language.
 */

void
compile_messages(const char *filename)	/* I - Message filename */
{
  int		i;			/* Looping var */
  char		msgname[1024],		/* Message filename */
		orig[1024],		/* Original message text */
		text[1024],		/* Message text */
		*ptr;			/* Pointer into filename */
  int		num_msgs;		/* Number of messages */
  espmsg_t	*msgs;			/* Messages */
  message_t	*temp;			/* Current message */


  num_msgs = 0;
  msgs     = NULL;

  for (i = num_messages, temp = messages; i > 0; i --, temp ++)
    if (temp->str && temp->str[0])
    {
      unquote(orig, temp->id);
      unquote(text, temp->str);

      if (strcmp(orig, text))
        num_msgs = espAddMessage(orig, text, num_msgs, &msgs);
    }

  if (num_msgs == 0)
  {
    printf("espmsg: No messages in \"%s\" to compile!\n", filename);
    return;
  }

  strcpy(msgname, filename);
  if ((ptr = strrchr(msgname, '.')) != NULL)
    *ptr = '\0';

  if (espSaveMessages(msgname, num_msgs, msgs))
    printf("espmsg: Unable to save messages into \"%s\" - %s\n", msgname,
           strerror(errno));
  else
    printf("espmsg: Compiled %d messages into \"%s\".\n", num_msgs, msgname);
}


/*
 * 'find_message()' - Find a message.
 */

message_t *				/* O - Message data */
find_message(const char *m)		/* I - Message string */
{
  message_t	key;			/* Message key */


  if (num_messages == 0)
    return (NULL);
  else if (num_messages == 1)
  {
    if (strcmp(m, messages[0].id) == 0)
      return (messages);
    else
      return (NULL);
  }

  key.id = (char *)m;

  return ((message_t *)bsearch(&key, messages, num_messages, sizeof(message_t), compare_messages));
}


/*
 * 'load_messages()' - Load messages from the named file.
 */

void
load_messages(const char *filename)	/* I - Message file */
{
  FILE		*fp;			/* Message file */
  message_t	*temp;			/* Current message */
  char		s[4096],		/* String buffer */
		*ptr;			/* Pointer into buffer */
  int		line;			/* Current line */
  int		length;			/* Length of combined strings */


  if ((fp = fopen(filename, "r")) == NULL)
    return;

  printf("espmsg: Loading \"%s\"...", filename);
  fflush(stdout);

  temp = NULL;
  line = 0;

  while (fgets(s, sizeof(s), fp) != NULL)
  {
    line ++;

   /*
    * Skip comment lines...
    */

    if (s[0] == '#')
      continue;

   /*
    * Strip trailing newline and quote...
    */

    for (ptr = s + strlen(s) - 1; ptr >= s && isspace(*ptr); *ptr-- = '\0');

    if (ptr < s)
      continue;

    if (*ptr != '\"')
    {
      printf("espmsg: Expected quoted string on line %d!\n", line);
      continue;
    }

    *ptr = '\0';

   /*
    * Find start of value...
    */
    
    if ((ptr = strchr(s, '\"')) == NULL)
    {
      printf("espmsg: Expected quoted string on line %d!\n", line);
      continue;
    }

    ptr ++;

    if (!strncmp(s, "msgid", 5))
      temp = add_message(ptr);
    else if (s[0] != '\"' && strncmp(s, "msgstr", 6))
    {
      printf("espmsg: Unexpected text on line %d!\n", line);
      continue;
    }
    else if (!temp)
    {
      printf("espmsg: Need a msgid line before any translation strings "
             "on line %d!\n", line);
      continue;
    }
    else if (temp->str)
    {
     /*
      * Append the string...
      */

      length    = strlen(temp->str) + strlen(ptr) + 1;
      temp->str = realloc(temp->str, length);

      strcat(temp->str, ptr);
    }
    else
    {
     /*
      * Set the string...
      */

      temp->str = strdup(ptr);
    }
  }

  fclose(fp);

  printf(" %d messages loaded.\n", num_messages);
}


/*
 * 'prune_messages()' - Prune any messages that are no longer used.
 */

void
prune_messages(void)
{
  int		i;		/* Looping var */
  message_t	*temp;		/* Current message */
  int		orig_messages;	/* Original number of messages */


  printf("espmsg: Pruning out unused messages...");
  fflush(stdout);

  orig_messages = num_messages;

  for (i = num_messages, temp = messages; i > 0; i --, temp ++)
    if (temp->ref_count == 0 && temp->id[0])
    {
      free(temp->id);

      if (temp->str)
        free(temp->str);

      if (i > 1)
        memcpy(temp, temp + 1, (i - 1) * sizeof(message_t));

      temp --;
      num_messages --;
    }

  if (orig_messages > num_messages)
    printf(" %d old messages removed.\n", orig_messages - num_messages);
  else
    puts(" no old messages to remove.");
}


/*
 * 'save_messages()' - Save messages to the named file.
 */

void
save_messages(const char *filename)	/* I - Message file */
{
  FILE		*fp;			/* Message file */
  char		backup[1024];		/* Backup file */
  int		i;			/* Looping var */
  message_t	*temp;			/* Current message */


  sprintf(backup, "%s.bck", filename);
  unlink(backup);
  rename(filename, backup);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    rename(backup, filename);
    perror(filename);
    return;
  }

  printf("espmsg: Saving \"%s\"...", filename);
  fflush(stdout);

  for (temp = messages, i = num_messages; i > 0; i --, temp ++)
  {
    fprintf(fp, "msgid \"%s\"\n", temp->id);
    fprintf(fp, "msgstr \"%s\"\n", temp->str ? temp->str : "");

    putc('\n', fp);
  }

  fclose(fp);
  printf(" %d messages.\n", num_messages);
}


/*
 * 'scan_file()' - Scan for message strings.
 */

void
scan_file(const char *filename)		/* I - File to scan */
{
  FILE		*fp;			/* File */
  int		ch, lastch;		/* Current and previous characters */
  char		s[4096],		/* String buffer */
		*ptr;			/* Pointer into buffer */
  int		count,			/* Number of messages */
		orig_messages;		/* Original number of messages */
  message_t	*temp;			/* Current message */


  if ((fp = fopen(filename, "r")) == NULL)
  {
    perror(filename);
    return;
  }

  printf("Scanning \"%s\"...", filename);
  fflush(stdout);

  lastch        = 0;
  count         = 0;
  orig_messages = num_messages;

  while ((ch = getc(fp)) != EOF)
  {
    switch (ch)
    {
      case '/' :
          if (lastch == '/')
          {
            while ((ch = getc(fp)) != EOF)
              if (ch == '\n')
                break;
          }
          break;

      case '*' :
          if (lastch == '/')
          {
            lastch = ch;
            while ((ch = getc(fp)) != EOF)
            {
              if (ch == '/' && lastch == '*')
                break;

              lastch = ch;
            }
          }
          break;

      case '(' :
          if (lastch == '_')
	  {
            if ((ch = getc(fp)) == '\"')
            {
	      ptr = s;

	      do
	      {
        	while ((ch = getc(fp)) != EOF)
        	{
                  if (ch == '\"')
                    break;
                  else if (ch == '\\')
                  {
		    lastch = ch;
		    ch     = getc(fp);

		    if (ch == '\n')
		      continue;

                    if (ptr < (s + sizeof(s) - 1))
                      *ptr++ = lastch;
                  }

                  if (ptr < (s + sizeof(s) - 1))
		  {
		    if (ch == '\n')
		    {
		      if (lastch != '\\')
			*ptr++ = '\\';

		      *ptr++ = 'n';
		    }
		    else
                      *ptr++ = ch;
		  }

		  lastch = ch;
        	}

               /*
	        * Handle merged strings ("string" "string")...
		*/

		if (ch == '\"')
		  while ((ch = getc(fp)) != EOF)
		    if (!isspace(ch))
		      break;
	      }
	      while (ch == '\"');

              *ptr = '\0';

             /*
	      * Skip symbol labels...
	      */

              if (s[0] == '@')
                break;

             /*
	      * Only localize strings with letters - numbers almost
	      * never need to be localized...
	      */

              for (ptr = s; *ptr; ptr ++)
                if (isalpha(*ptr))
                  break;

              if (*ptr)
                if ((temp = add_message(s)) != NULL)
		{
		 /*
		  * Added the message successfully...
		  */

                  temp->ref_count ++;
		  count ++;
		}
            }
            else
            {
              ungetc(ch, fp);
              ch = '(';
            }
	  }
          break;
    }

    lastch = ch;
  }

  fclose(fp);

  printf(" %d messages (%d new).\n", count, num_messages - orig_messages);
}


#ifdef HAVE_LIBCUPS
/*
 * 'translate_messages()' - Translate messages using Google.
 */

void
translate_messages(const char *language)/* I - Output language... */
{
 /*
  * Google provides a simple translation/language tool for translating
  * from one language to another.  It is far from perfect, however it
  * can be used to get a basic translation done or update an existing
  * translation when no other resources are available.
  *
  * Translation requests are sent as HTTP POSTs to
  * "http://translate.google.com/translate_t" with the following form
  * variables:
  *
  *   Name      Description                         Value
  *   --------  ----------------------------------  ----------------
  *   hl        Help language?                      "en"
  *   ie        Input encoding                      "UTF8"
  *   langpair  Language pair                       "en|" + language
  *   oe        Output encoding                     "UTF8"
  *   text      Text to translate                   translation string
  */

  int		i;			/* Looping var */
  message_t     *m;			/* Current message */
  int		tries;			/* Number of tries... */
  http_t	*http;			/* HTTP connection */
  http_status_t	status;			/* Status of POST request */
  char		*idptr,			/* Pointer into msgid */
		*strptr,		/* Pointer into msgstr */
		buffer[65536],		/* Input/output buffer */
		*bufptr,		/* Pointer into buffer */
		*bufend,		/* Pointer to end of buffer */
		length[16];		/* Content length */
  unsigned char	*utf8;			/* UTF-8 string */
  int		ch;			/* Current decoded character */
  int		bytes;			/* Number of bytes read */


 /*
  * Connect to translate.google.com...
  */

  puts("Connecting to translate.google.com...");

  if ((http = httpConnect("translate.google.com", 80)) == NULL)
  {
    perror("Unable to connect to translate.google.com");
    return;
  }

 /*
  * Scan the current messages, requesting a translation of any untranslated
  * messages...
  */

  for (i = num_messages, m = messages; i > 0; i --, m ++)
  {
   /*
    * Skip messages that are already translated...
    */

    if (m->str && m->str[0])
      continue;

   /*
    * Encode the form data into the buffer...
    */

    snprintf(buffer, sizeof(buffer),
             "hl=en&ie=UTF8&langpair=en|%s&oe=UTF8&text=", language);
    bufptr = buffer + strlen(buffer);
    bufend = buffer + sizeof(buffer) - 5;

    for (idptr = m->id; *idptr && bufptr < bufend; idptr ++)
      if (*idptr == ' ')
        *bufptr++ = '+';
      else if (*idptr < ' ' || *idptr == '%')
      {
        sprintf(bufptr, "%%%02X", *idptr & 255);
	bufptr += 3;
      }
      else if (*idptr != '&')
        *bufptr++ = *idptr;

    *bufptr++ = '&';
    *bufptr = '\0';

    sprintf(length, "%d", bufptr - buffer);

   /*
    * Send the request...
    */

    printf("\"%s\" = ", m->id);
    fflush(stdout);

    tries = 0;

    do
    {
      httpClearFields(http);
      httpSetField(http, HTTP_FIELD_CONTENT_TYPE,
                   "application/x-www-form-urlencoded");
      httpSetField(http, HTTP_FIELD_CONTENT_LENGTH, length);

      if (httpPost(http, "/translate_t"))
      {
	httpReconnect(http);
	httpPost(http, "/translate_t");
      }

      httpWrite(http, buffer, bufptr - buffer);

      while ((status = httpUpdate(http)) == HTTP_CONTINUE);

      if (status != HTTP_OK && status != HTTP_ERROR)
        httpFlush(http);

      tries ++;
    }
    while (status == HTTP_ERROR && tries < 10);

    if (status == HTTP_OK)
    {
     /*
      * OK, read the translation back...
      */

      bufptr = buffer;
      bufend = buffer + sizeof(buffer) - 1;

      while ((bytes = httpRead(http, bufptr, bufend - bufptr)) > 0)
        bufptr += bytes;

      if (bytes < 0)
      {
       /*
        * Read error, abort!
	*/

        puts("READ ERROR!");
	break;
      }

      *bufptr = '\0';

     /*
      * Find the first textarea element - that will have the translation data...
      */

      if ((bufptr = strstr(buffer, "<textarea")) == NULL)
      {
       /*
        * No textarea, abort!
	*/

        puts("NO TEXTAREA!");
	break;
      }

      if ((bufptr = strchr(bufptr, '>')) == NULL)
      {
       /*
        * textarea doesn't end, abort!
	*/

        puts("TEXTAREA SHORT DATA!");
	break;
      }

      utf8 = (unsigned char *)bufptr + 1;

      if ((bufend = strstr(bufptr, "</textarea>")) == NULL)
      {
       /*
        * textarea doesn't close, abort!
	*/

        puts("/TEXTAREA SHORT DATA!");
	break;
      }

      *bufend = '\0';

     /*
      * Copy the UTF-8 translation to ISO-8859-1 (for now)...
      */

      m->str = malloc(bufend - bufptr);

      for (strptr = m->str; *utf8;)
        if (*utf8 < 0x80)
	  *strptr++ = *utf8++;
	else
	{
	  if ((*utf8 & 0xe0) == 0xc0)
	  {
	   /*
	    * Two-byte encoding...
	    */

            ch   = ((utf8[0] & 0x1f) << 6) | (utf8[1] & 0x3f);
	    utf8 += 2;
	  }
	  else if ((ch & 0xf0) == 0xe0)
	  {
	   /*
	    * Three-byte encoding...
	    */

            ch   = ((((utf8[0] & 0x0f) << 6) | (utf8[1] & 0x3f)) << 6) |
	           (utf8[2] & 0x3f);
	    utf8 += 3;
	  }

          if (ch < 256)			/* ISO-8859-1 */
	    *strptr++ = ch;
	  else if (ch == 0x20ac)	/* Euro */
	    *strptr++ = 0xA4;		/* ISO-8859-15 mapping */
	}

      *strptr = '\0';

      printf("\"%s\"\n", m->str);
    }
    else if (status == HTTP_ERROR)
    {
      printf("NETWORK ERROR (%s)!\n", strerror(httpError(http)));
      break;
    }
    else
    {
      printf("HTTP ERROR %d!\n", status);
      break;
    }
  }

  httpClose(http);
}
#endif /* HAVE_LIBCUPS */


/*
 * End of "$Id: espmsg.c 321 2005-01-23 03:52:44Z easysw $".
 */
