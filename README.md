# server-from-scratch
# HTTP Server with Database Lookup Integration

## Overview
This project implements a C-based HTTP server that handles static file requests and performs dynamic database lookups via a remote server. The server adheres to HTTP/1.0 standards, processes GET requests, and responds with appropriate HTML content, including dynamically generated tables from the database.

## Features
- **Static File Handling**: Serves files from a specified web root directory.
- **Database Lookup**: Connects to a remote database server to fetch and display data based on user queries.
- **HTTP/1.0 Compliance**: Handles GET requests with proper URI validation and error handling.
- **Dynamic Content**: Generates HTML content on the fly, including tables for database query results.
- **Error Handling**: Responds with appropriate HTTP status codes for invalid requests or server errors.

## Usage

### Command Line Arguments
```bash
./server <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>
