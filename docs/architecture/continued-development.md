# Continued Development

This documentation is to help you extend this code for other purposes

# The Build System

All files that start with `server` will be built **only into the server executable** while all files that start with `client` will be built **only into the client executable**. Server and Client only files can use shared libraries but the server/client files themselves should be considered top level and not reference eachother in any way.