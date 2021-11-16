# Networking Architecture

Documentation on how networking works within this repository

## Message Protocol

The data layout of the packets being sent is very primative but allows enough flexability for what we need to do in this project.

Here is an example

```
||LOGIN||someusername||123456
```

Double pipes seperate the each property into a different segment. The initial property is the "route" which should be taken by the server or client, and the rest are arguments given in an expected order.

A more formal definition would be:

```
||ROUTE||ARG1||ARG2||...
```

A pipe is not necessary at the end and may give undefined behaviour

## Current Possible Client<->Server Interactions

Here's a table of current client<-> server interactions possible using the current code

|Route|Arguments|Description|Example|
|-----|--------|-----------|-------|
| GLOBAL | 0 - Message String (Max 4096 chars) | Send a message to a global chatroom | `\|\|GLOBAL\|\|Hello everyone!`
| LOGOUT | None | Sent from a user to the server to log them out of their session | `\|\|LOGOUT`
| SERVER_DISCONNECTED | None | Sent from the server to notify that the server is offline and unable to process requests | `\|\|SERVER_DISCONNECTED`

# More Information

Open the [api-messages.c](./../../src/api-messages.c) or the [api-messages.h](./../../src/api-messages.h) for further clarity on how the packets are structures

Open the [client-commands.c](./../../src/client-commands.c) or [server-commands.c](./../../src/server-commands.c) for further clarity on each currently available protocol