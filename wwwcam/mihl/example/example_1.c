/**
 * @file example1.c
 *
 */

/**
 * @mainpage MIHL: Minimal Httpd Library - Example #1
 * 
 * HTTP embedded server library \n
 * Copyright (C) 2006-2007  Olivier Singla \n
 * http://mihl.sourceforge.net/ \n\n
 *
 * Simple use of GET.
 * Display a page with a JPEG image and a link to a 2nd page. 
 * The 2nd page contains a link to the 1st page.
 * There is also a link to a non existent page.
 * The 2nd page install a user-provided handler to manage non existent page.
 * This example show also a simple use of a Basic Authentication.
 * 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "mihl.h"

#include "example_utils.h"

/**
 * GET Handler for the URL: /
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag TBD
 * @param host TBD
 * @param param User-provided parameter, was provided with mihl_handle_get 
 * @return 0
 */
static int http_root(mihl_cnx_t * cnx, char const *tag, char const *host,
		     void *param)
{
	mihl_add(cnx, "<html>");
	mihl_add(cnx, "<title>MIHL - Example 1</title>");
	mihl_add(cnx, "<body>");
	mihl_add(cnx, "This is a test HTML page for MIHL.<br>");
	mihl_add(cnx, "<br>Here is a JPEG Image:<br>");
	mihl_add(cnx,
		 "<img style='width: 70px; height: 72px;' alt='' src='image.jpg'><br><br>");
	mihl_add(cnx, "host= [%s]<br><br>", host);
	mihl_add(cnx, "<a href='nextpage.html'>Next Page<a><br><br>");
	mihl_add(cnx, "<a href='unknown.html'>Non-Existent Page<a><br><br>");
	if (access("../image.jpg", R_OK) == 0) {
		mihl_add(cnx,
			 "(Do <b>wget -r http://mihl.sourceforge.net</b> in the directory where the executable is)<br>");
		mihl_add(cnx, "<a href='index.html'>Static Pages<a><br><br>");
	}
	mihl_add(cnx, "<a href='protected.html'>Protected Page<a><br>");
	mihl_add(cnx, "   (username=John, password=Smith)<br><br>");
	mihl_add(cnx, "</body>");
	mihl_add(cnx, "</html>");
	mihl_send(cnx, NULL, "Content-type: text/html\r\n");
	return 0;
}				// http_root

/**
 * User-provided handler to manage non existent page.
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag URL of the non existent page
 * @param host TBD
 * @param param User-provided parameter, was provided with mihl_handle_get 
 * @return TBD
 */
static int http_page_not_found(mihl_cnx_t * cnx, char const *tag,
			       char const *host, void *param)
{
	mihl_add(cnx, "<html>");
	mihl_add(cnx, "<head>");
	mihl_add(cnx, "</head>");
	mihl_add(cnx, "<br>");
	mihl_add(cnx, " THIS PAGE IS NOT FOUND ");
	mihl_add(cnx, "<br>");
	mihl_add(cnx, "The tag is [%s]", tag);
	mihl_add(cnx, "<br>");
	mihl_add(cnx, "</body>");
	mihl_add(cnx, "</html>");
	return mihl_send(cnx, NULL, "Content-type: text/html\r\n");
}				// http_page_not_found

/**
 * TBD
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag TBD
 * @param host TBD
 * @param param User-provided parameter, was provided with mihl_handle_get 
 * @return 0
 */
int http_nextpage2(mihl_cnx_t * cnx, char const *tag, char const *host,
		   void *param)
{

	mihl_add(cnx, "<html>");
	mihl_add(cnx, "<body>");
	mihl_add(cnx, "This is another page, again... TWO ...<br>");
	mihl_add(cnx, "<a href='/'>Previous Page<a><br><br>");
	mihl_add(cnx,
		 "<a href='another_unknown.html'>Non-Existent Page<a><br><br>");
	mihl_add(cnx, "</body>");
	mihl_add(cnx, "</html>");
	mihl_send(cnx, NULL, "Content-type: text/html\r\n");
	return 0;
}				// http_nextpage2

/**
 * TBD
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag TBD
 * @param host TBD
 * @param param User-provided parameter, was provided with mihl_handle_get 
 * @return 0
 */
int http_nextpage1(mihl_cnx_t * cnx, char const *tag, char const *host,
		   void *param)
{

	// Install a handler for non existent pages
	mihl_handle_get(mihl_get_ctx(cnx), NULL, http_page_not_found, NULL);

	// Change one handler 
	mihl_handle_get(mihl_get_ctx(cnx), "/nextpage.html", http_nextpage2,
			NULL);

	mihl_add(cnx, "<html>");
	mihl_add(cnx, "<body>");
	mihl_add(cnx, "This is another page... ONE ...<br>");
	mihl_add(cnx, "<a href='/'>Previous Page<a><br><br>");
	mihl_add(cnx,
		 "<a href='another_unknown.html'>Non-Existent Page<a><br><br>");
	mihl_add(cnx, "</body>");
	mihl_add(cnx, "</html>");
	mihl_send(cnx, NULL, "Content-type: text/html\r\n");
	return 0;
}				// http_nextpage1

/**
 * User-provided handler to manage non existent page.
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag URL of the non existent page
 * @param host TBD
 * @param param User-provided parameter, was provided with mihl_handle_get 
 * @return TBD
 */
static int http_index_all_pages(mihl_cnx_t * cnx, char const *tag,
				char const *host, void *param)
{
	int l = strlen(tag);

	if ((l < 2) || (tag[0] != '/')) {
		return -1;
	}
	if ((l > 4) && !strcmp(&tag[l - 4], ".gif"))
		return send_file(cnx, tag, &tag[1], "image/gif", 0);
	else if ((l > 4) && !strcmp(&tag[l - 4], ".jpg"))
		return send_file(cnx, tag, &tag[1], "image/jpeg", 0);
	printf("-- %s\r\n", tag);
	fflush(stdout);
	return 0;
}				// http_index_all_pages

/**
 * GET Handler for the URL: /index.html
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag TBD
 * @param host TBD
 * @param param User-provided parameter, was provided with mihl_handle_get 
 * @return 0
 */
int http_index(mihl_cnx_t * cnx, char const *tag, char const *host, void *param)
{
	if (chdir("mihl.sourceforge.net") == -1) {
		mihl_add(cnx, "<html>");
		mihl_add(cnx, "<title>MIHL - Example 1 - index.html</title>");
		mihl_add(cnx, "<body>");
		mihl_add(cnx, "This is a test HTML page for MIHL.<br>");
		mihl_add(cnx,
			 "<br>Unable to chdir into 'mihl.sourceforge.net': %m<br>");
		mihl_add(cnx, "</body>");
		mihl_add(cnx, "</html>");
		mihl_send(cnx, NULL, "Content-type: text/html\r\n");
	}
	// Install a handler for non existent pages
	mihl_handle_get(mihl_get_ctx(cnx), NULL, http_index_all_pages, NULL);

	int status = send_file(cnx, tag, "index.html", "text/html", 0);

	printf("status=%d\n", status);
	return 0;
}				// http_index

/**
 * GET Handler for the URL: /protected.html
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag TBD
 * @param host TBD
 * @param param User-provided parameter, was provided with mihl_handle_get 
 * @return 0
 */
static int http_protected(mihl_cnx_t * cnx, char const *tag, char const *host,
			  void *param)
{

	char const *auth = mihl_authorization(cnx);

	if (!auth) {
		mihl_add(cnx, "<html>");
		mihl_add(cnx, "<title>MIHL - Example 1</title>");
		mihl_add(cnx, "<body>");
		mihl_add(cnx, "Blah blah<br>");
		mihl_add(cnx, "</body>");
		mihl_add(cnx, "</html>");
		mihl_send(cnx, "HTTP/1.0 401 Unauthorized\r\n",
			  "WWW-Authenticate: Basic realm=\"This page is protected!\"\r\n"
			  "Content-type: text/html\r\n");
	} else {
		int len = strlen(auth);

		if ((len < 7) || strncmp(auth, "Basic ", 6)) {

		}
		char realm[80];

		mihl_base64_decode(&auth[6], strlen(auth) - 6, realm,
				   sizeof(realm) - 1);
		if (!strcmp(realm, "John:Smith")) {
			mihl_add(cnx, "<html>");
			mihl_add(cnx, "<body>");
			mihl_add(cnx,
				 "Authentication succedded! Welcome John Smith!<br>");
			mihl_add(cnx, "<a href='/'>Previous Page<a><br><br>");
			mihl_add(cnx, "</body>");
			mihl_add(cnx, "</html>");
			mihl_send(cnx, NULL, "Content-type: text/html\r\n");
		} else {
			mihl_add(cnx, "<html>");
			mihl_add(cnx, "<body>");
			mihl_add(cnx,
				 "Authentication did not succedded! You are not John Smith!<br>");
			mihl_add(cnx, "<a href='/'>Previous Page<a><br><br>");
			mihl_add(cnx, "</body>");
			mihl_add(cnx, "</html>");
			mihl_send(cnx, NULL, "Content-type: text/html\r\n");
		}
	}
	return 0;
}				// http_protected

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

	mihl_ctx_t *ctx = mihl_init(NULL, 8080, 1,
				    MIHL_LOG_DEBUG | MIHL_LOG_ERROR |
				    MIHL_LOG_WARNING | MIHL_LOG_INFO |
				    MIHL_LOG_INFO_VERBOSE);
	if (!ctx)
		return -1;

	mihl_handle_get(ctx, "/", http_root, NULL);
	if (access("../image.jpg", R_OK) == 0)
		mihl_handle_file(ctx, "/image.jpg", "../image.jpg",
				 "image/jpeg", 0);
	else
		mihl_handle_file(ctx, "/image.jpg",
				 "/etc/mihl/examples/1/image.jpg", "image/jpeg",
				 0);
	mihl_handle_get(ctx, "/nextpage.html", http_nextpage1, NULL);
	mihl_handle_get(ctx, "/index.html", http_index, NULL);
	mihl_handle_get(ctx, "/protected.html", http_protected, NULL);

	for (;;) {
		int status = mihl_server(ctx);

		if (status == -2)
			break;
		if (peek_key(ctx))
			break;
	}

	return 0;
}
