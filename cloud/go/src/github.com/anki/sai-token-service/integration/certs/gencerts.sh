#!/bin/sh

# https://www.digitalocean.com/community/tutorials/openssl-essentials-working-with-ssl-certificates-private-keys-and-csrs

openssl req \
        -out tls-certificate.csr \
        -keyout tls-private-key.pem \
        -config san.cnf \
        -newkey rsa:2048 -nodes

openssl x509 \
       -signkey tls-private-key.pem \
       -in tls-certificate.csr \
       -out tls-certificate.pem \
       -req -days 3650 \
       -extfile v3.ext
