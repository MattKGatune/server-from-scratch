#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char **argv)
{
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        	die("signal() failed");
	if(argc != 5){
		fprintf(stderr, "usage: %s <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
		exit(1);
	}

	struct hostent *he;
        char *serverName = argv[3];
        // get server ip from server name
        if ((he = gethostbyname(serverName)) == NULL) {
            die("gethostbyname failed");
        }
        char *serverIP = inet_ntoa(*(struct in_addr *)he->h_addr);
	unsigned short mdb_port = atoi(argv[4]);

	unsigned short port = atoi(argv[1]);


	int servsock;
	if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("socket failed");
	int mdbsock;
	if((mdbsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("socket failed");
	struct sockaddr_in servaddr2;
    	memset(&servaddr2, 0, sizeof(servaddr2)); // must zero out the structure
    	servaddr2.sin_family      = AF_INET;
    	servaddr2.sin_addr.s_addr = inet_addr(serverIP);
    	servaddr2.sin_port        = htons(mdb_port);
	
	if (connect(mdbsock, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) < 0)
        	die("connect failed");

	FILE *results = fdopen(mdbsock, "r");
	FILE *mdbkey = fdopen(mdbsock, "w");

	struct sockaddr_in servaddr;
    	memset(&servaddr, 0, sizeof(servaddr));
    	servaddr.sin_family = AF_INET;
    	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    	servaddr.sin_port = htons(port);
	
	if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        	die("bind failed");

	if (listen(servsock, 5 /* queue size for connection requests */ ) < 0)
        	die("listen failed");

	int clntsock;
    	socklen_t clntlen;
    	struct sockaddr_in clntaddr;

	FILE *localfile;
	//Used 100 for both of these bc it wasn't specified in the lab spec
	char get_request[100];
	char line[100];

	while (1) 
	{
		clntlen = sizeof(clntaddr);
	
		if ((clntsock = accept(servsock, (struct sockaddr *) &clntaddr, &clntlen)) < 0){
			printf("accept failed\n"); 
			continue;
		}

		char *clientIP = inet_ntoa(clntaddr.sin_addr);
			
		FILE *request = fdopen(clntsock, "r");
		FILE *response = fdopen(clntsock, "wb");
	
		
		fgets(get_request, sizeof(get_request), request);
		if(get_request == NULL){
			fclose(request);
			fclose(response);
			continue;
		}
		
		
			strcpy(line, get_request);
			for(int i = 0; i < strlen(get_request); i++){
				if(get_request[i] == '\r' || get_request[i] == '\n'){
					get_request[i] = '\0';
				}
				//get_request[strlen(get_request) - 2] = '\0'; 
			}

			char *token_separators = "\t \r\n"; // tab, space, new line
		        char *method = strtok(line, token_separators);
        		char *requestURI = strtok(NULL, token_separators);
        		char *httpVersion = strtok(NULL, token_separators);
			

			char *path;	
			if(strcmp(method, "GET") != 0 || (strcmp(httpVersion, "HTTP/1.0") != 0 && strcmp(httpVersion, "HTTP/1.1") != 0)){
				fprintf(stdout, "%s \"%s\" 501 Not Implemneted\n", clientIP, get_request); 
				fprintf(response, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>");
				fclose(response);
				fclose(request);
				continue;
			}


			if(*requestURI != '/' || strstr(requestURI, "..") != NULL){
				fprintf(stdout, "%s \"%s\" 400 Bad Request\n", clientIP, get_request);
				fprintf(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>");
				fclose(response);
                                fclose(request);
                                continue;
			}

			if(requestURI[strlen(requestURI) - 1] == '/'){
				path = (char *)malloc(strlen(argv[2]) + strlen(requestURI) + 11);
				strcpy(path, argv[2]);
				strcat(path, requestURI);
				strcat(path, "index.html");
			}else{
				path = (char *)malloc(strlen(argv[2]) + strlen(requestURI) + 1);
                                strcpy(path, argv[2]);
                                strcat(path, requestURI);
			}


			struct stat filestat;
			

			if(stat(path, &filestat) != 0 && strstr(requestURI, "/mdb-lookup?key=") == NULL && strcmp(requestURI, "/mdb-lookup") != 0){
				fprintf(stdout, "%s \"%s\" 404 Not Found\n", clientIP, get_request);
				fprintf(response, "HTTP/1.0 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>");
				free(path);
				fclose(response);
                                fclose(request);
                                continue;
			}
			

			if(S_ISDIR(filestat.st_mode) && strstr(requestURI, "/mdb-lookup?key=") == NULL && strcmp(requestURI, "/mdb-lookup") != 0){
				if(path[strlen(path) - 1] != '/'){
					fprintf(stdout, "%s \"%s\" 501 Not Implemneted\n", clientIP, get_request);
					fprintf(response, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>");
					free(path);
					fclose(response);
                                	fclose(request);
                               		continue;
				}
			}
			const char *form =
                                "<h1>mdb-lookup</h1>\n"
                                "<p>\n"
                                "<form method=GET action=/mdb-lookup>\n"
                                "lookup: <input type=text name=key>\n"
                                "<input type=submit>\n"
                                "</form>\n"
                                "<p>\n";
			
			if(strcmp(requestURI, "/mdb-lookup") == 0){
				fprintf(response, "HTTP/1.0 200 OK\r\n\r\n");
				fprintf(response, "<html><body>");
				fprintf(response, "%s", form);
				fprintf(response, "</body></html>");
				fflush(response);
                        	fclose(request);
                        	fclose(response);
                        	fprintf(stdout, "%s \"%s\" 200 OK\n", clientIP, get_request);
                        	free(path);
				continue;

			}

			if(strstr(requestURI, "/mdb-lookup?key=") != NULL){
				char *key = (requestURI + 16);
				//strcat(key, "\n");
				fprintf(mdbkey, "%s\n", key);
				fflush(mdbkey);
				fprintf(response, "HTTP/1.0 200 OK\r\n\r\n");
				fprintf(response, "<html><body>");
				fprintf(response, "%s<p></p></body>", form);
				fprintf(response, "<p><table border =\"\"><tbody>");
				char data [100];
				int count = 0;
				while(fgets(data, sizeof(data), results)){
					if(strcmp(data, "\n") == 0)
						break;
					if(count%2 != 1){
						fprintf(response, "<tr><td>%s</td></tr>\n", data);
						fflush(response);
					}else{
						fprintf(response, "<tr><td bgcolor=\"yellow\">%s</td></tr>\n", data);
						fflush(response);
					}
					count++;
				}
			

				fprintf(response, "</tbody>\n</table>\n</p>\n</html>");
				fflush(response);
                                fclose(request);
                                fclose(response);
                                fprintf(stdout, "%s \"%s\" 200 OK\n", clientIP, get_request);
                                free(path);
                                continue;
			}



				
			
			localfile = fopen(path, "rb");
			
			char buf[4096];
			
			
			fprintf(response, "HTTP/1.0 200 OK\r\n\r\n");

			int r;
			while((r = fread(buf, 1, sizeof(buf), localfile)) > 0){
				fwrite(buf, r, 1, response);
			}
			
			fflush(response);
			fclose(request);
                	fclose(response);	
			fprintf(stdout, "%s \"%s\" 200 OK\n", clientIP, get_request);
			fclose(localfile);
			free(path);
	
		}


	}










