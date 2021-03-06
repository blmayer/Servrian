#include "methods.h"
#include "aux.h"
#include "defs.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int serve_get(int conn, struct request r) {
    struct response res;

    res.server = "Servrian/" VERSION;
    res.version = 1.1;
    res.status = 200;
    res.path = r.url;

    /* If / was passed, redirect to index page */
    if (strlen(res.path) == 1) {
        res.path = "/index.html";
    }

    /* Read file and create response ----------------------------------- */
    char *path = calloc(strlen(PAGES_DIR) + strlen(res.path) + 1, 1);
    strcpy(path, PAGES_DIR);
    strcat(path, res.path);

    /* Open the file for reading */
    FILE *page_file = fopen(path, "rb");

    if (page_file == NULL) {
        res.status = 404;
        res.clen = 0;
        res.conn = "Close";
    } else {
        fseek(page_file, 0, SEEK_END); /* Seek to the end */
        res.clen = ftell(page_file);   /* This position's the size */
        rewind(page_file);             /* Go back to the start */

        /* Read it all in one operation and close */
        res.body = malloc(res.clen + 1);
        fread(res.body, res.clen, sizeof(char), page_file);
        fclose(page_file);      /* Close the file */
        res.body[res.clen] = 0; /* Add the terminating zero */
        res.status = 200;
    }

    /* Verify the connection and request version */
    if (r.conn == NULL && r.version > 1) {
        res.conn = "Keep-Alive";
    } else if (r.conn != NULL && strcmp(r.conn, "Close")) {
        res.conn = "Close";
    }

    res.stext = status_text(res.status); /* Write the status text */
    res.ctype = mime_type(path);         /* Update the content type */
    res.date = date_line();              /* Put the date line */

    /* Create the head */
    char response[MAX_HEADER_SIZE];
    create_res_header(res, response);

    send(conn, response, strlen(response), 0); /* Send response */

    if (res.clen > 0) {
        send(conn, res.body, res.clen, 0);
    }
    free(path);
    free(res.body);

    return 0;
}

int serve_head(int conn, struct request r) {
    struct response res;

    res.server = "Servrian/" VERSION;
    res.version = 1.1;
    res.date = date_line();
    res.path = r.url;
    res.status = 200;
    res.conn = "Close";

    /* If / was passed, redirect to index page */
    if (strlen(res.path) == 1) {
        res.path = "/index.html";
    }

    /* Get the page file's size */
    res.clen = file_size(res.path);
    res.ctype = mime_type(res.path);

    if (res.clen == 0) {
        /* Something went wrong, probably file was not found */
        res.status = 404;
        res.clen = 0;
    }

    /* Create the head */
    res.stext = status_text(res.status);

    char head[MAX_HEADER_SIZE];
    create_res_header(res, head);

    /* Send the head */
    send(conn, head, strlen(head), 0);

    return 0;
}

int serve_status(int conn, struct request r, int status) {
    struct response res;

    res.server = "Servrian/" VERSION;
    res.version = 1.1;
    res.status = status;
    res.date = date_line();
    res.path = r.url;
    res.ctype = "text/html";
    res.clen = 0;
    res.conn = "Close";

    /* Create the head */
    res.stext = status_text(res.status);

    char head[MAX_HEADER_SIZE];
    create_res_header(res, head);

    /* Send the head */
    send(conn, head, strlen(head), 0);

    return 0;
}