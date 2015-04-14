/**
 * @file example3.c
 *
 */

/**
 * @mainpage MIHL: Minimal Httpd Library - Example #3
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
int http_root( mihl_cnx_t *cnx, char const *tag, char const *host, void *param ) {
    mihl_add( cnx, "<!DOCTYPE html PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>" );
    mihl_add( cnx, "<html>" );
    mihl_add( cnx, "<title>MIHL - Example 3</title>" );
    mihl_add( cnx, "<head>" );
    mihl_add( cnx, "  <meta content='text/html; charset=ISO-8859-1'" );
    mihl_add( cnx, " http-equiv='content-type'>" );
    mihl_add( cnx, "  <title>blah blah</title>" );
    mihl_add( cnx, "</head>" );
    mihl_add( cnx, "<body style='color: rgb(0, 0, 0); background-color: rgb(204, 204, 204);'" );
    mihl_add( cnx, " alink='#000099' link='#000099' vlink='#990099'>" );
    mihl_add( cnx, "<div style='text-align: center; font-weight: bold;'><big>Test AJAX" );
    mihl_add( cnx, "(prototype.js)<br>" );
    mihl_add( cnx, "</big></div>" );
    mihl_add( cnx, "<br>" );
    mihl_add( cnx, "<div style='text-align: center;'><img style='width: 70px; height: 72px;'" );
    mihl_add( cnx, " alt='' src='/image.jpg'><br>" );
    mihl_add( cnx, "</div>" );
    mihl_add( cnx, "<br>" );
    mihl_add( cnx, "<table" );
    mihl_add( cnx, " style='text-align: left; background-color: rgb(208, 208, 208); margin-left: auto; margin-right: auto;'" );
    mihl_add( cnx, " border='1' cellpadding='2' cellspacing='2'>" );
    mihl_add( cnx, "  <tbody>" );
    mihl_add( cnx, "    <tr>" );
    mihl_add( cnx, "      <td style='vertical-align: top;'>Refreshed each second<br>" );
    mihl_add( cnx, "      </td>" );
    mihl_add( cnx, "      <td style='vertical-align: top;' id='field1'>XXXX<br>" );
    mihl_add( cnx, "      </td>" );
    mihl_add( cnx, "    </tr>" );
    mihl_add( cnx, "    <tr>" );
    mihl_add( cnx, "      <td style='vertical-align: top;'>Refreshed each 5 seconds<br>" );
    mihl_add( cnx, "      </td>" );
    mihl_add( cnx, "      <td style='vertical-align: top;' id='field2'>XXXX<br>" );
    mihl_add( cnx, "      </td>" );
    mihl_add( cnx, "    </tr>" );
    mihl_add( cnx, "    <tr>" );
    mihl_add( cnx, "      <td style='vertical-align: top;'>Refreshed each 30 seconds<br>" );
    mihl_add( cnx, "      </td>" );
    mihl_add( cnx, "      <td style='vertical-align: top;' id='field3'>XXXX<br>" );
    mihl_add( cnx, "      </td>" );
    mihl_add( cnx, "    </tr>" );
    mihl_add( cnx, "  </tbody>" );
    mihl_add( cnx, "</table>" );
    mihl_add( cnx, "<br>" );
    mihl_add( cnx, "</body>" );

    mihl_add( cnx, "<SCRIPT type='text/javascript' src='/prototype.js'> </SCRIPT>" );

    mihl_add( cnx, "<SCRIPT type='text/javascript'>" );
    mihl_add( cnx, "<!--" );

    mihl_add( cnx, "var cpt = 0;" );
    mihl_add( cnx, "function onLoadHandler( ) {" );
    mihl_add( cnx, "  setInterval( 'timerFunction1()', 1000 );" );
    mihl_add( cnx, "  setInterval( 'timerFunction5()', 5000 );" );
    mihl_add( cnx, "  setInterval( 'timerFunction30()', 30000 );" );
    mihl_add( cnx, "};" );

    mihl_add( cnx, "function timerFunction1( ) {" );
    mihl_add( cnx, "  var url = 'http://%s/data1';", host );
    mihl_add( cnx, "  var myAjax = new Ajax.Request( url, { method: 'get', onComplete: showData1 });" );
    mihl_add( cnx, "};" );

    mihl_add( cnx, "function timerFunction5( ) {" );
    mihl_add( cnx, "  var url = 'http://%s/data2';", host );
    mihl_add( cnx, "  var myAjax = new Ajax.Request( url, { method: 'get', onComplete: showData2 });" );
    mihl_add( cnx, "};" );

    mihl_add( cnx, "function timerFunction30( ) {" );
    mihl_add( cnx, "  var url = 'http://%s/data3';", host );
    mihl_add( cnx, "  var myAjax = new Ajax.Request( url, { method: 'get', onComplete: showData3 });" );
    mihl_add( cnx, "};" );

    mihl_add( cnx, "function showData1( originalRequest ) {" );
    mihl_add( cnx, "  document.getElementById('field1').childNodes[0].nodeValue = originalRequest.responseText;" );
//  mihl_add( cnx, "  field1.innerText = originalRequest.responseText;" );
    mihl_add( cnx, "};" );

    mihl_add( cnx, "function showData2( originalRequest ) {" );
    mihl_add( cnx, "  document.getElementById('field2').childNodes[0].nodeValue = originalRequest.responseText;" );
//  mihl_add( cnx, "  field2.innerText = originalRequest.responseText;" );
    mihl_add( cnx, "};" );

    mihl_add( cnx, "function showData3( originalRequest ) {" );
    mihl_add( cnx, "  document.getElementById('field3').childNodes[0].nodeValue = originalRequest.responseText;" );
//  mihl_add( cnx, "  field3.innerText = originalRequest.responseText;" );
    mihl_add( cnx, "};" );

    mihl_add( cnx, "-->" );
    mihl_add( cnx, "</SCRIPT>" );

    mihl_add( cnx, "<BODY onLoad='onLoadHandler();'>" );
    mihl_add( cnx, "</BODY>" );

    mihl_add( cnx, "</html>" );
    mihl_send( cnx, NULL,
		"Content-type: text/html\r\n" );
    return 0;
}                               // http_root

/**
 * TBD
 * 
 * @param cnx opaque context structure as returned by mihl_init()
 * @param tag TBD
 * @param host TBD
 * @param param TBD
 * @return 0
 */
int http_data( mihl_cnx_t *cnx, char const *tag, char const *host, void *param ) {
    int index = *(int *)param;
    static int cpts[3] = { 0, 0, 0 };
    mihl_add( cnx, "cpt=%d", cpts[index]++ );
    mihl_send( cnx, NULL,
		"Content-type: text/xml\r\n" );
    return 0;
}                               // http_data

/**
 * Program entry point
 * 
 * @param argc Number of arguments
 * @param argv Arguments given on the command line
 * @return
 * 	- 0 if OK
 * 	- or -1 if an error occurred (errno is then set).
 */
int main( int argc, char *argv[] ) {

	help( 8080 );

    mihl_ctx_t *ctx = mihl_init( NULL, 8080, 8, 
        MIHL_LOG_ERROR | MIHL_LOG_WARNING | MIHL_LOG_INFO | MIHL_LOG_INFO_VERBOSE );
    if ( !ctx )
    	return -1;

    mihl_handle_get( ctx, "/", http_root, NULL );
    int *data0 = (int *)malloc(sizeof(int));
    *data0 = 0;
    mihl_handle_get( ctx, "/data1", http_data, data0 );
    int *data1 = (int *)malloc(sizeof(int));
    *data1 = 1;
    mihl_handle_get( ctx, "/data2", http_data, data1 );
    int *data2 = (int *)malloc(sizeof(int));
    *data2 = 2;
    mihl_handle_get( ctx, "/data3", http_data, data2 );
    if ( access( "../image.jpg", R_OK ) == 0 ) {
    	mihl_handle_file( ctx, "/image.jpg", "../image.jpg", "image/jpeg", 0 );
        mihl_handle_file( ctx, "/prototype.js", "../prototype.js", "text/javascript", 0 );
    }
    else {
    	mihl_handle_file( ctx, "/image.jpg", "/etc/mihl/examples/3/image.jpg", "image/jpeg", 0 );
        mihl_handle_file( ctx, "/prototype.js", "/etc/mihl/examples/3/prototype.js", "text/javascript", 0 );
    }

    for (;;) {
        int status = mihl_server( ctx );
        if ( status == -2 )
            break;
        if ( peek_key( ctx ) )
            break;
    }
    
    return 0;
}                               // main
