# Networking Architecture

Documentation on how networking works within this repository

## Message Protocol

The data layout of the messages being sent is very primative but allows enough flexability for what we need to do in this project.

Here is an example

```
29||LOGIN||someusername||123456
```

The initial integer represents how long the command string comming in will be.

The double pipes seperate each property into a different argument. The first of these arguments will always be considered the "route". This argument will be used by the client or server to figure out what sort of command to run.

A more formal definition would be:

```
STRCOUNT||ROUTE||ARG1||ARG2||...
```

## Current Possible Client<->Server Interactions

Here's a table of current client<-> server interactions possible using the current code

|Route                |Arguments                                      |Description                                                                               |Example                                             |
|---------------------|-----------------------------------------------|------------------------------------------------------------------------------------------|----------------------------------------------------|
| GLOBAL              | 0 - Message String                            | Send a message to a global chatroom                                                      | `25\|\|GLOBAL\|\|Hello everyone!`                  |
| CLOSE               | None                                          | Logout a user and free the socket connection                                             | `7\|\|CLOSE`                                       |
| REGISTER            | 0 - Username, 1 - Password                    | Register a user                                                                          | `24\|\|REGISTER\|\|user\|\|123456`                 |
| LOGIN               | 0 - Username, 1 - Password                    | Login a user                                                                             | `21\|\|LOGIN\|\|user\|\|123456`                    |
| LOGOUT              | None                                          | Sent from a user to the server to log them out of their session                          | `8\|\|LOGOUT`                                      |
| USERS               | None                                          | Get a list of active users                                                               | `7\|\|USERS`                                       |
| HISTORY             | 0 - Message Count                             | Get the public message history                                                           | `13\|\|HISTORY\|\|30`                              | 
| PMHISTORY           | 0 - User Message Recepient, 1 - Message Count | Get the dm history between two users                                                     | `21\|\|PMHISTORY\|\|user\|\|30`                    |
| DM                  | 0 - User Recepient, 1 - Message Count         | Send a private message to a user                                                         | `23\|\|DM\|\|user\|\|Hello user!`                  |
| HEALTH              | None                                          | A ping command used by the server to check for dead socket connections                   | `8\|\|HEALTH`                                      |
| SERVER_DISCONNECTED | None                                          | Sent from the server to notify that the server is offline and unable to process requests | `21\|\|SERVER_DISCONNECTED`                        |
| SERVER_RESPONSE     | 0 - Server Message                            | Sent from the server to the client for a generic response of something                   | `42\|\|SERVER_RESPONSE\|\|message from the server` |

# More Information

Open the [api-messages.c](./../../src/api-messages.c) or the [api-messages.h](./../../src/api-messages.h) for further clarity on how the packets are structures

Open the [client-commands.c](./../../src/client-commands.c) or [server-commands.c](./../../src/server-commands.c) for further clarity on each currently available protocol