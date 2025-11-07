#!/usr/bin/env python3

# Simple HTTP server script that sets up standard MIME types and CORS headers.
# Run it from install folder with optional port argument; call with '--help' to see description.
# Requires Python 3.7 or later.

import sys
import os

from http.server import HTTPServer, SimpleHTTPRequestHandler

import time
import threading
import webbrowser

hasThreadedServer: bool = sys.version_info.major >= 4 or sys.version_info.minor >= 7
if hasThreadedServer:
    from http.server import ThreadingHTTPServer

THE_ADDRESS: str = "localhost"
THE_PORT: int = 8000
THE_TO_CORS: bool = True
THE_TO_REVALIDATE_CACHE: bool = True
THE_DIRECTORY: str = os.getcwd()

def openUrlInBrowser(theUrl):
    time.sleep(1)
    webbrowser.open_new(theUrl)

class requestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=THE_DIRECTORY, **kwargs)

    def end_headers(self):
        # setup mandatory MIME types
        self.extensions_map.update({
            ".js": "application/javascript",
            ".wasm": "application/wasm",
            ".css": "text/css",
            ".svg": "image/svg+xml",
            ".ttf": "font/ttf",
            ".woff": "font/woff",
            ".woff2": "font/woff2"
        });

        # setup CORS and COEP policies to strict isolation and allowing multi-threaded WebAssembly
        if THE_TO_CORS:
            self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
            self.send_header('Cross-Origin-Opener-Policy',   'same-origin')

        # Force re-validating browser-cached files on each page load
        # using 'Last-Modified' file property returned by Simple HTTPServer.
        # Smarter servers may also generate ETag's as a hash sum.
        if THE_TO_REVALIDATE_CACHE:
            #self.send_header('Cache-Control', 'no-store, must-revalidate')
            #self.send_header('Cache-Control', 'public, no-cache')
            self.send_header('Cache-Control', 'max-age=2')

        SimpleHTTPRequestHandler.end_headers(self)

    def do_GET(self, body = True):
        #print(" @@@ do_GET {0}".format(self.path))
        if not THE_TO_REVALIDATE_CACHE:
            SimpleHTTPRequestHandler.do_GET(self)
            return

        self.path_local = os.path.join(THE_DIRECTORY, self.path.lstrip('/'))
        if not os.path.exists(self.path_local) or not os.path.isfile(self.path_local):
            # just redirect to basic implementation - which would also find 'index.html'
            #self.send_response(404)
            SimpleHTTPRequestHandler.do_GET(self)
            return

        # detect MIME type
        aContentType = ([aContentType for ext, aContentType in sorted(self.extensions_map.items(), reverse = True) if self.path_local.endswith(ext)] + ['application/octet-stream'])[0]

        # detect modification timestamps
        aCheckModifDate = self.headers.get('If-Modified-Since')
        aLastModified = self.date_time_string(os.stat(self.path_local).st_mtime)

        # get ETag
        #aCheckModifEtag = self.headers.get('If-None-Match')

        #print(" @@@ path_local: {0} [{1} {2}]".format(self.path_local, aLastModified, aCheckModifDate))
        if aLastModified == aCheckModifDate:
            self.protocol_version = 'HTTP/1.1'
            self.send_response(304)
            #self.send_header('ETag', checkETag[self.path_local])
            self.end_headers()
            return

        # write file in return
        with open(self.path_local, 'rb') as aFile:
            aBinContent = aFile.read()

        self.protocol_version = 'HTTP/1.1'
        self.send_response(200)
        self.send_header('Content-Length', len(aBinContent))
        self.send_header('Content-Type', aContentType)
        self.send_header('Last-Modified', aLastModified)
        #self.send_header('ETag', checkETag[self.path_local])
        self.end_headers()
        self.wfile.write(aBinContent)
        return

if __name__ == '__main__':
    hasPortArg: bool = False
    anOpenPage: str = ""
    aNbArgs: int = len(sys.argv)
    anArgIter: int = 1
    while anArgIter < aNbArgs:
        anArg: str = sys.argv[anArgIter]
        isParsed: bool = False
        if anArgIter + 1 < aNbArgs:
            isParsed = True
            if (anArg.lower() == "--port") or (anArg.lower() == "-port"):
                hasPortArg = True
                anArgIter += 1
                THE_PORT = int(sys.argv[anArgIter])
            elif (anArg.lower() == "--address") or (anArg.lower() == "-address"):
                anArgIter += 1
                THE_ADDRESS = sys.argv[anArgIter]
            elif (anArg.lower() == "--directory") or (anArg.lower() == "-d"):
                anArgIter += 1
                THE_DIRECTORY = sys.argv[anArgIter]
            elif (anArg.lower() == "--cors") or (anArg.lower() == "-cors"):
                anArgIter += 1
                isOn: int = int(sys.argv[anArgIter])
                THE_TO_CORS = isOn != 0
            elif (anArg.lower() == "--threaded") or (anArg.lower() == "-threaded"):
                anArgIter += 1
                isOn: int = int(sys.argv[anArgIter])
                hasThreadedServer = isOn != 0
            elif (anArg.lower() == "--open") or (anArg.lower() == "-open"):
                anArgIter += 1
                anOpenPage = sys.argv[anArgIter]
            else:
                isParsed = False

        if isParsed:
            anArgIter += 1
            continue

        if (anArg.lower() == "--help") or (anArg.lower() == "-help"):
            print ("Usage: server.py [--address ADDRESS]=localhost [--port PORT]=8000\n\
                   [--cors 0|1]=1 [--threaded 0|1]=1 [--directory DIR]=CWD\
                   [--open page]")
            sys.exit (0)
        elif not hasPortArg:
            hasPortArg = True
            THE_PORT = int(anArg)
        else:
            print ("Syntax error at '{0}'".format (anArg))
            sys.exit (1)

        anArgIter += 1

    if hasThreadedServer:
        anHttpServer = ThreadingHTTPServer ((THE_ADDRESS, THE_PORT), requestHandler)
    else:
        anHttpServer = HTTPServer ((THE_ADDRESS, THE_PORT), requestHandler)

    aServerUrl = "http://{0}:{1}/".format (anHttpServer.server_address[0], anHttpServer.server_address[1])
    print ("Serving  {0} [CORS:{1} THREADS:{2}]".format (aServerUrl, THE_TO_CORS, hasThreadedServer))

    if not anOpenPage == "":
        openUrlInBrowser("{0}{1}".format (aServerUrl, anOpenPage))

    anHttpServer.serve_forever()
