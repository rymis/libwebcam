/**
 * @file example2.c
 *
 */

/**
 * @mainpage MIHL: Minimal Httpd Library - Example #2
 * 
 * HTTP embedded server library \n
 * Copyright (C) 2006-2007  Olivier Singla \n
 * http://mihl.sourceforge.net/ \n\n
 *
 * Simple use of GET.
 * Display a page with a JPEG image and a link to a 2nd page. 
 * The 2nd page contains a link to the 1st page.
 * 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "mihl.h"

#include "example_utils.h"

/**
 * TBD
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag TBD
 * @param host TBD
 * @param param TBD
 * @return 0
 */
int http_root(mihl_cnx_t * cnx, char const *tag, char const *host, void *param)
{
	mihl_add(cnx,
		 "<!DOCTYPE html PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>");
	mihl_add(cnx, "<html>");
	mihl_add(cnx, "<title>MIHL - Example 2</title>");
	mihl_add(cnx, "  <meta content='text/html; charset=ISO-8859-1'");
	mihl_add(cnx, " http-equiv='content-type'>");
	mihl_add(cnx, "  <title>MIHL Test page</title>");
	mihl_add(cnx, "</head>");
	mihl_add(cnx, "<title>MIHL - Example 2</title>");
	mihl_add(cnx, "<body>");
	mihl_add(cnx, "This is a test HTML page for MIHL.<br>");
	mihl_add(cnx, "<br>");
	mihl_add(cnx, "Here is a JPEG Image:<br>");
	mihl_add(cnx, "<img style='width: 70px; height: 72px;' alt=''");
	mihl_add(cnx, " src='image.jpg'><br>");
	mihl_add(cnx, "<br>");
	mihl_add(cnx, "<form method='post' action='toto1'>");
	mihl_add(cnx,
		 "<table style='text-align: left; width: 100%;' border='1' cellpadding='2'");
	mihl_add(cnx, " cellspacing='2'>");
	mihl_add(cnx, "  <tbody>");
	mihl_add(cnx, "    <tr>");
	mihl_add(cnx, "      <td style='vertical-align: top;'>Name<br>");
	mihl_add(cnx, "      </td>");
	mihl_add(cnx, "      <td style='vertical-align: top;'>");
	mihl_add(cnx, "       <input name='myname1' type='text'>");
	mihl_add(cnx, "      </td>");
	mihl_add(cnx, "    </tr>");
	mihl_add(cnx, "    <tr>");
	mihl_add(cnx, "      <td style='vertical-align: top;'>City<br>");
	mihl_add(cnx, "      </td>");
	mihl_add(cnx, "      <td style='vertical-align: top;'>");
	mihl_add(cnx, "      <input name='myname2' type='text'>");
	mihl_add(cnx, "      </td>");
	mihl_add(cnx, "    </tr>");
	mihl_add(cnx, "    <tr>");
	mihl_add(cnx, "      <td style='vertical-align: top;'>Zip<br>");
	mihl_add(cnx, "      </td>");
	mihl_add(cnx, "      <td style='vertical-align: top;'>");
	mihl_add(cnx, "      <input name='myname3' type='text'>");
	mihl_add(cnx, "      </td>");
	mihl_add(cnx, "    </tr>");
	mihl_add(cnx, "  </tbody>");
	mihl_add(cnx, "</table>");
	mihl_add(cnx,
		 "Press <input type='submit' value='here'> to submit your query.");
	mihl_add(cnx, "</form>");
	mihl_add(cnx, "<br>");
	mihl_add(cnx, "</body>");
	mihl_add(cnx, "</html>");
	mihl_send(cnx, NULL, "Content-type: text/html\r\n");
	return 0;
}				// http_root

/**
 * TBD
 * 
 * myname1=AAA&myname2=BBB&myname3=CCC]
 * myname1=A+++B+C&myname2=HELLO&myname3=BONJOUR%2BMONDE]
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag TBD
 * @param host TBD
 * @param nb_variables TBD
 * @param vars_names TBD
 * @param vars_values TBD
 * @param param TBD
 * @return 0
 */
int http_root_post(mihl_cnx_t * cnx, char const *tag, char const *host,
		   int nb_variables, char **vars_names, char **vars_values,
		   void *param)
{
	int n;

	mihl_add(cnx, "<html>");
	mihl_add(cnx, "<head>");
	mihl_add(cnx, "nb_variables=%d<BR>", nb_variables);
	for (n = 0; n < nb_variables; n++)
		mihl_add(cnx, "  %2d: %s = [%s]<BR>", n, vars_names[n],
			 vars_values[n]);
	mihl_add(cnx, "</body>");
	mihl_add(cnx, "</html>");
	mihl_send(cnx, NULL, "Content-type: text/html\r\n");

	return 0;
}				// http_root_post

/**
 * Program entry point
 * 
 * @param argc Number of arguments
 * @param argv Arguments given on the command line
 * @return
 * 	- 0 if OK
 * 	- or -1 if an error occurred (errno is then set).
 */
int main(int argc, char *argv[])
{

	help(8080);

	mihl_ctx_t *ctx = mihl_init(NULL, 8080, 8,
				    MIHL_LOG_ERROR | MIHL_LOG_WARNING |
				    MIHL_LOG_INFO | MIHL_LOG_INFO_VERBOSE);
	if (!ctx)
		return -1;

	mihl_handle_get(ctx, "/", http_root, NULL);
	if (access("../image.jpg", R_OK) == 0)
		mihl_handle_file(ctx, "/image.jpg", "../image.jpg",
				 "image/jpeg", 0);
	else
		mihl_handle_file(ctx, "/image.jpg",
				 "/etc/mihl/examples/2/image.jpg", "image/jpeg",
				 0);
	mihl_handle_post(ctx, "/toto1", http_root_post, NULL);

	for (;;) {
		int status = mihl_server(ctx);

		if (status == -2)
			break;
		if (peek_key(ctx))
			break;
	}

	return 0;
}				// main
