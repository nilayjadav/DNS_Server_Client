# DNS_Server_Client
This repository contains a C-based implementation of a Domain Name System (DNS) client and server, developed as part of the course ENPM818M: Introduction to Networking, Distributed Systems, and 5G/6G. The project provides insight into the DNS protocol, including its key components, how it works, and how DNS packets are structured.

Overview

The Domain Name System (DNS) is a hierarchical and distributed system used to translate human-readable domain names (e.g., google.com) into machine-readable IP addresses (e.g., 8.8.8.8). This project involves both:

DNS Server: Handles domain name resolution requests, supporting multiple domains and multiple IPs for certain domains.

DNS Client: Sends domain name resolution queries to the server and parses responses, including handling multiple IPs for a single query.

Features

DNS Server

Resolves domain names to their corresponding IP addresses.

Supports multiple IP addresses for a single domain.

Handles up to 10 predefined domains.

Strips prefixes like http://, https://, and www. from incoming queries.

Responds with appropriate error codes for unsupported domains.

Implements standard DNS packet structure based on RFC 1035.

DNS Client

Sends DNS queries to the server and parses responses.

Supports multiple formats for domain names, including google.com, http://google.com, and www.google.com.

Processes all IP addresses returned by the server for a single domain.

Allows users to send multiple domain name queries in a single session.

Implements timeout handling for server responses.

Methodology

Key Components of DNS

Domain Name Space: A hierarchical structure to organize domain names.

Name Servers: Store domain name information and respond to queries.

Resolvers: Client-side programs that query name servers to retrieve IP addresses.

How DNS Works

The user enters a domain name in a browser (e.g., example.com).

The resolver (DNS client) sends a query to the DNS server.

The DNS server looks up the requested information.

The server returns the IP address(es) to the client.

The browser connects to the server using the resolved IP address.

DNS Packet Structure

Header: Contains metadata about the DNS packet type, flags, and record counts.

Question Section: Specifies the domain name being queried.

Answer Section: Contains resolved IP addresses, including multiple records for a single query.

Demo

Running the Server

Compile the server code:

gcc -o dns_server dns_server.c

Run the server with administrative privileges (for port 53):

sudo ./dns_server

Running the Client

Compile the client code:

gcc -o dns_client dns_client.c

Run the client:

./dns_client

Enter domain names for resolution (e.g., google.com, http://www.github.com). Type exit to terminate the client.

Using with a Standard Server

Change the IP to 8.8.8.8 and Port to 53 in the client code to test with a standard DNS server (e.g., Google Public DNS).

Using with a Standard Client

If you're testing with a standard DNS client like nslookup, you need to stop the system's default DNS resolver first:

sudo systemctl stop systemd-resolved
sudo systemctl disable systemd-resolved

To re-enable the resolver after testing:

sudo systemctl start systemd-resolved
sudo systemctl enable systemd-resolved

To send a query to your custom server:

nslookup google.com 127.0.0.1

Requirements

DNS Client

Ability to format and send DNS queries.

Parsing of server responses, including multiple answers.

Handling of domain names with various prefixes.

DNS Server

Ability to resolve predefined domain names.

Support for multiple IP addresses per domain.

Correct handling of DNS query and response structures.

References

RFC 1035: Domain Names - Implementation and Specification

DNS in Computer Networks

DNS Packet Primer

Authors

Nilay Jadav

Harshitha Yannam

Thank You

This project demonstrates the fundamentals of DNS through a hands-on implementation, providing a deeper understanding of networking concepts introduced in ENPM818M.
