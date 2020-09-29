# EdgeMailer
EdgeMailer is a tool that tests rate limits of mail providers, it uses libcurl and libuv to make concomitant assynchronous request.
For now this will be just a libuv + libcurl template in which we can code on top of, there is no implementation of actual rate limit testing

## Features:
1. Uses libuv
2. Because of libuv, we have a thread pool
4. **Strict** C standards, no warning should be emitted with gcc -Wall

## How to compile:
`gcc tool.c -lcurl -luv -Wall`

## Pre-requisites
1. LibCurl
2. LibUv
