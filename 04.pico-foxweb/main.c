#include "httpd.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>

#define CHUNK_SIZE 1024 // read 1024 bytes at a time

// Public directory settings
#define INDEX_HTML "/index.html"
#define NOT_FOUND_HTML "/404.html"

char * PUBLIC_DIR;

int main(int c, char **v) {
  char *port = c == 1 ? "8000" : v[1];
  PUBLIC_DIR = c <= 2 ? "./webroot": v[2];
  serve_forever(port);
  return 0;
}

int file_exists(const char *file_name) {
  struct stat buffer;
  int exists;

  exists = (stat(file_name, &buffer) == 0);

  return exists;
}

int read_file(const char *file_name) {
  char buf[CHUNK_SIZE];
  FILE *file;
  size_t nread;
  int err = 1;

  file = fopen(file_name, "r");

  if (file) {
    while ((nread = fread(buf, 1, sizeof buf, file)) > 0)
      fwrite(buf, 1, nread, stdout);

    err = ferror(file);
    fclose(file);
  }
  return err;
}

int check_command_injection(char* input_string) {
    regex_t regex;
    int reti;

    // компилируем регулярное выражение
    regcomp(&regex, "(.+( & ).+)|(.+( && ).+)|(.+( \\| ).+)|(.+( \\|\\| ).+)|(.+( ; ).+)|(.+( \\$\\(.+\\)).*)|(.+(\\n).+)|(.+(`.+`).*)|(.+(\" ).+)|(.+(\' ).+)", 1);

    // проверяем, соответствует ли строка регулярному выражению
    reti = regexec(&regex, input_string, 0, NULL, 0);
    if (!reti) {
        regfree(&regex);
        // если соответствует, возвращаем 1
        return 1;
    }

    regfree(&regex);
    return 0;
}

void route() {
  ROUTE_START()

  GET("/") {
    if (check_command_injection(qs)) {
      HTTP_400;
      return;
    }	
	
    char index_html[20];
    sprintf(index_html, "%s%s", PUBLIC_DIR, INDEX_HTML);
	
    HTTP_200;
    if (file_exists(index_html)) {
      read_file(index_html);
    } else {
      printf("Hello! You are using %s\n\n", request_header("User-Agent"));
    }
  }

  GET("/test") {
    if (check_command_injection(qs)) {
      HTTP_400;
      return;
    }	
	
    HTTP_200;
    printf("List of request headers:\n\n");
    header_t *h = request_headers();

    while (h->name) {
      printf("%s: %s\n", h->name, h->value);
      h++;
    }
  }

  POST("/") {
    if (check_command_injection(payload)) {
      HTTP_400;
      return;
    }	
	
    HTTP_201;
    printf("Wow, seems that you POSTed %d bytes.\n", payload_size);
    printf("Fetch the data using `payload` variable.\n");
    if (payload_size > 0)
      printf("Request body: %s", payload);
  }

  GET(uri) {
    if (check_command_injection(qs)) {
      HTTP_400;
      return;
    }	
	
    char file_name[255];
    sprintf(file_name, "%s%s", PUBLIC_DIR, uri);

    if (file_exists(file_name)) {
      HTTP_200;
      read_file(file_name);
    } else {
      HTTP_404;
      sprintf(file_name, "%s%s", PUBLIC_DIR, NOT_FOUND_HTML);
      if (file_exists(file_name))
        read_file(file_name);
    }
  }

  ROUTE_END()
}
